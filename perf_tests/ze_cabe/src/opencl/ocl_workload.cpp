/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ocl_workload.hpp"

namespace compute_api_bench {

OCLWorkload::OCLWorkload() : Workload() { workload_api = "OpenCL"; }

OCLWorkload::~OCLWorkload() {}

void OCLWorkload::create_device() {
  CL_CHECK_RESULT(clGetPlatformIDs(1, &platform_id, &ret_num_platforms));
  CL_CHECK_RESULT(clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id,
                                 &ret_num_devices));
  context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
  CL_CHECK_RESULT(ret);
}

void OCLWorkload::prepare_program_from_binary() {
  program =
      clCreateProgramWithIL(context, kernel_spv.data(), kernel_length, &ret);
  CL_CHECK_RESULT(ret);
  CL_CHECK_RESULT(clBuildProgram(program, 1, &device_id, NULL, NULL, NULL));
}

void OCLWorkload::prepare_program_from_text() {
  const char *code = kernel_code.c_str();
  program = clCreateProgramWithSource(context, 1, (const char **)&code,
                                      &kernel_length, &ret);
  CL_CHECK_RESULT(ret);
  CL_CHECK_RESULT(clBuildProgram(program, 1, &device_id, NULL, NULL, NULL));
}

} // namespace compute_api_bench
