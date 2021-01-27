/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

#include <level_zero/ze_api.h>
#include <thread>

namespace {

class EventProfilingTests : public ::testing::Test {
protected:
  void SetUp() override {
    context = lzt::create_context();
    const ze_device_handle_t device =
        lzt::zeDevice::get_instance()->get_device();
    ep = lzt::create_event_pool(context, 10,
                                (ZE_EVENT_POOL_FLAG_HOST_VISIBLE |
                                 ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP));
    ze_event_desc_t event_desc = {};
    event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    event_desc.index = 0;
    event_desc.signal = 0;
    event_desc.wait = 0;
    event = lzt::create_event(ep, event_desc);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
    size_t size = 10000;
    size_t buff_size = size * sizeof(int);
    src_buffer = lzt::allocate_host_memory(buff_size, 1, context);
    dst_buffer = lzt::allocate_host_memory(buff_size, 1, context);
    const int addval = 0x11223344;
    memset(src_buffer, 0, buff_size);
    cmdlist = lzt::create_command_list(context, device, 0);
    cmdqueue = lzt::create_command_queue(context, device, 0,
                                         ZE_COMMAND_QUEUE_MODE_DEFAULT,
                                         ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    module = lzt::create_module(context, device, "profile_add.spv",
                                ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
    kernel = lzt::create_function(module, "profile_add_constant");
    lzt::set_group_size(kernel, 1, 1, 1);
    ze_group_count_t args = {static_cast<uint32_t>(size), 1, 1};
    lzt::set_argument_value(kernel, 0, sizeof(src_buffer), &src_buffer);
    lzt::set_argument_value(kernel, 1, sizeof(dst_buffer), &dst_buffer);
    lzt::set_argument_value(kernel, 2, sizeof(addval), &addval);
    lzt::append_launch_function(cmdlist, kernel, &args, event, 0, nullptr);
  }

  void TearDown() override {

    lzt::free_memory(context, src_buffer);
    lzt::free_memory(context, dst_buffer);
    lzt::destroy_event(event);
    lzt::destroy_event_pool(ep);
    lzt::destroy_command_list(cmdlist);
    lzt::destroy_command_queue(cmdqueue);
    lzt::destroy_module(module);
    lzt::destroy_function(kernel);
    lzt::destroy_context(context);
  }

  void *src_buffer;
  void *dst_buffer;
  ze_context_handle_t context;
  ze_event_pool_handle_t ep;
  ze_event_handle_t event;
  ze_command_list_handle_t cmdlist;
  ze_command_queue_handle_t cmdqueue;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;
};

TEST_F(
    EventProfilingTests,
    GivenProfilingEventWhenCommandCompletesThenTimestampsAreRelationallyCorrect) {

  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);
  lzt::event_host_synchronize(event, UINT64_MAX);
  ze_kernel_timestamp_result_t timestamp =
      lzt::get_event_kernel_timestamp(event);

  EXPECT_GT(timestamp.global.kernelStart, 0);
  EXPECT_GT(timestamp.global.kernelEnd, 0);
  EXPECT_GT(timestamp.context.kernelStart, 0);
  EXPECT_GT(timestamp.context.kernelEnd, 0);
}

TEST_F(EventProfilingTests,
       GivenSetProfilingEventWhenResettingEventThenEventStatusNotReady) {

  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);
  lzt::event_host_synchronize(event, UINT64_MAX);
  ze_kernel_timestamp_result_t timestamp =
      lzt::get_event_kernel_timestamp(event);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryKernelTimestamp(event, &timestamp));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(event));
  EXPECT_EQ(ZE_RESULT_NOT_READY,
            zeEventQueryKernelTimestamp(event, &timestamp));
}

TEST_F(EventProfilingTests,
       GivenRegularAndTimestampEventPoolsWhenProfilingThenTimestampsStillWork) {
  const int mem_size = 1000;
  auto other_buffer = lzt::allocate_host_memory(mem_size, 1, context);
  const uint8_t value1 = 0x55;
  auto ep_no_timestamps =
      lzt::create_event_pool(context, 10, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  ze_event_desc_t regular_event_desc = {};
  regular_event_desc.index = 1;
  regular_event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  regular_event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto regular_event = lzt::create_event(ep_no_timestamps, regular_event_desc);

  lzt::append_memory_set(cmdlist, other_buffer, &value1, mem_size,
                         regular_event);
  lzt::append_wait_on_events(cmdlist, 1, &regular_event);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);
  lzt::event_host_synchronize(event, UINT64_MAX);
  lzt::event_host_synchronize(regular_event, UINT64_MAX);
  ze_kernel_timestamp_result_t timestamp =
      lzt::get_event_kernel_timestamp(event);

  EXPECT_GT(timestamp.global.kernelStart, 0);
  EXPECT_GT(timestamp.global.kernelEnd, 0);
  EXPECT_GT(timestamp.context.kernelStart, 0);
  EXPECT_GT(timestamp.context.kernelEnd, 0);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event));

  for (int i = 0; i++; i < mem_size) {
    uint8_t byte = ((uint8_t *)other_buffer)[i];
    ASSERT_EQ(byte, value1);
  }

  lzt::free_memory(context, other_buffer);
  lzt::destroy_event(regular_event);
  lzt::destroy_event_pool(ep_no_timestamps);
}

class EventProfilingCacheCoherencyTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_event_pool_flags_t> {};

TEST_P(EventProfilingCacheCoherencyTests,
       GivenEventWhenUsingEventToSyncThenCacheIsCoherent) {
  const uint32_t size = 10000;
  const uint8_t value = 0x55;
  ze_context_handle_t context = lzt::create_context();
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  auto ep = lzt::create_event_pool(context, 10, GetParam());

  auto cmdlist = lzt::create_command_list(context, device, 0);
  auto cmdqueue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  auto buffer1 = lzt::allocate_device_memory(size, 8, 0, 0, device, context);
  auto buffer2 = lzt::allocate_device_memory(size, 8, 0, 0, device, context);
  auto buffer3 = lzt::allocate_host_memory(size, 1, context);
  auto buffer4 = lzt::allocate_device_memory(size, 8, 0, 0, device, context);
  auto buffer5 = lzt::allocate_host_memory(size, 1, context);
  memset(buffer3, 0, size);
  memset(buffer5, 0, size);

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 1;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto event1 = lzt::create_event(ep, event_desc);
  event_desc.index = 2;
  auto event2 = lzt::create_event(ep, event_desc);
  event_desc.index = 3;
  auto event3 = lzt::create_event(ep, event_desc);
  event_desc.index = 4;
  auto event4 = lzt::create_event(ep, event_desc);
  event_desc.index = 5;
  auto event5 = lzt::create_event(ep, event_desc);

  lzt::append_memory_set(cmdlist, buffer1, &value, size, event1);
  lzt::append_wait_on_events(cmdlist, 1, &event1);
  lzt::append_memory_copy(cmdlist, buffer2, buffer1, size,
                          event2); // Note: Need to add wait on events
  lzt::append_wait_on_events(cmdlist, 1, &event2);
  lzt::append_memory_copy(cmdlist, buffer3, buffer2, size, event3);
  lzt::append_wait_on_events(cmdlist, 1, &event3);
  lzt::append_memory_copy(cmdlist, buffer4, buffer3, size, event4);
  lzt::append_wait_on_events(cmdlist, 1, &event4);
  lzt::append_memory_copy(cmdlist, buffer5, buffer4, size, event5);

  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event5, UINT64_MAX));
  std::unique_ptr<uint8_t> output(new uint8_t[size]);
  memcpy(output.get(), buffer5, size);

  for (int i = 0; i < size; i++) {
    ASSERT_EQ(output.get()[i], value);
  }

  lzt::destroy_event(event1);
  lzt::destroy_event(event2);
  lzt::destroy_event(event3);
  lzt::destroy_event(event4);
  lzt::destroy_event(event5);
  lzt::destroy_event_pool(ep);
  lzt::free_memory(context, buffer1);
  lzt::free_memory(context, buffer2);
  lzt::free_memory(context, buffer3);
  lzt::free_memory(context, buffer4);
  lzt::free_memory(context, buffer5);
  lzt::destroy_context(context);
}

INSTANTIATE_TEST_CASE_P(CacheCoherencyTimeStampEventVsRegularEvent,
                        EventProfilingCacheCoherencyTests,
                        ::testing::Values(ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP |
                                              ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
                                          ZE_EVENT_POOL_FLAG_HOST_VISIBLE));

class KernelEventProfilingCacheCoherencyTests : public ::testing::Test {};

TEST_F(KernelEventProfilingCacheCoherencyTests,
       GivenEventWhenUsingEventToSyncThenCacheIsCoherent) {

  ze_context_handle_t context = lzt::create_context();
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  auto ep = lzt::create_event_pool(
      context, 10,
      (ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP));
  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  ze_event_handle_t event = lzt::create_event(ep, event_desc);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
  size_t size = 10000;
  size_t buff_size = size * sizeof(int);
  void *src_buffer = lzt::allocate_host_memory(buff_size, 1, context);
  void *dst_buffer =
      lzt::allocate_device_memory(buff_size, 8, 0, 0, device, context);
  void *xfr_buffer =
      lzt::allocate_device_memory(buff_size, 8, 0, 0, device, context);
  void *timestamp_buffer = lzt::allocate_host_memory(
      sizeof(ze_kernel_timestamp_result_t), 8, context);
  ze_kernel_timestamp_result_t *tsResult =
      static_cast<ze_kernel_timestamp_result_t *>(timestamp_buffer);
  const int addval = 0x11223344;
  memset(src_buffer, 0, buff_size);
  ze_command_list_handle_t cmdlist =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmdqueue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  ze_module_handle_t module =
      lzt::create_module(context, device, "profile_add.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "profile_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);
  ze_group_count_t args = {static_cast<uint32_t>(size), 1, 1};
  lzt::set_argument_value(kernel, 0, sizeof(src_buffer), &src_buffer);
  lzt::set_argument_value(kernel, 1, sizeof(dst_buffer), &dst_buffer);
  lzt::set_argument_value(kernel, 2, sizeof(addval), &addval);
  lzt::append_launch_function(cmdlist, kernel, &args, event, 0, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendQueryKernelTimestamps(
                cmdlist, 1, &event, tsResult, nullptr, nullptr, 1, &event));
  lzt::append_memory_copy(cmdlist, xfr_buffer, dst_buffer, buff_size);
  lzt::append_memory_copy(cmdlist, src_buffer, xfr_buffer, buff_size);

  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);
  lzt::event_host_synchronize(event, UINT64_MAX);

  for (int i = 0; i++; i < size) {
    int value = ((int *)src_buffer)[i];
    ASSERT_EQ(value, addval);
  }

  lzt::free_memory(context, src_buffer);
  lzt::free_memory(context, dst_buffer);
  lzt::free_memory(context, xfr_buffer);
  lzt::free_memory(context, timestamp_buffer);
  lzt::destroy_event(event);
  lzt::destroy_event_pool(ep);
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
  lzt::destroy_module(module);
  lzt::destroy_function(kernel);
  lzt::destroy_context(context);
}

static void
kernel_timestamp_event_test(ze_context_handle_t context,
                            std::vector<ze_device_handle_t> devices) {
  const size_t size = 1000;
  auto device0 = devices[0];
  auto device1 = devices[1];

  ze_event_pool_desc_t event_pool_desc = {};
  event_pool_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  event_pool_desc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
  event_pool_desc.count = 1;
  auto event_pool = lzt::create_event_pool(context, event_pool_desc, devices);

  auto command_queue0 = lzt::create_command_queue(
      context, device0, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list0 = lzt::create_command_list(context, device0, 0, 0);

  auto command_queue1 = lzt::create_command_queue(
      context, device1, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list1 = lzt::create_command_list(context, device1, 0, 0);

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto event = lzt::create_event(event_pool, event_desc);

  auto module0 =
      lzt::create_module(context, device0, "profile_add.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  auto kernel0 = lzt::create_function(module0, "profile_add_constant");

  ze_group_count_t args = {static_cast<uint32_t>(size), 1, 1};
  const int addval = 1;

  void *src_buffer0 =
      lzt::allocate_shared_memory(size, 1, 0, 0, device0, context);
  void *dst_buffer0 =
      lzt::allocate_shared_memory(size, 1, 0, 0, device0, context);

  lzt::set_argument_value(kernel0, 0, sizeof(src_buffer0), &src_buffer0);
  lzt::set_argument_value(kernel0, 1, sizeof(dst_buffer0), &dst_buffer0);
  lzt::set_argument_value(kernel0, 2, sizeof(addval), &addval);
  lzt::append_launch_function(command_list0, kernel0, &args, event, 0, nullptr);
  lzt::close_command_list(command_list0);
  lzt::execute_command_lists(command_queue0, 1, &command_list0, nullptr);

  ze_kernel_timestamp_result_t *time_result0 = nullptr;
  time_result0 =
      static_cast<ze_kernel_timestamp_result_t *>(lzt::allocate_shared_memory(
          sizeof(ze_kernel_timestamp_result_t), 1, 0, 0, device1, context));

  ze_kernel_timestamp_result_t *time_result1 = nullptr;
  time_result1 =
      static_cast<ze_kernel_timestamp_result_t *>(lzt::allocate_shared_memory(
          sizeof(ze_kernel_timestamp_result_t), 1, 0, 0, device1, context));

  // Verify kernel timestamp can be queried from other device
  // with accessibility to event
  auto event_initial = event;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendQueryKernelTimestamps(
                                   command_list1, 1, &event, time_result0,
                                   nullptr, nullptr, 1, &event));
  EXPECT_EQ(event, event_initial);
  lzt::append_barrier(command_list1);

  // Verify kernel timestamp event can be added for reset
  // to second commandlist/device
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendEventReset(command_list1, event));

  auto module1 =
      lzt::create_module(context, device1, "profile_add.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  auto kernel1 = lzt::create_function(module1, "profile_add_constant");

  void *src_buffer1 =
      lzt::allocate_shared_memory(size, 1, 0, 0, device1, context);
  void *dst_buffer1 =
      lzt::allocate_shared_memory(size, 1, 0, 0, device1, context);

  lzt::set_argument_value(kernel1, 0, sizeof(src_buffer1), &src_buffer1);
  lzt::set_argument_value(kernel1, 1, sizeof(dst_buffer1), &dst_buffer1);
  lzt::set_argument_value(kernel1, 2, sizeof(addval), &addval);
  // Re-use event on second device
  lzt::append_launch_function(command_list1, kernel1, &args, event, 0, nullptr);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendQueryKernelTimestamps(
                                   command_list1, 1, &event, time_result1,
                                   nullptr, nullptr, 1, &event));

  lzt::close_command_list(command_list1);
  lzt::execute_command_lists(command_queue1, 1, &command_list1, nullptr);

  lzt::synchronize(command_queue0, UINT64_MAX);
  lzt::synchronize(command_queue1, UINT64_MAX);

  // timeresutlt0 and timeresult1 should be populated with nonzero
  // durations
  auto global_duration_0 =
      lzt::get_timestamp_global_duration(time_result0, device0);
  auto global_duration_1 =
      lzt::get_timestamp_global_duration(time_result1, device1);

  EXPECT_GT(global_duration_0, 0);
  EXPECT_GT(global_duration_1, 0);

  // cleanup
  lzt::free_memory(context, src_buffer0);
  lzt::free_memory(context, dst_buffer0);
  lzt::free_memory(context, time_result0);
  lzt::free_memory(context, time_result1);
  lzt::free_memory(context, src_buffer1);
  lzt::free_memory(context, dst_buffer1);
  lzt::destroy_command_list(command_list0);
  lzt::destroy_command_queue(command_queue0);
  lzt::destroy_command_list(command_list1);
  lzt::destroy_command_queue(command_queue1);
  lzt::destroy_function(kernel0);
  lzt::destroy_module(module0);
  lzt::destroy_function(kernel1);
  lzt::destroy_module(module1);
  lzt::destroy_event_pool(event_pool);
}

TEST_F(
    KernelEventProfilingCacheCoherencyTests,
    GivenKernelTimestampEventsWhenUsingMultipleDevicesThenBothDevicesCanAccessAndUpdateEvent) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  if (devices.size() < 2) {
    LOG_WARNING << "Less than 2 devices, skipping test";
  } else {
    auto context = lzt::create_context(driver);
    kernel_timestamp_event_test(context, devices);
    lzt::destroy_context(context);
  }
}

TEST_F(
    KernelEventProfilingCacheCoherencyTests,
    GivenKernelTimestampEventsWhenUsingMultipleSubDevicesThenBothDevicesCanAccessAndUpdateEvent) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  auto test_run = false;
  for (auto device : devices) {
    auto sub_devices = lzt::get_ze_sub_devices(device);
    if (sub_devices.size() < 2) {
      continue;
    }
    test_run = true;
    auto context = lzt::create_context(driver);
    kernel_timestamp_event_test(context, sub_devices);
    lzt::destroy_context(context);
  }

  if (!test_run) {
    LOG_WARNING << "Less than two sub devices, skipping test";
  }
}

} // namespace
