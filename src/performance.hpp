#ifndef PERFORMANCE_HPP
#define PERFORMANCE_HPP

#include <chrono>
#include <vector>
#include <string>
#include <iostream>

/**
 * @brief Accumulates execution durations and reports the average on destruction.
 *
 * Pair with PerformanceGuard to measure a scope.
 * Prints results to stdout when destroyed (end of program for static instances).
 */
class PerformanceWatch
{
public:
    /**
     * @brief Constructs the watch with a display name.
     * @param name Label printed in the final report
     */
    PerformanceWatch(const std::string& name);
    ~PerformanceWatch();
private:
    std::vector<std::chrono::microseconds> m_durations;
    std::string m_name;
    friend class PerformanceGuard;
};

inline PerformanceWatch::PerformanceWatch(const std::string& name)
    : m_name(name)
{}

inline PerformanceWatch::~PerformanceWatch()
{
    double result = 0.0;
    if (!m_durations.empty()) {
        std::chrono::microseconds total(0);
        for (const auto& d : m_durations)
            total += d;
        result = static_cast<double>(total.count()) / m_durations.size();
    }
    std::cout << m_name << ": avg=" << result << "us over " << m_durations.size() << " calls" << std::endl;
}

/**
 * @brief RAII guard that measures the duration of its enclosing scope.
 *
 * Records start time on construction, computes elapsed time on destruction
 * and pushes it into the associated PerformanceWatch.
 *
 * @code
 * void my_function() {
 *     static PerformanceWatch pw("my_function");
 *     PerformanceGuard guard(pw);
 *     // ... code to measure ...
 * }
 * @endcode
 */
class PerformanceGuard
{
public:
    /**
     * @brief Starts the timer.
     * @param watch PerformanceWatch that will collect this measurement
     */
    PerformanceGuard(PerformanceWatch& chrono);
    ~PerformanceGuard();
private:
    PerformanceWatch& m_chrono;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_end;
};

inline PerformanceGuard::PerformanceGuard(PerformanceWatch& chrono)
    : m_chrono(chrono)
{
    m_start = std::chrono::high_resolution_clock::now();
}

inline PerformanceGuard::~PerformanceGuard()
{
    m_end = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(m_end - m_start);

    m_chrono.m_durations.push_back(duration);
}

#endif // PERFORMANCE_HPP