/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness_fence.hpp"

namespace level_zero_tests {

ze_fence_handle_t create_fence(ze_command_queue_handle_t cmd_queue) {
  ze_fence_handle_t fence;
  ze_fence_desc_t desc;
  desc.version = ZE_FENCE_DESC_VERSION_CURRENT;
  desc.flags = ZE_FENCE_FLAG_NONE;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceCreate(cmd_queue, &desc, &fence));
  return fence;
}

void destroy_fence(ze_fence_handle_t fence) {

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceDestroy(fence));
}

void reset_fence(ze_fence_handle_t fence) {

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeFenceReset(fence));
}

ze_result_t query_fence(ze_fence_handle_t fence) {
  ze_result_t result;
  EXPECT_EQ(ZE_RESULT_SUCCESS, result = zeFenceQueryStatus(fence));
  return result;
}

ze_result_t sync_fence(ze_fence_handle_t fence, uint32_t timeout) {

  ze_result_t result;
  EXPECT_EQ(ZE_RESULT_SUCCESS, result = zeFenceHostSynchronize(fence, timeout));
  return result;
}

}; // namespace level_zero_tests
