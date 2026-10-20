/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/ze_api.h>

namespace level_zero_tests {

ze_fence_handle_t create_fence(ze_command_queue_handle_t cmd_queue);
void destroy_fence(ze_fence_handle_t fence);
void reset_fence(ze_fence_handle_t fence);
ze_result_t query_fence(ze_fence_handle_t fence);
ze_result_t sync_fence(ze_fence_handle_t fence, uint64_t timeout);

}; // namespace level_zero_tests
