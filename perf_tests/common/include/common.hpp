/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _COMMON_HPP_
#define _COMMON_HPP_
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

// Bandwidth reported in GBPS (10^9), instead of GiBPS (2^30).
#define ONE_KB (1 * 1000ULL)
#define ONE_MB (1 * ONE_KB * ONE_KB)
#define ONE_GB (1 * ONE_KB * ONE_MB)

template <typename T = std::chrono::nanoseconds::period> class Timer {
public:
  Timer() { overhead(); }
  inline void start() {
    time_start = std::chrono::high_resolution_clock::now();
  }

  inline void end() { time_end = std::chrono::high_resolution_clock::now(); }

  inline long double period_minus_overhead() {
    return std::chrono::duration<long double, T>(time_end - time_start)
               .count() -
           time_overhead;
  }

  long double stopAndTime() {
    end();
    return period_minus_overhead();
  }

  inline bool has_it_been(long long int moment) {
    std::chrono::high_resolution_clock::time_point time_now =
        std::chrono::high_resolution_clock::now();
    auto period = std::chrono::duration<long double, T>(time_now - time_start);
    return (period.count() >= moment);
  }

private:
  std::chrono::high_resolution_clock::time_point time_start, time_end;
  long double time_overhead;

  inline void overhead() {
    start();
    end();
    auto period = std::chrono::duration<long double, T>(time_end - time_start);
    time_overhead = period.count();
  }
};

inline uint64_t roundToMultipleOf(uint64_t number, uint64_t base,
                                  uint64_t maxValue) {
  uint64_t n = (number > maxValue) ? maxValue : number;
  return (n / base) * base;
}

extern bool verbose;

template <bool TerminateOnFailure, typename ResulT>
inline void validate(ResulT result, const char *message) {
  if (result != 0) { /* assumption 0 is success */
    std::cerr << (TerminateOnFailure ? "ERROR : " : "WARNING : ") << message
              << " : " << result << std::endl;
    if (TerminateOnFailure) {
      std::terminate();
    }
    return;
  }

  if (verbose) {
    std::cerr << " SUCCESS : " << message << std::endl;
  }
}

#define SUCCESS_OR_TERMINATE(CALL) validate<true>(CALL, #CALL)

template <typename T> int size_in_bytes(const std::vector<T> &v) {
  return static_cast<int>(sizeof(T) * v.size());
}

#define ERROR_RETURN(retval)                                                   \
  {                                                                            \
    std::cerr << "ERROR : " << __FILE__ << ":" << __LINE__ << " " << ret       \
              << std::endl;                                                    \
    std::terminate();                                                          \
  }

#endif /* _COMMON_HPP_ */
