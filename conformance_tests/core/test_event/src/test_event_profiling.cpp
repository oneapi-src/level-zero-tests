/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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

class EventProfilingTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<bool, bool>> {
protected:
  bool use_mapped_timestamp = false;
  bool is_immediate = false;
  void SetUp() override {
    context = lzt::create_context();
    const ze_device_handle_t device =
        lzt::zeDevice::get_instance()->get_device();
    use_mapped_timestamp = std::get<0>(GetParam());
    is_immediate = std::get<1>(GetParam());

    if (use_mapped_timestamp) {
      const auto driver = lzt::get_default_driver();
      if (!lzt::check_if_extension_supported(
              driver, "ZE_extension_event_query_kernel_timestamps")) {
        LOG_WARNING << "driver does not support "
                       "ZE_extension_event_query_kernel_timestamps, "
                       "skipping test";
        GTEST_SKIP();
      }

      ep = lzt::create_event_pool(context, 10,
                                  (ZE_EVENT_POOL_FLAG_HOST_VISIBLE |
                                   ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP));
    } else {
      ep = lzt::create_event_pool(context, 10,
                                  (ZE_EVENT_POOL_FLAG_HOST_VISIBLE |
                                   ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP));
    }
    ze_event_desc_t event_desc = {};
    event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    event_desc.index = 0;
    event_desc.signal = 0;
    event_desc.wait = 0;
    event = lzt::create_event(ep, event_desc);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));

    uint32_t elems_nb{10000};
    size_t buff_size{elems_nb * sizeof(int)};
    src_buffer = lzt::allocate_host_memory(buff_size, 1, context);
    dst_buffer = lzt::allocate_host_memory(buff_size, 1, context);
    const int addval = 0x11223344;
    memset(src_buffer, 0, buff_size);
    cmd_bundle = lzt::create_command_bundle(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);
    module = lzt::create_module(context, device, "profile_add.spv",
                                ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
    kernel = lzt::create_function(module, "profile_add_constant");
    lzt::set_group_size(kernel, 1, 1, 1);
    ze_group_count_t args = {elems_nb, 1, 1};
    lzt::set_argument_value(kernel, 0, sizeof(src_buffer), &src_buffer);
    lzt::set_argument_value(kernel, 1, sizeof(dst_buffer), &dst_buffer);
    lzt::set_argument_value(kernel, 2, sizeof(addval), &addval);
    lzt::append_launch_function(cmd_bundle.list, kernel, &args, event, 0,
                                nullptr);
  }

  void TearDown() override {

    if (src_buffer != nullptr) {
      lzt::free_memory(context, src_buffer);
      src_buffer = nullptr;
    }
    if (dst_buffer != nullptr) {
      lzt::free_memory(context, dst_buffer);
      dst_buffer = nullptr;
    }
    if (event != nullptr) {
      lzt::destroy_event(event);
      event = nullptr;
    }
    if (ep != nullptr) {
      lzt::destroy_event_pool(ep);
      ep = nullptr;
    }
    if (cmd_bundle.list != nullptr || cmd_bundle.queue != nullptr) {
      lzt::destroy_command_bundle(cmd_bundle);
    }

    if (kernel != nullptr) {
      lzt::destroy_function(kernel);
      kernel = nullptr;
    }
    if (module != nullptr) {
      lzt::destroy_module(module);
      module = nullptr;
    }
    if (context != nullptr) {
      lzt::destroy_context(context);
      context = nullptr;
    }
  }

  void verify_mapped_timestamp_values() {
    std::vector<ze_kernel_timestamp_result_t> kernel_timestamp_buffer{};
    std::vector<ze_synchronized_timestamp_result_ext_t>
        synchronized_timestamp_buffer{};

    lzt::get_event_kernel_timestamps_from_mapped_timestamp_event(
        event, lzt::zeDevice::get_instance()->get_device(),
        kernel_timestamp_buffer, synchronized_timestamp_buffer);
    EXPECT_GT(kernel_timestamp_buffer.size(), 0u);
    for (uint32_t index = 0u; index < kernel_timestamp_buffer.size(); index++) {
      EXPECT_GT(kernel_timestamp_buffer[index].global.kernelStart, 0);
      EXPECT_GT(kernel_timestamp_buffer[index].global.kernelEnd, 0);
      EXPECT_GT(kernel_timestamp_buffer[index].context.kernelStart, 0);
      EXPECT_GT(kernel_timestamp_buffer[index].context.kernelEnd, 0);

      EXPECT_GT(synchronized_timestamp_buffer[index].global.kernelStart, 0);
      EXPECT_GT(synchronized_timestamp_buffer[index].global.kernelEnd, 0);
      EXPECT_GT(synchronized_timestamp_buffer[index].context.kernelStart, 0);
      EXPECT_GT(synchronized_timestamp_buffer[index].context.kernelEnd, 0);
    }
  }

  void *src_buffer = nullptr;
  void *dst_buffer = nullptr;
  ze_context_handle_t context = nullptr;
  ze_event_pool_handle_t ep = nullptr;
  ze_event_handle_t event = nullptr;
  lzt::zeCommandBundle cmd_bundle{};
  ze_module_handle_t module = nullptr;
  ze_kernel_handle_t kernel = nullptr;
};

TEST_P(
    EventProfilingTests,
    GivenProfilingEventWhenCommandCompletesThenTimestampsAreRelationallyCorrect) {

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  lzt::event_host_synchronize(event, UINT64_MAX);

  if (use_mapped_timestamp) {
    verify_mapped_timestamp_values();
  } else {
    ze_kernel_timestamp_result_t timestamp =
        lzt::get_event_kernel_timestamp(event);

    EXPECT_GT(timestamp.global.kernelStart, 0);
    EXPECT_GT(timestamp.global.kernelEnd, 0);
    EXPECT_GT(timestamp.context.kernelStart, 0);
    EXPECT_GT(timestamp.context.kernelEnd, 0);
  }
}

TEST_P(EventProfilingTests,
       GivenSetProfilingEventWhenResettingEventThenEventStatusNotReady) {

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  lzt::event_host_synchronize(event, UINT64_MAX);
  if (use_mapped_timestamp) {
    verify_mapped_timestamp_values();
  } else {
    ze_kernel_timestamp_result_t timestamp =
        lzt::get_event_kernel_timestamp(event);

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeEventQueryKernelTimestamp(event, &timestamp));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(event));
    EXPECT_EQ(ZE_RESULT_NOT_READY,
              zeEventQueryKernelTimestamp(event, &timestamp));
  }
}

TEST_P(EventProfilingTests,
       GivenRegularAndTimestampEventPoolsWhenProfilingThenTimestampsStillWork) {
  const int mem_size = 1000;
  auto other_buffer = lzt::allocate_host_memory(mem_size, 1, context);
  const uint8_t value1 = 0x55;
  auto ep_no_timestamps =
      lzt::create_event_pool(context, 10, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  ze_event_desc_t regular_event_desc = {};
  regular_event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  regular_event_desc.index = 1;
  regular_event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  regular_event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto regular_event = lzt::create_event(ep_no_timestamps, regular_event_desc);

  lzt::append_memory_set(cmd_bundle.list, other_buffer, &value1, mem_size,
                         regular_event);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &regular_event);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  lzt::event_host_synchronize(event, UINT64_MAX);
  lzt::event_host_synchronize(regular_event, UINT64_MAX);

  if (use_mapped_timestamp) {
    verify_mapped_timestamp_values();
  } else {
    ze_kernel_timestamp_result_t timestamp =
        lzt::get_event_kernel_timestamp(event);

    EXPECT_GT(timestamp.global.kernelStart, 0);
    EXPECT_GT(timestamp.global.kernelEnd, 0);
    EXPECT_GT(timestamp.context.kernelStart, 0);
    EXPECT_GT(timestamp.context.kernelEnd, 0);
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event));

  for (int i = 0; i < mem_size; i++) {
    uint8_t byte = ((uint8_t *)other_buffer)[i];
    ASSERT_EQ(byte, value1);
  }

  lzt::free_memory(context, other_buffer);
  lzt::destroy_event(regular_event);
  lzt::destroy_event_pool(ep_no_timestamps);
}

INSTANTIATE_TEST_SUITE_P(VerifyEventProfilingTests, EventProfilingTests,
                         ::testing::Combine(::testing::Bool(),
                                            ::testing::Bool()));

class EventProfilingCacheCoherencyTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_event_pool_flags_t, bool>> {};

TEST_P(EventProfilingCacheCoherencyTests,
       GivenEventWhenUsingEventToSyncThenCacheIsCoherent) {
  const uint32_t size = 10000;
  const uint8_t value = 0x55;
  ze_context_handle_t context = lzt::create_context();
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  auto ep = lzt::create_event_pool(context, 10, std::get<0>(GetParam()));
  bool is_immediate = std::get<1>(GetParam());

  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

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

  lzt::append_memory_set(cmd_bundle.list, buffer1, &value, size, event1);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &event1);
  lzt::append_memory_copy(cmd_bundle.list, buffer2, buffer1, size,
                          event2); // Note: Need to add wait on events
  lzt::append_wait_on_events(cmd_bundle.list, 1, &event2);
  lzt::append_memory_copy(cmd_bundle.list, buffer3, buffer2, size, event3);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &event3);
  lzt::append_memory_copy(cmd_bundle.list, buffer4, buffer3, size, event4);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &event4);
  lzt::append_memory_copy(cmd_bundle.list, buffer5, buffer4, size, event5);

  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

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
  lzt::destroy_command_bundle(cmd_bundle);
}

INSTANTIATE_TEST_SUITE_P(
    CacheCoherencyTimeStampEventVsRegularEvent,
    EventProfilingCacheCoherencyTests,
    ::testing::Combine(::testing::Values(ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP |
                                             ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
                                         ZE_EVENT_POOL_FLAG_HOST_VISIBLE),
                       ::testing::Bool()));

class KernelEventProfilingCacheCoherencyTests : public ::testing::Test {};

void RunGivenKernelEventWhenUsingEventToSyncTest(bool is_immediate) {
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

  uint32_t elems_nb{10000};
  size_t buff_size{elems_nb * sizeof(int)};
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
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);
  ze_module_handle_t module =
      lzt::create_module(context, device, "profile_add.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "profile_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);
  ze_group_count_t args = {elems_nb, 1, 1};
  lzt::set_argument_value(kernel, 0, sizeof(src_buffer), &src_buffer);
  lzt::set_argument_value(kernel, 1, sizeof(dst_buffer), &dst_buffer);
  lzt::set_argument_value(kernel, 2, sizeof(addval), &addval);
  lzt::append_launch_function(cmd_bundle.list, kernel, &args, event, 0,
                              nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendQueryKernelTimestamps(
                                   cmd_bundle.list, 1, &event, tsResult,
                                   nullptr, nullptr, 1, &event));
  lzt::append_barrier(cmd_bundle.list);
  lzt::append_memory_copy(cmd_bundle.list, xfr_buffer, dst_buffer, buff_size);
  lzt::append_barrier(cmd_bundle.list);
  lzt::append_memory_copy(cmd_bundle.list, src_buffer, xfr_buffer, buff_size);

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  lzt::event_host_synchronize(event, UINT64_MAX);

  for (int i = 0; i < elems_nb; i++) {
    int value = ((int *)src_buffer)[i];
    ASSERT_EQ(value, addval);
  }

  lzt::free_memory(context, src_buffer);
  lzt::free_memory(context, dst_buffer);
  lzt::free_memory(context, xfr_buffer);
  lzt::free_memory(context, timestamp_buffer);
  lzt::destroy_event(event);
  lzt::destroy_event_pool(ep);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_context(context);
}

TEST_F(KernelEventProfilingCacheCoherencyTests,
       GivenKernelEventWhenUsingEventToSyncThenCacheIsCoherent) {
  RunGivenKernelEventWhenUsingEventToSyncTest(false);
}

TEST_F(
    KernelEventProfilingCacheCoherencyTests,
    GivenKernelEventWhenUsingEventToSyncOnImmediateCmdListThenCacheIsCoherent) {
  RunGivenKernelEventWhenUsingEventToSyncTest(true);
}

static void kernel_timestamp_event_test(ze_context_handle_t context,
                                        std::vector<ze_device_handle_t> devices,
                                        ze_driver_handle_t driver,
                                        bool is_immediate) {
  auto device0 = devices[0];
  auto device1 = devices[1];

  ze_event_pool_desc_t event_pool_desc = {};
  event_pool_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  event_pool_desc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
  event_pool_desc.count = 1;
  auto event_pool = lzt::create_event_pool(context, event_pool_desc, devices);

  auto cmd_bundle0 = lzt::create_command_bundle(
      context, device0, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);
  auto cmd_bundle1 = lzt::create_command_bundle(
      context, device1, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto event = lzt::create_event(event_pool, event_desc);

  auto module0 =
      lzt::create_module(context, device0, "profile_add.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  auto kernel0 = lzt::create_function(module0, "profile_add_constant");
  lzt::set_group_size(kernel0, 1, 1, 1);

  constexpr uint32_t elems_nb{1000};
  constexpr size_t buff_size{elems_nb * sizeof(int)};
  constexpr int addval{1};
  constexpr int alignment{1};
  ze_group_count_t args = {elems_nb, 1, 1};

  void *src_buffer0 =
      lzt::allocate_shared_memory(buff_size, alignment, 0, 0, device0, context);
  void *dst_buffer0 =
      lzt::allocate_shared_memory(buff_size, alignment, 0, 0, device0, context);

  lzt::set_argument_value(kernel0, 0, sizeof(src_buffer0), &src_buffer0);
  lzt::set_argument_value(kernel0, 1, sizeof(dst_buffer0), &dst_buffer0);
  lzt::set_argument_value(kernel0, 2, sizeof(addval), &addval);
  lzt::append_launch_function(cmd_bundle0.list, kernel0, &args, event, 0,
                              nullptr);
  lzt::close_command_list(cmd_bundle0.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle0.queue, 1, &cmd_bundle0.list,
                               nullptr);
  }

  ze_kernel_timestamp_result_t *time_result0 = nullptr;
  time_result0 =
      static_cast<ze_kernel_timestamp_result_t *>(lzt::allocate_shared_memory(
          sizeof(ze_kernel_timestamp_result_t), alignment, 0, 0, device1, context));

  ze_kernel_timestamp_result_t *time_result1 = nullptr;
  time_result1 =
      static_cast<ze_kernel_timestamp_result_t *>(lzt::allocate_shared_memory(
          sizeof(ze_kernel_timestamp_result_t), alignment, 0, 0, device1, context));

  // Verify kernel timestamp can be queried from other device
  // with accessibility to event
  auto event_initial = event;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendQueryKernelTimestamps(
                                   cmd_bundle1.list, 1, &event, time_result0,
                                   nullptr, nullptr, 1, &event));
  EXPECT_EQ(event, event_initial);
  lzt::append_barrier(cmd_bundle1.list);

  // Verify kernel timestamp event can be added for reset
  // to second commandlist/device
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendEventReset(cmd_bundle1.list, event));

  auto module1 =
      lzt::create_module(context, device1, "profile_add.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  auto kernel1 = lzt::create_function(module1, "profile_add_constant");
  lzt::set_group_size(kernel1, 1, 1, 1);

  void *src_buffer1 =
      lzt::allocate_shared_memory(buff_size, alignment, 0, 0, device1, context);
  void *dst_buffer1 =
      lzt::allocate_shared_memory(buff_size, alignment, 0, 0, device1, context);

  lzt::set_argument_value(kernel1, 0, sizeof(src_buffer1), &src_buffer1);
  lzt::set_argument_value(kernel1, 1, sizeof(dst_buffer1), &dst_buffer1);
  lzt::set_argument_value(kernel1, 2, sizeof(addval), &addval);
  // Re-use event on second device
  lzt::append_launch_function(cmd_bundle1.list, kernel1, &args, event, 0,
                              nullptr);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendQueryKernelTimestamps(
                                   cmd_bundle1.list, 1, &event, time_result1,
                                   nullptr, nullptr, 1, &event));

  lzt::close_command_list(cmd_bundle1.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle1.queue, 1, &cmd_bundle1.list,
                               nullptr);
  }

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle0.list, UINT64_MAX);
    lzt::synchronize_command_list_host(cmd_bundle1.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle0.queue, UINT64_MAX);
    lzt::synchronize(cmd_bundle1.queue, UINT64_MAX);
  }

  // timeresutlt0 and timeresult1 should be populated with nonzero
  // durations
  auto global_duration_0 =
      lzt::get_timestamp_global_duration(time_result0, device0, driver);
  auto global_duration_1 =
      lzt::get_timestamp_global_duration(time_result1, device1, driver);

  EXPECT_GT(global_duration_0, 0);
  EXPECT_GT(global_duration_1, 0);

  // cleanup
  lzt::free_memory(context, src_buffer0);
  lzt::free_memory(context, dst_buffer0);
  lzt::free_memory(context, time_result0);
  lzt::free_memory(context, time_result1);
  lzt::free_memory(context, src_buffer1);
  lzt::free_memory(context, dst_buffer1);
  lzt::destroy_command_bundle(cmd_bundle0);
  lzt::destroy_command_bundle(cmd_bundle1);
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
    kernel_timestamp_event_test(context, devices, driver, false);
    lzt::destroy_context(context);
  }
}

TEST_F(
    KernelEventProfilingCacheCoherencyTests,
    GivenKernelTimestampEventsWhenUsingMultipleDevicesOnImmediateCmdListThenBothDevicesCanAccessAndUpdateEvent) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  if (devices.size() < 2) {
    LOG_WARNING << "Less than 2 devices, skipping test";
  } else {
    auto context = lzt::create_context(driver);
    kernel_timestamp_event_test(context, devices, driver, true);
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
    kernel_timestamp_event_test(context, sub_devices, driver, false);
    lzt::destroy_context(context);
  }

  if (!test_run) {
    LOG_WARNING << "Less than two sub devices, skipping test";
  }
}

TEST_F(
    KernelEventProfilingCacheCoherencyTests,
    GivenKernelTimestampEventsWhenUsingMultipleSubDevicesOnImmediateCmdListThenBothDevicesCanAccessAndUpdateEvent) {
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
    kernel_timestamp_event_test(context, sub_devices, driver, true);
    lzt::destroy_context(context);
  }

  if (!test_run) {
    LOG_WARNING << "Less than two sub devices, skipping test";
  }
}

#ifdef ZE_EVENT_QUERY_TIMESTAMPS_EXP_NAME
void RunGivenDeviceWithSubDevicesWhenQueryingForMultipleTimestampsTest(
    bool is_immediate) {

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);

  auto driver_extension_properties = lzt::get_extension_properties(driver);
  bool supports_extended_timestamps = false;
  for (auto &extension : driver_extension_properties) {
    if (!std::strcmp(ZE_EVENT_QUERY_TIMESTAMPS_EXP_NAME, extension.name)) {
      supports_extended_timestamps = true;
      break;
    }
  }

  if (!supports_extended_timestamps) {
    LOG_WARNING << "driver does not support experimental timestamps query, "
                   "skipping test";
    GTEST_SKIP();
  }

  auto sub_device_count = lzt::get_ze_sub_device_count(device);

  if (!sub_device_count) {
    LOG_WARNING << "Device does not have any sub-devices";
    GTEST_SKIP();
  }

  auto context = lzt::create_context(driver);

  ze_event_pool_desc_t event_pool_desc = {};
  event_pool_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  event_pool_desc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
  event_pool_desc.count = 1;
  auto event_pool = lzt::create_event_pool(context, event_pool_desc);

  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto event = lzt::create_event(event_pool, event_desc);

  auto module = lzt::create_module(context, device, "profile_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);

  auto kernel = lzt::create_function(module, "profile_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  constexpr uint32_t elems_nb{1000};
  constexpr size_t buff_size{elems_nb * sizeof(int)};
  ze_group_count_t args = {elems_nb, 1, 1};
  const int addval = 1;

  void *src_buffer =
      lzt::allocate_shared_memory(buff_size, 1, 0, 0, device, context);
  void *dst_buffer =
      lzt::allocate_shared_memory(buff_size, 1, 0, 0, device, context);

  lzt::set_argument_value(kernel, 0, sizeof(src_buffer), &src_buffer);
  lzt::set_argument_value(kernel, 1, sizeof(dst_buffer), &dst_buffer);
  lzt::set_argument_value(kernel, 2, sizeof(addval), &addval);
  lzt::append_launch_function(cmd_bundle.list, kernel, &args, event, 0,
                              nullptr);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  auto timestamp_results = lzt::get_event_timestamps_exp(event, device);

  LOG_INFO << "Timestamp results returned: " << timestamp_results.size();

  for (auto &result : timestamp_results) {
    auto duration = lzt::get_timestamp_global_duration(&result, device, driver);
    EXPECT_GT(duration, 0);
  }

  // cleanup
  lzt::free_memory(context, src_buffer);
  lzt::free_memory(context, dst_buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_event_pool(event_pool);
  lzt::destroy_context(context);
}

TEST_F(
    KernelEventProfilingCacheCoherencyTests,
    GivenDeviceWithSubDevicesWhenQueryingForMultipleTimestampsThenSuccessReturned) {
  RunGivenDeviceWithSubDevicesWhenQueryingForMultipleTimestampsTest(false);
}

TEST_F(
    KernelEventProfilingCacheCoherencyTests,
    GivenDeviceWithSubDevicesWhenQueryingForMultipleTimestampsOnImmediateCmdListThenSuccessReturned) {
  RunGivenDeviceWithSubDevicesWhenQueryingForMultipleTimestampsTest(true);
}
#else
#ifdef __linux__
#warning                                                                       \
    "ZE_EVENT_QUERY_TIMESTAMPS_EXP support not found, not building tests for it"
#else
#pragma message(                                                               \
    "warning: ZE_EVENT_QUERY_TIMESTAMPS_EXP support not found, not building tests for it")
#endif
#endif // ifdef ZE_EVENT_QUERY_TIMESTAMPS_EXP_NAME

class EventMappedTimestampProfilingTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<bool> {
protected:
  bool use_immediate_cmd_lists = false;
  void SetUp() override {
    context = lzt::create_context();
    auto driver = lzt::get_default_driver();
    if (!lzt::check_if_extension_supported(
            driver, "ZE_extension_event_query_kernel_timestamps")) {
      LOG_WARNING << "driver does not support "
                     "ZE_extension_event_query_kernel_timestamps, "
                     "skipping test";
      GTEST_SKIP();
    }

    ep = lzt::create_event_pool(context, 10,
                                (ZE_EVENT_POOL_FLAG_HOST_VISIBLE |
                                 ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP));
    use_immediate_cmd_lists = GetParam();
    event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    event_desc.index = 0;
    event_desc.signal = 0;
    event_desc.wait = 0;
    size_t buff_size{elems_nb * sizeof(int)};
    src_buffer = lzt::allocate_host_memory(buff_size, 1, context);
    dst_buffer = lzt::allocate_host_memory(buff_size, 1, context);
    memset(src_buffer, 0, buff_size);
  }

  void TearDown() override {

    if (src_buffer != nullptr) {
      lzt::free_memory(context, src_buffer);
      src_buffer = nullptr;
    }
    if (dst_buffer != nullptr) {
      lzt::free_memory(context, dst_buffer);
      dst_buffer = nullptr;
    }

    if (ep != nullptr) {
      lzt::destroy_event_pool(ep);
      ep = nullptr;
    }

    if (context != nullptr) {
      lzt::destroy_context(context);
      context = nullptr;
    }
  }

  void *src_buffer = nullptr;
  void *dst_buffer = nullptr;
  ze_context_handle_t context = nullptr;
  ze_event_pool_handle_t ep = nullptr;
  ze_event_desc_t event_desc = {};
  uint32_t elems_nb{10000};
  ze_group_count_t args = {elems_nb, 1, 1};
};

TEST_P(
    EventMappedTimestampProfilingTests,
    GivenEventMappedTimestampWhenCommandCompletesThenTimestampsAreRelationallyCorrect) {

  ze_command_list_handle_t cmdlist{};
  ze_command_queue_handle_t cmdqueue{};
  ze_module_handle_t module{};
  ze_kernel_handle_t kernel{};

  std::vector<ze_device_handle_t> test_devices = lzt::get_all_sub_devices();
  test_devices.push_back(lzt::get_default_device(lzt::get_default_driver()));

  ze_event_handle_t event = lzt::create_event(ep, event_desc);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));

  for (auto &test_device : test_devices) {

    ze_device_properties_t deviceProperties = {
        ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
    zeDeviceGetProperties(test_device, &deviceProperties);

    LOG_INFO << "test device name " << deviceProperties.name << " uuid "
             << lzt::to_string(deviceProperties.uuid);
    if (deviceProperties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
      LOG_INFO << "test subdevice id " << deviceProperties.subdeviceId;
    } else {
      LOG_INFO << "test device is a root device";
    }

    // Prepare Command list
    if (use_immediate_cmd_lists) {
      cmdlist = lzt::create_immediate_command_list(test_device);
    } else {
      cmdlist = lzt::create_command_list(context, test_device, 0);
      cmdqueue = lzt::create_command_queue(context, test_device, 0,
                                           ZE_COMMAND_QUEUE_MODE_DEFAULT,
                                           ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    }

    // Prepare Kernel
    module = lzt::create_module(context, test_device, "profile_add.spv",
                                ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
    kernel = lzt::create_function(module, "profile_add_constant");
    lzt::set_group_size(kernel, 1, 1, 1);
    lzt::set_argument_value(kernel, 0, sizeof(src_buffer), &src_buffer);
    lzt::set_argument_value(kernel, 1, sizeof(dst_buffer), &dst_buffer);
    const int addval = 0x11223344;
    lzt::set_argument_value(kernel, 2, sizeof(addval), &addval);
    lzt::append_launch_function(cmdlist, kernel, &args, event, 0, nullptr);

    // Execute
    lzt::close_command_list(cmdlist);
    if (!use_immediate_cmd_lists) {
      lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
      lzt::synchronize(cmdqueue, UINT64_MAX);
    }
    lzt::event_host_synchronize(event, UINT64_MAX);

    // Read and verify timestamps
    std::vector<ze_kernel_timestamp_result_t> kernel_timestamp_buffer;
    std::vector<ze_synchronized_timestamp_result_ext_t>
        synchronized_timestamp_buffer;
    lzt::get_event_kernel_timestamps_from_mapped_timestamp_event(
        event, test_device, kernel_timestamp_buffer,
        synchronized_timestamp_buffer);
    uint64_t previous_maximum_sync_ts = std::numeric_limits<uint64_t>::min();

    EXPECT_GT(kernel_timestamp_buffer.size(), 0u);
    for (uint32_t index = 0u; index < kernel_timestamp_buffer.size(); index++) {
      EXPECT_GT(kernel_timestamp_buffer[index].global.kernelStart, 0);
      EXPECT_GT(kernel_timestamp_buffer[index].global.kernelEnd, 0);
      EXPECT_GT(kernel_timestamp_buffer[index].context.kernelStart, 0);
      EXPECT_GT(kernel_timestamp_buffer[index].context.kernelEnd, 0);

      EXPECT_GT(synchronized_timestamp_buffer[index].global.kernelStart, 0);
      EXPECT_GT(synchronized_timestamp_buffer[index].global.kernelEnd, 0);
      EXPECT_GT(synchronized_timestamp_buffer[index].context.kernelStart, 0);
      EXPECT_GT(synchronized_timestamp_buffer[index].context.kernelEnd, 0);

      LOG_DEBUG
          << "\n\n [1]synchronized_timestamp_buffer[index].global.kernelStart "
             ": "
          << synchronized_timestamp_buffer[index].global.kernelStart;
      LOG_DEBUG << "\n synchronized_timestamp_buffer[index].global.kernelEnd : "
                << synchronized_timestamp_buffer[index].global.kernelEnd;
      LOG_DEBUG
          << "\n synchronized_timestamp_buffer[index].context.kernelStart : "
          << synchronized_timestamp_buffer[index].context.kernelStart;
      LOG_DEBUG
          << "\n synchronized_timestamp_buffer[index].context.kernelEnd : "
          << synchronized_timestamp_buffer[index].context.kernelEnd;

      previous_maximum_sync_ts =
          std::max(previous_maximum_sync_ts,
                   synchronized_timestamp_buffer[index].global.kernelStart);
      previous_maximum_sync_ts =
          std::max(previous_maximum_sync_ts,
                   synchronized_timestamp_buffer[index].global.kernelEnd);
    }

    lzt::event_host_reset(event);
    lzt::reset_command_list(cmdlist);
    lzt::append_launch_function(cmdlist, kernel, &args, event, 0, nullptr);
    lzt::close_command_list(cmdlist);
    if (!use_immediate_cmd_lists) {
      lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
      lzt::synchronize(cmdqueue, UINT64_MAX);
    }

    lzt::event_host_synchronize(event, UINT64_MAX);
    kernel_timestamp_buffer.clear();
    std::vector<ze_synchronized_timestamp_result_ext_t>
        current_synchronized_timestamp_buffer;

    lzt::get_event_kernel_timestamps_from_mapped_timestamp_event(
        event, lzt::zeDevice::get_instance()->get_device(),
        kernel_timestamp_buffer, current_synchronized_timestamp_buffer);
    uint64_t current_minimum_sync_ts = std::numeric_limits<uint64_t>::max();

    for (uint32_t index = 0u; index < kernel_timestamp_buffer.size(); index++) {
      EXPECT_GT(current_synchronized_timestamp_buffer[index].global.kernelStart,
                0);
      EXPECT_GT(current_synchronized_timestamp_buffer[index].global.kernelEnd,
                0);
      EXPECT_GT(
          current_synchronized_timestamp_buffer[index].context.kernelStart, 0);
      EXPECT_GT(current_synchronized_timestamp_buffer[index].context.kernelEnd,
                0);
      current_minimum_sync_ts = std::min(
          current_minimum_sync_ts,
          current_synchronized_timestamp_buffer[index].global.kernelStart);
      current_minimum_sync_ts = std::min(
          current_minimum_sync_ts,
          current_synchronized_timestamp_buffer[index].global.kernelEnd);

      LOG_DEBUG
          << "\n\n [1]synchronized_timestamp_buffer[index].global.kernelStart "
             ": "
          << synchronized_timestamp_buffer[index].global.kernelStart;
      LOG_DEBUG << "\n synchronized_timestamp_buffer[index].global.kernelEnd : "
                << synchronized_timestamp_buffer[index].global.kernelEnd;
      LOG_DEBUG
          << "\n synchronized_timestamp_buffer[index].context.kernelStart : "
          << synchronized_timestamp_buffer[index].context.kernelStart;
      LOG_DEBUG
          << "\n synchronized_timestamp_buffer[index].context.kernelEnd : "
          << synchronized_timestamp_buffer[index].context.kernelEnd;
    }

    LOG_DEBUG << "current_minimum_sync_ts : " << current_minimum_sync_ts
              << " | previous_maximum_sync_ts : " << previous_maximum_sync_ts
              << std::endl;
    EXPECT_GE(current_minimum_sync_ts, previous_maximum_sync_ts);

    lzt::destroy_command_list(cmdlist);
    if (!use_immediate_cmd_lists) {
      lzt::destroy_command_queue(cmdqueue);
    }
    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::event_host_reset(event);
  }
  lzt::destroy_event(event);
}

INSTANTIATE_TEST_SUITE_P(VerifyEventMappedTimestampProfilingTests,
                         EventMappedTimestampProfilingTests,
                         ::testing::Values(true, false));

} // namespace
