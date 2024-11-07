/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "random/random.hpp"
#include "logging/logging.hpp"
#include <chrono>
#include <thread>
#include "image/image.hpp"
namespace lzt = level_zero_tests;
#include <level_zero/ze_api.h>
#include <chrono>

namespace {

void print_cmdqueue_descriptor(const ze_command_queue_desc_t descriptor) {
  LOG_INFO << "VERSION = " << descriptor.stype
           << "   FLAG = " << descriptor.flags
           << "   MODE = " << descriptor.mode
           << "   PRIORITY = " << descriptor.priority
           << "   ORDINAL = " << descriptor.ordinal;
}

void print_cmdqueue_exec(const uint32_t num_command_lists,
                         const uint32_t sync_timeout) {
  LOG_INFO << " num_command_lists = " << num_command_lists
           << " sync_timeout = " << sync_timeout;
}

class zeCommandQueueCreateTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flag_t, ze_command_queue_mode_t,
                     ze_command_queue_priority_t>> {};

TEST_P(zeCommandQueueCreateTests,
       GivenValidDescriptorWhenCreatingCommandQueueThenSuccessIsReturned) {

  ze_command_queue_desc_t descriptor = {};
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  descriptor.flags = std::get<0>(GetParam());
  descriptor.mode = std::get<1>(GetParam());
  descriptor.priority = std::get<2>(GetParam());

  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  const ze_driver_handle_t driver = lzt::get_default_driver();
  const ze_context_handle_t context = lzt::get_default_context();

  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES,
                                       nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &properties));

  auto cmd_q_group_properties = lzt::get_command_queue_group_properties(device);

  for (int i = 0; i < cmd_q_group_properties.size(); i++) {
    descriptor.ordinal = i;
    print_cmdqueue_descriptor(descriptor);

    ze_command_queue_handle_t command_queue = nullptr;
    EXPECT_EQ(
        ZE_RESULT_SUCCESS,
        zeCommandQueueCreate(context, device, &descriptor, &command_queue));
    EXPECT_NE(nullptr, command_queue);

    lzt::destroy_command_queue(command_queue);
  }
}

INSTANTIATE_TEST_SUITE_P(
    TestAllInputPermuations, zeCommandQueueCreateTests,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
                          ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW,
                          ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH)));

class zeCommandQueueDestroyTests : public ::testing::Test {};

TEST_F(
    zeCommandQueueDestroyTests,
    GivenValidDeviceAndNonNullCommandQueueWhenDestroyingCommandQueueThenSuccessIsReturned) {
  ze_command_queue_desc_t descriptor = {};
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;

  descriptor.pNext = nullptr;

  ze_command_queue_handle_t command_queue = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandQueueCreate(lzt::get_default_context(),
                                 lzt::zeDevice::get_instance()->get_device(),
                                 &descriptor, &command_queue));
  EXPECT_NE(nullptr, command_queue);

  lzt::destroy_command_queue(command_queue);
}

struct CustomExecuteParams {
  uint32_t num_command_lists;
  uint64_t sync_timeout;
};

class zeCommandQueueExecuteCommandListTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<CustomExecuteParams> {
protected:
  void SetUp() override {
    const ze_device_handle_t device =
        lzt::zeDevice::get_instance()->get_device();
    const ze_driver_handle_t driver = lzt::get_default_driver();
    const ze_context_handle_t context = lzt::get_default_context();
    EXPECT_GT(params.num_command_lists, 0);

    print_cmdqueue_exec(params.num_command_lists, params.sync_timeout);

    for (uint32_t i = 0; i < params.num_command_lists; i++) {
      void *host_shared = nullptr;
      ze_device_mem_alloc_desc_t h_device_desc = {};
      h_device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

      h_device_desc.pNext = nullptr;
      h_device_desc.ordinal = 1;
      h_device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
      ze_host_mem_alloc_desc_t h_host_desc = {};
      h_host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

      h_host_desc.pNext = nullptr;
      h_host_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeMemAllocShared(context, &h_device_desc, &h_host_desc,
                                 buff_size_bytes, 1, device, &host_shared));

      EXPECT_NE(nullptr, host_shared);
      host_buffer.push_back(static_cast<uint8_t *>(host_shared));
      void *device_shared = nullptr;
      ze_device_mem_alloc_desc_t d_device_desc = {};
      d_device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

      d_device_desc.pNext = nullptr;
      d_device_desc.ordinal = 1;
      d_device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
      ze_host_mem_alloc_desc_t d_host_desc = {};
      d_host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

      d_host_desc.pNext = nullptr;
      d_host_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeMemAllocShared(context, &d_device_desc, &d_host_desc,
                                 buff_size_bytes, 1, device, &device_shared));

      EXPECT_NE(nullptr, device_shared);
      device_buffer.push_back(static_cast<uint8_t *>(device_shared));
      ze_command_list_handle_t command_list;
      ze_command_list_desc_t list_descriptor = {};
      list_descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;

      list_descriptor.pNext = nullptr;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListCreate(context, device, &list_descriptor,
                                    &command_list));
      EXPECT_NE(nullptr, command_list);

      uint8_t *char_input = static_cast<uint8_t *>(host_shared);
      for (uint32_t j = 0; j < buff_size_bytes; j++) {
        char_input[j] = lzt::generate_value<uint8_t>(0, 255, 0);
      }
      lzt::append_memory_copy(command_list, device_buffer.at(i),
                              host_buffer.at(i), buff_size_bytes, nullptr);
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(command_list));
      list_of_command_lists.push_back(command_list);
    }

    ze_command_queue_desc_t queue_descriptor = {};
    queue_descriptor.flags = 0;
    queue_descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;

    queue_descriptor.pNext = nullptr;
    queue_descriptor.mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;
    queue_descriptor.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_descriptor.ordinal = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueCreate(context, device, &queue_descriptor,
                                   &command_queue));
    EXPECT_NE(nullptr, command_queue);
  }

  void TearDown() override {
    for (uint32_t i = 0; i < params.num_command_lists; i++) {
      EXPECT_EQ(
          0, memcmp(host_buffer.at(i), device_buffer.at(i), buff_size_bytes));
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListDestroy(list_of_command_lists.at(i)));
      lzt::free_memory(host_buffer.at(i));
      lzt::free_memory(device_buffer.at(i));
    }
    lzt::destroy_command_queue(command_queue);
  }

  const uint32_t buff_size_bytes = 12;
  CustomExecuteParams params = GetParam();

  ze_command_queue_handle_t command_queue = nullptr;
  std::vector<ze_command_list_handle_t> list_of_command_lists;
  std::vector<uint8_t *> host_buffer;
  std::vector<uint8_t *> device_buffer;
};

class zeCommandQueueExecuteCommandListTestsSynchronize
    : public zeCommandQueueExecuteCommandListTests {};

TEST_P(
    zeCommandQueueExecuteCommandListTestsSynchronize,
    GivenCommandQueueSynchronizationWhenExecutingCommandListsThenSuccessIsReturned) {
  auto command_lists_initial = list_of_command_lists;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                   command_queue, params.num_command_lists,
                                   list_of_command_lists.data(), nullptr));
  for (int i = 0; i < list_of_command_lists.size(); i++) {
    ASSERT_EQ(list_of_command_lists[i], command_lists_initial[i]);
  }
  ze_result_t sync_status = ZE_RESULT_NOT_READY;
  while (sync_status != ZE_RESULT_SUCCESS) {
    EXPECT_EQ(sync_status, ZE_RESULT_NOT_READY);
    sync_status = zeCommandQueueSynchronize(command_queue, params.sync_timeout);
    std::this_thread::yield();
  }
}

CustomExecuteParams synchronize_test_input[] = {{1, 0},
                                                {2, UINT32_MAX >> 30},
                                                {3, UINT32_MAX >> 28},
                                                {4, UINT32_MAX >> 26},
                                                {5, UINT32_MAX >> 24},
                                                {6, UINT32_MAX >> 22},
                                                {7, UINT32_MAX >> 20},
                                                {8, UINT32_MAX >> 18},
                                                {9, UINT32_MAX >> 16},
                                                {10, UINT32_MAX >> 8},
                                                {20, UINT32_MAX >> 4},
                                                {30, UINT32_MAX >> 2},
                                                {50, UINT32_MAX >> 1},
                                                {100, UINT64_MAX}};

INSTANTIATE_TEST_SUITE_P(TestIncreasingNumberCommandListsWithSynchronize,
                         zeCommandQueueExecuteCommandListTestsSynchronize,
                         testing::ValuesIn(synchronize_test_input));

class zeCommandQueueExecuteCommandListTestsFence
    : public zeCommandQueueExecuteCommandListTests {};

TEST_P(
    zeCommandQueueExecuteCommandListTestsFence,
    GivenFenceSynchronizationWhenExecutingCommandListsThenSuccessIsReturned) {
  ze_fence_desc_t fence_descriptor = {};
  fence_descriptor.stype = ZE_STRUCTURE_TYPE_FENCE_DESC;

  fence_descriptor.pNext = nullptr;
  ze_fence_handle_t hFence = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFenceCreate(command_queue, &fence_descriptor, &hFence));
  EXPECT_NE(nullptr, hFence);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                   command_queue, params.num_command_lists,
                                   list_of_command_lists.data(), hFence));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFenceHostSynchronize(hFence, params.sync_timeout));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceDestroy(hFence));
}

CustomExecuteParams fence_test_input[] = {
    {1, UINT64_MAX},  {2, UINT64_MAX},  {3, UINT64_MAX},  {4, UINT64_MAX},
    {5, UINT64_MAX},  {6, UINT64_MAX},  {7, UINT64_MAX},  {8, UINT64_MAX},
    {9, UINT64_MAX},  {10, UINT64_MAX}, {20, UINT64_MAX}, {30, UINT64_MAX},
    {50, UINT64_MAX}, {100, UINT64_MAX}};

INSTANTIATE_TEST_SUITE_P(TestIncreasingNumberCommandListsWithCommandQueueFence,
                         zeCommandQueueExecuteCommandListTestsFence,
                         testing::ValuesIn(fence_test_input));

static void ExecuteCommandQueue(ze_command_queue_handle_t &cq,
                                ze_command_list_handle_t &cl,
                                std::promise<bool> &exited) {
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  exited.set_value(true);
}

class zeCommandQueueSynchronousTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_command_queue_mode_t> {};

TEST_P(
    zeCommandQueueSynchronousTests,
    GivenModeWhenCreatingCommandQueueThenSynchronousOrAsynchronousBehaviorIsConfirmed) {
  ze_command_queue_mode_t mode = GetParam();
  ze_command_queue_handle_t cq = lzt::create_command_queue(mode);
  ze_command_list_handle_t cl = lzt::create_command_list();
  lzt::zeEventPool ep;
  ze_event_handle_t hEvent;
  ze_event_handle_t hSignalEvent;
  std::promise<bool> exitPromise;
  auto exitFuture = exitPromise.get_future();

  ep.create_event(hEvent, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hSignalEvent, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(hEvent, 0));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(hSignalEvent, 0));

  lzt::append_signal_event(cl, hSignalEvent);
  lzt::append_wait_on_events(cl, 1, &hEvent);
  lzt::close_command_list(cl);

  std::thread child_thread(&ExecuteCommandQueue, std::ref(cq), std::ref(cl),
                           std::ref(exitPromise));

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeEventHostSynchronize(hSignalEvent, UINT64_MAX));

  auto status = exitFuture.wait_for(std::chrono::milliseconds(500));
  if (mode == ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS) {
    EXPECT_EQ(status, std::future_status::ready);
  } else {
    EXPECT_EQ(status, std::future_status::timeout);
  }

  // Note: if the command queue has a mode of: ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS
  // It won't ever finish unless we signal the event that is waiting on:
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(hEvent));

  child_thread.join();
  if (mode == ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS) {
    lzt::synchronize(cq, UINT64_MAX);
  }

  ep.destroy_event(hEvent);
  ep.destroy_event(hSignalEvent);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
}

INSTANTIATE_TEST_SUITE_P(SynchronousAndAsynchronousCommandQueueTests,
                         zeCommandQueueSynchronousTests,
                         testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                         ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS));

class CommandQueueFlagTest : public ::testing::Test {
protected:
  const uint32_t buff_size_bytes = 16;
  ze_command_queue_handle_t command_queue = nullptr;
  void *host_buffer;
  void *device_buffer;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  uint8_t *char_input;
  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES,
                                       nullptr};

  CommandQueueFlagTest() {
    host_buffer = lzt::allocate_shared_memory(buff_size_bytes);
    device_buffer = lzt::allocate_shared_memory(buff_size_bytes);
    char_input = static_cast<uint8_t *>(host_buffer);
    properties = lzt::get_device_properties(device);
    for (uint32_t i = 0; i < buff_size_bytes; i++) {
      char_input[i] = lzt::generate_value<uint8_t>(0, 255, 0);
    }
  }
  ~CommandQueueFlagTest() {
    lzt::free_memory(host_buffer);
    lzt::free_memory(device_buffer);
  }
};

TEST_F(
    CommandQueueFlagTest,
    GivenDefaultFlagInCommandListWhenCommandQueueFlagisExplicitOnlyThenSucessIsReturned) {
  ze_command_list_handle_t command_list = lzt::create_command_list(device);

  lzt::append_memory_copy(command_list, device_buffer, host_buffer,
                          buff_size_bytes, nullptr);
  lzt::append_barrier(command_list);
  lzt::close_command_list(command_list);

  command_queue = lzt::create_command_queue(
      device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY,
      ZE_COMMAND_QUEUE_MODE_DEFAULT, ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);
  EXPECT_EQ(0, memcmp(host_buffer, device_buffer, buff_size_bytes));
  lzt::reset_command_list(command_list);
  for (uint32_t i = 0; i < buff_size_bytes; i++) {
    char_input[i] = lzt::generate_value<uint8_t>(0, 255, 0);
  }
  lzt::append_memory_copy(command_list, device_buffer, host_buffer,
                          buff_size_bytes, nullptr);
  lzt::append_barrier(command_list);
  lzt::close_command_list(command_list);
  ze_fence_handle_t hFence = lzt::create_fence(command_queue);
  lzt::execute_command_lists(command_queue, 1, &command_list, hFence);
  EXPECT_EQ(ZE_RESULT_SUCCESS, lzt::sync_fence(hFence, UINT64_MAX));

  EXPECT_EQ(0, memcmp(host_buffer, device_buffer, buff_size_bytes));

  /*cleanup*/
  lzt::destroy_fence(hFence);
  lzt::synchronize(command_queue, UINT64_MAX);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_command_list(command_list);
}

TEST(
    CommandQueuePriorityTest,
    GivenConcurrentLogicalCommandQueuesWhenStartSynchronizedThenHighPriorityCompletesFirst) {
  const int buff_size_high = 1 << 10;
  const int buff_size_low = 1 << 20;
  const uint8_t value_high = 0x55;
  const uint8_t value_low = 0x22;
  std::vector<ze_device_handle_t> devices;
  std::vector<ze_device_handle_t> sub_devices;
  ze_device_handle_t device0 =
      lzt::get_default_device(lzt::get_default_driver());
  devices.push_back(device0);
  if (lzt::get_ze_sub_device_count(device0) > 0) {
    sub_devices = lzt::get_ze_sub_devices(device0);
    devices.insert(devices.end(), sub_devices.begin(), sub_devices.end());
    sub_devices.clear();
  }
  for (uint32_t num_low_priority_compute_queues = 5;
       num_low_priority_compute_queues < 20;
       num_low_priority_compute_queues++) {
    LOG_INFO << "TESTING WITH " << num_low_priority_compute_queues
             << " LOW-PRIORITY QUEUES";
    uint32_t device_index = 0;
    for (auto device : devices) {
      if (device_index == 0) {
        LOG_INFO << "Testing Root Device:  Index = " << device_index;
      } else {
        LOG_INFO << "Testing Sub Device:  Index = " << device_index;
      }
      const ze_context_handle_t context = lzt::get_default_context();
      ze_event_pool_desc_t ep_time_desc = {};
      ep_time_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
      ep_time_desc.flags =
          ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
      ep_time_desc.count = 256;
      auto ep_time = lzt::create_event_pool(context, ep_time_desc);
      ze_event_pool_desc_t ep_sync_desc = {};
      ep_sync_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
      ep_sync_desc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
      ep_sync_desc.count = 1;
      auto ep_sync = lzt::create_event_pool(context, ep_sync_desc);
      ze_event_desc_t event_desc = {};
      event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
      event_desc.index = 0;
      event_desc.signal = 0;
      event_desc.wait = 0;
      auto event_compute_high = lzt::create_event(ep_time, event_desc);
      auto event_sync = lzt::create_event(ep_sync, event_desc);
      std::vector<ze_event_handle_t> event_compute_lows;

      auto cmdqueue_compute_high = lzt::create_command_queue(
          device, static_cast<ze_command_queue_flag_t>(0),
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
          ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH, 0);

      std::vector<ze_command_queue_handle_t> cmdqueue_compute_lows;
      std::vector<ze_command_list_handle_t> cmdlist_compute_lows;

      std::vector<void *> buffer_compute_lows;
      for (int i = 0; i < num_low_priority_compute_queues; i++) {
        auto cmdqueue_compute_low = lzt::create_command_queue(
            device, static_cast<ze_command_queue_flag_t>(0),
            ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
            ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW, 0);
        cmdqueue_compute_lows.push_back(cmdqueue_compute_low);

        auto cmdlist_compute_low = lzt::create_command_list(device);
        cmdlist_compute_lows.push_back(cmdlist_compute_low);

        auto buffer_compute_low = lzt::allocate_shared_memory(buff_size_low);
        buffer_compute_lows.push_back(buffer_compute_low);

        event_desc.index = i + 1;
        auto event_compute_low = lzt::create_event(ep_time, event_desc);
        event_compute_lows.push_back(event_compute_low);
      }

      auto cmdlist_compute_high = lzt::create_command_list(device);
      auto buffer_compute_high = lzt::allocate_shared_memory(buff_size_high);

      lzt::append_wait_on_events(cmdlist_compute_high, 1, &event_sync);
      lzt::append_memory_set(cmdlist_compute_high, buffer_compute_high,
                             &value_high, buff_size_high, event_compute_high);
      lzt::close_command_list(cmdlist_compute_high);

      for (int i = 0; i < num_low_priority_compute_queues; i++) {
        auto cmdlist_compute_low = cmdlist_compute_lows[i];
        auto buffer_compute_low = buffer_compute_lows[i];
        auto event_compute_low = event_compute_lows[i];
        lzt::append_wait_on_events(cmdlist_compute_low, 1, &event_sync);
        lzt::append_memory_set(cmdlist_compute_low, buffer_compute_low,
                               &value_low, buff_size_low, event_compute_low);
        lzt::close_command_list(cmdlist_compute_low);
      }

      for (int i = 0; i < num_low_priority_compute_queues; i++) {
        auto cmdqueue_compute_low = cmdqueue_compute_lows[i];
        auto cmdlist_compute_low = cmdlist_compute_lows[i];

        lzt::execute_command_lists(cmdqueue_compute_low, 1,
                                   &cmdlist_compute_low, nullptr);
      }
      lzt::execute_command_lists(cmdqueue_compute_high, 1,
                                 &cmdlist_compute_high, nullptr);
      lzt::signal_event_from_host(event_sync);

      for (int i = 0; i < num_low_priority_compute_queues; i++) {
        auto cmdqueue_compute_low = cmdqueue_compute_lows[i];
        lzt::synchronize(cmdqueue_compute_low, UINT64_MAX);
      }
      lzt::synchronize(cmdqueue_compute_high, UINT64_MAX);

      uint8_t *uchar_compute_high = static_cast<uint8_t *>(buffer_compute_high);
      for (size_t i = 0; i < buff_size_high; i++) {
        EXPECT_EQ(value_high, uchar_compute_high[i]);
      }

      for (int i = 0; i < num_low_priority_compute_queues; i++) {
        auto buffer_compute_low = buffer_compute_lows[i];
        uint8_t *uchar_compute_low = static_cast<uint8_t *>(buffer_compute_low);
        for (size_t i = 0; i < buff_size_low; i++) {
          EXPECT_EQ(value_low, uchar_compute_low[i]);
        }
      }

      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_compute_high));

      ze_kernel_timestamp_result_t event_compute_high_timestamp =
          lzt::get_event_kernel_timestamp(event_compute_high);

      uint32_t
          number_of_low_priority_queues_finished_after_high_priority_queue = 0;
      for (int i = 0; i < num_low_priority_compute_queues; i++) {
        auto event_compute_low = event_compute_lows[i];
        EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_compute_low));
        ze_kernel_timestamp_result_t event_compute_low_timestamp =
            lzt::get_event_kernel_timestamp(event_compute_low);

        if (event_compute_low_timestamp.global.kernelEnd >
            event_compute_high_timestamp.global.kernelEnd) {
          number_of_low_priority_queues_finished_after_high_priority_queue++;
        }
      }

      if ((devices.size() > 1) && (device_index == 0)) {
        EXPECT_EQ(
            number_of_low_priority_queues_finished_after_high_priority_queue,
            0u);
      } else {
        EXPECT_GE(
            number_of_low_priority_queues_finished_after_high_priority_queue,
            1u);
      }
      LOG_INFO
          << "number_of_low_priority_queues_finished_after_high_priority_queue "
          << " = "
          << number_of_low_priority_queues_finished_after_high_priority_queue;
      /*cleanup*/

      for (int i = 0; i < num_low_priority_compute_queues; i++) {
        auto cmdqueue_compute_low = cmdqueue_compute_lows[i];
        lzt::destroy_command_queue(cmdqueue_compute_low);
        auto cmdlist_compute_low = cmdlist_compute_lows[i];
        lzt::destroy_command_list(cmdlist_compute_low);
        auto buffer_compute_low = buffer_compute_lows[i];
        lzt::free_memory(buffer_compute_low);
      }

      lzt::free_memory(buffer_compute_high);
      lzt::destroy_command_queue(cmdqueue_compute_high);
      lzt::destroy_command_list(cmdlist_compute_high);

      lzt::destroy_event(event_compute_high);

      for (int i = 0; i < num_low_priority_compute_queues; i++) {
        auto event_compute_low = event_compute_lows[i];
        lzt::destroy_event(event_compute_low);
      }
      lzt::destroy_event(event_sync);
      lzt::destroy_event_pool(ep_sync);
      device_index++;
    }
  }
}

class CommandQueueCopyOnlyTest : public ::testing::Test {
protected:
  CommandQueueCopyOnlyTest() {
    device = lzt::get_default_device(lzt::get_default_driver());
    context = lzt::create_context();
  }
  ~CommandQueueCopyOnlyTest() { lzt::destroy_context(context); }

  ze_device_handle_t device;
  ze_context_handle_t context;
  ze_command_list_handle_t cmdlist;

public:
  bool get_copy_only_cmd_queue_and_list(ze_command_list_handle_t &cmdlist,
                                        ze_command_queue_handle_t &cmdqueue,
                                        bool use_sync_queue) {
    auto cmdq_group_properties =
        lzt::get_command_queue_group_properties(device);
    int copy_ordinal = -1;
    for (int i = 0; i < cmdq_group_properties.size(); i++) {
      if ((cmdq_group_properties[i].flags &
           ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) &&
          !(cmdq_group_properties[i].flags &
            ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE)) {

        if (cmdq_group_properties[i].numQueues == 0)
          continue;

        copy_ordinal = i;
        break;
      }
    }
    if (copy_ordinal < 0) {
      return false;
    }

    ze_command_queue_mode_t mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (use_sync_queue) {
      mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
    }

    cmdqueue = lzt::create_command_queue(context, device, 0, mode,
                                         ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
                                         copy_ordinal);

    cmdlist = lzt::create_command_list(context, device, 0, copy_ordinal);

    return true;
  }
};

class CommandQueueCopyOnlyTestQueueMode
    : public CommandQueueCopyOnlyTest,
      public ::testing::WithParamInterface<bool> {};

TEST_P(CommandQueueCopyOnlyTestQueueMode,
       GivenCopyOnlyCommandQueueWhenCopyingMemoryThenResultIsCorrect) {

  ze_command_list_handle_t cmdlist;
  ze_command_queue_handle_t cmdqueue;
  if (get_copy_only_cmd_queue_and_list(cmdlist, cmdqueue, GetParam()) ==
      false) {
    GTEST_SKIP() << "No Copy-Only command queue group found, can't run test";
  }

  const size_t size = 1000;
  auto source = lzt::allocate_host_memory(size, 1, context);
  auto dest = lzt::allocate_host_memory(size, 1, context);
  memset(source, 1, size);
  memset(dest, 2, size);

  lzt::append_memory_copy(cmdlist, dest, source, size);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);

  ASSERT_EQ(0, memcmp(dest, source, size));

  lzt::free_memory(context, source);
  lzt::free_memory(context, dest);
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
}

TEST_P(CommandQueueCopyOnlyTestQueueMode,
       GivenCopyOnlyCommandQueueWhenCopyingImageThenResultIsCorrect) {

  ze_command_list_handle_t cmdlist;
  ze_command_queue_handle_t cmdqueue;
  if (get_copy_only_cmd_queue_and_list(cmdlist, cmdqueue, GetParam()) ==
      false) {
    GTEST_SKIP() << "No Copy-Only command queue group found, can't run test";
  }
  if (!(lzt::image_support())) {
    GTEST_SKIP() << "device does not support images, cannot run test";
  }
  ze_image_handle_t ze_img_src, ze_img_dest;
  lzt::ImagePNG32Bit png_img_src, png_img_dest;

  png_img_src = lzt::ImagePNG32Bit("test_input.png");
  auto image_width = png_img_src.width();
  auto image_height = png_img_src.height();
  auto image_size = image_width * image_height * sizeof(uint32_t);
  png_img_dest = lzt::ImagePNG32Bit(image_width, image_height);

  ze_image_desc_t img_desc = {};
  img_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  img_desc.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
  img_desc.type = ZE_IMAGE_TYPE_2D;
  img_desc.format = {ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_TYPE_UNORM,
                     ZE_IMAGE_FORMAT_SWIZZLE_R,      ZE_IMAGE_FORMAT_SWIZZLE_G,
                     ZE_IMAGE_FORMAT_SWIZZLE_B,      ZE_IMAGE_FORMAT_SWIZZLE_A};
  img_desc.width = image_width;
  img_desc.height = image_height;
  img_desc.depth = 1;
  img_desc.arraylevels = 0;
  img_desc.miplevels = 0;
  ze_img_src = lzt::create_ze_image(context, device, img_desc);
  ze_img_dest = lzt::create_ze_image(context, device, img_desc);

  lzt::append_image_copy_from_mem(cmdlist, ze_img_src, png_img_src.raw_data(),
                                  nullptr);
  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
  lzt::append_image_copy(cmdlist, ze_img_dest, ze_img_src, nullptr);
  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
  lzt::append_image_copy_to_mem(cmdlist, png_img_dest.raw_data(), ze_img_dest,
                                nullptr);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);

  EXPECT_EQ(png_img_src, png_img_dest);

  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
  lzt::destroy_ze_image(ze_img_src);
  lzt::destroy_ze_image(ze_img_dest);
}

INSTANTIATE_TEST_SUITE_P(TestCopyOnlyQueueSyncAndAsync,
                         CommandQueueCopyOnlyTestQueueMode,
                         testing::Values(true, false));

class CommandQueueCopyOnlyTestForGlobalTimeStamp
    : public CommandQueueCopyOnlyTest,
      public ::testing::WithParamInterface<std::string> {};

TEST_P(
    CommandQueueCopyOnlyTestForGlobalTimeStamp,
    GivenCopyOnlyCommandQueueWhenGlobalTimeStampIsRequestedThenResultIsValid) {

  ze_command_list_handle_t cmdlist;
  ze_command_queue_handle_t cmdqueue;
  if (get_copy_only_cmd_queue_and_list(cmdlist, cmdqueue, true) == false) {
    GTEST_SKIP() << "No Copy-Only command queue group found, can't run test";
  }

  // Prepare for global time stamp
  const size_t ts_size = 8;
  void *ts_start = nullptr;
  void *ts_end = nullptr;
  if (GetParam() == "DeviceMemory") {
    ts_start = lzt::allocate_device_memory(ts_size, 8, 0, context);
    ts_end = lzt::allocate_device_memory(ts_size, 8, 0, context);
  } else if (GetParam() == "HostMemory") {
    ts_start = lzt::allocate_host_memory(ts_size, 8, context);
    ts_end = lzt::allocate_host_memory(ts_size, 8, context);
  } else {
    FAIL() << "Unsupported Input \"" << GetParam() << "\" for the Test";
    return;
  }

  auto ts_readback_start = lzt::allocate_shared_memory(ts_size, 8, context);
  auto ts_readback_end = lzt::allocate_shared_memory(ts_size, 8, context);

  // Prepare for copy operation
  const size_t copy_size = 8192;
  auto copy_source = lzt::allocate_host_memory(copy_size, 1, context);
  auto copy_dest = lzt::allocate_host_memory(copy_size, 1, context);
  memset(copy_source, 1, copy_size);

  // Initial clear
  const uint8_t clear_value = 0;
  lzt::append_memory_set(cmdlist, ts_start, &clear_value, ts_size);
  lzt::append_memory_set(cmdlist, ts_end, &clear_value, ts_size);
  lzt::append_barrier(cmdlist);

  // Get timestamp
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendWriteGlobalTimestamp(
                                   cmdlist, static_cast<uint64_t *>(ts_start),
                                   nullptr, 0, nullptr));
  lzt::append_memory_copy(cmdlist, copy_dest, copy_source, copy_size);
  lzt::append_barrier(cmdlist);
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendWriteGlobalTimestamp(
                cmdlist, static_cast<uint64_t *>(ts_end), nullptr, 0, nullptr));
  // Readback
  lzt::append_memory_copy(cmdlist, ts_readback_start, ts_start, ts_size);
  lzt::append_memory_copy(cmdlist, ts_readback_end, ts_end, ts_size);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);

  auto ts_readback_start_alias = static_cast<uint64_t *>(ts_readback_start);
  auto ts_readback_end_alias = static_cast<uint64_t *>(ts_readback_end);

  LOG_DEBUG << "Global Time Stamp: start[" << *ts_readback_start_alias
            << "] end[" << *ts_readback_end_alias << "] diff["
            << (*ts_readback_end_alias - *ts_readback_start_alias) << "]\n";

  EXPECT_NE(*ts_readback_start_alias, 0);
  EXPECT_NE(*ts_readback_end_alias, *ts_readback_start_alias);

  lzt::free_memory(context, copy_source);
  lzt::free_memory(context, copy_dest);
  lzt::free_memory(context, ts_start);
  lzt::free_memory(context, ts_end);
  lzt::free_memory(context, ts_readback_start);
  lzt::free_memory(context, ts_readback_end);
}

INSTANTIATE_TEST_SUITE_P(TestCopyOnlyQueueGlobalTimeStampDeviceAndHostMemory,
                         CommandQueueCopyOnlyTestForGlobalTimeStamp,
                         testing::Values("HostMemory", "DeviceMemory"));

TEST(ConcurrentCommandQueueExecutionTests,
     GivenMultipleCommandQueuesWhenExecutedOnSameDeviceResultsAreCorrect) {
  const size_t num_queues = 100;
  const size_t buff_size = 10000;
  uint32_t ordinal = 0;
  uint32_t index = 0;
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);
  auto cmdq_groups = lzt::get_command_queue_group_properties(device);

  std::vector<int> num_queues_per_group;
  for (auto properties : cmdq_groups) {
    num_queues_per_group.push_back(properties.numQueues);
  }

  struct queue_data_t {
    ze_command_queue_handle_t cmdqueue;
    ze_command_list_handle_t cmdlist;
    void *src_buff;
    void *dst_buff;
    uint8_t data;
  };
  std::vector<queue_data_t> queues;

  for (size_t i = 0; i < num_queues; i++) {
    if (index >= num_queues_per_group[ordinal]) {
      index = 0;
      ordinal = (ordinal + 1) % cmdq_groups.size();
    }
    queue_data_t queue;
    queue.cmdqueue = lzt::create_command_queue(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, index);
    queue.cmdlist = lzt::create_command_list(context, device, 0, ordinal);
    queue.src_buff = lzt::allocate_host_memory(buff_size, 1, context);
    memset(queue.src_buff, 0, buff_size);
    queue.dst_buff = lzt::allocate_host_memory(buff_size, 1, context);
    memset(queue.dst_buff, 0, buff_size);
    queue.data = i;
    queues.push_back(queue);

    index++;
  }

  // populate each command list with operations
  for (auto queue : queues) {
    lzt::append_memory_set(queue.cmdlist, queue.src_buff, &queue.data,
                           buff_size);
    lzt::append_barrier(queue.cmdlist);
    lzt::append_memory_copy(queue.cmdlist, queue.dst_buff, queue.src_buff,
                            buff_size);
    lzt::close_command_list(queue.cmdlist);
  }

  // execute all
  for (auto queue : queues) {
    lzt::execute_command_lists(queue.cmdqueue, 1, &queue.cmdlist, nullptr);
  }

  // Wait for completion and verify
  for (auto queue : queues) {
    lzt::synchronize(queue.cmdqueue, UINT64_MAX);
    for (size_t i = 0; i < buff_size; i++) {
      ASSERT_EQ(queue.data, ((uint8_t *)queue.src_buff)[i]);
    }
    ASSERT_EQ(0, memcmp(queue.src_buff, queue.dst_buff, buff_size));
  }

  // cleanup
  for (auto queue : queues) {
    lzt::destroy_command_list(queue.cmdlist);
    lzt::destroy_command_queue(queue.cmdqueue);
    lzt::free_memory(context, queue.src_buff);
    lzt::free_memory(context, queue.dst_buff);
  }
  lzt::destroy_context(context);
}

class zeCommandQueueSynchronizeTimeoutTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_event_pool_flags_t, ze_command_queue_flag_t,
                     ze_command_list_flags_t, bool, bool>> {
protected:
  void SetUp() override {
    const auto ep_flags =
        std::get<0>(GetParam()) | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    const auto cq_flags = std::get<1>(GetParam());
    const auto cl_flags = std::get<2>(GetParam());

    auto context = lzt::get_default_context();
    auto device = lzt::get_default_device(lzt::get_default_driver());

    ze_event_pool_desc_t ep_desc{};
    ep_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    ep_desc.pNext = nullptr;
    ep_desc.flags = ep_flags;
    ep_desc.count = 1;
    ep = lzt::create_event_pool(context, ep_desc);

    ze_event_desc_t ev_desc{};
    ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    ev_desc.pNext = nullptr;
    ev_desc.index = 0;
    ev_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ev = lzt::create_event(ep, ev_desc);

    cq = lzt::create_command_queue(context, device, cq_flags,
                                   ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                   ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0);

    cl = lzt::create_command_list(context, device, cl_flags, 0);
  }

  void TearDown() override {
    lzt::destroy_event(ev);
    lzt::destroy_event_pool(ep);
    lzt::destroy_command_list(cl);
    lzt::destroy_command_queue(cq);
  }

  const uint64_t timeout = 5000000;
  ze_event_pool_handle_t ep = nullptr;
  ze_event_handle_t ev = nullptr;
  ze_command_queue_handle_t cq = nullptr;
  ze_command_list_handle_t cl = nullptr;
  ze_fence_handle_t fence = nullptr;
};

TEST_P(zeCommandQueueSynchronizeTimeoutTests,
       GivenTimeoutWhenWaitingForCommandQueueThenWaitForSpecifiedTime) {
  const bool use_fence = std::get<3>(GetParam());
  const bool use_barrier = std::get<4>(GetParam());

  if (use_fence) {
    fence = lzt::create_fence(cq);
  }

  if (use_barrier) {
    lzt::append_barrier(cl, nullptr, 1, &ev);
  } else {
    lzt::append_wait_on_events(cl, 1, &ev);
  }
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, fence);

  if (use_fence) {
    const auto t0 = std::chrono::steady_clock::now();
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceHostSynchronize(fence, timeout));
    const auto t1 = std::chrono::steady_clock::now();

    const uint64_t wall_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    const float ratio_fence = static_cast<float>(wall_time) / timeout;
    // Tolerance: 2%
    EXPECT_GE(ratio_fence, 0.98f);
    EXPECT_LE(ratio_fence, 1.02f);
  }

  const auto t0 = std::chrono::steady_clock::now();
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeCommandQueueSynchronize(cq, timeout));
  const auto t1 = std::chrono::steady_clock::now();

  const uint64_t wall_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
  const float ratio_cq = static_cast<float>(wall_time) / timeout;
  // Tolerance: 2%
  EXPECT_GE(ratio_cq, 0.98f);
  EXPECT_LE(ratio_cq, 1.02f);

  lzt::signal_event_from_host(ev);
  lzt::synchronize(cq, UINT64_MAX);
  lzt::event_host_synchronize(ev, UINT64_MAX);

  if (use_fence) {
    lzt::sync_fence(fence, UINT64_MAX);
    lzt::destroy_fence(fence);
  }
}

INSTANTIATE_TEST_SUITE_P(
    SyncTimeoutParams, zeCommandQueueSynchronizeTimeoutTests,
    ::testing::Combine(
        ::testing::Values(0, ZE_EVENT_POOL_FLAG_IPC,
                          ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP,
                          ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP,
                          ZE_EVENT_POOL_FLAG_IPC |
                              ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP,
                          ZE_EVENT_POOL_FLAG_IPC |
                              ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP),
        ::testing::Values(0, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(0, ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY,
                          ZE_COMMAND_LIST_FLAG_IN_ORDER),
        ::testing::Bool(), ::testing::Bool()));

} // namespace
