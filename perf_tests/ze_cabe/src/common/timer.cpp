/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "timer.hpp"

namespace compute_api_bench {

Timer::Timer() {}

void Timer::start() { start_time_point = std::chrono::system_clock::now(); }

double Timer::elapsed_time() {
  end_time_point = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds =
      end_time_point - start_time_point;
  start_time_point = end_time_point;
  return elapsed_seconds.count();
}

} // namespace compute_api_bench
