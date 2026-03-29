#include <cmath>
#include <vector>
#include <random>
#include <numeric>
#include <tuple>

#include "lru-cache.hpp"
#include "performance.hpp"

constexpr int REPEAT    = 500;
constexpr int POOL_SIZE = 32;

bool is_prime(long long n)
{
    if (n < 2) return false;
    for (long long i = 2; i <= std::sqrt(n); ++i)
        if (n % i == 0) return false;
    return true;
}

 
double integrate(double a, double b, int steps)
{
    auto f = [](double x) { return std::sin(x) * std::exp(-x * 0.1) * std::cos(x * 0.5); };
    double h = (b - a) / steps;
    double result = 0.5 * (f(a) + f(b));
    for (int i = 1; i < steps; ++i)
        result += f(a + i * h);
    return result * h;
}

std::vector<int> sieve(int n)
{
    std::vector<bool> is_p(n + 1, true);
    is_p[0] = is_p[1] = false;
    for (int i = 2; i * i <= n; ++i)
        if (is_p[i])
            for (int j = i * i; j <= n; j += i)
                is_p[j] = false;
    std::vector<int> primes;
    for (int i = 2; i <= n; ++i)
        if (is_p[i]) primes.push_back(i);
    return primes;
}

int main()
{
    std::mt19937 rng(42);
 
    PerformanceWatch pw_prime            ("is_prime");
    PerformanceWatch pw_prime_cached     ("is_prime_cached");
    PerformanceWatch pw_integrate        ("integrate");
    PerformanceWatch pw_integrate_cached ("integrate_cached");
    PerformanceWatch pw_sieve            ("sieve");
    PerformanceWatch pw_sieve_cached     ("sieve_cached");
 
    auto is_prime_cached    = memoize(is_prime,   256);
    auto integrate_cached   = memoize(integrate,  256);
    auto sieve_cached       = memoize(sieve,       32);
 
    {
        std::uniform_int_distribution<long long> dist(100'000, 9'999'999);
        std::uniform_int_distribution<int>       pick(0, POOL_SIZE - 1);
 
        std::vector<long long> pool(POOL_SIZE);
        for (auto& v : pool) v = dist(rng);
 
        std::vector<long long> sequence(REPEAT);
        for (auto& v : sequence) v = pool[pick(rng)];
 
        for (long long n : sequence) { PerformanceGuard g(pw_prime);        is_prime(n);        }
        for (long long n : sequence) { PerformanceGuard g(pw_prime_cached);  is_prime_cached(n); }
    }
 
    {
        std::uniform_real_distribution<double> dist_a(0.0, 50.0);
        std::uniform_real_distribution<double> dist_len(1.0, 50.0);
        std::uniform_int_distribution<int>     dist_steps(500'000, 1'000'000);
        std::uniform_int_distribution<int>     pick(0, POOL_SIZE - 1);
 
        using Params = std::tuple<double, double, int>;
        std::vector<Params> pool(POOL_SIZE);
        for (auto& [a, b, s] : pool)
        {
            a = dist_a(rng);
            b = a + dist_len(rng);
            s = dist_steps(rng);
        }
 
        std::vector<int> sequence(REPEAT);
        for (auto& v : sequence) v = pick(rng);
 
        for (int i : sequence) { PerformanceGuard g(pw_integrate);          integrate(std::get<0>(pool[i]), std::get<1>(pool[i]), std::get<2>(pool[i]));        }
        for (int i : sequence) { PerformanceGuard g(pw_integrate_cached);   integrate_cached(std::get<0>(pool[i]), std::get<1>(pool[i]), std::get<2>(pool[i])); }
    }
 
    {
        std::uniform_int_distribution<int> dist(100'000, 500'000);
        std::uniform_int_distribution<int> pick(0, POOL_SIZE - 1);
 
        std::vector<int> pool(POOL_SIZE);
        for (auto& v : pool) v = dist(rng);
 
        std::vector<int> sequence(REPEAT);
        for (auto& v : sequence) v = pool[pick(rng)];
 
        for (int n : sequence) { PerformanceGuard g(pw_sieve);          sieve(n);       }
        for (int n : sequence) { PerformanceGuard g(pw_sieve_cached);   sieve_cached(n);}
    }
 
    std::cout << "=== Results ===" << std::endl;
    return 0;
}