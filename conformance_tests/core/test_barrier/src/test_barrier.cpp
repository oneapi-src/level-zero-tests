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

namespace lzt = level_zero_tests;

namespace {

class zeCommandListAppendBarrierTests : public lzt::zeCommandListTests {};

TEST_F(zeCommandListAppendBarrierTests,
       GivenEmptyCommandListWhenAppendingBarrierThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendBarrier(cl.command_list_, nullptr, 0, nullptr));
}

class zeEventPoolCommandListAppendBarrierTests : public ::testing::Test {};

TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyCommandListWhenAppendingBarrierWithEventThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_event_handle_t event = nullptr;
  ep.create_event(event);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendBarrier(cmd_list, event, 0, nullptr));
  ep.destroy_event(event);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyCommandListWhenAppendingBarrierWithEventsThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  const size_t event_count = 2;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 2);
  std::vector<ze_event_handle_t> events(event_count, nullptr);
  ep.create_events(events, event_count);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendBarrier(cmd_list, nullptr, events.size(),
                                       events.data()));
  ep.destroy_events(events);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyCommandListWhenAppendingBarrierWithSignalEventAndWaitEventsThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_event_handle_t event = nullptr;
  const size_t event_count = 2;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 3);
  std::vector<ze_event_handle_t> events(event_count, nullptr);

  ep.create_event(event);
  ep.create_events(events, event_count);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  auto events_initial = events;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendBarrier(cmd_list, event, events.size(),
                                       events.data()));
  for (int i = 0; i < events.size(); i++) {
    ASSERT_EQ(events[i], events_initial[i]);
  }
  ep.destroy_events(events);
  ep.destroy_event(event);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

void AppendMemoryRangesBarrierTest(
    ze_context_handle_t context, ze_device_handle_t device,
    ze_command_list_handle_t command_list, ze_event_handle_t signaling_event,
    std::vector<ze_event_handle_t> &waiting_events) {
  const std::vector<size_t> range_sizes{4096, 8192};
  std::vector<const void *> ranges{
      lzt::allocate_device_memory((range_sizes[0] * 2), 1, 0, 0, device,
                                  context),
      lzt::allocate_device_memory((range_sizes[1] * 2), 1, 0, 0, device,
                                  context)};

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendMemoryRangesBarrier(
                command_list, ranges.size(), range_sizes.data(), ranges.data(),
                signaling_event, waiting_events.size(), waiting_events.data()));

  for (auto &range : ranges)
    lzt::free_memory(context, range);
}

class zeCommandListAppendMemoryRangesBarrierTests : public ::testing::Test {};

TEST_F(
    zeCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyCommandListWhenAppendingMemoryRangesBarrierThenSuccessIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_event_handle_t> waiting_events;
  ze_context_handle_t context = lzt::create_context();
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  AppendMemoryRangesBarrierTest(context, device, cmd_list, nullptr,
                                waiting_events);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

class zeEventPoolCommandListAppendMemoryRangesBarrierTests
    : public ::testing::Test {};

TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyCommandListEventPoolWhenAppendingMemoryRangesBarrierSignalEventThenSuccessIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  lzt::zeEventPool ep;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_event_handle_t signaling_event = nullptr;
  std::vector<ze_event_handle_t> waiting_events;

  ep.create_event(signaling_event);
  AppendMemoryRangesBarrierTest(context, device, cmd_list, signaling_event,
                                waiting_events);
  ep.destroy_event(signaling_event);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyCommandListEventPoolWhenAppendingMemoryRangesBarrierWaitEventsThenSuccessIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  const size_t event_count = 2;
  std::vector<ze_event_handle_t> waiting_events(event_count, nullptr);
  lzt::zeEventPool ep;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 2);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ep.create_events(waiting_events, event_count);
  auto wait_events_initial = waiting_events;
  AppendMemoryRangesBarrierTest(context, device, cmd_list, nullptr,
                                waiting_events);
  for (int i = 0; i < waiting_events.size(); i++) {
    ASSERT_EQ(waiting_events[i], wait_events_initial[i]);
  }
  ep.destroy_events(waiting_events);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyCommandListEventPoolWhenAppendingMemoryRangesBarrierSignalEventAndWaitEventsThenSuccessIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  lzt::zeEventPool ep;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 3);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_event_handle_t signaling_event = nullptr;
  const size_t event_count = 2;

  std::vector<ze_event_handle_t> waiting_events(event_count, nullptr);
  ep.create_event(signaling_event);
  ep.create_events(waiting_events, event_count);
  AppendMemoryRangesBarrierTest(context, device, cmd_list, signaling_event,
                                waiting_events);
  ep.destroy_event(signaling_event);
  ep.destroy_events(waiting_events);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_context(context);
}

class zeContextSystemBarrierTests : public ::testing::Test {};

TEST_F(zeContextSystemBarrierTests,
       GivenDeviceWhenAddingSystemBarrierThenSuccessIsReturned) {

  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeContextSystemBarrier(lzt::get_default_context(),
                             lzt::zeDevice::get_instance()->get_device()));
}

class zeBarrierEventSignalingTests : public lzt::zeEventPoolTests {};

TEST_F(
    zeBarrierEventSignalingTests,
    GivenEventSignaledWhenCommandListAppendingBarrierThenHostDetectsEventSuccessfully) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_event_handle_t event_barrier_to_host = nullptr;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  ep.create_event(event_barrier_to_host, ZE_EVENT_SCOPE_FLAG_HOST,
                  ZE_EVENT_SCOPE_FLAG_HOST);
  lzt::append_memory_copy(cmd_list, dev_mem, inpa.data(), xfer_size, nullptr);
  lzt::append_barrier(cmd_list, event_barrier_to_host, 0, nullptr);
  lzt::append_memory_copy(cmd_list, host_mem, dev_mem, xfer_size, nullptr);
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
  lzt::synchronize(cmd_q, UINT64_MAX);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeEventHostSynchronize(event_barrier_to_host, UINT32_MAX - 1));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_barrier_to_host));

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_event(event_barrier_to_host);
  lzt::destroy_context(context);
}

TEST_F(
    zeBarrierEventSignalingTests,
    GivenCommandListAppendingBarrierWaitsForEventsWhenHostAndCommandListSendSignalsThenCommandListExecutesSuccessfully) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  uint32_t num_events = 6;
  ASSERT_GE(num_events, 3);
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 6);
  std::vector<ze_event_handle_t> events_to_barrier(num_events, nullptr);
  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  ep.create_events(events_to_barrier, num_events, ZE_EVENT_SCOPE_FLAG_HOST,
                   ZE_EVENT_SCOPE_FLAG_HOST);
  lzt::append_signal_event(cmd_list, events_to_barrier[0]);
  lzt::append_memory_copy(cmd_list, dev_mem, inpa.data(), xfer_size, nullptr);
  lzt::append_signal_event(cmd_list, events_to_barrier[1]);
  lzt::append_barrier(cmd_list, nullptr, num_events, events_to_barrier.data());
  lzt::append_memory_copy(cmd_list, host_mem, dev_mem, xfer_size, nullptr);
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);

  for (uint32_t i = 2; i < num_events; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(events_to_barrier[i]));
  }
  lzt::synchronize(cmd_q, UINT64_MAX);
  for (uint32_t i = 0; i < num_events; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events_to_barrier[i]));
  }

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_events(events_to_barrier);
  lzt::destroy_context(context);
}

TEST_F(
    zeBarrierEventSignalingTests,
    GivenEventSignaledWhenCommandListAppendingMemoryRangesBarrierThenHostDetectsEventSuccessfully) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_event_handle_t event_barrier_to_host = nullptr;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  const std::vector<size_t> range_sizes{inpa.size(), inpa.size()};
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  std::vector<const void *> ranges{dev_mem, host_mem};
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  ep.create_event(event_barrier_to_host, ZE_EVENT_SCOPE_FLAG_HOST,
                  ZE_EVENT_SCOPE_FLAG_HOST);
  lzt::append_memory_copy(cmd_list, dev_mem, inpa.data(), xfer_size, nullptr);
  lzt::append_memory_ranges_barrier(cmd_list, ranges.size(), range_sizes.data(),
                                    ranges.data(), event_barrier_to_host, 0,
                                    nullptr);
  lzt::append_memory_copy(cmd_list, host_mem, dev_mem, xfer_size, nullptr);
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
  lzt::synchronize(cmd_q, UINT64_MAX);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeEventHostSynchronize(event_barrier_to_host, UINT32_MAX - 1));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_barrier_to_host));

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_event(event_barrier_to_host);
  lzt::destroy_context(context);
}

TEST_F(
    zeBarrierEventSignalingTests,
    GivenCommandListAppendingMemoryRangesBarrierWaitsForEventsWhenHostAndCommandListSendSignalsThenCommandListExecutesSuccessfully) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  uint32_t num_events = 6;
  ASSERT_GE(num_events, 3);
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 6);
  std::vector<ze_event_handle_t> events_to_barrier(num_events, nullptr);
  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  const std::vector<size_t> range_sizes{inpa.size(), inpa.size()};
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  std::vector<const void *> ranges{dev_mem, host_mem};
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  ep.create_events(events_to_barrier, num_events, ZE_EVENT_SCOPE_FLAG_HOST,
                   ZE_EVENT_SCOPE_FLAG_HOST);

  lzt::append_signal_event(cmd_list, events_to_barrier[0]);
  lzt::append_memory_copy(cmd_list, dev_mem, inpa.data(), xfer_size, nullptr);
  lzt::append_signal_event(cmd_list, events_to_barrier[1]);
  lzt::append_memory_ranges_barrier(cmd_list, ranges.size(), range_sizes.data(),
                                    ranges.data(), nullptr, num_events,
                                    events_to_barrier.data());
  lzt::append_memory_copy(cmd_list, host_mem, dev_mem, xfer_size, nullptr);
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);

  for (uint32_t i = 2; i < num_events; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(events_to_barrier[i]));
  }
  lzt::synchronize(cmd_q, UINT64_MAX);
  for (uint32_t i = 0; i < num_events; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events_to_barrier[i]));
  }

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_events(events_to_barrier);
  lzt::destroy_context(context);
}

enum BarrierType {
  BT_GLOBAL_BARRIER,
  BT_MEMORY_RANGES_BARRIER,
  BT_EVENTS_BARRIER
};

class zeBarrierKernelTests
    : public lzt::zeEventPoolTests,
      public ::testing::WithParamInterface<enum BarrierType> {};

TEST_P(
    zeBarrierKernelTests,
    GivenKernelFunctionsAndMemoryCopyWhenBarrierInsertedThenExecutionCompletesSuccesfully) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_context_handle_t context = lzt::create_context();

  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  size_t num_int = 1000;
  void *dev_buff = lzt::allocate_device_memory((num_int * sizeof(int)), 1, 0, 0,
                                               device, context);
  void *host_buff =
      lzt::allocate_host_memory((num_int * sizeof(int)), 1, context);

  int *p_host = static_cast<int *>(host_buff);
  enum BarrierType barrier_type = GetParam();

  // Initialize host buffer with postive and negative integer values
  const int addval_1 = -100;
  const int val_1 = 50000;
  int val = val_1;
  for (size_t i = 0; i < num_int; i++) {
    p_host[i] = val;
    val += addval_1;
  }

  // Setup 2 events if running EVENTS_BARRIER test.  Flags set for coherency
  // between device and host
  const size_t num_event = 2;
  std::vector<ze_event_handle_t> coherency_event(num_event, nullptr);
  ep.InitEventPool(context, 2);
  ep.create_events(coherency_event, num_event, ZE_EVENT_SCOPE_FLAG_HOST,
                   ZE_EVENT_SCOPE_FLAG_HOST);

  ze_event_handle_t signal_event_copy = nullptr;
  ze_event_handle_t signal_event_func1 = nullptr;
  uint32_t num_wait = 0;
  ze_event_handle_t *p_wait_event_func1 = nullptr;
  ze_event_handle_t *p_wait_event_func2 = nullptr;

  // Coherency test:
  // 1) Command list memcopy from Host to Device Buffer.
  // 2) First kernel function uses Device Buffer as input buffer and adds
  // constant 3) Second kernel function adds Device Buffer output of first to
  // Host buffer 4) Coherency via barriers or events needed betwen 1&2 and
  // between 2&3

  if (barrier_type == BT_EVENTS_BARRIER) {
    signal_event_copy = coherency_event[0];
    signal_event_func1 = coherency_event[1];
    num_wait = 1;
    p_wait_event_func1 = &coherency_event[0];
    p_wait_event_func2 = &coherency_event[1];
  }

  const int addval_2 = 50;
  ze_module_handle_t module =
      lzt::create_module(context, device, "barrier_add.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function_1 =
      lzt::create_function(module, "barrier_add_constant");
  lzt::set_group_size(function_1, 1, 1, 1);

  int *p_dev = static_cast<int *>(dev_buff);
  lzt::set_argument_value(function_1, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(function_1, 1, sizeof(addval_2), &addval_2);
  ze_group_count_t tg;
  tg.groupCountX = num_int;
  tg.groupCountY = 1;
  tg.groupCountZ = 1;

  lzt::append_memory_copy(cmd_list, dev_buff, host_buff, num_int * sizeof(int),
                          signal_event_copy);
  // Memory barrier to ensure memory coherency after copy to device memory
  if (barrier_type == BT_MEMORY_RANGES_BARRIER) {
    const std::vector<size_t> range_sizes{num_int * sizeof(int),
                                          num_int * sizeof(int)};
    std::vector<const void *> ranges{dev_buff, host_buff};
    lzt::append_memory_ranges_barrier(cmd_list, ranges.size(),
                                      range_sizes.data(), ranges.data(),
                                      nullptr, 0, nullptr);
  } else if (barrier_type == BT_GLOBAL_BARRIER) {
    lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  }
  lzt::append_launch_function(cmd_list, function_1, &tg, signal_event_func1,
                              num_wait, p_wait_event_func1);
  // Execution barrier to ensure function_1 completes before function_2 starts
  if (barrier_type != BT_EVENTS_BARRIER) {
    lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  }
  ze_kernel_handle_t function_2 =
      lzt::create_function(module, "barrier_add_two_arrays");
  lzt::set_group_size(function_2, 1, 1, 1);

  lzt::set_argument_value(function_2, 0, sizeof(p_host), &p_host);
  lzt::set_argument_value(function_2, 1, sizeof(p_dev), &p_dev);
  lzt::append_launch_function(cmd_list, function_2, &tg, nullptr, num_wait,
                              p_wait_event_func2);

  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
  lzt::synchronize(cmd_q, UINT64_MAX);
  val = (2 * val_1) + addval_2;
  for (size_t i = 0; i < num_int; i++) {
    EXPECT_EQ(p_host[i], val);
    val += (2 * addval_1);
  }

  lzt::destroy_function(function_2);
  lzt::destroy_function(function_1);
  lzt::destroy_module(module);
  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(context, host_buff);
  lzt::free_memory(context, dev_buff);
  ep.destroy_events(coherency_event);
  lzt::destroy_context(context);
}

INSTANTIATE_TEST_CASE_P(TestKernelWithBarrierAndMemoryRangesBarrier,
                        zeBarrierKernelTests,
                        testing::Values(BT_GLOBAL_BARRIER,
                                        BT_MEMORY_RANGES_BARRIER,
                                        BT_EVENTS_BARRIER));

} // namespace
