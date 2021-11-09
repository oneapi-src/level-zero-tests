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
#include "stress_common_func.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <bitset>

namespace {

class zeDriverMemoryAllocationStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<float, float, uint32_t, enum memory_test_type>> {
protected:
  typedef struct KernelFunctions {
    ze_kernel_desc_t test_function_description;
    ze_kernel_handle_t test_function;
  } KernelFunctions_t;

  void dispatch_kernels(const ze_device_handle_t device,
                        ze_module_handle_t module_handle,
                        std::vector<uint8_t *> src_allocations,
                        std::vector<uint8_t *> dst_allocations,
                        std::vector<std::vector<uint8_t>> &data_out,
                        const std::vector<std::string> &test_kernel_names,
                        uint32_t number_of_dispatch,
                        uint64_t one_case_allocation_count,
                        ze_context_handle_t context) {

    std::vector<ze_kernel_handle_t> test_functions;
    ze_command_list_handle_t command_list =
        lzt::create_command_list(context, device, 0);
    for (uint64_t dispatch_id = 0; dispatch_id < number_of_dispatch;
         dispatch_id++) {

      uint8_t *src_allocation = src_allocations[dispatch_id];
      uint8_t *dst_allocation = dst_allocations[dispatch_id];

      ze_kernel_handle_t kernel_handle =
          lzt::create_function(module_handle, test_kernel_names[dispatch_id]);

      lzt::set_group_size(kernel_handle, workgroup_size_x_, 1, 1);
      lzt::set_argument_value(kernel_handle, 0, sizeof(src_allocation),
                              &src_allocation);
      lzt::set_argument_value(kernel_handle, 1, sizeof(dst_allocation),
                              &dst_allocation);

      uint32_t group_count_x = one_case_allocation_count / workgroup_size_x_;
      ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};

      lzt::append_memory_fill(
          command_list, src_allocation, &init_value_2_, sizeof(uint8_t),
          one_case_allocation_count * sizeof(uint8_t), nullptr);

      lzt::append_memory_fill(
          command_list, dst_allocation, &init_value_3_, sizeof(uint8_t),
          one_case_allocation_count * sizeof(uint8_t), nullptr);

      lzt::append_launch_function(command_list, kernel_handle,
                                  &thread_group_dimensions, nullptr, 0,
                                  nullptr);
      lzt::append_barrier(command_list, nullptr);

      lzt::append_memory_copy(
          command_list, data_out[dispatch_id].data(), dst_allocation,
          data_out[dispatch_id].size() * sizeof(uint8_t), nullptr);
      lzt::append_barrier(command_list, nullptr);

      test_functions.push_back(kernel_handle);
    }

    ze_command_queue_handle_t command_queue = lzt::create_command_queue(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

    lzt::close_command_list(command_list);
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
    for (uint64_t dispatch_id = 0; dispatch_id < test_functions.size();
         dispatch_id++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelDestroy(test_functions[dispatch_id]));
    }
  }

  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernels_in_module_ = 10;
  uint8_t init_value_1_ = 0;
  uint8_t init_value_2_ = 0xAA; // 1010 1010
  uint8_t init_value_3_ = 0x55; // 0101 0101
};

TEST_P(
    zeDriverMemoryAllocationStressTest,
    AlocateFullAvailableMemoryNumberOfKernelDispatchesDependsOnUserChunkAllocaitonRequest) {

  TestArguments_t test_arguments = {
      std::get<0>(GetParam()), // total memory size limit
      std::get<1>(GetParam()), // one allocation size limit
      std::get<2>(GetParam()), // dispatch multiplier
      std::get<3>(GetParam())  // memory type
  };

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  ze_device_properties_t device_properties = lzt::get_device_properties(device);
  test_arguments.print_test_arguments(device_properties);

  std::vector<ze_device_memory_properties_t> device_memory_properties =
      lzt::get_memory_properties(device);

  uint32_t number_of_dispatches = test_arguments.multiplier;
  uint64_t number_of_all_allocations = 3 * number_of_dispatches;
  uint64_t test_single_allocation_memory_size = 0;
  uint64_t test_total_memory_size = 0;
  bool relax_memory_capability;

  adjust_max_memory_allocation(
      driver, device_properties, device_memory_properties,
      test_total_memory_size, test_single_allocation_memory_size,
      number_of_all_allocations, test_arguments.total_memory_size_limit,
      test_arguments.one_allocation_size_limit, relax_memory_capability);

  uint64_t tmp_count = test_single_allocation_memory_size / sizeof(uint8_t);
  uint64_t test_single_allocation_count =
      tmp_count - tmp_count % workgroup_size_x_;

  LOG_INFO << "Test one allocation data count: "
           << test_single_allocation_count;
  LOG_INFO << "Test number of allocations: " << number_of_all_allocations;
  LOG_INFO << "Test kernel dispatches count: " << number_of_dispatches;

  std::vector<uint8_t *> input_allocations;
  std::vector<uint8_t *> output_allocations;
  std::vector<std::vector<uint8_t>> data_out_vector;
  std::vector<std::string> test_kernel_names;

  LOG_INFO << "call allocation memory... ";
  for (uint32_t dispatch_id = 0; dispatch_id < number_of_dispatches;
       dispatch_id++) {
    uint8_t *input_allocation = allocate_memory<uint8_t>(
        context, device, test_arguments.memory_type,
        test_single_allocation_memory_size, relax_memory_capability);
    uint8_t *output_allocation = allocate_memory<uint8_t>(
        context, device, test_arguments.memory_type,
        test_single_allocation_memory_size, relax_memory_capability);

    input_allocations.push_back(input_allocation);
    output_allocations.push_back(output_allocation);

    std::vector<uint8_t> data_out(test_single_allocation_count, init_value_1_);
    data_out_vector.push_back(data_out);

    std::string kernel_name;
    kernel_name =
        "test_device_memory" +
        std::to_string((dispatch_id % number_of_kernels_in_module_) + 1);
    test_kernel_names.push_back(kernel_name);
  }

  LOG_INFO << "call create module";
  ze_module_handle_t module_handle = lzt::create_module(
      context, device, "test_multiple_memory_allocations.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);

  LOG_INFO << "call dispatch_kernels";
  dispatch_kernels(device, module_handle, input_allocations, output_allocations,
                   data_out_vector, test_kernel_names, number_of_dispatches,
                   test_single_allocation_count, context);

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
    for (uint32_t i = 0; i < test_single_allocation_count; i++) {
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

struct CombinationsTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    ss << "dispatches_" << std::get<2>(info.param);
    ss << "_memoryType_" << print_allocation_type(std::get<3>(info.param));
    return ss.str();
  }
};

std::vector<uint32_t> multiple_dispatches = {1, 10, 1000, 5000, 10000, 20000};

INSTANTIATE_TEST_CASE_P(
    TestAllocationMemoryMatrixMaxMemory, zeDriverMemoryAllocationStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(hundred_percent),
                       ::testing::ValuesIn(multiple_dispatches),
                       ::testing::Values(MTT_HOST, MTT_SHARED, MTT_DEVICE)),
    CombinationsTestNameSuffix());

INSTANTIATE_TEST_CASE_P(
    TestAllocationMemoryMatrixMinMemory, zeDriverMemoryAllocationStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(ten_percent),
                       ::testing::ValuesIn(multiple_dispatches),
                       ::testing::Values(MTT_HOST, MTT_SHARED, MTT_DEVICE)),
    CombinationsTestNameSuffix());

} // namespace
