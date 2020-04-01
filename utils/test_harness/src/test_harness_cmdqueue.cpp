/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "gtest/gtest.h"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

ze_command_queue_handle_t create_command_queue() {
  return create_command_queue(zeDevice::get_instance()->get_device());
}

ze_command_queue_handle_t create_command_queue(ze_command_queue_mode_t mode) {
  return create_command_queue(zeDevice::get_instance()->get_device(),
                              ZE_COMMAND_QUEUE_FLAG_NONE, mode,
                              ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
}

ze_command_queue_handle_t create_command_queue(ze_device_handle_t device) {
  return create_command_queue(device, ZE_COMMAND_QUEUE_FLAG_NONE,
                              ZE_COMMAND_QUEUE_MODE_DEFAULT,
                              ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
}

ze_command_queue_handle_t
create_command_queue(ze_device_handle_t device, ze_command_queue_flag_t flags,
                     ze_command_queue_mode_t mode,
                     ze_command_queue_priority_t priority, uint32_t ordinal) {

  ze_command_queue_desc_t descriptor;
  descriptor.version = ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT;
  descriptor.flags = flags;
  descriptor.mode = mode;
  descriptor.priority = priority;
  ze_device_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &properties));
  if ((flags == ZE_COMMAND_QUEUE_FLAG_NONE) ||
      (flags == ZE_COMMAND_QUEUE_FLAG_SINGLE_SLICE_ONLY)) {
    EXPECT_GE(static_cast<uint32_t>(properties.numAsyncComputeEngines),
              ordinal);
  } else if (flags == ZE_COMMAND_QUEUE_FLAG_COPY_ONLY) {
    EXPECT_GE(static_cast<uint32_t>(properties.numAsyncCopyEngines), ordinal);
  }
  descriptor.ordinal = ordinal;
  ze_command_queue_handle_t command_queue = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandQueueCreate(device, &descriptor, &command_queue));
  EXPECT_NE(nullptr, command_queue);

  return command_queue;
}

zeCommandQueue::zeCommandQueue() { command_queue_ = create_command_queue(); }

zeCommandQueue::~zeCommandQueue() {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueDestroy(command_queue_));
}

void execute_command_lists(ze_command_queue_handle_t cq,
                           uint32_t numCommandLists,
                           ze_command_list_handle_t *phCommandLists,
                           ze_fence_handle_t hFence) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandQueueExecuteCommandLists(cq, numCommandLists,
                                              phCommandLists, hFence));
}

void synchronize(ze_command_queue_handle_t cq, uint32_t timeout) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueSynchronize(cq, timeout));
}

void destroy_command_queue(ze_command_queue_handle_t cq) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueDestroy(cq));
}

}; // namespace level_zero_tests
