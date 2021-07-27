/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_blackscholes.hpp"

namespace compute_api_bench {

template <class T>
ZeBlackScholes<T>::ZeBlackScholes(BlackScholesData<T> *bs_io_data,
                                  unsigned int num_iterations)
    : ZeWorkload(), num_options(bs_io_data->num_options),
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

template <class T> ZeBlackScholes<T>::~ZeBlackScholes() {}

template <class T> void ZeBlackScholes<T>::build_program() {
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;

  module_description.pNext = nullptr;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = kernel_length;
  module_description.pInputModule = kernel_spv.data();
  module_description.pBuildFlags = nullptr;
  ZE_CHECK_RESULT(
      zeModuleCreate(context, device, &module_description, &module, nullptr));

  ze_kernel_desc_t function_description = {};
  function_description.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
  function_description.pNext = nullptr;
  function_description.flags = 0;
  function_description.pKernelName = "blackscholes";
  ZE_CHECK_RESULT(zeKernelCreate(module, &function_description, &function));
}

template <class T> void ZeBlackScholes<T>::create_buffers() {
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  ZE_CHECK_RESULT(zeMemAllocDevice(context, &device_desc, buffer_size, 1,
                                   device, &mem_option_years));
  ZE_CHECK_RESULT(zeMemAllocDevice(context, &device_desc, buffer_size, 1,
                                   device, &mem_option_strike));
  ZE_CHECK_RESULT(zeMemAllocDevice(context, &device_desc, buffer_size, 1,
                                   device, &mem_stock_price));
  ZE_CHECK_RESULT(zeMemAllocDevice(context, &device_desc, buffer_size, 1,
                                   device, &mem_put_result));
  ZE_CHECK_RESULT(zeMemAllocDevice(context, &device_desc, buffer_size, 1,
                                   device, &mem_call_result));
}

template <class T> void ZeBlackScholes<T>::create_cmdlist() {
  uint32_t group_size_x = 256;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  ZE_CHECK_RESULT(
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));

  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 0, sizeof(T), &riskfree));
  ZE_CHECK_RESULT(
      zeKernelSetArgumentValue(function, 1, sizeof(T), &volatility));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(
      function, 2, sizeof(mem_option_years), &mem_option_years));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(
      function, 3, sizeof(mem_option_strike), &mem_option_strike));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 4, sizeof(mem_stock_price),
                                           &mem_stock_price));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 5, sizeof(mem_call_result),
                                           &mem_call_result));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 6, sizeof(mem_put_result),
                                           &mem_put_result));

  ze_command_list_desc_t command_list_description = {};
  command_list_description.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
  command_list_description.pNext = nullptr;
  ZE_CHECK_RESULT(zeCommandListCreate(
      context, device, &command_list_description, &command_list));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(
      command_list, mem_option_years, option_years.data(), buffer_size, nullptr,
      0, nullptr));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(
      command_list, mem_option_strike, option_strike.data(), buffer_size,
      nullptr, 0, nullptr));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(command_list, mem_stock_price,
                                                stock_price.data(), buffer_size,
                                                nullptr, 0, nullptr));
  ZE_CHECK_RESULT(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));

  ze_group_count_t group_count;
  group_count.groupCountX = num_options / group_size_x;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;
  for (unsigned int i = 0; i < num_iterations; ++i) {
    ZE_CHECK_RESULT(zeCommandListAppendLaunchKernel(
        command_list, function, &group_count, nullptr, 0, nullptr));
  }
  ZE_CHECK_RESULT(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(
      command_list, call_result.data(), mem_call_result, buffer_size, nullptr,
      0, nullptr));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(command_list, put_result.data(),
                                                mem_put_result, buffer_size,
                                                nullptr, 0, nullptr));
  ZE_CHECK_RESULT(zeCommandListClose(command_list));

  const uint32_t command_queue_id = 0;
  ze_command_queue_desc_t command_queue_description = {};
  command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  command_queue_description.pNext = nullptr;
  command_queue_description.ordinal = command_queue_id;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
  ZE_CHECK_RESULT(zeCommandQueueCreate(
      context, device, &command_queue_description, &command_queue));
}

template <class T> void ZeBlackScholes<T>::execute_work() {
  ZE_CHECK_RESULT(zeCommandQueueExecuteCommandLists(command_queue, 1,
                                                    &command_list, nullptr));
  ZE_CHECK_RESULT(zeCommandQueueSynchronize(command_queue, UINT64_MAX));
}

template <class T> bool ZeBlackScholes<T>::verify_results() {
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

template <class T> void ZeBlackScholes<T>::cleanup() {
  ZE_CHECK_RESULT(zeCommandQueueDestroy(command_queue));
  ZE_CHECK_RESULT(zeCommandListDestroy(command_list));
  ZE_CHECK_RESULT(zeKernelDestroy(function));
  ZE_CHECK_RESULT(zeModuleDestroy(module));
  ZE_CHECK_RESULT(zeMemFree(context, mem_option_years));
  ZE_CHECK_RESULT(zeMemFree(context, mem_option_strike));
  ZE_CHECK_RESULT(zeMemFree(context, mem_stock_price));
  ZE_CHECK_RESULT(zeMemFree(context, mem_call_result));
  ZE_CHECK_RESULT(zeMemFree(context, mem_put_result));
  ZE_CHECK_RESULT(zeContextDestroy(context));
}

} // namespace compute_api_bench
