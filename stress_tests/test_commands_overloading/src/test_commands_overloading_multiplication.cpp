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

class zeDriverMultiplyObjectsStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>> {
protected:
  ze_kernel_handle_t create_kernel(ze_module_handle_t module,
                                   const std::string &kernel_name,
                                   std::vector<uint32_t *> &memory_allocations,
                                   uint32_t dispatch_id) {
    ze_kernel_handle_t test_function =
        lzt::create_function(module, kernel_name);
    lzt::set_group_size(test_function, workgroup_size_x_, 1, 1);
    lzt::set_argument_value(test_function, 0,
                            sizeof(memory_allocations[dispatch_id]),
                            &memory_allocations[dispatch_id]);
    return test_function;
  };

  void set_command_list(ze_command_list_handle_t &command_list,
                        const ze_command_queue_handle_t &command_queue,
                        const ze_kernel_handle_t &test_kernel,
                        uint32_t one_case_data_count, uint32_t dispatch_id,
                        std::vector<uint32_t *> &memory_allocations,
                        std::vector<std::vector<uint32_t>> &data_out) {
    uint32_t group_count_x = one_case_data_count / workgroup_size_x_;
    ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};
    init_value_ = dispatch_id;
    lzt::append_memory_fill(command_list, memory_allocations[dispatch_id],
                            &init_value_, sizeof(uint32_t),
                            one_case_data_count * sizeof(uint32_t), nullptr);

    lzt::append_barrier(command_list, nullptr);

    lzt::append_launch_function(command_list, test_kernel,
                                &thread_group_dimensions, nullptr, 0, nullptr);
    lzt::append_barrier(command_list, nullptr);

    lzt::append_memory_copy(command_list, data_out[dispatch_id].data(),
                            memory_allocations[dispatch_id],
                            data_out[dispatch_id].size() * sizeof(uint32_t),
                            nullptr);
    lzt::append_barrier(command_list, nullptr);
  };

  void
  send_command_to_execution(ze_command_list_handle_t command_list,
                            const ze_command_queue_handle_t &command_queue) {

    LOG_INFO << "call close command list";
    lzt::close_command_list(command_list);
    LOG_INFO << "call execute command list";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_INFO << "call synchronize command queue";
    lzt::synchronize(command_queue, UINT64_MAX);
  };

  typedef struct TestArguments {
    uint64_t multiply_kernels;
    uint64_t multiply_modules;
    uint64_t multiply_command_lists;
    uint64_t multiply_command_queues;

  } TestArguments_t;

  uint32_t use_this_ordinal_on_device_ = 0;
  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernel_args_ = 1;
  uint32_t number_of_kernels_in_source_ = 1;
  uint32_t init_value_ = 0;
  uint32_t requested_allocation_size_ = 32 * sizeof(uint32_t);
}; // namespace

TEST_P(zeDriverMultiplyObjectsStressTest,
       MultiplyKernelsModulsCommandsListsAndCommandQueues) {

  TestArguments_t test_arguments = {
      std::get<0>(GetParam()), // multiplcation counter - kernels
      std::get<1>(GetParam()), // multiplcation counter - modules
      std::get<2>(GetParam()), // multiplcation counter - command lists
      std::get<3>(GetParam())  // multiplcation counter - command queues

  };

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  ze_device_properties_t device_properties = lzt::get_device_properties(device);

  LOG_INFO << "TESTING on device: " << device_properties.name;
  LOG_INFO << "TESTING user ARGS: single allocation size: "
           << requested_allocation_size_ << " Bytes"
           << " | Kernels multiplication = " << test_arguments.multiply_kernels
           << " | Command queues muliplication = "
           << test_arguments.multiply_command_queues
           << " | Command lists multiplication = "
           << test_arguments.multiply_command_lists
           << " | Moudles multiplication = " << test_arguments.multiply_modules;

  LOG_INFO << "Device max available memory allocation size: "
           << device_properties.maxMemAllocSize / (1024 * 1024) << "MB";

  uint64_t max_allocation_size = device_properties.maxMemAllocSize;
  uint64_t requested_dispatches = test_arguments.multiply_kernels *
                                  test_arguments.multiply_modules *
                                  test_arguments.multiply_command_lists *
                                  test_arguments.multiply_command_queues;
  bool memory_test_failure = false;

  // base on user repetition verify if we do not reach memory limit.
  uint64_t full_test_requested_allocation_size = requested_allocation_size_ *
                                                 requested_dispatches *
                                                 number_of_kernel_args_;

  uint64_t requested_dispatch_allocation_size =
      requested_allocation_size_ * number_of_kernel_args_;

  ASSERT_GE(max_allocation_size, full_test_requested_allocation_size)
      << "Too small memory on the device to run this test!";

  size_t one_case_data_count =
      requested_dispatch_allocation_size / sizeof(uint32_t);

  LOG_INFO << "One case data allocation size: "
           << requested_dispatch_allocation_size << " Bytes";
  LOG_INFO << "Number of data elements for one allocation: "
           << one_case_data_count;
  LOG_INFO << "Number of kernel dispatches: " << requested_dispatches;
  LOG_INFO << "Total max memory used by test: "
           << requested_dispatches * requested_dispatch_allocation_size
           << " Bytes";

  std::vector<uint32_t *> output_allocations;
  std::vector<std::vector<uint32_t>> data_for_all_dispatches;
  std::vector<std::string> test_kernel_names;
  std::vector<ze_module_handle_t> multiple_module_handle;
  std::vector<ze_kernel_handle_t> multiple_kernel_handle;
  std::vector<ze_command_list_handle_t> multiple_command_list_handle;
  std::vector<ze_command_queue_handle_t> multiple_command_queue_handle;

  LOG_INFO << "call allocation memory... ";
  for (uint64_t dispatch_id = 0; dispatch_id < requested_dispatches;
       dispatch_id++) {
    uint32_t *output_allocation;
    output_allocation = (uint32_t *)lzt::allocate_host_memory(
        requested_dispatch_allocation_size, 32, context);
    output_allocations.push_back(output_allocation);

    std::vector<uint32_t> data_out(one_case_data_count, init_value_);
    data_for_all_dispatches.push_back(data_out);
  }

  LOG_INFO << "call create command queues... ";
  for (uint64_t cmd_queue_id = 0;
       cmd_queue_id < test_arguments.multiply_command_queues; cmd_queue_id++) {
    ze_command_queue_handle_t command_queue = lzt::create_command_queue(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    multiple_command_queue_handle.push_back(command_queue);
  }

  LOG_INFO << "call create command lists... ";
  for (uint64_t cmd_list_id = 0;
       cmd_list_id < test_arguments.multiply_command_lists; cmd_list_id++) {
    ze_command_list_handle_t command_list =
        lzt::create_command_list(context, device, 0);
    multiple_command_list_handle.push_back(command_list);
  }

  LOG_INFO << "call create modules and kernels, set command lists.";
  LOG_INFO << "finally send command list to execution... ";
  uint64_t dispatch_id = 0;
  for (auto next_cmd_queue : multiple_command_queue_handle) {
    for (auto next_cmd_list : multiple_command_list_handle) {
      // for (auto next_module : multiple_module_handle) {
      for (uint64_t module_id = 0; module_id < test_arguments.multiply_modules;
           module_id++) {
        ze_module_handle_t module_handle =
            lzt::create_module(context, device, "test_commands_overloading.spv",
                               ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
        multiple_module_handle.push_back(module_handle);
        for (uint64_t kernel_id = 0;
             kernel_id < test_arguments.multiply_kernels; kernel_id++) {
          std::string kernel_name = "test_device_memory1";

          ze_kernel_handle_t kernel_handle = create_kernel(
              module_handle, kernel_name, output_allocations, dispatch_id);

          multiple_kernel_handle.push_back(kernel_handle);
          set_command_list(next_cmd_list, next_cmd_queue, kernel_handle,
                           one_case_data_count, dispatch_id, output_allocations,
                           data_for_all_dispatches);
          dispatch_id++;
        }
      }

      send_command_to_execution(next_cmd_list, next_cmd_queue);
    }
  }

  LOG_INFO << "call free test memory objects";
  for (auto next_allocation : output_allocations) {
    lzt::free_memory(context, next_allocation);
  }

  for (auto next_kernel : multiple_kernel_handle) {
    lzt::destroy_function(next_kernel);
  }

  for (auto next_module : multiple_module_handle) {
    lzt::destroy_module(next_module);
  }

  for (auto next_cmd_list : multiple_command_list_handle) {
    lzt::destroy_command_list(next_cmd_list);
  }

  for (auto next_cmd_queue : multiple_command_queue_handle) {
    lzt::destroy_command_queue(next_cmd_queue);
  }
  lzt::destroy_context(context);

  LOG_INFO << "call verification output";
  dispatch_id = 0;
  uint64_t data_id = 0;
  for (auto each_data_out : data_for_all_dispatches) {

    for (uint32_t i = 0; i < one_case_data_count; i++) {
      if (data_id + dispatch_id != each_data_out[i]) {
        LOG_ERROR << "Results for dispatch ==  " << dispatch_id << " failed. "
                  << " The index " << i << " found = " << each_data_out[i]
                  << " expected = " << data_id + dispatch_id;
        memory_test_failure = true;
        break;
      }
      data_id++;
    }
    dispatch_id++;
    data_id = 0;
  }

  EXPECT_EQ(false, memory_test_failure);
}

struct CombinationsTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    ss << "kernels_" << std::get<0>(info.param) << "_modules_"
       << std::get<1>(info.param) << "_cmd_lists_" << std::get<2>(info.param)
       << "_cmd_queues_" << std::get<3>(info.param);
    return ss.str();
  }
};

std::vector<uint64_t> input_values = {1,     4,     10,    100,    1000,
                                      10000, 40000, 80000, 1000000};
std::vector<uint64_t> multiple_kernels = input_values;
std::vector<uint64_t> multiple_command_lists = input_values;
std::vector<uint64_t> multiple_command_queues = input_values;
std::vector<uint64_t> multiple_modules = input_values;

INSTANTIATE_TEST_CASE_P(
    TestKernelsModulsCmdListsCmdQueuesMatrix, zeDriverMultiplyObjectsStressTest,
    ::testing::Combine(::testing::ValuesIn(multiple_kernels),
                       ::testing::ValuesIn(multiple_modules),
                       ::testing::ValuesIn(multiple_command_lists),
                       ::testing::ValuesIn(multiple_command_queues)),
    CombinationsTestNameSuffix());

} // namespace
