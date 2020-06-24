/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_OCL_MANDELBROT_HPP
#define COMPUTE_API_BENCH_OCL_MANDELBROT_HPP

#include <vector>
#include <string>
#include "ocl_workload.hpp"

namespace compute_api_bench {

class OCLMandelbrot : public OCLWorkload {
public:
  OCLMandelbrot(unsigned int width, unsigned int height,
                unsigned int num_iterations);
  ~OCLMandelbrot();

  void build_program();
  void create_buffers();
  void create_cmdlist();
  void execute_work();
  bool verify_results();
  void cleanup();

private:
  cl_kernel kernel = NULL;
  cl_mem memobjResult = NULL;
  unsigned int width;
  unsigned int height;
  unsigned int num_iterations;
  std::vector<float> result;
  std::vector<float> result_CPU;
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_OCL_MANDELBROT_HPP
