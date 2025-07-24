/*
 *
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_CMDQUEUE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_CMDQUEUE_HPP

#include "test_harness/test_harness_device.hpp"
#include <level_zero/ze_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

class zeCommandQueue {
public:
  zeCommandQueue();
  ~zeCommandQueue();

  ze_command_queue_handle_t command_queue_ = nullptr;
};

class zeCommandQueueTests : public ::testing::Test {
protected:
  zeCommandQueue cq;
};

std::vector<uint32_t>
get_compute_queue_group_ordinals(ze_device_handle_t device);
ze_command_queue_handle_t create_command_queue();
ze_command_queue_handle_t create_command_queue(ze_device_handle_t device);
ze_command_queue_handle_t create_command_queue(ze_context_handle_t context,
                                               ze_device_handle_t device);
ze_command_queue_handle_t create_command_queue(ze_command_queue_mode_t mode);
ze_command_queue_handle_t
create_command_queue(ze_device_handle_t device, ze_command_queue_flags_t flags,
                     ze_command_queue_mode_t mode,
                     ze_command_queue_priority_t priority, uint32_t ordinal);
ze_command_queue_handle_t
create_command_queue(ze_context_handle_t context, ze_device_handle_t device,
                     ze_command_queue_flags_t flags,
                     ze_command_queue_mode_t mode,
                     ze_command_queue_priority_t priority, uint32_t ordinal);
ze_command_queue_handle_t create_command_queue(
    ze_context_handle_t context, ze_device_handle_t device,
    ze_command_queue_flags_t flags, ze_command_queue_mode_t mode,
    ze_command_queue_priority_t priority, uint32_t ordinal, uint32_t index);
ze_command_queue_handle_t create_command_queue(
    ze_context_handle_t context, ze_device_handle_t device, const void *pNext,
    ze_command_queue_flags_t flags, ze_command_queue_mode_t mode,
    ze_command_queue_priority_t priority, uint32_t ordinal, uint32_t index);
void execute_command_lists(ze_command_queue_handle_t cq,
                           uint32_t numCommandLists,
                           ze_command_list_handle_t *phCommandLists,
                           ze_fence_handle_t hFence);
void synchronize(ze_command_queue_handle_t cq, uint64_t timeout);

void destroy_command_queue(ze_command_queue_handle_t cq);

}; // namespace level_zero_tests
#endif
