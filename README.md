# LRU Cache

Thread-safe LRU cache in C++17 with a `memoize` helper.

## Usage

```cpp
#include "lru_cache.hpp"

// manual
LRUCache<int, std::string> cache(128);
cache.set(1, "hello");
auto v = (std::optional<std::string>)cache[1]; // "hello"
cache[2] = "world";

// memoize
bool is_prime(long long n) { ... }
static auto cached = memoize(is_prime, 256);
cached(999983); // computed
cached(999983); // from cache

// tuple key (multi-arg functions)
double integrate(double a, double b, int steps) { ... }
static auto cached = memoize(integrate, 128);
cached(0.0, 100.0, 1000000);
```

## Files

| File                  | Description                                      |
|-----------------------|--------------------------------------------------|
| `src/lru_cache.hpp`   | LRUCache, TupleHash, memoize                     |
| `src/performance.hpp` | PerformanceWatch / PerformanceGuard (RAII timer) |
| `testbench/main.cpp`  | Benchmark sans/avec cache                        |

## Build

```bash
g++ -std=c++17 -O2 testbench/main.cpp -o testbench && ./build
```

## Notes

- `memoize` works with function pointers and lambdas, not member functions
- tuple keys require `TupleHash` — passed automatically by `memoize`
- cache size matters: too small = frequent evictions, too large = memory overhead