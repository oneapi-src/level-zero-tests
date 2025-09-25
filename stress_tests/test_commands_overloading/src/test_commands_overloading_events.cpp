/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "stress_common_func.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <bitset>

namespace {

using lzt::to_u32;

class zeDriverMultiplyEventsStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<double, double, uint64_t, ze_memory_type_t>> {
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

  void
  send_command_to_execution(ze_command_list_handle_t command_list,
                            const ze_command_queue_handle_t &command_queue) {
    LOG_INFO << "call close command list";
    lzt::close_command_list(command_list);
    LOG_INFO << "call execute command list";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_INFO << "call synchronize command queue";
    lzt::synchronize(command_queue, UINT64_MAX);
    // can be used in next iteration. Need to be recycled
    lzt::reset_command_list(command_list);
  };

  uint32_t workgroup_size_x_ = 8;
  uint32_t set_value_ = 0xFFFEFEFF;
}; // namespace

LZT_TEST_P(zeDriverMultiplyEventsStressTest, RunKernelDispatchesUsingEvents) {
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

  const uint32_t used_vectors_in_test = 2;
  uint64_t number_of_all_allocations =
      used_vectors_in_test * test_arguments.multiplier;

  uint64_t test_single_allocation_memory_size = 0;
  uint64_t test_total_memory_size = 0;
  bool relax_memory_capability = false;

  adjust_max_memory_allocation(
      driver, device_properties, device_memory_properties,
      test_total_memory_size, test_single_allocation_memory_size,
      number_of_all_allocations, test_arguments, relax_memory_capability);

  if (number_of_all_allocations !=
      used_vectors_in_test * test_arguments.multiplier) {

    LOG_INFO << "Need to limit dispatches from : " << test_arguments.multiplier
             << " to: " << number_of_all_allocations / used_vectors_in_test;
    test_arguments.multiplier =
        number_of_all_allocations / used_vectors_in_test;
  }
  uint64_t tmp_count = test_single_allocation_memory_size / sizeof(uint32_t);
  uint64_t test_single_allocation_count =
      tmp_count - tmp_count % workgroup_size_x_;
  LOG_INFO << "Test one allocation data count: "
           << test_single_allocation_count;
  LOG_INFO << "Test number of allocations: " << number_of_all_allocations;
  LOG_INFO << "Test kernel dispatches count: " << test_arguments.multiplier;
  bool memory_test_failure = false;

  std::vector<uint32_t *> output_allocations;
  std::vector<std::vector<uint32_t>> data_for_all_dispatches;
  std::vector<std::string> test_kernel_names;

  // prepare memory allaction and kernels names
  LOG_INFO << "call allocation memory... ";
  for (uint64_t dispatch_id = 0; dispatch_id < test_arguments.multiplier;
       dispatch_id++) {
    uint32_t *output_allocation;
    output_allocation = allocate_memory<uint32_t>(
        context, device, test_arguments.memory_type,
        test_single_allocation_memory_size, relax_memory_capability);
    output_allocations.push_back(output_allocation);

    std::vector<uint32_t> data_out(test_single_allocation_count, 0);
    data_for_all_dispatches.push_back(data_out);
  }

  LOG_INFO << "call create command queues... ";
  ze_command_queue_handle_t command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  LOG_INFO << "call create command lists... ";
  ze_command_list_handle_t command_list =
      lzt::create_command_list(context, device, 0);

  LOG_INFO << "call create modules, kernels, events, set command lists.";
  uint64_t dispatch_id = 0;
  ze_module_handle_t module_handle =
      lzt::create_module(context, device, "test_commands_overloading.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV,
                         "-ze-opt-greater-than-4GB-buffer-required", nullptr);
  std::string kernel_name = "test_device_memory1";
  ze_kernel_handle_t kernel_handle =
      create_kernel(module_handle, kernel_name, output_allocations,
                    to_u32(dispatch_id));

  uint32_t final_events_count = to_u32(3 * test_arguments.multiplier);
  LOG_INFO << "Test events count: " << final_events_count;
  ze_event_pool_desc_t event_pool_desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
      ZE_EVENT_POOL_FLAG_HOST_VISIBLE, final_events_count};
  std::vector<ze_event_handle_t> set_memory_events(test_arguments.multiplier);
  std::vector<ze_event_handle_t> exec_memory_events(test_arguments.multiplier);
  std::vector<ze_event_handle_t> read_memory_events(test_arguments.multiplier);

  ze_event_pool_handle_t memory_pool =
      lzt::create_event_pool(context, event_pool_desc);
  LOG_INFO << "call create event in number of " << test_arguments.multiplier
           << " to memory set and read";
  for (uint32_t data_idx = 0; data_idx < test_arguments.multiplier;
       data_idx++) {
    ze_event_desc_t event_desc_set = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                      3 * data_idx, 0, 0};

    ze_event_desc_t event_desc_exec = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                       3 * data_idx + 1, 0, 0};

    ze_event_desc_t event_desc_read = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                       3 * data_idx + 2, 0, 0};
    set_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_desc_set);
    exec_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_desc_exec);
    read_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_desc_read);
  }

  LOG_INFO << "call commands to copy memory in number == "
           << test_arguments.multiplier << " of steps";
  for (uint64_t data_idx = 0U; data_idx < test_arguments.multiplier;
       data_idx++) {
    uint32_t group_count_x =
        to_u32(test_single_allocation_count / workgroup_size_x_);
    ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};

    lzt::append_memory_fill(command_list, output_allocations[dispatch_id],
                            &data_idx, sizeof(uint32_t),
                            test_single_allocation_count * sizeof(uint32_t),
                            set_memory_events[data_idx]);

    lzt::append_launch_function(
        command_list, kernel_handle, &thread_group_dimensions,
        exec_memory_events[data_idx], 1, &set_memory_events[data_idx]);

    lzt::append_memory_copy(
        command_list, data_for_all_dispatches[data_idx].data(),
        output_allocations[dispatch_id],
        data_for_all_dispatches[data_idx].size() * sizeof(uint32_t),
        read_memory_events[data_idx], 1, &exec_memory_events[data_idx]);

    lzt::append_wait_on_events(command_list, 1, &read_memory_events[data_idx]);
  }

  LOG_INFO << "call send command queue to execution.";
  send_command_to_execution(command_list, command_queue);

  LOG_INFO << "call free test memory objects";
  for (auto next_allocation : output_allocations) {
    lzt::free_memory(context, next_allocation);
  }
  for (auto each_event : read_memory_events) {
    lzt::destroy_event(each_event);
  }
  for (auto each_event : exec_memory_events) {
    lzt::destroy_event(each_event);
  }
  for (auto each_event : set_memory_events) {
    lzt::destroy_event(each_event);
  }

  lzt::destroy_event_pool(memory_pool);
  lzt::destroy_function(kernel_handle);
  lzt::destroy_module(module_handle);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_context(context);

  LOG_INFO << "call verification output";
  dispatch_id = 0;
  uint64_t data_id = 0;
  for (auto each_data_out : data_for_all_dispatches) {

    for (uint32_t i = 0; i < test_single_allocation_count; i++) {
      if (data_id + dispatch_id != each_data_out[i]) {
        LOG_ERROR << "Results for dispatch ==  " << dispatch_id << " failed."
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

LZT_TEST_P(zeDriverMultiplyEventsStressTest, RunCopyBytesWithEvents) {

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

  uint64_t used_vectors_in_test = 4;
  uint64_t number_of_all_allocations =
      used_vectors_in_test * test_arguments.multiplier;

  uint64_t test_single_allocation_memory_size = 0;
  uint64_t test_total_memory_size = 0;
  bool relax_memory_capability = false;

  adjust_max_memory_allocation(
      driver, device_properties, device_memory_properties,
      test_total_memory_size, test_single_allocation_memory_size,
      number_of_all_allocations, test_arguments, relax_memory_capability);

  uint64_t test_single_allocation_count =
      test_single_allocation_memory_size / sizeof(uint32_t);

  if (number_of_all_allocations !=
      used_vectors_in_test * test_arguments.multiplier) {

    LOG_INFO << "Need to limit dispatches from : " << test_arguments.multiplier
             << " to: " << number_of_all_allocations / used_vectors_in_test;
    test_arguments.multiplier =
        number_of_all_allocations / used_vectors_in_test;
  }

  // We have relation between single allocation size and events/multiplier
  // Need to override single allocation size/count to multiplier value
  // if these calculated values are bigger than requested
  // No need to adjust run with user event multiplier request
  if (test_single_allocation_count > test_arguments.multiplier) {
    test_single_allocation_count = test_arguments.multiplier;
    test_single_allocation_memory_size =
        sizeof(uint32_t) * test_single_allocation_count;
    LOG_INFO << "Test choose single allocation memory size: "
             << (float)test_single_allocation_memory_size / (1024 * 1024)
             << "MB";
  }
  uint32_t final_events_count =to_u32(2 * test_single_allocation_count);

  LOG_INFO << "Test one allocation data count: "
           << test_single_allocation_count;
  LOG_INFO << "test_single_allocation_memory_size: "
           << test_single_allocation_memory_size;
  LOG_INFO << "Test number of allocations: " << number_of_all_allocations;
  LOG_INFO << "Test events count: " << final_events_count;
  bool memory_test_failure = false;

  std::vector<uint32_t *> output_allocations;
  std::vector<std::vector<uint32_t>> data_for_all_dispatches;
  LOG_INFO << "call allocation memory... ";
  for (uint32_t data_idx = 0; data_idx < test_single_allocation_count;
       data_idx++) {
    uint32_t *output_allocation;
    output_allocation = allocate_memory<uint32_t>(
        context, device, test_arguments.memory_type,
        test_single_allocation_memory_size, relax_memory_capability);
    output_allocations.push_back(output_allocation);

    std::vector<uint32_t> data_out(test_single_allocation_count, 0);
    data_for_all_dispatches.push_back(data_out);
  }

  LOG_INFO << "call create command queues... ";
  ze_command_queue_handle_t command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  LOG_INFO << "call create command lists... ";
  ze_command_list_handle_t command_list =
      lzt::create_command_list(context, device, 0);

  ze_event_pool_desc_t event_pool_desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
      ZE_EVENT_POOL_FLAG_HOST_VISIBLE, final_events_count};
  std::vector<ze_event_handle_t> set_memory_events(
      test_single_allocation_count);
  std::vector<ze_event_handle_t> read_memory_events(
      test_single_allocation_count);

  ze_event_pool_handle_t memory_pool =
      lzt::create_event_pool(context, event_pool_desc);

  LOG_INFO << "call create event in number of " << test_single_allocation_count
           << " to memory set and read";
  for (uint32_t data_idx = 0; data_idx < test_single_allocation_count;
       data_idx++) {
    ze_event_desc_t event_desc_set = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                      2 * data_idx, 0,
                                      ZE_EVENT_SCOPE_FLAG_HOST};

    ze_event_desc_t event_desc_read = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                       2 * data_idx + 1, 0,
                                       ZE_EVENT_SCOPE_FLAG_HOST};

    set_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_desc_set);
    read_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_desc_read);
  }

  LOG_INFO << "call commands to copy memory in number == "
           << test_single_allocation_count << " of steps";
  for (uint32_t data_idx = 0; data_idx < test_single_allocation_count;
       data_idx++) {
    lzt::append_memory_fill(command_list, &output_allocations[data_idx][0],
                            &data_idx, sizeof(uint32_t), sizeof(uint32_t),
                            set_memory_events[data_idx]);

    lzt::append_memory_copy(
        command_list, &data_for_all_dispatches[data_idx][0],
        &output_allocations[data_idx][0], (size_t)sizeof(uint32_t),
        read_memory_events[data_idx], 1, &set_memory_events[data_idx]);

    lzt::append_wait_on_events(command_list, 1, &read_memory_events[data_idx]);
  }

  LOG_INFO << "call send commands for execution ";
  send_command_to_execution(command_list, command_queue);

  LOG_INFO << "call free test memory objects";
  for (auto next_allocation : output_allocations) {
    lzt::free_memory(context, next_allocation);
  }
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);

  for (auto each_event : read_memory_events) {
    lzt::destroy_event(each_event);
  }
  for (auto each_event : set_memory_events) {
    lzt::destroy_event(each_event);
  }

  lzt::destroy_event_pool(memory_pool);
  lzt::destroy_context(context);

  LOG_INFO << "call verification output";
  for (uint64_t byte_idx = 0; byte_idx < test_single_allocation_count;
       byte_idx++) {
    if (data_for_all_dispatches[byte_idx][0] != byte_idx) {
      LOG_ERROR << "Results for byte offset ==  " << byte_idx << " failed. "
                << "Found = 0x"
                << std::bitset<32>(data_for_all_dispatches[byte_idx][0])
                << " expected = 0x" << std::bitset<32>(byte_idx);
      memory_test_failure = true;
      break;
    }
  }

  EXPECT_EQ(false, memory_test_failure);
}

struct CombinationsTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    ss << "events_" << std::get<2>(info.param);
    return ss.str();
  }
};

std::vector<uint64_t> multiple_events = {1,    32,   64,   128,   256,  512,
                                         1024, 5000, 9000, 10000, 50000};

INSTANTIATE_TEST_CASE_P(
    TestEventsMatrixMinMemory, zeDriverMultiplyEventsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(one_percent),
                       testing::ValuesIn(multiple_events),
                       testing::Values(ZE_MEMORY_TYPE_DEVICE)),
    CombinationsTestNameSuffix());
INSTANTIATE_TEST_CASE_P(
    TestEventsMatrixMaxMemory, zeDriverMultiplyEventsStressTest,
    ::testing::Combine(::testing::Values(hundred_percent),
                       ::testing::Values(hundred_percent),
                       testing::ValuesIn(multiple_events),
                       testing::Values(ZE_MEMORY_TYPE_DEVICE)),
    CombinationsTestNameSuffix());

} // namespace
