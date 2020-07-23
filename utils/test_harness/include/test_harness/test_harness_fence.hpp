/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_FENCE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_FENCE_HPP

#include <level_zero/ze_api.h>

namespace level_zero_tests {

ze_fence_handle_t create_fence(ze_command_queue_handle_t cmd_queue);
void destroy_fence(ze_fence_handle_t fence);
void reset_fence(ze_fence_handle_t fence);
ze_result_t query_fence(ze_fence_handle_t fence);
ze_result_t sync_fence(ze_fence_handle_t fence, uint64_t timeout);

}; // namespace level_zero_tests

#endif
