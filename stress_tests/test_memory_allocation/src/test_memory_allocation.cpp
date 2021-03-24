/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <bitset>

namespace {

enum memory_test_type { MTT_SHARED, MTT_HOST, MTT_DEVICE };

std::string print_allocation_type(memory_test_type mem_type) {
  switch (mem_type) {
  case MTT_HOST:
    return "HOST";
  case MTT_SHARED:
    return "SHARED";
  case MTT_DEVICE:
    return "DEVICE";
  default:
    return "UNKNOWN";
  }
}

class zeDriverMemoryAllocationStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<uint64_t, enum memory_test_type, float>> {
protected:
  ze_module_handle_t create_module(ze_context_handle_t context,
                                   const ze_device_handle_t device,
                                   const std::string path) {
    const std::vector<uint8_t> binary_file = lzt::load_binary_file(path);

    LOG_INFO << "set up module description for path " << path;
    ze_module_desc_t module_description = {};
    module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;

    module_description.pNext = nullptr;
    module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
    module_description.inputSize = static_cast<uint32_t>(binary_file.size());
    module_description.pInputModule = binary_file.data();
    module_description.pBuildFlags = nullptr;

    ze_module_handle_t module = nullptr;
    EXPECT_EQ(
        ZE_RESULT_SUCCESS,
        zeModuleCreate(context, device, &module_description, &module, nullptr));

    LOG_INFO << "return module";
    return module;
  }
  typedef struct KernelFunctions {
    ze_kernel_desc_t test_function_description;
    ze_kernel_handle_t test_function;
  } KernelFunctions_t;

  void dispatch_kernels(const ze_device_handle_t device,
                        ze_module_handle_t module,
                        std::vector<uint8_t *> src_allocations,
                        std::vector<uint8_t *> dst_allocations,
                        std::vector<std::vector<uint8_t>> &data_out,
                        const std::vector<std::string> &test_kernel_names,
                        uint64_t number_of_dispatch,
                        uint64_t one_case_allocation_count,
                        ze_context_handle_t context) {

    ze_command_list_desc_t command_list_description = {};
    command_list_description.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;

    command_list_description.pNext = nullptr;
    std::vector<ze_kernel_handle_t> test_functions;

    ze_command_list_handle_t command_list = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListCreate(context, device, &command_list_description,
                                  &command_list));
    for (uint64_t dispatch_id = 0; dispatch_id < number_of_dispatch;
         dispatch_id++) {

      uint8_t *src_allocation = src_allocations[dispatch_id];
      uint8_t *dst_allocation = dst_allocations[dispatch_id];

      ze_kernel_desc_t test_function_description = {};
      test_function_description.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;

      test_function_description.pNext = nullptr;
      test_function_description.flags = 0;
      test_function_description.pKernelName =
          test_kernel_names[dispatch_id].c_str();
      ze_kernel_handle_t test_function = nullptr;

      EXPECT_EQ(
          ZE_RESULT_SUCCESS,
          zeKernelCreate(module, &test_function_description, &test_function));

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSetGroupSize(test_function, workgroup_size_x_, 1, 1));

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSetArgumentValue(
                    test_function, 0, sizeof(src_allocation), &src_allocation));
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSetArgumentValue(
                    test_function, 1, sizeof(dst_allocation), &dst_allocation));

      uint32_t group_count_x = one_case_allocation_count / workgroup_size_x_;
      ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendMemoryFill(command_list, src_allocation,
                                              &init_value_2_, sizeof(uint8_t),
                                              one_case_allocation_count *
                                                  sizeof(uint8_t),
                                              nullptr, 0, nullptr));

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendMemoryFill(command_list, dst_allocation,
                                              &init_value_3_, sizeof(uint8_t),
                                              one_case_allocation_count *
                                                  sizeof(uint8_t),
                                              nullptr, 0, nullptr));

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendLaunchKernel(command_list, test_function,
                                                &thread_group_dimensions,
                                                nullptr, 0, nullptr));

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendLaunchKernel(command_list, test_function,
                                                &thread_group_dimensions,
                                                nullptr, 0, nullptr));

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));

      lzt::append_memory_copy(
          command_list, data_out[dispatch_id].data(), dst_allocation,
          data_out[dispatch_id].size() * sizeof(uint8_t), nullptr);

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
      test_functions.push_back(test_function);
    }
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(command_list));

    const uint32_t command_queue_id = 0;
    ze_command_queue_desc_t command_queue_description = {};
    command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;

    command_queue_description.pNext = nullptr;
    command_queue_description.ordinal = command_queue_id;
    command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    command_queue_description.flags = 0;

    ze_command_queue_handle_t command_queue = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueCreate(context, device, &command_queue_description,
                                   &command_queue));

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                     command_queue, 1, &command_list, nullptr));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(command_queue, UINT64_MAX));

    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
    for (uint64_t dispatch_id = 0; dispatch_id < test_functions.size();
         dispatch_id++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelDestroy(test_functions[dispatch_id]));
    }
  }

  typedef struct MemoryTestArguments {
    uint64_t requested_allocation_size;
    memory_test_type mtt;
    float mem_usage;
  } MemoryTestArguments_t;

  uint32_t use_this_ordinal_on_device_ = 0;
  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernel_args_ = 2;
  uint32_t number_of_kernels_in_module_ = 10;
  uint8_t init_value_1_ = 0;
  uint8_t init_value_2_ = 0xAA; // 1010 1010
  uint8_t init_value_3_ = 0x55; // 0101 0101
};

TEST_P(
    zeDriverMemoryAllocationStressTest,
    AlocateFullAvailableMemoryNumberOfKernelDispatchesDependsOnUserChunkAllocaitonRequest) {

  MemoryTestArguments_t test_arguments = {
      std::get<0>(GetParam()), // allocation
      std::get<1>(GetParam()), // memory test type
      std::get<2>(GetParam())  // which part of max available memory to use
  };

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  ze_device_properties_t device_properties;
  device_properties.pNext = nullptr;
  device_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(device, &device_properties));

  LOG_INFO << "TESTING on device: " << device_properties.name;
  LOG_INFO << "TESTING user ARGS: MEM_USAGE: " << test_arguments.mem_usage * 100
           << "% | MEMORY TYPE: " << print_allocation_type(test_arguments.mtt)
           << " | SINGLE ALLOCATION SIZE: "
           << test_arguments.requested_allocation_size / (1024 * 1024) << "MB";
  LOG_INFO << "Device max available memory allocation size: "
           << device_properties.maxMemAllocSize / (1024 * 1024) << "MB";

  uint64_t max_allocation_size =
      test_arguments.mem_usage * workgroup_size_x_ *
      (device_properties.maxMemAllocSize / workgroup_size_x_);
  LOG_INFO << "Device max available memory allocation size after user "
              "limit: "
           << max_allocation_size / (1024 * 1024) << "MB";
  uint64_t one_case_requested_allocation_size =
      workgroup_size_x_ *
      (test_arguments.requested_allocation_size * number_of_kernel_args_) /
      workgroup_size_x_;

  if (one_case_requested_allocation_size > max_allocation_size) {
    LOG_INFO << "One dispatch allocation size too big: "
             << test_arguments.requested_allocation_size / (1024 * 1024)
             << "MB "
             << " Limit to max available memory: "
             << max_allocation_size / (1024 * 1024) << "MB";
    one_case_requested_allocation_size = max_allocation_size;
  } else {
    LOG_INFO << "One dispatch allocation size: "
             << test_arguments.requested_allocation_size;
  }

  size_t one_case_one_allocation_count =
      one_case_requested_allocation_size /
      (number_of_kernel_args_ * sizeof(uint8_t));

  uint64_t number_of_dispatch =
      max_allocation_size / one_case_requested_allocation_size;

  LOG_INFO << "One case data allocation size: "
           << one_case_requested_allocation_size << " Bytes";
  LOG_INFO << "One case data to verify count: " << one_case_one_allocation_count
           << " Bytes";
  LOG_INFO << "Number of kernel dispatches: " << number_of_dispatch;
  LOG_INFO << "Total max memory used by test: "
           << number_of_dispatch * one_case_requested_allocation_size /
                  (1024 * 1024)
           << "MB";

  std::vector<uint8_t *> input_allocations;
  std::vector<uint8_t *> output_allocations;
  std::vector<std::vector<uint8_t>> data_out_vector;
  std::vector<std::string> test_kernel_names;

  LOG_INFO << "call allocation memory... ";
  for (uint64_t dispatch_id = 0; dispatch_id < number_of_dispatch;
       dispatch_id++) {
    uint8_t *input_allocation;
    uint8_t *output_allocation;
    if (test_arguments.mtt == MTT_DEVICE) {
      input_allocation = (uint8_t *)lzt::allocate_device_memory(
          one_case_one_allocation_count, 8, 0, use_this_ordinal_on_device_,
          device, context);
      output_allocation = (uint8_t *)lzt::allocate_device_memory(
          one_case_one_allocation_count, 8, 0, use_this_ordinal_on_device_,
          device, context);
    } else if (test_arguments.mtt == MTT_HOST) {
      input_allocation = (uint8_t *)lzt::allocate_host_memory(
          one_case_one_allocation_count, 8, context);
      output_allocation = (uint8_t *)lzt::allocate_host_memory(
          one_case_one_allocation_count, 8, context);
    } else if (test_arguments.mtt == MTT_SHARED) {
      input_allocation = (uint8_t *)lzt::allocate_shared_memory(
          one_case_one_allocation_count, 8, 0, 0, device, context);
      output_allocation = (uint8_t *)lzt::allocate_shared_memory(
          one_case_one_allocation_count, 8, 0, 0, device, context);
    }
    input_allocations.push_back(input_allocation);
    output_allocations.push_back(output_allocation);

    std::vector<uint8_t> data_out(one_case_one_allocation_count, init_value_1_);
    data_out_vector.push_back(data_out);

    std::string kernel_name;
    kernel_name =
        "test_device_memory" +
        std::to_string((dispatch_id % number_of_kernels_in_module_) + 1);
    test_kernel_names.push_back(kernel_name);
  }

  LOG_INFO << "call create module";
  ze_module_handle_t module_handle =
      create_module(context, device, "test_multiple_memory_allocations.spv");

  LOG_INFO << "call dispatch_kernels";
  dispatch_kernels(device, module_handle, input_allocations, output_allocations,
                   data_out_vector, test_kernel_names, number_of_dispatch,
                   one_case_one_allocation_count, context);

  LOG_INFO << "call free input allocation memory";
  for (auto each_allocation : input_allocations) {
    lzt::free_memory(context, each_allocation);
  }
  LOG_INFO << "call free output allocation memory";
  for (auto each_allocation : output_allocations) {
    lzt::free_memory(context, each_allocation);
  }
  lzt::destroy_context(context);

  LOG_INFO << "call destroy module";
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module_handle));

  LOG_INFO << "call verification output";
  bool memory_test_failure = false;
  for (auto each_data_out : data_out_vector) {
    for (uint32_t i = 0; i < one_case_one_allocation_count; i++) {
      if (init_value_2_ != each_data_out[i]) {
        LOG_INFO << "Index of difference " << i << " found = 0x"
                 << std::bitset<8>(each_data_out[i]) << " expected = 0x"
                 << std::bitset<8>(init_value_2_);
        memory_test_failure = true;
        break;
      }
    }
  }

  EXPECT_EQ(false, memory_test_failure);
}

uint64_t one_MB = 1024UL * 1024UL;
uint64_t twenty_MB = 20 * one_MB;
uint64_t two_GB = 2UL * 1024UL * 1024UL * 1024UL;
uint64_t three_GB = 3UL * 1024UL * 1024UL * 1024UL;
uint64_t four_GB = 2UL * two_GB;
uint64_t eight_GB = 4UL * two_GB;
uint64_t sixteen_GB = 8UL * two_GB;
uint64_t two_hundred_GB = 100UL * two_GB;

INSTANTIATE_TEST_CASE_P(
    TestAllMemoryKindsUseMaxMemAllocSize, zeDriverMemoryAllocationStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       testing::Values(MTT_HOST, MTT_SHARED, MTT_DEVICE),
                       ::testing::Values(1)));
INSTANTIATE_TEST_CASE_P(
    TestAllMemoryKinds90LimitMemAllocSize, zeDriverMemoryAllocationStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       testing::Values(MTT_HOST, MTT_SHARED, MTT_DEVICE),
                       ::testing::Values(0.9)));
INSTANTIATE_TEST_CASE_P(
    TestAllMemoryKinds50LimitMemAllocSize, zeDriverMemoryAllocationStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       testing::Values(MTT_HOST, MTT_SHARED, MTT_DEVICE),
                       ::testing::Values(0.5)));
INSTANTIATE_TEST_CASE_P(
    TestAllMemoryKinds10LimitMemAllocSize, zeDriverMemoryAllocationStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       testing::Values(MTT_HOST, MTT_SHARED, MTT_DEVICE),
                       ::testing::Values(0.1)));

} // namespace
