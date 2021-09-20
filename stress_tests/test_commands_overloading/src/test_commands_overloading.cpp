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

class zeDriverSpreadKernelsStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<uint64_t, bool, bool, bool>> {
protected:
  void dispatch_kernels(const ze_device_handle_t device,
                        const std::vector<ze_module_handle_t> &module,
                        std::vector<uint32_t *> memory_allocations,
                        std::vector<std::vector<uint32_t>> &data_out,
                        const std::vector<std::string> &test_kernel_names,
                        uint32_t dispatch_count, uint32_t one_case_data_count,
                        ze_context_handle_t context,
                        bool separate_command_lists,
                        bool separate_command_queues, bool separate_modules) {

    std::vector<ze_kernel_handle_t> test_functions;

    ze_command_list_handle_t current_command_list = nullptr;
    std::vector<ze_command_list_handle_t> command_lists;

    ze_command_queue_handle_t current_command_queue = nullptr;
    std::vector<ze_command_queue_handle_t> command_queues;

    if (separate_command_lists || separate_command_queues) {

      for (uint32_t dispatch_id = 0; dispatch_id < dispatch_count;
           dispatch_id++) {
        if (separate_command_queues) {
          current_command_queue = lzt::create_command_queue(
              context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
              ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
          command_queues.push_back(current_command_queue);
        }
        if (separate_command_lists) {
          current_command_list = lzt::create_command_list(context, device, 0);
          command_lists.push_back(current_command_list);
        }
      }
    }
    if (!separate_command_queues) {
      current_command_queue = lzt::create_command_queue(
          context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
      command_queues.push_back(current_command_queue);
    }
    if (!separate_command_lists) {
      current_command_list = lzt::create_command_list(context, device, 0);
    }

    for (uint32_t dispatch_id = 0; dispatch_id < dispatch_count;
         dispatch_id++) {

      if (separate_command_lists) {
        current_command_list = command_lists[dispatch_id];
      }

      ze_kernel_handle_t test_function = nullptr;
      if (separate_modules) {
        test_function = lzt::create_function(module[dispatch_id],
                                             test_kernel_names[dispatch_id]);
      } else {
        test_function =
            lzt::create_function(module[0], test_kernel_names[dispatch_id]);
      }
      lzt::set_group_size(test_function, workgroup_size_x_, 1, 1);
      lzt::set_argument_value(test_function, 0,
                              sizeof(memory_allocations[dispatch_id]),
                              &memory_allocations[dispatch_id]);

      uint32_t group_count_x = one_case_data_count / workgroup_size_x_;
      ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};
      init_value_ = dispatch_id;

      lzt::append_memory_fill(
          current_command_list, memory_allocations[dispatch_id], &init_value_,
          sizeof(uint32_t), one_case_data_count * sizeof(uint32_t), nullptr);

      lzt::append_barrier(current_command_list, nullptr);

      lzt::append_launch_function(current_command_list, test_function,
                                  &thread_group_dimensions, nullptr, 0,
                                  nullptr);

      lzt::append_barrier(current_command_list, nullptr);

      lzt::append_memory_copy(
          current_command_list, data_out[dispatch_id].data(),
          memory_allocations[dispatch_id],
          data_out[dispatch_id].size() * sizeof(uint32_t), nullptr);

      lzt::append_barrier(current_command_list, nullptr);

      test_functions.push_back(test_function);
    }
    auto it_command_queues = command_queues.begin();
    if (separate_command_lists) {
      for (auto command_list : command_lists) {
        lzt::close_command_list(command_list);
        if (separate_command_queues) {
          current_command_queue = *it_command_queues;
          it_command_queues++;
        }
        lzt::execute_command_lists(current_command_queue, 1, &command_list,
                                   nullptr);
        lzt::synchronize(current_command_queue, UINT64_MAX);
      }
    } else {
      lzt::close_command_list(current_command_list);
      if (separate_command_queues) {
        for (auto command_queue : command_queues) {
          lzt::execute_command_lists(command_queue, 1, &current_command_list,
                                     nullptr);
          lzt::synchronize(command_queue, UINT64_MAX);
        }
      } else {
        lzt::execute_command_lists(current_command_queue, 1,
                                   &current_command_list, nullptr);
        lzt::synchronize(current_command_queue, UINT64_MAX);
      }
    }

    if (!separate_command_queues) {
      lzt::destroy_command_queue(current_command_queue);
    }
    if (!separate_command_lists) {
      lzt::destroy_command_list(current_command_list);
    }
    if (separate_command_lists) {
      for (auto command_list : command_lists) {
        lzt::destroy_command_list(command_list);
      }
    }
    if (separate_command_queues) {
      for (auto command_queue : command_queues) {
        lzt::destroy_command_queue(command_queue);
      }
    }

    for (uint64_t dispatch_id = 0; dispatch_id < test_functions.size();
         dispatch_id++) {
      lzt::destroy_function(test_functions[dispatch_id]);
    }
  }

  typedef struct TestArguments {
    uint64_t requested_dispatches;
    bool separate_modules;
    bool separate_command_lists;
    bool separate_command_queues;

  } TestArguments_t;

  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernel_args_ = 1;
  uint32_t number_of_kernels_in_module_ = 1;
  uint32_t init_value_ = 0;
  uint32_t requested_allocation_size_ = 32 * sizeof(uint32_t);
}; // namespace

TEST_P(
    zeDriverSpreadKernelsStressTest,
    MultiplyKernelDispatchesSpreadAmongModulesCommandsListsAndCommandQueues) {

  TestArguments_t test_arguments = {
      std::get<0>(GetParam()), // dispatches
      std::get<1>(GetParam()), // separated modules mode
      std::get<2>(GetParam()), // separated command lists mode
      std::get<3>(GetParam())  // separated command queues mode

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
  LOG_INFO << "TESTING user ARGS: DISPATCHES AMOUNT: "
           << test_arguments.requested_dispatches
           << " | SINGLE ALLOCATION SIZE: " << requested_allocation_size_
           << " Bytes"
           << " | MULTIPLE COMMAND QUEUES = "
           << (test_arguments.separate_command_queues ? "yes" : "no")
           << " | MULTIPLE COMMAND LISTS = "
           << (test_arguments.separate_command_lists ? "yes" : "no")
           << " | MULTIPLE MODULES = "
           << (test_arguments.separate_modules ? "yes" : "no");

  LOG_INFO << "Device max available memory allocation size: "
           << device_properties.maxMemAllocSize / (1024 * 1024) << "MB";

  uint64_t max_allocation_size = device_properties.maxMemAllocSize;
  uint64_t number_of_requested_dispatches = test_arguments.requested_dispatches;
  bool memory_test_failure = false;

  uint64_t full_test_requested_allocation_size =
      requested_allocation_size_ * number_of_requested_dispatches *
      number_of_kernel_args_;

  uint64_t requested_dispatch_allocation_size =
      requested_allocation_size_ * number_of_kernel_args_;

  while (full_test_requested_allocation_size > max_allocation_size) {
    number_of_requested_dispatches -= 1;
    full_test_requested_allocation_size = number_of_requested_dispatches *
                                          test_arguments.requested_dispatches *
                                          number_of_kernel_args_;
    ASSERT_NE(0, number_of_requested_dispatches)
        << "Too small memory on the device to run this test!";
  }

  if (number_of_requested_dispatches != test_arguments.requested_dispatches) {
    LOG_INFO << "Number of dispatches has beed decresed from"
             << test_arguments.requested_dispatches
             << " to value : " << number_of_requested_dispatches;
  }

  size_t one_case_data_count =
      requested_dispatch_allocation_size / sizeof(uint32_t);

  LOG_INFO << "One case data allocation size: "
           << requested_dispatch_allocation_size << " Bytes";
  LOG_INFO << "Number of data elements for one allocation: "
           << one_case_data_count;
  LOG_INFO << "Number of kernel dispatches: " << number_of_requested_dispatches;
  LOG_INFO << "Total max memory used by test: "
           << number_of_requested_dispatches *
                  requested_dispatch_allocation_size
           << " Bytes";

  std::vector<uint32_t *> output_allocations;
  std::vector<std::vector<uint32_t>> data_for_all_dispatches;
  std::vector<std::string> test_kernel_names;
  std::vector<ze_module_handle_t> multiple_module_handle;
  std::string kernel_file_name = "test_commands_overloading.spv";

  LOG_INFO << "call allocation memory... ";
  for (uint64_t dispatch_id = 0; dispatch_id < number_of_requested_dispatches;
       dispatch_id++) {
    uint32_t *output_allocation;

    output_allocation = (uint32_t *)lzt::allocate_host_memory(
        requested_dispatch_allocation_size, 32, context);
    output_allocations.push_back(output_allocation);

    std::vector<uint32_t> data_out(one_case_data_count, init_value_);
    data_for_all_dispatches.push_back(data_out);

    std::string kernel_name = "test_device_memory1";
    test_kernel_names.push_back(kernel_name);
    if (test_arguments.separate_modules || dispatch_id == 0) {
      LOG_DEBUG << "call create module";
      ze_module_handle_t module_handle =
          lzt::create_module(context, device, kernel_file_name,
                             ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
      multiple_module_handle.push_back(module_handle);
    }
  }

  LOG_INFO << "call create module";
  ze_module_handle_t module_handle =
      lzt::create_module(context, device, kernel_file_name,
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);

  LOG_INFO << "call dispatch_kernels";
  dispatch_kernels(device, multiple_module_handle, output_allocations,
                   data_for_all_dispatches, test_kernel_names,
                   number_of_requested_dispatches, one_case_data_count, context,
                   test_arguments.separate_command_lists,
                   test_arguments.separate_command_queues,
                   test_arguments.separate_modules);

  LOG_INFO << "call free output allocation memory";
  for (auto each_allocation : output_allocations) {
    lzt::free_memory(context, each_allocation);
  }

  LOG_INFO << "call destroy multiple modules";
  for (auto next_module : multiple_module_handle) {
    lzt::destroy_module(next_module);
  }

  lzt::destroy_context(context);

  LOG_INFO << "call destroy module";
  lzt::destroy_module(module_handle);

  LOG_INFO << "call verification output";
  uint64_t dispatch_id = 0;
  uint64_t data_id = 0;
  for (auto each_data_out : data_for_all_dispatches) {
    for (uint32_t i = 0; i < one_case_data_count; i++) {
      if (data_id + dispatch_id != each_data_out[i]) {
        LOG_ERROR << "Index of difference " << i
                  << " found = " << each_data_out[i]
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
    ss << "dispatches_" << std::get<0>(info.param) << "_spread_modules_"
       << std::get<1>(info.param) << "_spread_cmd_lists_"
       << std::get<2>(info.param) << "_spread_cmd_queues_"
       << std::get<3>(info.param);
    return ss.str();
  }
};

std::vector<uint64_t> input_values = {1,     4,     100,   1000,
                                      10000, 40000, 80000, 1000000};
std::vector<uint64_t> dispatches = input_values;
std::vector<bool> use_separate_modules = {true, false};
std::vector<bool> use_separate_command_lists = {true, false};
std::vector<bool> use_separate_command_queues = {true, false};

INSTANTIATE_TEST_CASE_P(
    TestKernelsModulsCmdListsCmdQueuesMatrix, zeDriverSpreadKernelsStressTest,
    ::testing::Combine(::testing::ValuesIn(dispatches),
                       ::testing::ValuesIn(use_separate_command_lists),
                       ::testing::ValuesIn(use_separate_command_queues),
                       ::testing::ValuesIn(use_separate_modules)),
    CombinationsTestNameSuffix());

} // namespace
