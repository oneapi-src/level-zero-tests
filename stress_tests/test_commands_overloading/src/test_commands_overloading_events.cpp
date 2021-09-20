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

class zeDriverMultiplyEventsStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<uint64_t> {
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
  };

  typedef struct TestArguments {
    uint64_t multiplier;

  } TestArguments_t;

  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernel_args_ = 1;
  uint32_t init_value_ = 0;
  uint32_t set_value_ = 0xFFFEFEFF;
  uint32_t requested_allocation_size_ = 32 * sizeof(uint32_t);
}; // namespace

TEST_P(zeDriverMultiplyEventsStressTest, RunKernelDispatchesUsingEvents) {

  TestArguments_t test_arguments = {GetParam()};

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  ze_device_properties_t device_properties = lzt::get_device_properties(device);

  LOG_INFO << "TESTING on device: " << device_properties.name;
  LOG_INFO << "TESTING user ARGS: single allocation size: "
           << requested_allocation_size_ << " Bytes"
           << " | Events multiplication = " << test_arguments.multiplier;

  LOG_INFO << "Device max available memory allocation size: "
           << device_properties.maxMemAllocSize / (1024 * 1024) << "MB";

  uint64_t number_of_requested_dispatches = test_arguments.multiplier;
  bool memory_test_failure = false;

  uint64_t full_test_requested_allocation_size =
      requested_allocation_size_ * number_of_requested_dispatches *
      number_of_kernel_args_;

  uint64_t requested_dispatch_allocation_size =
      requested_allocation_size_ * number_of_kernel_args_;

  ASSERT_GE(device_properties.maxMemAllocSize,
            full_test_requested_allocation_size)
      << "Too small memory on the device to run this test!";

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

  // prepare memory allaction and kernels names
  LOG_INFO << "call allocation memory... ";
  for (uint64_t dispatch_id = 0; dispatch_id < number_of_requested_dispatches;
       dispatch_id++) {
    uint32_t *output_allocation;
    output_allocation = (uint32_t *)lzt::allocate_host_memory(
        requested_dispatch_allocation_size, 32, context);
    output_allocations.push_back(output_allocation);

    std::vector<uint32_t> data_out(one_case_data_count, init_value_);
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
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  std::string kernel_name = "test_device_memory1";
  ze_kernel_handle_t kernel_handle = create_kernel(
      module_handle, kernel_name, output_allocations, dispatch_id);

  uint32_t final_events_count = 3 * test_arguments.multiplier;
  ze_event_pool_desc_t event_pool_desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
      ZE_EVENT_POOL_FLAG_HOST_VISIBLE, final_events_count};
  std::vector<ze_event_handle_t> set_memory_events(test_arguments.multiplier);
  std::vector<ze_event_handle_t> exec_memory_events(test_arguments.multiplier);
  std::vector<ze_event_handle_t> read_memory_events(test_arguments.multiplier);
  std::vector<ze_event_desc_t> event_descriptions;

  ze_event_pool_handle_t memory_pool =
      lzt::create_event_pool(context, event_pool_desc);
  LOG_INFO << "call create event in number of " << test_arguments.multiplier
           << " to memory set and read";
  for (uint32_t data_idx = 0; data_idx < test_arguments.multiplier;
       data_idx++) {
    ze_event_desc_t event_desc_set = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                      2 * data_idx, 0,
                                      ZE_EVENT_SCOPE_FLAG_HOST};
    event_descriptions.push_back(event_desc_set);

    ze_event_desc_t event_desc_exec = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                       2 * data_idx + 1, 0,
                                       ZE_EVENT_SCOPE_FLAG_HOST};
    event_descriptions.push_back(event_desc_exec);

    ze_event_desc_t event_desc_read = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                       2 * data_idx + 2, 0,
                                       ZE_EVENT_SCOPE_FLAG_HOST};
    event_descriptions.push_back(event_desc_read);
    set_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_descriptions[2 * data_idx]);
    exec_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_descriptions[2 * data_idx + 1]);
    read_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_descriptions[2 * data_idx + 2]);
  }

  LOG_INFO << "call commands to copy memory in number == "
           << test_arguments.multiplier << " of steps";
  for (uint32_t data_idx = 0; data_idx < test_arguments.multiplier;
       data_idx++) {
    uint32_t group_count_x = one_case_data_count / workgroup_size_x_;
    ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};
    init_value_ = data_idx;

    lzt::append_memory_fill(command_list, output_allocations[dispatch_id],
                            &init_value_, sizeof(uint32_t),
                            one_case_data_count * sizeof(uint32_t),
                            set_memory_events[data_idx]);

    lzt::append_launch_function(
        command_list, kernel_handle, &thread_group_dimensions,
        exec_memory_events[data_idx], 1, &set_memory_events[data_idx]);

    lzt::append_memory_copy(
        command_list, data_for_all_dispatches[data_idx].data(),
        output_allocations[dispatch_id],
        data_for_all_dispatches[data_idx].size() * sizeof(uint32_t),
        read_memory_events[data_idx], 1, &set_memory_events[data_idx]);
  }

  LOG_INFO << "call send command queue to execution.";
  send_command_to_execution(command_list, command_queue);

  LOG_INFO << "call free test memory objects";
  for (auto next_allocation : output_allocations) {
    lzt::free_memory(context, next_allocation);
  }
  lzt::destroy_event_pool(memory_pool);
  for (auto each_event : read_memory_events) {
    lzt::destroy_event(each_event);
  }
  for (auto each_event : exec_memory_events) {
    lzt::destroy_event(each_event);
  }
  for (auto each_event : set_memory_events) {
    lzt::destroy_event(each_event);
  }

  lzt::destroy_function(kernel_handle);
  lzt::destroy_module(module_handle);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_context(context);

  LOG_INFO << "call verification output";
  dispatch_id = 0;
  uint64_t data_id = 0;
  for (auto each_data_out : data_for_all_dispatches) {

    for (uint32_t i = 0; i < one_case_data_count; i++) {
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

TEST_P(zeDriverMultiplyEventsStressTest, RunCopyBytesWithEvents) {

  TestArguments_t test_arguments = {GetParam()};

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  ze_device_properties_t device_properties = lzt::get_device_properties(device);

  LOG_INFO << "TESTING on device: " << device_properties.name;
  LOG_INFO << "TESTING user ARGS: Events multiplication = "
           << test_arguments.multiplier;

  LOG_INFO << "Device max available memory allocation size: "
           << device_properties.maxMemAllocSize / (1024 * 1024) << "MB";

  uint64_t max_allocation_size = device_properties.maxMemAllocSize;
  uint64_t requested_copy_bytes = test_arguments.multiplier * sizeof(uint32_t);
  bool memory_test_failure = false;

  ASSERT_GE(max_allocation_size, requested_copy_bytes)
      << "Too small memory on the device to run this test!";

  LOG_INFO << "Allocation size to copy: " << requested_copy_bytes << " Bytes";

  LOG_INFO << "call allocation memory... ";
  uint32_t *output_allocation;
  output_allocation =
      (uint32_t *)lzt::allocate_host_memory(requested_copy_bytes, 32, context);

  std::vector<uint32_t> data_out(test_arguments.multiplier, init_value_);

  LOG_INFO << "call create command queues... ";
  ze_command_queue_handle_t command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  LOG_INFO << "call create command lists... ";
  ze_command_list_handle_t command_list =
      lzt::create_command_list(context, device, 0);
  ;

  uint32_t final_events_count = 2 * test_arguments.multiplier;
  ze_event_pool_desc_t event_pool_desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
      ZE_EVENT_POOL_FLAG_HOST_VISIBLE, final_events_count};
  std::vector<ze_event_handle_t> set_memory_events(test_arguments.multiplier);
  std::vector<ze_event_handle_t> read_memory_events(test_arguments.multiplier);
  std::vector<ze_event_desc_t> event_descriptions;

  ze_event_pool_handle_t memory_pool =
      lzt::create_event_pool(context, event_pool_desc);

  LOG_INFO << "call create event in number of " << test_arguments.multiplier
           << " to memory set and read";
  for (uint32_t data_idx = 0; data_idx < test_arguments.multiplier;
       data_idx++) {
    ze_event_desc_t event_desc_set = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                      2 * data_idx, 0,
                                      ZE_EVENT_SCOPE_FLAG_HOST};
    event_descriptions.push_back(event_desc_set);

    ze_event_desc_t event_desc_read = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                       2 * data_idx + 1, 0,
                                       ZE_EVENT_SCOPE_FLAG_HOST};
    event_descriptions.push_back(event_desc_read);

    set_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_descriptions[2 * data_idx]);
    read_memory_events[data_idx] =
        lzt::create_event(memory_pool, event_descriptions[2 * data_idx + 1]);
  }

  LOG_INFO << "call commands to copy memory in number == "
           << test_arguments.multiplier << " of steps";
  for (uint32_t data_idx = 0; data_idx < test_arguments.multiplier;
       data_idx++) {
    lzt::append_memory_fill(command_list, &output_allocation[data_idx],
                            &set_value_, sizeof(uint32_t), sizeof(uint32_t),
                            set_memory_events[data_idx]);

    lzt::append_memory_copy(
        command_list, &data_out[data_idx], &output_allocation[data_idx],
        (size_t)sizeof(uint32_t), read_memory_events[data_idx], 1,
        &set_memory_events[data_idx]);
  }

  lzt::append_wait_on_events(command_list, read_memory_events.size(),
                             read_memory_events.data());

  for (auto each_event : read_memory_events) {
    lzt::query_event(each_event, ZE_RESULT_NOT_READY);
  }

  LOG_INFO << "call send commands for execution ";
  send_command_to_execution(command_list, command_queue);

  LOG_INFO << "call query event to verify if read memory events status "
              "is completed ";
  for (auto each_event : read_memory_events) {
    lzt::query_event(each_event);
  }

  LOG_INFO << "call free test memory objects";
  lzt::free_memory(context, output_allocation);
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
  for (uint64_t byte_idx = 0; byte_idx < test_arguments.multiplier;
       byte_idx++) {
    if (data_out[byte_idx] != set_value_) {
      LOG_ERROR << "Results for byte offset ==  " << byte_idx << " failed. "
                << "Found = 0x" << std::bitset<32>(data_out[byte_idx])
                << " expected = 0x" << std::bitset<32>(set_value_);
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
    ss << "events_" << info.param;
    return ss.str();
  }
};

std::vector<uint64_t> multiple_events = {
    1, 32, 1024, 5000, 9000, 10000, 100000, 1000000, 2000000, 10000000};

INSTANTIATE_TEST_CASE_P(TestEventsMatrix, zeDriverMultiplyEventsStressTest,
                        testing::ValuesIn(multiple_events),
                        CombinationsTestNameSuffix());

} // namespace
