/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_OCL_WORKLOAD_HPP
#define COMPUTE_API_BENCH_OCL_WORKLOAD_HPP

#define CL_CHECK_RESULT(result)                                                \
  {                                                                            \
    if (result != CL_SUCCESS) {                                                \
      printf("Fatal : CL code is %d in %s at line %d\n", result, __FILE__,     \
             __LINE__);                                                        \
      assert(result == CL_SUCCESS);                                            \
    }                                                                          \
  }

#include "CL/cl.h"
#include <vector>
#include <string>
#include "../common/workload.hpp"

namespace compute_api_bench {

class OCLWorkload : public Workload {

public:
  OCLWorkload();
  virtual ~OCLWorkload();

protected:
  void create_device();
  void prepare_program_from_binary();
  void prepare_program_from_text();
  virtual void build_program() = 0;
  virtual void create_buffers() = 0;
  virtual void create_cmdlist() = 0;
  virtual void execute_work() = 0;
  virtual bool verify_results() = 0;
  virtual void cleanup() = 0;

  cl_platform_id platform_id = NULL;
  cl_device_id device_id = NULL;
  cl_context context = NULL;
  cl_command_queue command_queue = NULL;
  cl_program program = NULL;
  cl_uint ret_num_devices;
  cl_uint ret_num_platforms;
  cl_int ret;
  size_t kernel_length;
  std::string kernel_code;
  std::vector<uint8_t> kernel_spv;
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_OCL_WORKLOAD_HPP
