/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_WORKLOAD_HPP
#define COMPUTE_API_BENCH_WORKLOAD_HPP

#include <vector>
#include <numeric>
#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <assert.h>
#include "timer.hpp"
#include "utils.hpp"

namespace compute_api_bench {

class Workload {

public:
  Workload();

  struct Result {
    std::vector<double> times;
    double time_min;
    double time_max;
    double time_mean;
    double time_median;
    double time_standard_deviation;

    Result()
        : time_min(0), time_max(0), time_mean(0), time_median(0),
          time_standard_deviation(0) {}
  } result[Stages::COUNT];

  virtual ~Workload() = default;
  void run(unsigned int iterations);
  void print_total_mean_time();
  void print_stage_mean_sd(unsigned int stage, std::string &csv_string,
                           bool colored, bool useMedian);
  static void print_apis(std::string api, std::string &csv, bool colored);

  unsigned int iterations;
  std::string workload_name;
  std::string workload_api;

protected:
  virtual void create_device() = 0;
  virtual void build_program() = 0;
  virtual void create_buffers() = 0;
  virtual void create_cmdlist() = 0;
  virtual void execute_work() = 0;
  virtual bool verify_results() = 0;
  virtual void cleanup() = 0;

private:
  void calculate_results();
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_WORKLOAD_HPP
