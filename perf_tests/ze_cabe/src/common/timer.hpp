/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_TIMER_HPP
#define COMPUTE_API_BENCH_TIMER_HPP

#include <chrono>

namespace compute_api_bench {

class Timer {
public:
  Timer();
  ~Timer() = default;
  void start();
  double elapsed_time();

private:
  std::chrono::system_clock::time_point start_time_point;
  std::chrono::system_clock::time_point end_time_point;
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_TIMER_HPP
