/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_OCL_BLACKSCHOLES_HPP
#define COMPUTE_API_BENCH_OCL_BLACKSCHOLES_HPP

#include <vector>
#include <string>
#include "ocl_workload.hpp"

namespace compute_api_bench {

template <class T> class OCLBlackScholes : public OCLWorkload {
public:
  OCLBlackScholes(BlackScholesData<T> *data, unsigned int num_iterations);
  ~OCLBlackScholes();

  void build_program();
  void create_buffers();
  void create_cmdlist();
  void execute_work();
  bool verify_results();
  void cleanup();

private:
  cl_kernel kernel = NULL;
  cl_mem mem_option_years = NULL;
  cl_mem mem_option_strike = NULL;
  cl_mem mem_stock_price = NULL;
  cl_mem mem_call_result = NULL;
  cl_mem mem_put_result = NULL;
  unsigned int num_iterations;
  unsigned int num_options;
  const T riskfree = 0.02;
  const T volatility = 0.3;
  T max_delta;
  std::vector<T> option_years;
  std::vector<T> option_strike;
  std::vector<T> stock_price;
  std::vector<T> call_result;
  std::vector<T> put_result;
  uint32_t buffer_size;
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_OCL_BLACKSCHOLES_HPP
