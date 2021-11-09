/*
 *
 * Copyright (C) 2021 Intel Corporation
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

typedef struct AtomicCases {
  std::string module_name;
  std::string kernel_name;
  std::vector<uint32_t> expected_results;
} AtomicCases_t;

class zeDriverAtomicsStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<
          float, float, uint32_t, enum memory_test_type, AtomicCases_t>> {
protected:
  typedef struct AtomicsTestArguments {
    TestArguments base_arguments;
    const AtomicCases_t &atomic_subcase;
  } AtomicsTestArguments_t;

  uint32_t use_this_ordinal_on_device_ = 0;
  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernels_in_module_ = 1;
  uint32_t init_value_1_ = 1;
}; // namespace

TEST_P(zeDriverAtomicsStressTest, RunAtomicWithMemoryLimit) {

  AtomicsTestArguments_t test_arguments = {
      std::get<0>(GetParam()), // total memory size limit
      std::get<1>(GetParam()), // one allocation size limit
      std::get<2>(GetParam()), // dispatch multiplier
      std::get<3>(GetParam()), // memory type
      std::get<4>(GetParam())  // Atomics subcase parameters
  };

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  ze_device_properties_t device_properties = lzt::get_device_properties(device);
  test_arguments.base_arguments.print_test_arguments(device_properties);

  std::vector<ze_device_memory_properties_t> device_memory_properties =
      lzt::get_memory_properties(device);

  const uint64_t number_of_all_allocations = 2;
  float total_memory_size_limit =
      test_arguments.base_arguments.total_memory_size_limit;
  float one_allocation_size_limit =
      test_arguments.base_arguments.one_allocation_size_limit;
  uint64_t test_single_allocation_memory_size = 0;
  uint64_t test_total_memory_size = 0;
  bool relax_memory_capability;
  adjust_max_memory_allocation(
      driver, device_properties, device_memory_properties,
      test_total_memory_size, test_single_allocation_memory_size,
      number_of_all_allocations, total_memory_size_limit,
      one_allocation_size_limit, relax_memory_capability);

  uint64_t tmp_count = test_single_allocation_memory_size / sizeof(uint32_t);
  uint64_t test_single_allocation_count =
      tmp_count - tmp_count % workgroup_size_x_;

  LOG_INFO << "Test one allocation data count: "
           << test_single_allocation_count;
  LOG_INFO << "Test number of allocations: " << number_of_all_allocations;
  LOG_INFO << "Test kernel dispatches count: " << 1;

  LOG_INFO << "call allocation memory... ";
  uint32_t *input_allocation = allocate_memory<uint32_t>(
      context, device, test_arguments.base_arguments.memory_type,
      test_single_allocation_memory_size, relax_memory_capability);
  std::vector<uint32_t> data_out(test_single_allocation_count, init_value_1_);

  std::string kernel_name = test_arguments.atomic_subcase.kernel_name;
  std::string module_name = test_arguments.atomic_subcase.module_name;

  LOG_INFO << "call create module";
  ze_module_handle_t module_handle =
      lzt::create_module(context, device, module_name,
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);

  LOG_INFO << "call create kernel";
  ze_kernel_handle_t test_function =
      lzt::create_function(module_handle, kernel_name);

  LOG_INFO << "call set subgroup size";
  lzt::set_group_size(test_function, workgroup_size_x_, 1, 1);

  LOG_INFO << "call set kernel args 0 ";
  lzt::set_argument_value(test_function, 0, sizeof(input_allocation),
                          &input_allocation);

  uint32_t group_count_x = test_single_allocation_count / workgroup_size_x_;

  ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};

  LOG_INFO << "call create cmd_list";
  ze_command_list_handle_t command_list =
      lzt::create_command_list(context, device, 0);

  LOG_INFO << "call append fill memory";
  lzt::append_memory_fill(command_list, input_allocation, &init_value_1_,
                          sizeof(uint32_t), test_single_allocation_memory_size,
                          nullptr);

  lzt::append_barrier(command_list, nullptr);

  LOG_INFO << "call launch kernel";
  lzt::append_launch_function(command_list, test_function,
                              &thread_group_dimensions, nullptr, 0, nullptr);

  lzt::append_barrier(command_list, nullptr);

  LOG_INFO << "call read results from gpu allocation";
  lzt::append_memory_copy(command_list, data_out.data(), input_allocation,
                          test_single_allocation_memory_size, nullptr);

  lzt::append_barrier(command_list, nullptr);

  LOG_INFO << "call close command list";
  lzt::close_command_list(command_list);

  LOG_INFO << "call create command queue";
  ze_command_queue_handle_t command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  LOG_INFO << "call execute command list";
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);

  LOG_INFO << "call synchronize";
  lzt::synchronize(command_queue, UINT64_MAX);

  LOG_INFO << "call free output/input allocation memory";
  lzt::free_memory(context, input_allocation);

  lzt::destroy_context(context);
  lzt::destroy_module(module_handle);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);

  LOG_INFO << "call verification output";
  bool test_failure = false;
  size_t expected_size = test_arguments.atomic_subcase.expected_results.size();
  for (uint64_t data_out_id = 0; data_out_id < data_out.size(); data_out_id++) {

    if (test_arguments.atomic_subcase
            .expected_results[data_out_id % expected_size] !=
        data_out[data_out_id]) {
      LOG_INFO << " Difference data idx == " << data_out_id
               << " get result = " << data_out[data_out_id] << " expected = "
               << test_arguments.atomic_subcase
                      .expected_results[data_out_id % expected_size];
      test_failure = true;
      break;
    }
  }

  EXPECT_EQ(false, test_failure);
} // namespace

AtomicCases_t one_operation_memory_overlap_10 = {
    "test_atomics_10.spv", "test_atomics_add_overlap", {81}};
AtomicCases_t one_operation_memory_non_overlap_10 = {
    "test_atomics_10.spv",
    "test_atomics_add_non_overlap",
    {11, 21, 31, 41, 51, 61, 71, 81}};
AtomicCases_t multi_operations_memory_overlap_10 = {
    "test_atomics_10.spv", "test_atomics_multi_op_overlap", {12}};
AtomicCases_t multi_operations_memory_non_overlap_10 = {
    "test_atomics_10.spv",
    "test_atomics_multi_op_non_overlap",
    {9, 10, 11, 12, 13, 14, 15, 16}};

// min memory
INSTANTIATE_TEST_CASE_P(
    TestAtomicsAllMustPass, zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(one_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(one_operation_memory_overlap_10,
                          one_operation_memory_non_overlap_10,
                          multi_operations_memory_overlap_10,
                          multi_operations_memory_non_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap10LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(ten_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(one_operation_memory_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap10LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(ten_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(one_operation_memory_non_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap10LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(ten_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(multi_operations_memory_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap10LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(ten_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(multi_operations_memory_non_overlap_10)));

// max memory
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap10LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(hundred_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(one_operation_memory_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap10LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(hundred_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(one_operation_memory_non_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap10LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(hundred_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(multi_operations_memory_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap10LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(hundred_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(multi_operations_memory_non_overlap_10)));

AtomicCases_t one_operation_memory_overlap_100 = {
    "test_atomics_100.spv", "test_atomics_add_overlap", {801}};
AtomicCases_t one_operation_memory_non_overlap_100 = {
    "test_atomics_100.spv",
    "test_atomics_add_non_overlap",
    {101, 201, 301, 401, 501, 601, 701, 801}};
AtomicCases_t multi_operations_memory_overlap_100 = {
    "test_atomics_100.spv", "test_atomics_multi_op_overlap", {12}};
AtomicCases_t multi_operations_memory_non_overlap_100 = {
    "test_atomics_100.spv",
    "test_atomics_multi_op_non_overlap",
    {9, 10, 11, 12, 13, 14, 15, 16}};

// min memory
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap100LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(ten_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(one_operation_memory_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap100LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(ten_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(one_operation_memory_non_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap100LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(ten_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(multi_operations_memory_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap100LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(ten_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(multi_operations_memory_non_overlap_100)));

// max memory
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap100LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(hundred_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(one_operation_memory_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap100LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(hundred_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(one_operation_memory_non_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap100LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(hundred_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(multi_operations_memory_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap100LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(hundred_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(multi_operations_memory_non_overlap_100)));

AtomicCases_t one_operation_memory_overlap_1000 = {
    "test_atomics_1000.spv", "test_atomics_add_overlap", {8001}};
AtomicCases_t one_operation_memory_non_overlap_1000 = {
    "test_atomics_1000.spv",
    "test_atomics_add_non_overlap",
    {1001, 2001, 3001, 4001, 5001, 6001, 7001, 8001}};
AtomicCases_t multi_operations_memory_overlap_1000 = {
    "test_atomics_1000.spv", "test_atomics_multi_op_overlap", {12}};
AtomicCases_t multi_operations_memory_non_overlap_1000 = {
    "test_atomics_1000.spv",
    "test_atomics_multi_op_non_overlap",
    {9, 10, 11, 12, 13, 14, 15, 16}};

// min memory
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap1000LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(ten_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(one_operation_memory_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap1000LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(ten_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(one_operation_memory_non_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap1000LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(ten_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(multi_operations_memory_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap1000LoopsMinMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(ten_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(multi_operations_memory_non_overlap_1000)));

// max memory
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap1000LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(hundred_percent), ::testing::Values(1),
                       ::testing::Values(MTT_DEVICE),
                       ::testing::Values(one_operation_memory_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap1000LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(hundred_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(one_operation_memory_non_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap1000LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(hundred_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(multi_operations_memory_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap1000LoopsMaxMemory,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(hundred_percent),
        ::testing::Values(1), ::testing::Values(MTT_DEVICE),
        ::testing::Values(multi_operations_memory_non_overlap_1000)));
} // namespace
