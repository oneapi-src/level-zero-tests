/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ocl_blackscholes.hpp"

namespace compute_api_bench {

template <class T>
OCLBlackScholes<T>::OCLBlackScholes(BlackScholesData<T> *bs_io_data,
                                    unsigned int num_iterations)
    : OCLWorkload(), num_options(bs_io_data->num_options),
      num_iterations(num_iterations) {

  buffer_size = num_options * sizeof(T);
  option_years = bs_io_data->option_years;
  option_strike = bs_io_data->option_strike;
  stock_price = bs_io_data->stock_price;
  call_result = bs_io_data->call_result;
  put_result = bs_io_data->put_result;

  if (sizeof(T) == sizeof(float)) {
    kernel_spv =
        load_binary_file(kernel_length, "ze_cabe_blackscholes_fp32.spv");
    max_delta = 1e-4;
    workload_name = "BlackScholesFP32";
  } else {
    kernel_spv =
        load_binary_file(kernel_length, "ze_cabe_blackscholes_fp64.spv");
    max_delta = 1e-13;
    workload_name = "BlackScholesFP64";
  }
}

template <class T> OCLBlackScholes<T>::~OCLBlackScholes() {}

template <class T> void OCLBlackScholes<T>::build_program() {
  prepare_program_from_binary();
}

template <class T> void OCLBlackScholes<T>::create_buffers() {
  mem_option_years =
      clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, NULL, &ret);
  CL_CHECK_RESULT(ret);
  mem_option_strike =
      clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, NULL, &ret);
  CL_CHECK_RESULT(ret);
  mem_stock_price =
      clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, NULL, &ret);
  CL_CHECK_RESULT(ret);
  mem_call_result =
      clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, NULL, &ret);
  CL_CHECK_RESULT(ret);
  mem_put_result =
      clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, NULL, &ret);
  CL_CHECK_RESULT(ret);
}

template <class T> void OCLBlackScholes<T>::create_cmdlist() {
  command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
  CL_CHECK_RESULT(ret);
  kernel = clCreateKernel(program, "blackscholes", &ret);
  CL_CHECK_RESULT(ret);
  CL_CHECK_RESULT(clSetKernelArg(kernel, 0, sizeof(T), &riskfree));
  CL_CHECK_RESULT(clSetKernelArg(kernel, 1, sizeof(T), &volatility));
  CL_CHECK_RESULT(
      clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&mem_option_years));
  CL_CHECK_RESULT(
      clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&mem_option_strike));
  CL_CHECK_RESULT(
      clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&mem_stock_price));
  CL_CHECK_RESULT(
      clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&mem_call_result));
  CL_CHECK_RESULT(
      clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&mem_put_result));
}

template <class T> void OCLBlackScholes<T>::execute_work() {
  CL_CHECK_RESULT(clEnqueueWriteBuffer(command_queue, mem_option_years, CL_TRUE,
                                       0, buffer_size, option_years.data(), 0,
                                       NULL, NULL));
  CL_CHECK_RESULT(clEnqueueWriteBuffer(command_queue, mem_option_strike,
                                       CL_TRUE, 0, buffer_size,
                                       option_strike.data(), 0, NULL, NULL));
  CL_CHECK_RESULT(clEnqueueWriteBuffer(command_queue, mem_stock_price, CL_TRUE,
                                       0, buffer_size, stock_price.data(), 0,
                                       NULL, NULL));
  size_t global_item_size = num_options;
  for (unsigned int i = 0; i < num_iterations; ++i) {
    CL_CHECK_RESULT(clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
                                           &global_item_size, NULL, 0, NULL,
                                           NULL));
  }
  CL_CHECK_RESULT(clEnqueueReadBuffer(command_queue, mem_call_result, CL_TRUE,
                                      0, buffer_size, call_result.data(), 0,
                                      NULL, NULL));
  CL_CHECK_RESULT(clEnqueueReadBuffer(command_queue, mem_put_result, CL_TRUE, 0,
                                      buffer_size, put_result.data(), 0, NULL,
                                      NULL));
}

template <class T> bool OCLBlackScholes<T>::verify_results() {
  T call_result_CPU;
  T put_result_CPU;
  // std::cout.precision(20);

  for (unsigned int i = 0; i < 10; ++i) {
    black_scholes_cpu(riskfree, volatility, option_years[i], option_strike[i],
                      stock_price[i], call_result_CPU, put_result_CPU);
    // std::cout << call_result[i] << " vs. " << call_result_CPU << std::endl;
    // std::cout << put_result[i] << " vs. " << put_result_CPU << std::endl;
    if (fabs(call_result[i] - call_result_CPU) > max_delta)
      return false;
    if (fabs(put_result[i] - put_result_CPU) > max_delta)
      return false;
  }
  return true;
}

template <class T> void OCLBlackScholes<T>::cleanup() {
  CL_CHECK_RESULT(clFlush(command_queue));
  CL_CHECK_RESULT(clFinish(command_queue));
  CL_CHECK_RESULT(clReleaseKernel(kernel));
  CL_CHECK_RESULT(clReleaseProgram(program));
  CL_CHECK_RESULT(clReleaseMemObject(mem_option_years));
  CL_CHECK_RESULT(clReleaseMemObject(mem_option_strike));
  CL_CHECK_RESULT(clReleaseMemObject(mem_stock_price));
  CL_CHECK_RESULT(clReleaseMemObject(mem_call_result));
  CL_CHECK_RESULT(clReleaseMemObject(mem_put_result));
  CL_CHECK_RESULT(clReleaseCommandQueue(command_queue));
  CL_CHECK_RESULT(clReleaseContext(context));
}

} // namespace compute_api_bench
