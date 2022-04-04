/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness_fence.hpp"
#include <thread>

namespace level_zero_tests {

ze_fence_handle_t create_fence(ze_command_queue_handle_t cmd_queue) {
  ze_fence_handle_t fence;
  ze_fence_desc_t desc = {};
  desc.stype = ZE_STRUCTURE_TYPE_FENCE_DESC;
  desc.pNext = nullptr;
  desc.flags = 0;

  auto cmd_queue_initial = cmd_queue;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceCreate(cmd_queue, &desc, &fence));
  EXPECT_EQ(cmd_queue, cmd_queue_initial);
  return fence;
}

void destroy_fence(ze_fence_handle_t fence) {

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceDestroy(fence));
}

void reset_fence(ze_fence_handle_t fence) {

  auto fence_initial = fence;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceReset(fence));
  EXPECT_EQ(fence, fence_initial);
}

ze_result_t query_fence(ze_fence_handle_t fence) {
  ze_result_t result;
  auto fence_initial = fence;
  EXPECT_EQ(ZE_RESULT_SUCCESS, result = zeFenceQueryStatus(fence));
  EXPECT_EQ(fence, fence_initial);
  return result;
}

ze_result_t sync_fence(ze_fence_handle_t fence, uint64_t timeout) {

  ze_result_t result;
  auto fence_initial = fence;
  int retry_count = 5;
  while (retry_count > 0) {
    result = zeFenceHostSynchronize(fence, timeout);
    if (result != ZE_RESULT_NOT_READY) {
      break;
    }
    retry_count--;
    std::this_thread::yield();
  }
  EXPECT_EQ(ZE_RESULT_SUCCESS, result);
  EXPECT_EQ(fence, fence_initial);
  return result;
}

}; // namespace level_zero_tests
