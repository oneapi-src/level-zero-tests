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
#include "test_harness/test_harness_fence.hpp"
#include "logging/logging.hpp"
#include <chrono>
#include <thread>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeCommandQueueCreateFenceTests : public lzt::zeCommandQueueTests {};

TEST_F(
    zeCommandQueueCreateFenceTests,
    GivenDefaultFenceDescriptorWhenCreatingFenceThenNotNullPointerIsReturned) {
  ze_fence_desc_t descriptor = {};

  descriptor.pNext = nullptr;
  descriptor.stype = ZE_STRUCTURE_TYPE_FENCE_DESC;

  ze_fence_handle_t fence = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFenceCreate(cq.command_queue_, &descriptor, &fence));
  EXPECT_NE(nullptr, fence);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceDestroy(fence));
}

class zeFenceTests : public ::testing::Test {
public:
  zeFenceTests() {
    context_ = lzt::create_context();
    device_ = lzt::zeDevice::get_instance()->get_device();
    cmd_list_ = lzt::create_command_list(context_, device_, 0);
    cmd_q_ = lzt::create_command_queue(context_, device_, 0,
                                       ZE_COMMAND_QUEUE_MODE_DEFAULT,
                                       ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    ze_fence_desc_t descriptor = {};
    descriptor.stype = ZE_STRUCTURE_TYPE_FENCE_DESC;

    descriptor.pNext = nullptr;

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceCreate(cmd_q_, &descriptor, &fence_));
    EXPECT_NE(nullptr, fence_);
  }
  ~zeFenceTests() {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceDestroy(fence_));
    lzt::destroy_command_queue(cmd_q_);
    lzt::destroy_command_list(cmd_list_);
    lzt::destroy_context(context_);
  }

protected:
  ze_fence_handle_t fence_ = nullptr;
  ze_context_handle_t context_ = nullptr;
  ze_device_handle_t device_ = nullptr;
  ze_command_list_handle_t cmd_list_ = nullptr;
  ze_command_queue_handle_t cmd_q_ = nullptr;
};

class zeFenceSynchronizeTests : public zeFenceTests,
                                public ::testing::WithParamInterface<uint32_t> {
};

TEST_P(zeFenceSynchronizeTests,
       GivenSignaledFenceWhenSynchronizingThenSuccessIsReturned) {

  const std::vector<int8_t> input = {72, 101, 108, 108, 111, 32,
                                     87, 111, 114, 108, 100, 33};

  void *output_buffer =
      lzt::allocate_device_memory(input.size(), 1, 0, 0, device_, context_);

  lzt::append_memory_copy(cmd_list_, output_buffer, input.data(), input.size(),
                          nullptr);
  lzt::close_command_list(cmd_list_);
  lzt::execute_command_lists(cmd_q_, 1, &cmd_list_, fence_);
  lzt::sync_fence(fence_, GetParam());
  lzt::free_memory(context_, output_buffer);
}

INSTANTIATE_TEST_CASE_P(FenceSyncParameterizedTest, zeFenceSynchronizeTests,
                        ::testing::Values(UINT32_MAX - 1, UINT64_MAX));

TEST_F(zeFenceSynchronizeTests,
       GivenSignaledFenceWhenQueryingThenSuccessIsReturned) {

  const std::vector<int8_t> input = {72, 101, 108, 108, 111, 32,
                                     87, 111, 114, 108, 100, 33};

  void *output_buffer =
      lzt::allocate_device_memory(input.size(), 1, 0, 0, device_, context_);

  lzt::append_memory_copy(cmd_list_, output_buffer, input.data(), input.size(),
                          nullptr);
  lzt::close_command_list(cmd_list_);
  lzt::execute_command_lists(cmd_q_, 1, &cmd_list_, fence_);
  lzt::sync_fence(fence_, UINT64_MAX);
  lzt::query_fence(fence_);
  lzt::free_memory(context_, output_buffer);
}

TEST_F(zeFenceSynchronizeTests,
       GivenSignaledFenceWhenResettingThenSuccessIsReturned) {

  const std::vector<int8_t> input = {72, 101, 108, 108, 111, 32,
                                     87, 111, 114, 108, 100, 33};

  void *output_buffer =
      lzt::allocate_device_memory(input.size(), 1, 0, 0, device_, context_);

  lzt::append_memory_copy(cmd_list_, output_buffer, input.data(), input.size(),
                          nullptr);

  lzt::close_command_list(cmd_list_);
  lzt::execute_command_lists(cmd_q_, 1, &cmd_list_, fence_);
  lzt::sync_fence(fence_, UINT64_MAX);
  lzt::reset_fence(fence_);
  lzt::free_memory(context_, output_buffer);
}

TEST_F(zeFenceSynchronizeTests,
       GivenSignaledFenceWhenResettingThenFenceStatusIsNotReadyWhenQueried) {

  const std::vector<int8_t> input = {72, 101, 108, 108, 111, 32,
                                     87, 111, 114, 108, 100, 33};

  void *output_buffer =
      lzt::allocate_device_memory(input.size(), 1, 0, 0, device_, context_);

  lzt::append_memory_copy(cmd_list_, output_buffer, input.data(), input.size(),
                          nullptr);

  lzt::close_command_list(cmd_list_);
  lzt::execute_command_lists(cmd_q_, 1, &cmd_list_, fence_);
  lzt::sync_fence(fence_, UINT64_MAX);
  lzt::reset_fence(fence_);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));
  lzt::free_memory(context_, output_buffer);
}

class zeMultipleFenceTests : public lzt::zeEventPoolTests,
                             public ::testing::WithParamInterface<bool> {};

TEST_P(
    zeMultipleFenceTests,
    GivenMultipleCommandQueuesWhenFenceAndEventSetThenVerifyAllSignaledSuccessful) {
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_context_handle_t context = lzt::create_context();
  ze_device_properties_t properties = {};

  properties.pNext = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &properties));

  auto cmdq_properties = lzt::get_command_queue_group_properties(device);

  size_t num_cmdq = 0;
  std::vector<int> num_queues_per_group;
  for (auto properties : cmdq_properties) {
    num_queues_per_group.push_back(properties.numQueues);
    num_cmdq += properties.numQueues;
  }

  bool use_event = GetParam();

  const size_t size = 16;
  std::vector<ze_event_handle_t> host_to_dev_event(num_cmdq, nullptr);
  if (use_event) {
    ep.InitEventPool(context, num_cmdq);
  }
  std::vector<ze_fence_handle_t> fence(num_cmdq, nullptr);
  std::vector<ze_command_queue_handle_t> cmdq(num_cmdq, nullptr);
  std::vector<ze_command_list_handle_t> cmdlist(num_cmdq, nullptr);
  std::vector<void *> buffer(num_cmdq, nullptr);
  std::vector<uint8_t> val(num_cmdq, 0);
  ze_command_list_flags_t cmdlist_flag = 0;
  ze_command_queue_flags_t cmdq_flag = 0;
  uint32_t ordinal = 0;

  uint32_t group_ordinal = 0;
  uint32_t queue_index = 0;
  for (size_t i = 0; i < num_cmdq; i++) {
    if (queue_index >= num_queues_per_group[group_ordinal]) {
      group_ordinal++;
      queue_index = 0;
    }
    cmdq[i] = lzt::create_command_queue(
        context, device, cmdq_flag, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, group_ordinal, queue_index);
    cmdlist[i] =
        lzt::create_command_list(context, device, cmdlist_flag, group_ordinal);
    fence[i] = lzt::create_fence(cmdq[i]);
    buffer[i] = lzt::allocate_shared_memory(size, 1, 0, 0, device, context);
    val[i] = static_cast<uint8_t>(i + 1);

    if (use_event) {
      ep.create_event(host_to_dev_event[i], ZE_EVENT_SCOPE_FLAG_HOST,
                      ZE_EVENT_SCOPE_FLAG_HOST);
      lzt::append_wait_on_events(cmdlist[i], 1, &host_to_dev_event[i]);
    }
    lzt::append_memory_set(cmdlist[i], buffer[i], &val[i], size);
    lzt::append_barrier(cmdlist[i], nullptr, 0, nullptr);
    lzt::close_command_list(cmdlist[i]);
    queue_index++;
  }

  // attempt to execute command queues simutanesously
  for (size_t i = 0; i < num_cmdq; i++) {
    lzt::execute_command_lists(cmdq[i], 1, &cmdlist[i], fence[i]);
  }

  if (use_event) {
    for (size_t i = 0; i < num_cmdq; i++) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence[i]));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(host_to_dev_event[i]));
    }
  }

  for (size_t i = 0; i < num_cmdq; i++) {
    lzt::sync_fence(fence[i], UINT64_MAX);
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceQueryStatus(fence[i]));
    for (size_t j = 0; j < size; j++) {
      EXPECT_EQ(static_cast<uint8_t *>(buffer[i])[j], val[i]);
    }
  }

  for (size_t i = 0; i < num_cmdq; i++) {
    if (use_event) {
      ep.destroy_event(host_to_dev_event[i]);
    }
    lzt::free_memory(context, buffer[i]);
    lzt::destroy_fence(fence[i]);
    lzt::destroy_command_list(cmdlist[i]);
    lzt::destroy_command_queue(cmdq[i]);
  }

  lzt::destroy_context(context);
  context = nullptr;
}

INSTANTIATE_TEST_CASE_P(TestMultipleFenceWithAndWithoutEvent,
                        zeMultipleFenceTests, testing::Values(false, true));

} // namespace
