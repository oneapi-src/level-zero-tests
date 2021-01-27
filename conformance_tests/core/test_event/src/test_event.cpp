/*
 *
 * Copyright (C) 2019 Intel Corporation
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

class zeDeviceCreateEventPoolTests : public lzt::zeEventPoolTests {};

TEST_F(
    zeDeviceCreateEventPoolTests,
    GivenDefaultDeviceWhenCreatingEventPoolWithDefaultFlagsThenNotNullEventPoolIsReturned) {
  ep.InitEventPool();
}

TEST_F(
    zeDeviceCreateEventPoolTests,
    GivenDefaultDeviceWhenCreatingEventPoolWithHostVisibleFlagsThenNotNullEventPoolIsReturned) {
  ep.InitEventPool(32, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
}

TEST_F(
    zeDeviceCreateEventPoolTests,
    GivenDefaultDeviceWhenCreatingEventPoolWithIPCFlagsThenNotNullEventPoolIsReturned) {
  ep.InitEventPool(32, ZE_EVENT_POOL_FLAG_IPC);
}

class zeDeviceCreateEventPermuteEventsTests
    : public lzt::zeEventPoolTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_event_scope_flag_t, ze_event_scope_flag_t>> {};

TEST_P(
    zeDeviceCreateEventPermuteEventsTests,
    GivenDefaultDeviceAndEventPoolWhenCreatingEventsWithSignalAndWaitEventsThenNotNullEventIsReturned) {

  ze_event_handle_t event = nullptr;

  ep.create_event(event, std::get<0>(GetParam()), std::get<1>(GetParam()));
  ep.destroy_event(event);
}

INSTANTIATE_TEST_CASE_P(
    ImplictEventCreateParameterizedTest, zeDeviceCreateEventPermuteEventsTests,
    ::testing::Combine(::testing::Values(0, ZE_EVENT_SCOPE_FLAG_SUBDEVICE,
                                         ZE_EVENT_SCOPE_FLAG_DEVICE,
                                         ZE_EVENT_SCOPE_FLAG_HOST),
                       ::testing::Values(0, ZE_EVENT_SCOPE_FLAG_SUBDEVICE,
                                         ZE_EVENT_SCOPE_FLAG_DEVICE,
                                         ZE_EVENT_SCOPE_FLAG_HOST)));

TEST_F(zeDeviceCreateEventPoolTests,
       GivenDefaultDeviceWhenGettingIpcEventHandleThenNotNullisReturned) {
  ze_ipc_event_pool_handle_t hIpc;
  ep.InitEventPool();

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolGetIpcHandle(ep.event_pool_, &hIpc));
}

TEST_F(
    zeDeviceCreateEventPoolTests,
    GivenDefaultDeviceWhenGettingIpcEventHandleAndOpeningAndClosingThenSuccessIsReturned) {
  ze_ipc_event_pool_handle_t hIpc;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 32);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolGetIpcHandle(ep.event_pool_, &hIpc));

  ze_event_pool_handle_t eventPool;
  lzt::open_ipc_event_handle(context, hIpc, &eventPool);
  EXPECT_NE(nullptr, eventPool);
  lzt::close_ipc_event_handle(eventPool);
}

class zeDeviceCreateEventAndCommandListTests : public ::testing::Test {};

TEST_F(
    zeDeviceCreateEventAndCommandListTests,
    GivenDefaultDeviceWhenAppendingSignalEventToComandListThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_event_handle_t event = nullptr;

  ep.create_event(event);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendSignalEvent(cmd_list, event));
  ep.destroy_event(event);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

TEST_F(
    zeDeviceCreateEventAndCommandListTests,
    GivenDefaultDeviceWhenAppendingWaitEventsToComandListThenSuccessIsReturned) {
  const size_t event_count = 2;
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 2);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  std::vector<ze_event_handle_t> events(event_count, nullptr);

  ep.create_events(events, event_count);
  auto events_initial = events;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendWaitOnEvents(
                                   cmd_list, event_count, events.data()));
  for (int i = 0; i < events.size(); i++) {
    ASSERT_EQ(events[i], events_initial[i]);
  }
  ep.destroy_events(events);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

class zeDeviceCreateEventTests : public lzt::zeEventPoolTests {};

TEST_F(
    zeDeviceCreateEventTests,
    GivenDefaultDeviceAndEventPoolWhenSignalingHostEventThenSuccessIsReturned) {
  ze_event_handle_t event = nullptr;

  ep.create_event(event);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(event));
  ep.destroy_event(event);
}

class zeHostEventSyncPermuteTimeoutTests
    : public lzt::zeEventPoolTests,
      public ::testing::WithParamInterface<uint32_t> {};

void child_thread_function(ze_event_handle_t event, uint32_t timeout) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event, timeout));
}

TEST_P(zeHostEventSyncPermuteTimeoutTests,
       GivenDefaultDeviceAndEventPoolWhenSyncingEventThenSuccessIsReturned) {
  ze_event_handle_t event = nullptr;

  ep.create_event(event, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);

  std::thread child_thread(child_thread_function, event, GetParam());
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(event));
  child_thread.join();
  ep.destroy_event(event);
}

INSTANTIATE_TEST_CASE_P(ImplictHostSynchronizeEventParameterizedTest,
                        zeHostEventSyncPermuteTimeoutTests,
                        ::testing::Values(0, 10000000, UINT64_MAX));

TEST_F(
    zeDeviceCreateEventTests,
    GivenDefaultDeviceAndEventPoolWhenCreatingAnEventAndQueryingItsStatusThenNotReadyIsReturned) {
  ze_event_handle_t event = nullptr;

  ep.create_event(event);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
  ep.destroy_event(event);
}

TEST_F(
    zeDeviceCreateEventAndCommandListTests,
    GivenDefaultDeviceAndEventPoolWhenAppendingEventResetThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_event_handle_t event = nullptr;

  ep.create_event(event);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendEventReset(cmd_list, event));
  ep.destroy_event(event);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

TEST_F(zeDeviceCreateEventTests,
       GivenDefaultDeviceAndEventPoolWhenResettingEventThenSuccessIsReturned) {
  ze_event_handle_t event = nullptr;

  ep.create_event(event);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(event));
  ep.destroy_event(event);
}

class zeEventSignalingTests : public lzt::zeEventPoolTests {};

TEST_F(
    zeEventSignalingTests,
    GivenOneEventSignaledbyHostWhenQueryStatusThenVerifyOnlyOneEventDetected) {
  size_t num_event = 10;
  std::vector<ze_event_handle_t> host_event(num_event, nullptr);

  ep.InitEventPool(num_event);
  ep.create_events(host_event, num_event);
  for (uint32_t i = 0; i < num_event; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(host_event[i]));
    for (uint32_t j = 0; j < num_event; j++) {
      if (j == i) {
        EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(host_event[j]));
      } else {
        EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(host_event[j]));
      }
    }
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(host_event[i]));
  }
  ep.destroy_events(host_event);
}

TEST_F(
    zeEventSignalingTests,
    GivenOneEventSignaledbyCommandListWhenQueryStatusOnHostThenVerifyOnlyOneEventDetected) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  size_t num_event = 10;
  ASSERT_GE(num_event, 3);
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 10);
  std::vector<ze_event_handle_t> device_event(num_event, nullptr);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  size_t copy_size = 4096;
  void *src_buff = lzt::allocate_host_memory(copy_size, 1, context);
  void *dst_buff =
      lzt::allocate_shared_memory(copy_size, 1, 0, 0, device, context);
  ep.create_events(device_event, num_event);

  lzt::write_data_pattern(src_buff, copy_size, 1);
  lzt::write_data_pattern(dst_buff, copy_size, 0);

  for (uint32_t i = 0; i < num_event - 1; i++) {
    lzt::append_signal_event(cmd_list, device_event[i]);
    lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
    lzt::append_wait_on_events(cmd_list, 1, &device_event[num_event - 1]);
    lzt::append_memory_copy(cmd_list, dst_buff, src_buff, copy_size, nullptr);
    lzt::close_command_list(cmd_list);
    lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeEventHostSynchronize(device_event[i], UINT32_MAX - 1));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeEventHostSignal(device_event[num_event - 1]));
    lzt::synchronize(cmd_q, UINT64_MAX);

    for (uint32_t j = 0; j < num_event - 1; j++) {
      if (j == i) {
        EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(device_event[j]));
      } else {
        EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(device_event[j]));
      }
    }
    lzt::validate_data_pattern(dst_buff, copy_size, 1);
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(device_event[i]));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(device_event[num_event - 1]));
    lzt::reset_command_list(cmd_list);
  }

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(context, src_buff);
  lzt::free_memory(context, dst_buff);
  ep.destroy_events(device_event);
  lzt::destroy_context(context);
}

TEST_F(
    zeEventSignalingTests,
    GivenCommandListWaitsForEventsWhenHostAndCommandListSendsSignalsThenCommandListExecutesSuccessfully) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  size_t num_event = 10;
  ASSERT_GE(num_event, 3);
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 10);
  std::vector<ze_event_handle_t> device_event(num_event, nullptr);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  size_t copy_size = 4096;
  void *src_buff = lzt::allocate_host_memory(copy_size, 1, context);
  void *dev_buff =
      lzt::allocate_device_memory(copy_size, 1, 0, 0, device, context);
  void *dst_buff =
      lzt::allocate_shared_memory(copy_size, 1, 0, 0, device, context);

  ep.create_events(device_event, num_event);

  lzt::write_data_pattern(src_buff, copy_size, 1);
  lzt::write_data_pattern(dst_buff, copy_size, 0);

  lzt::append_signal_event(cmd_list, device_event[0]);
  lzt::append_memory_copy(cmd_list, dev_buff, src_buff, copy_size, nullptr);
  lzt::append_signal_event(cmd_list, device_event[1]);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  lzt::append_wait_on_events(cmd_list, num_event, device_event.data());
  lzt::append_memory_copy(cmd_list, dst_buff, dev_buff, copy_size, nullptr);
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
  for (uint32_t i = 2; i < num_event; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(device_event[i]));
  }
  lzt::synchronize(cmd_q, UINT64_MAX);

  for (uint32_t i = 0; i < num_event; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(device_event[i]));
  }

  lzt::validate_data_pattern(dst_buff, copy_size, 1);

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(context, src_buff);
  lzt::free_memory(context, dev_buff);
  lzt::free_memory(context, dst_buff);
  ep.destroy_events(device_event);
  lzt::destroy_context(context);
}

TEST_F(zeEventSignalingTests,
       GivenEventsSignaledWhenResetThenQueryStatusReturnsNotReady) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  size_t num_event = 10;
  ASSERT_GE(num_event, 4);
  size_t num_loop = 4;
  ASSERT_GE(num_loop, 4);
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 10);
  std::vector<ze_event_handle_t> device_event(num_event, nullptr);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  size_t loop_data_size = 300; // do not make this N*256
  ASSERT_TRUE(loop_data_size % 256);
  size_t copy_size = num_loop * loop_data_size;
  void *src_buff = lzt::allocate_host_memory(copy_size);
  void *dst_buff = lzt::allocate_shared_memory(copy_size);
  uint8_t *src_char = static_cast<uint8_t *>(src_buff);
  uint8_t *dst_char = static_cast<uint8_t *>(dst_buff);
  ep.create_events(device_event, num_event);

  lzt::write_data_pattern(src_buff, copy_size, 1);
  lzt::write_data_pattern(dst_buff, copy_size, 0);
  for (size_t i = 0; i < num_loop; i++) {
    for (size_t j = 0; j < num_event; j++) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(device_event[j]));
    }
    lzt::append_signal_event(cmd_list, device_event[i]);
    lzt::append_wait_on_events(cmd_list, num_event, device_event.data());
    lzt::append_memory_copy(cmd_list, static_cast<void *>(dst_char),
                            static_cast<void *>(src_char), loop_data_size,
                            nullptr);
    lzt::append_reset_event(cmd_list, device_event[i]);
    lzt::close_command_list(cmd_list);
    lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
    for (size_t j = 0; j < num_event; j++) {
      if (i != j) {
        EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(device_event[j]));
      }
    }
    lzt::synchronize(cmd_q, UINT64_MAX);

    for (size_t j = 0; j < num_event; j++) {
      if (i == j) {
        EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(device_event[j]));
      } else {
        EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(device_event[j]));
        EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(device_event[j]));
        EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(device_event[j]));
      }
    }
    src_char += loop_data_size;
    dst_char += loop_data_size;
    lzt::reset_command_list(cmd_list);
  }
  lzt::validate_data_pattern(dst_buff, copy_size, 1);

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(context, src_buff);
  lzt::free_memory(context, dst_buff);
  ep.destroy_events(device_event);
  lzt::destroy_context(context);
}

static void
multi_device_event_signal_read(std::vector<ze_device_handle_t> devices) {
  ze_event_pool_desc_t ep_desc = {};
  ep_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  ep_desc.flags = 0;
  ep_desc.count = 10;
  ze_context_handle_t context = lzt::create_context();
  auto ep = lzt::create_event_pool(context, ep_desc, devices);
  ze_event_scope_flag_t flag = (ze_event_scope_flag_t)(
      ZE_EVENT_SCOPE_FLAG_DEVICE | ZE_EVENT_SCOPE_FLAG_SUBDEVICE);
  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = flag;
  event_desc.wait = flag;

  auto event = lzt::create_event(ep, event_desc);

  // dev0 signals
  {
    auto cmdlist = lzt::create_command_list(context, devices[0], 0);
    auto cmdqueue = lzt::create_command_queue(
        context, devices[0], 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

    lzt::append_signal_event(cmdlist, event);
    lzt::close_command_list(cmdlist);
    lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
    lzt::synchronize(cmdqueue, UINT64_MAX);

    // cleanup
    lzt::destroy_command_list(cmdlist);
    lzt::destroy_command_queue(cmdqueue);
  }

  // all devices can read it.
  for (auto device : devices) {
    auto cmdlist = lzt::create_command_list(context, device, 0);
    auto cmdqueue = lzt::create_command_queue(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

    // lzt::append_wait_on_events(cmdlist, 1, &event);
    lzt::close_command_list(cmdlist);
    lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
    lzt::synchronize(cmdqueue, UINT64_MAX);

    // cleanup
    lzt::destroy_command_list(cmdlist);
    lzt::destroy_command_queue(cmdqueue);
  }

  lzt::destroy_event(event);
  event = lzt::create_event(ep, event_desc);

  // dev0 signals at kernel completion
  {
    auto cmdlist = lzt::create_command_list(context, devices[0], 0);
    auto cmdqueue = lzt::create_command_queue(
        context, devices[0], 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    size_t size = 10000;
    size_t buff_size = size * sizeof(int);
    auto src_buffer = lzt::allocate_host_memory(buff_size, 1, context);
    auto dst_buffer = lzt::allocate_host_memory(buff_size, 1, context);
    const int addval = 0x11223344;
    memset(src_buffer, 0, buff_size);
    auto module =
        lzt::create_module(context, devices[0], "profile_add.spv",
                           ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
    auto kernel = lzt::create_function(module, "profile_add_constant");
    lzt::set_group_size(kernel, 1, 1, 1);
    ze_group_count_t args = {static_cast<uint32_t>(size), 1, 1};
    lzt::set_argument_value(kernel, 0, sizeof(src_buffer), &src_buffer);
    lzt::set_argument_value(kernel, 1, sizeof(dst_buffer), &dst_buffer);
    lzt::set_argument_value(kernel, 2, sizeof(addval), &addval);
    lzt::append_launch_function(cmdlist, kernel, &args, event, 0, nullptr);
    lzt::close_command_list(cmdlist);
    lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
    lzt::synchronize(cmdqueue, UINT64_MAX);

    // cleanup
    lzt::destroy_command_list(cmdlist);
    lzt::destroy_command_queue(cmdqueue);
    lzt::destroy_module(module);
    lzt::destroy_function(kernel);
    lzt::free_memory(src_buffer);
    lzt::free_memory(dst_buffer);
  }

  // all devices can read it.
  for (auto device : devices) {
    auto cmdlist = lzt::create_command_list(context, device, 0);
    auto cmdqueue = lzt::create_command_queue(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

    lzt::append_wait_on_events(cmdlist, 1, &event);
    lzt::close_command_list(cmdlist);
    lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
    lzt::synchronize(cmdqueue, UINT64_MAX);

    // cleanup
    lzt::destroy_command_list(cmdlist);
    lzt::destroy_command_queue(cmdqueue);
  }

  lzt::destroy_event(event);
  lzt::destroy_event_pool(ep);
  lzt::destroy_context(context);
}

TEST(MultiDeviceEventTests,
     GivenMultipleDeviceEventPoolwhenSignalledFromOneDeviceThenAllDevicesRead) {
  auto devices = lzt::get_ze_devices();
  if (devices.size() < 2) {
    LOG_WARNING << "Less than two devices, skipping test";
  } else {
    multi_device_event_signal_read(devices);
  }
}

TEST(
    MultiDeviceEventTests,
    GivenMultipleSubDevicesEventPoolwhenSignalledFromOneSubDeviceThenAllSubDevicesRead) {
  auto devices = lzt::get_ze_devices();
  bool test_run = false;
  for (auto device : devices) {
    auto sub_devices = lzt::get_ze_sub_devices(device);
    if (sub_devices.size() < 2) {
      continue;
    }
    test_run = true;
    multi_device_event_signal_read(sub_devices);
  }
  if (!test_run)
    LOG_WARNING << "Less than two sub devices, skipping test";
}

} // namespace
