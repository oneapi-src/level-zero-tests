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
#include "random/random.hpp"
#include "logging/logging.hpp"
#include <chrono>
#include <thread>
namespace lzt = level_zero_tests;
#include <level_zero/ze_api.h>

namespace {

void print_cmdqueue_descriptor(const ze_command_queue_desc_t descriptor) {
  LOG_INFO << "VERSION = " << descriptor.version
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

  ze_command_queue_desc_t descriptor = {
      ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT, // version
      std::get<0>(GetParam()),               // flags
      std::get<1>(GetParam()),               // mode
      std::get<2>(GetParam())                // priority
  };
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  const ze_driver_handle_t driver = lzt::get_default_driver();

  ze_device_properties_t properties;
  properties.version = ZE_DEVICE_PROPERTIES_VERSION_CURRENT;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &properties));

  if ((descriptor.flags == ZE_COMMAND_QUEUE_FLAG_NONE) ||
      (descriptor.flags == ZE_COMMAND_QUEUE_FLAG_SINGLE_SLICE_ONLY)) {
    EXPECT_GT(static_cast<uint32_t>(properties.numAsyncComputeEngines), 0);
    descriptor.ordinal =
        static_cast<uint32_t>(properties.numAsyncComputeEngines - 1);
  } else if (descriptor.flags == ZE_COMMAND_QUEUE_FLAG_COPY_ONLY) {
    EXPECT_GT(static_cast<uint32_t>(properties.numAsyncCopyEngines), 0);
    descriptor.ordinal =
        static_cast<uint32_t>(properties.numAsyncCopyEngines - 1);
  }
  print_cmdqueue_descriptor(descriptor);

  ze_command_queue_handle_t command_queue = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandQueueCreate(device, &descriptor, &command_queue));
  EXPECT_NE(nullptr, command_queue);

  lzt::destroy_command_queue(command_queue);
}

INSTANTIATE_TEST_CASE_P(
    TestAllInputPermuations, zeCommandQueueCreateTests,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_NONE,
                          ZE_COMMAND_QUEUE_FLAG_COPY_ONLY,
                          ZE_COMMAND_QUEUE_FLAG_LOGICAL_ONLY,
                          ZE_COMMAND_QUEUE_FLAG_SINGLE_SLICE_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
                          ZE_COMMAND_QUEUE_PRIORITY_LOW,
                          ZE_COMMAND_QUEUE_PRIORITY_HIGH)));

class zeCommandQueueDestroyTests : public ::testing::Test {};

TEST_F(
    zeCommandQueueDestroyTests,
    GivenValidDeviceAndNonNullCommandQueueWhenDestroyingCommandQueueThenSuccessIsReturned) {
  ze_command_queue_desc_t descriptor;
  descriptor.version = ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT;

  ze_command_queue_handle_t command_queue = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandQueueCreate(lzt::zeDevice::get_instance()->get_device(),
                                 &descriptor, &command_queue));
  EXPECT_NE(nullptr, command_queue);

  lzt::destroy_command_queue(command_queue);
}

struct CustomExecuteParams {
  uint32_t num_command_lists;
  uint32_t sync_timeout;
};

class zeCommandQueueExecuteCommandListTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<CustomExecuteParams> {
protected:
  void SetUp() override {
    const ze_device_handle_t device =
        lzt::zeDevice::get_instance()->get_device();
    const ze_driver_handle_t driver = lzt::get_default_driver();
    EXPECT_GT(params.num_command_lists, 0);

    print_cmdqueue_exec(params.num_command_lists, params.sync_timeout);
    ze_command_list_desc_t list_descriptor;
    list_descriptor.version = ZE_COMMAND_LIST_DESC_VERSION_CURRENT;

    for (uint32_t i = 0; i < params.num_command_lists; i++) {
      void *host_shared = nullptr;
      ze_device_mem_alloc_desc_t h_device_desc;
      h_device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
      h_device_desc.ordinal = 1;
      h_device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
      ze_host_mem_alloc_desc_t h_host_desc;
      h_host_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;
      h_host_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDriverAllocSharedMem(driver, &h_device_desc, &h_host_desc,
                                       buff_size_bytes, 1, device,
                                       &host_shared));

      EXPECT_NE(nullptr, host_shared);
      host_buffer.push_back(static_cast<uint8_t *>(host_shared));
      void *device_shared = nullptr;
      ze_device_mem_alloc_desc_t d_device_desc;
      d_device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
      d_device_desc.ordinal = 1;
      d_device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
      ze_host_mem_alloc_desc_t d_host_desc;
      d_host_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;
      d_host_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDriverAllocSharedMem(driver, &d_device_desc, &d_host_desc,
                                       buff_size_bytes, 1, device,
                                       &device_shared));

      EXPECT_NE(nullptr, device_shared);
      device_buffer.push_back(static_cast<uint8_t *>(device_shared));
      ze_command_list_handle_t command_list;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListCreate(device, &list_descriptor, &command_list));
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

    ze_command_queue_desc_t queue_descriptor;
    queue_descriptor.version = ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT;
    queue_descriptor.flags = ZE_COMMAND_QUEUE_FLAG_COPY_ONLY;
    queue_descriptor.mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;
    queue_descriptor.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;

    ze_device_properties_t properties;
    properties.version = ZE_DEVICE_PROPERTIES_VERSION_CURRENT;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &properties));
    EXPECT_GT((uint32_t)properties.numAsyncCopyEngines, 0);
    queue_descriptor.ordinal = (uint32_t)properties.numAsyncCopyEngines - 1;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueCreate(device, &queue_descriptor, &command_queue));
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

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                   command_queue, params.num_command_lists,
                                   list_of_command_lists.data(), nullptr));
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
                                                {100, UINT32_MAX}};

INSTANTIATE_TEST_CASE_P(TestIncreasingNumberCommandListsWithSynchronize,
                        zeCommandQueueExecuteCommandListTestsSynchronize,
                        testing::ValuesIn(synchronize_test_input));

class zeCommandQueueExecuteCommandListTestsFence
    : public zeCommandQueueExecuteCommandListTests {};

TEST_P(
    zeCommandQueueExecuteCommandListTestsFence,
    GivenFenceSynchronizationWhenExecutingCommandListsThenSuccessIsReturned) {

  ze_fence_desc_t fence_descriptor;
  fence_descriptor.version = ZE_FENCE_DESC_VERSION_CURRENT;
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
    {1, UINT32_MAX},  {2, UINT32_MAX},  {3, UINT32_MAX},  {4, UINT32_MAX},
    {5, UINT32_MAX},  {6, UINT32_MAX},  {7, UINT32_MAX},  {8, UINT32_MAX},
    {9, UINT32_MAX},  {10, UINT32_MAX}, {20, UINT32_MAX}, {30, UINT32_MAX},
    {50, UINT32_MAX}, {100, UINT32_MAX}};

INSTANTIATE_TEST_CASE_P(TestIncreasingNumberCommandListsWithCommandQueueFence,
                        zeCommandQueueExecuteCommandListTestsFence,
                        testing::ValuesIn(fence_test_input));

static void ExecuteCommandQueue(ze_command_queue_handle_t &cq,
                                ze_command_list_handle_t &cl,
                                volatile bool &exited) {
  exited = false;
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  exited = true;
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

  ep.create_event(hEvent, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_NONE);
  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(hEvent, 0));

  lzt::append_wait_on_events(cl, 1, &hEvent);
  lzt::close_command_list(cl);
  volatile bool exited = false;
  std::thread child_thread(&ExecuteCommandQueue, std::ref(cq), std::ref(cl),
                           std::ref(exited));

  // sleep for 5 seconds to give execution a chance to complete
  std::chrono::milliseconds timespan(500);
  std::this_thread::sleep_for(timespan);

  // We expect the child thread to exit if we are in async mode:
  if (exited)
    EXPECT_EQ(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, mode);
  else
    // We expect if we are in synchronous mode that the child thread will never
    // exit:

    EXPECT_EQ(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS, mode);

  // Note: if the command queue has a mode of: ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS
  // It won't ever finish unless we signal the event that is waiting on:
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(hEvent));

  child_thread.join();

  ep.destroy_event(hEvent);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
}

INSTANTIATE_TEST_CASE_P(SynchronousAndAsynchronousCommandQueueTests,
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
  ze_device_properties_t properties;

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
    GivenCopyFlagInCommandListWhenCommandQueueFlagisCopyThenSucessIsReturned) {
  ze_command_list_handle_t command_list =
      lzt::create_command_list(device, ZE_COMMAND_LIST_FLAG_COPY_ONLY);

  lzt::append_memory_copy(command_list, device_buffer, host_buffer,
                          buff_size_bytes, nullptr);
  lzt::append_barrier(command_list);
  lzt::close_command_list(command_list);

  EXPECT_GT((uint32_t)properties.numAsyncCopyEngines, 0);
  command_queue = lzt::create_command_queue(
      device, ZE_COMMAND_QUEUE_FLAG_COPY_ONLY, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
      ((uint32_t)properties.numAsyncCopyEngines - 1));

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT32_MAX);
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
  EXPECT_EQ(ZE_RESULT_SUCCESS, lzt::sync_fence(hFence, UINT32_MAX));

  EXPECT_EQ(0, memcmp(host_buffer, device_buffer, buff_size_bytes));

  /*cleanup*/
  lzt::destroy_fence(hFence);
  lzt::synchronize(command_queue, UINT32_MAX);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_command_list(command_list);
}

TEST_F(
    CommandQueueFlagTest,
    GivenCopyFlagInCommandListWhenCommandQueueFlagisLogicalThenSucessIsReturned) {

  ze_command_list_handle_t command_list =
      lzt::create_command_list(device, ZE_COMMAND_LIST_FLAG_COPY_ONLY);

  lzt::append_memory_copy(command_list, device_buffer, host_buffer,
                          buff_size_bytes, nullptr);
  lzt::append_barrier(command_list);
  lzt::close_command_list(command_list);

  ze_device_properties_t properties = lzt::get_device_properties(device);
  command_queue = lzt::create_command_queue(
      device, ZE_COMMAND_QUEUE_FLAG_LOGICAL_ONLY, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT32_MAX);
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
  EXPECT_EQ(ZE_RESULT_SUCCESS, lzt::sync_fence(hFence, UINT32_MAX));

  EXPECT_EQ(0, memcmp(host_buffer, device_buffer, buff_size_bytes));

  /*cleanup*/
  lzt::destroy_fence(hFence);
  lzt::synchronize(command_queue, UINT32_MAX);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_command_list(command_list);
}

TEST_F(
    CommandQueueFlagTest,
    GivenDefaultFlagInCommandListWhenCommandQueueFlagisSingleSliceOnlyThenSucessIsReturned) {
  ze_command_list_handle_t command_list =
      lzt::create_command_list(device, ZE_COMMAND_LIST_FLAG_NONE);

  lzt::append_memory_copy(command_list, device_buffer, host_buffer,
                          buff_size_bytes, nullptr);
  lzt::append_barrier(command_list);
  lzt::close_command_list(command_list);

  EXPECT_GT((uint32_t)properties.numAsyncComputeEngines, 0);
  command_queue = lzt::create_command_queue(
      device, ZE_COMMAND_QUEUE_FLAG_SINGLE_SLICE_ONLY,
      ZE_COMMAND_QUEUE_MODE_DEFAULT, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
      ((uint32_t)properties.numAsyncComputeEngines - 1));

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT32_MAX);
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
  EXPECT_EQ(ZE_RESULT_SUCCESS, lzt::sync_fence(hFence, UINT32_MAX));

  EXPECT_EQ(0, memcmp(host_buffer, device_buffer, buff_size_bytes));

  /*cleanup*/
  lzt::destroy_fence(hFence);
  lzt::synchronize(command_queue, UINT32_MAX);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_command_list(command_list);
}

TEST(
    CommandQueuePriorityTest,
    GivenConcurrentLogicalCommandQueuesWhenStartSynchronizedThenHighPriorityCompletesFirst) {

  const int buff_size_high = 1000;
  const int buff_size_low = 100000;
  const uint8_t value_high = 0x55;
  const uint8_t value_low = 0x22;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_event_pool_desc_t ep_time_desc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                       ZE_EVENT_POOL_FLAG_TIMESTAMP, 4};
  auto ep_time = lzt::create_event_pool(ep_time_desc);
  ze_event_pool_desc_t ep_sync_desc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                       ZE_EVENT_POOL_FLAG_DEFAULT, 1};
  auto ep_sync = lzt::create_event_pool(ep_sync_desc);
  ze_event_desc_t event_desc = {ZE_EVENT_DESC_VERSION_CURRENT, 0,
                                ZE_EVENT_SCOPE_FLAG_NONE,
                                ZE_EVENT_SCOPE_FLAG_NONE};
  auto event_compute_high = lzt::create_event(ep_time, event_desc);
  auto event_sync = lzt::create_event(ep_sync, event_desc);
  event_desc.index = 1;
  auto event_compute_low = lzt::create_event(ep_time, event_desc);
  event_desc.index = 2;
  auto event_copy_high = lzt::create_event(ep_time, event_desc);
  event_desc.index = 3;
  auto event_copy_low = lzt::create_event(ep_time, event_desc);
  auto cmdqueue_compute_high = lzt::create_command_queue(
      device, ZE_COMMAND_QUEUE_FLAG_NONE, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_HIGH, 0);
  auto cmdqueue_compute_low = lzt::create_command_queue(
      device, ZE_COMMAND_QUEUE_FLAG_NONE, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_LOW, 0);
  auto cmdqueue_copy_high = lzt::create_command_queue(
      device, ZE_COMMAND_QUEUE_FLAG_COPY_ONLY,
      ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_HIGH, 0);
  auto cmdqueue_copy_low = lzt::create_command_queue(
      device, ZE_COMMAND_QUEUE_FLAG_COPY_ONLY,
      ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_LOW, 0);

  auto cmdlist_compute_high = lzt::create_command_list();
  auto cmdlist_copy_high = lzt::create_command_list();
  auto cmdlist_compute_low = lzt::create_command_list();
  auto cmdlist_copy_low = lzt::create_command_list();
  auto buffer_compute_high = lzt::allocate_shared_memory(buff_size_high);
  auto buffer_copy_high = lzt::allocate_shared_memory(buff_size_high);
  auto buffer_compute_low = lzt::allocate_shared_memory(buff_size_low);
  auto buffer_copy_low = lzt::allocate_shared_memory(buff_size_low);

  lzt::append_wait_on_events(cmdlist_compute_high, 1, &event_sync);
  lzt::append_memory_set(cmdlist_compute_high, buffer_compute_high, &value_high,
                         buff_size_high, event_compute_high);
  lzt::close_command_list(cmdlist_compute_high);

  lzt::append_wait_on_events(cmdlist_compute_low, 1, &event_sync);
  lzt::append_memory_set(cmdlist_compute_low, buffer_compute_low, &value_low,
                         buff_size_low, event_compute_low);
  lzt::close_command_list(cmdlist_compute_low);

  lzt::append_wait_on_events(cmdlist_copy_high, 1, &event_sync);
  lzt::append_memory_set(cmdlist_copy_high, buffer_copy_high, &value_high,
                         buff_size_high, event_copy_high);
  lzt::close_command_list(cmdlist_copy_high);

  lzt::append_wait_on_events(cmdlist_copy_low, 1, &event_sync);
  lzt::append_memory_set(cmdlist_copy_low, buffer_copy_low, &value_low,
                         buff_size_low, event_copy_low);
  lzt::close_command_list(cmdlist_copy_low);

  lzt::execute_command_lists(cmdqueue_compute_low, 1, &cmdlist_compute_low,
                             nullptr);
  lzt::execute_command_lists(cmdqueue_compute_high, 1, &cmdlist_compute_high,
                             nullptr);
  lzt::execute_command_lists(cmdqueue_copy_low, 1, &cmdlist_copy_low, nullptr);
  lzt::execute_command_lists(cmdqueue_copy_high, 1, &cmdlist_copy_high,
                             nullptr);

  lzt::signal_event_from_host(event_sync);
  lzt::synchronize(cmdqueue_compute_high, UINT32_MAX);
  lzt::synchronize(cmdqueue_compute_low, UINT32_MAX);
  lzt::synchronize(cmdqueue_copy_high, UINT32_MAX);
  lzt::synchronize(cmdqueue_copy_low, UINT32_MAX);

  uint8_t *uchar_compute_high = static_cast<uint8_t *>(buffer_compute_high);
  uint8_t *uchar_copy_high = static_cast<uint8_t *>(buffer_copy_high);
  for (size_t i = 0; i < buff_size_high; i++) {
    EXPECT_EQ(value_high, uchar_compute_high[i]);
    EXPECT_EQ(value_high, uchar_copy_high[i]);
  }
  uint8_t *uchar_compute_low = static_cast<uint8_t *>(buffer_compute_low);
  uint8_t *uchar_copy_low = static_cast<uint8_t *>(buffer_copy_low);
  for (size_t i = 0; i < buff_size_low; i++) {
    EXPECT_EQ(value_low, uchar_compute_low[i]);
    EXPECT_EQ(value_low, uchar_copy_low[i]);
  }
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_compute_high));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_compute_low));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_copy_high));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_copy_low));

  EXPECT_GT(lzt::get_event_timestamp(event_compute_low,
                                     ZE_EVENT_TIMESTAMP_GLOBAL_END),
            lzt::get_event_timestamp(event_compute_high,
                                     ZE_EVENT_TIMESTAMP_GLOBAL_END));
  EXPECT_GT(
      lzt::get_event_timestamp(event_copy_low, ZE_EVENT_TIMESTAMP_GLOBAL_END),
      lzt::get_event_timestamp(event_copy_high, ZE_EVENT_TIMESTAMP_GLOBAL_END));
  /*cleanup*/
  lzt::free_memory(buffer_compute_high);
  lzt::free_memory(buffer_compute_low);
  lzt::free_memory(buffer_copy_high);
  lzt::free_memory(buffer_copy_low);
  lzt::destroy_command_queue(cmdqueue_compute_high);
  lzt::destroy_command_queue(cmdqueue_compute_low);
  lzt::destroy_command_queue(cmdqueue_copy_high);
  lzt::destroy_command_queue(cmdqueue_copy_low);
  lzt::destroy_command_list(cmdlist_compute_high);
  lzt::destroy_command_list(cmdlist_compute_low);
  lzt::destroy_command_list(cmdlist_copy_high);
  lzt::destroy_command_list(cmdlist_copy_low);
  lzt::destroy_event(event_compute_high);
  lzt::destroy_event(event_compute_low);
  lzt::destroy_event(event_copy_high);
  lzt::destroy_event(event_copy_low);
  lzt::destroy_event(event_sync);
}
} // namespace
