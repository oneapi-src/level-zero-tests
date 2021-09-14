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
      public ::testing::WithParamInterface<
          std::tuple<uint64_t, float, AtomicCases_t>> {
protected:
  typedef struct AtomicsTestArguments {
    uint64_t requested_allocation_size;
    float mem_usage;
    const AtomicCases_t &atomic_subcase;
  } AtomicsTestArguments_t;

  uint32_t use_this_ordinal_on_device_ = 0;
  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernel_args_ = 1;
  uint32_t number_of_kernels_in_module_ = 1;
  uint32_t init_value_1_ = 1;
};

TEST_P(zeDriverAtomicsStressTest, RunAtomicWithMemoryLimit) {

  AtomicsTestArguments_t test_arguments = {
      std::get<0>(GetParam()), // allocation
      std::get<1>(GetParam()), // which part of max available memory to use
      std::get<2>(GetParam())  // Atomics subcase parameters
  };

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  ze_device_properties_t device_properties = lzt::get_device_properties(device);

  LOG_INFO << "TESTING on device: " << device_properties.name;
  LOG_INFO << "TESTING user ARGS: MEM_USAGE: " << test_arguments.mem_usage * 100
           << "% | SINGLE ALLOCATION SIZE: "
           << test_arguments.requested_allocation_size / (1024 * 1024) << "MB";
  LOG_INFO << "Device max available memory allocation size: "
           << device_properties.maxMemAllocSize / (1024 * 1024) << "MB";

  uint64_t max_allocation_size =
      test_arguments.mem_usage * device_properties.maxMemAllocSize;
  LOG_INFO << "Device max available memory allocation size after user "
              "limit: "
           << max_allocation_size / (1024 * 1024) << "MB";
  uint64_t one_case_requested_allocation_size =
      test_arguments.requested_allocation_size * number_of_kernel_args_;

  if (one_case_requested_allocation_size > max_allocation_size) {
    LOG_INFO << "Requested allocation size too big: "
             << test_arguments.requested_allocation_size / (1024 * 1024)
             << "MB "
             << " Limit to max available memory: "
             << max_allocation_size / (1024 * 1024) << "MB";
    one_case_requested_allocation_size = max_allocation_size;
  } else {
    LOG_INFO << "Requsted allocation size: "
             << test_arguments.requested_allocation_size / (1024 * 1024)
             << "MB ";
  }

  size_t one_case_one_allocation_count =
      one_case_requested_allocation_size /
      (number_of_kernel_args_ * sizeof(uint32_t));
  size_t one_case_one_allocation_size =
      one_case_one_allocation_count * sizeof(uint32_t);

  LOG_INFO << "One case data allocation size: "
           << one_case_one_allocation_size / (1024 * 1024) << "MB ";
  LOG_INFO << "One case data to verify count: "
           << one_case_one_allocation_count;

  LOG_INFO << "call allocation memory... ";
  uint32_t *input_allocation;

  input_allocation = (uint32_t *)lzt::allocate_device_memory(
      one_case_one_allocation_size, 8, 0, use_this_ordinal_on_device_, device,
      context);

  std::vector<uint32_t> data_out(one_case_one_allocation_count, init_value_1_);
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

  uint32_t group_count_x = one_case_one_allocation_count / workgroup_size_x_;
  ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};

  LOG_INFO << "call create cmd_list";
  ze_command_list_handle_t command_list =
      lzt::create_command_list(context, device, 0);

  LOG_INFO << "call append fill memory";
  lzt::append_memory_fill(
      command_list, input_allocation, &init_value_1_, sizeof(uint32_t),
      one_case_one_allocation_count * sizeof(uint32_t), nullptr);

  lzt::append_barrier(command_list, nullptr);

  LOG_INFO << "call launch kernel";
  lzt::append_launch_function(command_list, test_function,
                              &thread_group_dimensions, nullptr, 0, nullptr);

  lzt::append_barrier(command_list, nullptr);

  LOG_INFO << "call read results from gpu allocation";
  lzt::append_memory_copy(command_list, data_out.data(), input_allocation,
                          one_case_one_allocation_size, nullptr);

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

uint64_t one_MB = 1024UL * 1024UL;
uint64_t twenty_MB = 20 * one_MB;
uint64_t two_GB = 2UL * 1024UL * 1024UL * 1024UL;
uint64_t three_GB = 3UL * 1024UL * 1024UL * 1024UL;
uint64_t four_GB = 2UL * two_GB;
uint64_t eight_GB = 4UL * two_GB;
uint64_t sixteen_GB = 8UL * two_GB;
uint64_t two_hundred_GB = 100UL * two_GB;

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

INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap10Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       ::testing::Values(1),
                       ::testing::Values(one_operation_memory_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap10Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       ::testing::Values(1),
                       ::testing::Values(one_operation_memory_non_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap10Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       ::testing::Values(1),
                       ::testing::Values(multi_operations_memory_overlap_10)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap10Loops,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB, three_GB,
                                    four_GB, eight_GB, sixteen_GB,
                                    two_hundred_GB),
        ::testing::Values(1),
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

INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap100Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       ::testing::Values(1),
                       ::testing::Values(one_operation_memory_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap100Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB, three_GB,
                                    four_GB, eight_GB, sixteen_GB,
                                    two_hundred_GB),
        ::testing::Values(1),
        ::testing::Values(one_operation_memory_non_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap100Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       ::testing::Values(1),
                       ::testing::Values(multi_operations_memory_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap100Loops,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB, three_GB,
                                    four_GB, eight_GB, sixteen_GB,
                                    two_hundred_GB),
        ::testing::Values(1),
        ::testing::Values(multi_operations_memory_non_overlap_100)));

INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap100LoopsSmallMemoryOnly,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB),
                       ::testing::Values(1),
                       ::testing::Values(one_operation_memory_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap100LoopsSmallMemoryOnly,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values<uint64_t>(one_MB, twenty_MB), ::testing::Values(1),
        ::testing::Values(one_operation_memory_non_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap100LoopsSmallMemoryOnly,
    zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB),
                       ::testing::Values(1),
                       ::testing::Values(multi_operations_memory_overlap_100)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap100LoopsSmallMemoryOnly,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values<uint64_t>(one_MB, twenty_MB), ::testing::Values(1),
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

INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryOverlap1000Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB,
                                                   three_GB, four_GB, eight_GB,
                                                   sixteen_GB, two_hundred_GB),
                       ::testing::Values(1),
                       ::testing::Values(one_operation_memory_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsOneOperationMemoryNonOverlap1000Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB, three_GB,
                                    four_GB, eight_GB, sixteen_GB,
                                    two_hundred_GB),
        ::testing::Values(1),
        ::testing::Values(one_operation_memory_non_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryOverlap1000Loops, zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB, three_GB,
                                    four_GB, eight_GB, sixteen_GB,
                                    two_hundred_GB),
        ::testing::Values(1),
        ::testing::Values(multi_operations_memory_overlap_1000)));
INSTANTIATE_TEST_CASE_P(
    TestAtomicsMultiOperationsMemoryNonOverlap1000Loops,
    zeDriverAtomicsStressTest,
    ::testing::Combine(
        ::testing::Values<uint64_t>(one_MB, twenty_MB, two_GB, three_GB,
                                    four_GB, eight_GB, sixteen_GB,
                                    two_hundred_GB),
        ::testing::Values(1),
        ::testing::Values(multi_operations_memory_non_overlap_1000)));

} // namespace
