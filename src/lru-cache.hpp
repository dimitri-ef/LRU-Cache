#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <unordered_map>
#include <list>
#include <mutex>
#include <optional>
#include <utility>
#include <type_traits>
#include <functional>

/**
 * @brief Universal hash for std::tuple.
 * Combines each element's hash using XOR + shift (boost::hash_combine algorithm).
 */
struct TupleHash
{
    template<typename... Args>
    size_t operator()(const std::tuple<Args...>& t) const
    {
        size_t seed = 0;
        std::apply([&seed](const auto&... args) {
            ((seed ^= std::hash<std::decay_t<decltype(args)>>{}(args)
                + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
        }, t);
        return seed;
    }
};

/**
 * @brief Extracts the return type and argument types from a callable.
 * Specialized for function pointers, std::function and lambdas.
 * @tparam Fn Callable type
 */
template<typename Fn>
struct function_traits;
 
template<typename R, typename... Args>
struct function_traits<R(*)(Args...)>
{
    using return_type = R;
    using args_tuple  = std::tuple<Args...>;
};
 
template<typename R, typename... Args>
struct function_traits<std::function<R(Args...)>>
{
    using return_type = R;
    using args_tuple  = std::tuple<Args...>;
};
 
template<typename Fn>
struct function_traits : function_traits<decltype(&Fn::operator())> {};
 
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...) const>
{
    using return_type = R;
    using args_tuple  = std::tuple<Args...>;
};

/**
 * @brief Thread-safe LRU (Least Recently Used) cache.
 *
 * Stores up to @p capacity entries. When full, the least recently
 * used entry is automatically evicted.
 *
 * @tparam Key   Key type (must be hashable by Hash)
 * @tparam Value Value type
 * @tparam Hash  Hash function (default: std::hash<Key>)
 */
template<typename Key, typename Value, typename Hash = std::hash<Key>>
class LRUCache
{
public:
    /**
     * @brief Constructs the cache with a maximum capacity.
     * @param capacity Maximum number of entries
     */
    LRUCache(size_t capacity);
    ~LRUCache() = default;

    /**
     * @brief Inserts or updates an entry.
     * Evicts the least recently used entry if capacity is reached.
     * @param key   Entry key
     * @param value Associated value
     */
    void set(Key key, Value value);

    /**
     * @brief Retrieves a value and promotes it to the front (most recent).
     * @param key Key to look up
     * @return The value if found, std::nullopt otherwise
     */
    std::optional<Value> get(Key key);

    /**
     * @brief Proxy returned by operator[] to support both read and write.
     * - Write : cache[key] = value
     * - Read  : auto v = (std::optional<Value>)cache[key]
     */
    struct Proxy
    {
        LRUCache& cache;
        const Key& key;
        operator std::optional<Value>() const { return cache.get(key); }
        Proxy& operator=(const Value& value) { cache.set(key, value); return *this; }
    };
    Proxy operator[](const Key& key) { return Proxy{*this, key}; }

private:
    using ListIterator = typename std::list<std::pair<Key, Value>>::iterator;
    std::unordered_map<Key, ListIterator, Hash> m_cache;
    std::list<std::pair<Key, Value>> m_items;

    size_t m_capacity;

    std::mutex m_mutex;
};

template<typename Key, typename Value, typename Hash>
inline LRUCache<Key, Value, Hash>::LRUCache(size_t capacity)
    : m_capacity(capacity)
{}

template<typename Key, typename Value, typename Hash>
inline void LRUCache<Key, Value, Hash>::set(Key key, Value value)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);

    if (it != m_cache.end())
    {
        it->second->second = value;
        m_items.splice(m_items.begin(), m_items, it->second);
        return;
    }

    if (m_items.size() >= m_capacity)
    {
        auto last = m_items.back();
        m_cache.erase(last.first);
        m_items.pop_back();
    }

    m_items.emplace_front(key, value);
    m_cache[key] = m_items.begin();
}

template<typename Key, typename Value, typename Hash>
inline std::optional<Value> LRUCache<Key, Value, Hash>::get(Key key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it == m_cache.end())
        return std::nullopt;
    
    m_items.splice(m_items.begin(), m_items, it->second);

    return std::optional<Value>{it->second->second};
}

/**
 * @brief Wraps a function into a memoized version backed by an internal LRUCache.
 *
 * The cache key is the tuple of all arguments.
 * Works with function pointers, std::function and lambdas.
 *
 * @tparam Fn       Callable type
 * @param  fn       Pure function to memoize
 * @param  capacity Cache size (default: 128)
 * @return Memoized lambda that returns cached results when available
 *
 * @code
 * auto cached_prime = memoize(is_prime, 256);
 * cached_prime(999983); // computed
 * cached_prime(999983); // from cache
 * @endcode
 */
template<typename Fn>
auto memoize(Fn fn, size_t capacity = 128)
{
    using Key    = typename function_traits<Fn>::args_tuple;
    using Result = typename function_traits<Fn>::return_type;
 
    return [fn, cache = LRUCache<Key, Result, TupleHash>(capacity)](auto&&... args) mutable
    {
        auto key = std::make_tuple(std::forward<decltype(args)>(args)...);
        if (auto cached = (std::optional<Result>)cache[key]; cached.has_value())
            return *cached;
        Result result = fn(std::forward<decltype(args)>(args)...);
        cache[key] = result;
        return result;
    };
}

#endif // LRU_CACHE_HPP