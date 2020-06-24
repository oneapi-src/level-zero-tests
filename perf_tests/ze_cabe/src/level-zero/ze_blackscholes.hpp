/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_ZE_BLACKSCHOLES_HPP
#define COMPUTE_API_BENCH_ZE_BLACKSCHOLES_HPP

#include <vector>
#include <string>
#include "ze_workload.hpp"

namespace compute_api_bench {

template <class T> class ZeBlackScholes : public ZeWorkload {
public:
  ZeBlackScholes(BlackScholesData<T> *bs_io_data, unsigned int num_iterations);
  ~ZeBlackScholes();

  void build_program();
  void create_buffers();
  void create_cmdlist();
  void execute_work();
  bool verify_results();
  void cleanup();

private:
  size_t kernel_length;
  std::string kernel_code;
  std::vector<uint8_t> kernel_spv;
  void *mem_option_years = nullptr;
  void *mem_option_strike = nullptr;
  void *mem_stock_price = nullptr;
  void *mem_call_result = nullptr;
  void *mem_put_result = nullptr;
  ze_module_handle_t module = nullptr;
  ze_kernel_handle_t function = nullptr;
  ze_command_queue_handle_t command_queue = nullptr;
  ze_command_list_handle_t command_list = nullptr;
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

#endif // COMPUTE_API_BENCH_ZE_BLACKSCHOLES_HPP
