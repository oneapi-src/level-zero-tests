/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_OCL_SOBEL_HPP
#define COMPUTE_API_BENCH_OCL_SOBEL_HPP

#include <vector>
#include <string>
#include "ocl_workload.hpp"

namespace compute_api_bench {

class OCLSobel : public OCLWorkload {
public:
  OCLSobel(level_zero_tests::ImageBMP8Bit image, unsigned int num_iterations);
  ~OCLSobel();

  void build_program();
  void create_buffers();
  void create_cmdlist();
  void execute_work();
  bool verify_results();
  void cleanup();

private:
  cl_kernel kernel = NULL;
  cl_mem memobj_original = NULL;
  cl_mem memobj_filtered = NULL;
  uint32_t image_buffer_size;
  std::vector<uint32_t> lena_original;
  std::vector<uint32_t> lena_filtered_GPU;
  std::vector<uint32_t> lena_filtered_CPU;
  unsigned int num_iterations;
  unsigned int width;
  unsigned int height;
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_OCL_SOBEL_HPP
