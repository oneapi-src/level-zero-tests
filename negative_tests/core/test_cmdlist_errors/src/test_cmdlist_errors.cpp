/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {
TEST(
    CommandListCreateNegativeTests,
    GivenInvalidDeviceHandleWhileCreatingCommandListThenInvalidNullHandleIsReturned) {
  ze_command_list_desc_t descriptor = {};
  descriptor.flags = 0;
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;

  descriptor.pNext = nullptr;
  ze_command_list_handle_t cmdlist = nullptr;
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
            uint64_t(zeCommandListCreate(lzt::get_default_context(), nullptr,
                                         &descriptor, &cmdlist)));
}
TEST(
    CommandListCreateNegativeTests,
    GivenInvalidCommandQueueDescriptiorWhileCreatingCommandListThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_handle_t cmdlist = nullptr;
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeCommandListCreate(lzt::get_default_context(), device,
                                         nullptr, &cmdlist)));
}
TEST(
    CommandListCreateNegativeTests,
    GivenInvalidOutPointerToTheCommandListWhileCreatingCommandListThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_desc_t descriptor = {};
  descriptor.flags = 0;
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;

  descriptor.pNext = nullptr;
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeCommandListCreate(lzt::get_default_context(), device,
                                         &descriptor, nullptr)));
}
TEST(
    CommandListCreateImmediateNegativeTests,
    GivenInvalidDeviceHandleWhileCreatingCommandListImmediateThenInvalidNullHandleIsReturned) {
  ze_command_queue_desc_t descriptor = {};
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  descriptor.flags = 0;
  descriptor.pNext = nullptr;
  descriptor.ordinal = 0;
  descriptor.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
  descriptor.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
  ze_command_list_handle_t cmdlist_immediate = nullptr;
  EXPECT_EQ(
      uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
      uint64_t(zeCommandListCreateImmediate(lzt::get_default_context(), nullptr,
                                            &descriptor, &cmdlist_immediate)));
}
TEST(
    CommandListCreateImmediateNegativeTests,
    GivenInvalidCommandQueueDescriptionWhileCreatingCommandListImmediateThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_handle_t cmdlist_immediate = nullptr;
  EXPECT_EQ(
      uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
      uint64_t(zeCommandListCreateImmediate(lzt::get_default_context(), device,
                                            nullptr, &cmdlist_immediate)));
}
TEST(
    CommandListCreateImmediateNegativeTests,
    GivenInvalidOutPointerToTheCommandListWhileCreatingCommandListImmediateThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_queue_desc_t descriptor = {};
  descriptor.flags = 0;
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;

  descriptor.pNext = nullptr;
  descriptor.ordinal = 0;
  descriptor.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
  descriptor.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;

  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeCommandListCreateImmediate(
                lzt::get_default_context(), device, &descriptor, nullptr)));
}

} // namespace
