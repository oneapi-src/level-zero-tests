/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_ZE_MANDELBROT_HPP
#define COMPUTE_API_BENCH_ZE_MANDELBROT_HPP

#include <vector>
#include <string>
#include "ze_workload.hpp"

namespace compute_api_bench {

class ZeMandelbrot : public ZeWorkload {
public:
  ZeMandelbrot(unsigned int width, unsigned int height,
               unsigned int num_iterations);
  ~ZeMandelbrot();

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
  unsigned int num_iterations;
  unsigned int width;
  unsigned int height;
  std::vector<float> result;
  std::vector<float> result_CPU;
  void *output_buffer = nullptr;
  ze_module_handle_t module = nullptr;
  ze_kernel_handle_t function = nullptr;
  ze_command_queue_handle_t command_queue = nullptr;
  ze_command_list_handle_t command_list = nullptr;
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_ZE_MANDELBROT_HPP
