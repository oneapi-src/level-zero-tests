/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ocl_simpleadd.hpp"

namespace compute_api_bench {

OCLSimpleAdd::OCLSimpleAdd(unsigned int num_elements)
    : OCLWorkload(), num(num_elements) {
  workload_name = "SimpleAdd";
  x.assign(num_elements, 1);
  y.assign(num_elements, 0);
  kernel_spv = load_binary_file(kernel_length, "ze_cabe_simpleadd.spv");
}

OCLSimpleAdd::~OCLSimpleAdd() {}

void OCLSimpleAdd::build_program() { prepare_program_from_binary(); }

void OCLSimpleAdd::create_buffers() {
  memobjX =
      clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * num, NULL, &ret);
  CL_CHECK_RESULT(ret);
  memobjY =
      clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int) * num, NULL, &ret);
  CL_CHECK_RESULT(ret);
}

void OCLSimpleAdd::create_cmdlist() {
  command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
  CL_CHECK_RESULT(ret);
  kernel = clCreateKernel(program, "NaiveAdd", &ret);
  CL_CHECK_RESULT(ret);
  CL_CHECK_RESULT(clSetKernelArg(kernel, 0, sizeof(int), &num));
  CL_CHECK_RESULT(clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&memobjX));
  CL_CHECK_RESULT(clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&memobjY));
}

void OCLSimpleAdd::execute_work() {
  CL_CHECK_RESULT(clEnqueueWriteBuffer(command_queue, memobjX, CL_TRUE, 0,
                                       sizeof(int) * num, x.data(), 0, NULL,
                                       NULL));
  CL_CHECK_RESULT(clEnqueueTask(command_queue, kernel, 0, NULL, NULL));
  CL_CHECK_RESULT(clEnqueueReadBuffer(command_queue, memobjY, CL_TRUE, 0,
                                      sizeof(int) * num, y.data(), 0, NULL,
                                      NULL));
}

bool OCLSimpleAdd::verify_results() {
  for (unsigned int i = 0; i < 10; ++i) {
    if (y[i] != 2) {
      return false;
    }
  }
  return true;
}

void OCLSimpleAdd::cleanup() {
  CL_CHECK_RESULT(clFlush(command_queue));
  CL_CHECK_RESULT(clFinish(command_queue));
  CL_CHECK_RESULT(clReleaseKernel(kernel));
  CL_CHECK_RESULT(clReleaseProgram(program));
  CL_CHECK_RESULT(clReleaseMemObject(memobjX));
  CL_CHECK_RESULT(clReleaseMemObject(memobjY));
  CL_CHECK_RESULT(clReleaseCommandQueue(command_queue));
  CL_CHECK_RESULT(clReleaseContext(context));
}

} // namespace compute_api_bench
