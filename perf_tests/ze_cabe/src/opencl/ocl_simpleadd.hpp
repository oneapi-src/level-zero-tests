/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_OCL_SIMPLEADD_HPP
#define COMPUTE_API_BENCH_OCL_SIMPLEADD_HPP

#include <vector>
#include <string>
#include "ocl_workload.hpp"

namespace compute_api_bench {

class OCLSimpleAdd : public OCLWorkload {
public:
  OCLSimpleAdd(unsigned int num_elements);
  ~OCLSimpleAdd();

  void build_program();
  void create_buffers();
  void create_cmdlist();
  void execute_work();
  bool verify_results();
  void cleanup();

private:
  cl_kernel kernel = NULL;
  cl_mem memobjX = NULL;
  cl_mem memobjY = NULL;
  int num;
  std::vector<int> x, y;
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_OCL_SIMPLEADD_HPP
