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

class CommandQueueCreateNegativeTests : public ::testing::Test {};

TEST_F(
    CommandQueueCreateNegativeTests,
    GivenInValidDeviceHandleWhenCreatingCommandQueueThenErrorInvalidNullHandleIsReturned) {

  ze_command_queue_desc_t descriptor = {};
  descriptor.flags = 0;
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;

  descriptor.pNext = nullptr;
  descriptor.mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;
  descriptor.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
  ze_device_handle_t device = nullptr;

  ze_command_queue_handle_t command_queue = nullptr;
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
            uint64_t(zeCommandQueueCreate(lzt::get_default_context(), device,
                                          &descriptor, &command_queue)));
}

TEST_F(
    CommandQueueCreateNegativeTests,
    GivenInValidDescWhenCreatingCommandQueueThenErrorInvalidNullHandleIsReturned) {

  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_queue_handle_t command_queue = nullptr;

  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeCommandQueueCreate(lzt::get_default_context(), device,
                                          nullptr, &command_queue)));
}

TEST_F(
    CommandQueueCreateNegativeTests,
    GivenInValidOutputCmdQueueWhenCreatingCommandQueueThenErrorInvalidNullHandleIsReturned) {

  ze_command_queue_desc_t descriptor = {};
  descriptor.flags = 0;
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;

  descriptor.pNext = nullptr;
  descriptor.mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;
  descriptor.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();

  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeCommandQueueCreate(lzt::get_default_context(), device,
                                          &descriptor, nullptr)));
}

class CommandQueueDestroyNegativeTests : public ::testing::Test {};

TEST_F(
    CommandQueueDestroyNegativeTests,
    GivenValidDeviceAndNonNullCommandQueueWhenDestroyingCommandQueueThenSuccessIsReturned) {
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
            uint64_t(zeCommandQueueDestroy(nullptr)));
}

class CommandQueueExecuteCommandListNegativeTests : public ::testing::Test {};

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenInvalidCommandQueueHandleWhenExecutingCommandListsThenInvalidNullHandleErrorIsReturned) {

  ze_command_list_handle_t command_list;

  command_list = lzt::create_command_list();

  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
            uint64_t(zeCommandQueueExecuteCommandLists(
                nullptr, 1, &command_list, nullptr)));
  lzt::destroy_command_list(command_list);
}

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenInvalidCommandListPointerWhenExecutingCommandListsThenInvalidNullPointerErrorIsReturned) {
  ze_command_queue_handle_t command_queue = nullptr;
  command_queue = lzt::create_command_queue();

  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeCommandQueueExecuteCommandLists(command_queue, 1,
                                                       nullptr, nullptr)));
  lzt::destroy_command_queue(command_queue);
}

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenInvalidNumberOfCommandListWhenExecutingCommandListsThenInvalidSizeErrorIsReturned) {

  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue = nullptr;
  command_queue = lzt::create_command_queue();
  command_list = lzt::create_command_list();

  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_SIZE),
            uint64_t(zeCommandQueueExecuteCommandLists(
                command_queue, 0, &command_list, nullptr)));
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}
TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenAlreadySignalledFenceWhenExecutingCommandListsThenInvalidSynchronizationObjectErrorIsReturned) {

  ze_command_list_handle_t command_list1, command_list2;
  ze_command_queue_handle_t command_queue = nullptr;
  ze_fence_handle_t hFence = nullptr;
  command_queue = lzt::create_command_queue();
  command_list1 = lzt::create_command_list();
  hFence = lzt::create_fence(command_queue);

  lzt::execute_command_lists(command_queue, 1, &command_list1, hFence);

  EXPECT_EQ(ZE_RESULT_SUCCESS, lzt::sync_fence(hFence, UINT64_MAX));

  // Now use the same signalled fence above for below other commandlist
  // execution
  command_list2 = lzt::create_command_list();
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT),
            uint64_t(zeCommandQueueExecuteCommandLists(
                command_queue, 0, &command_list2, hFence)));
  lzt::destroy_command_list(command_list1);
  lzt::destroy_command_list(command_list2);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_fence(hFence);
}

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenInvalidCommandQueueHandleWhenExecutingCommandQueueSynchronizeThenInvalidNullHandleErrorIsReturned) {
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue = nullptr;
  command_queue = lzt::create_command_queue();
  command_list = lzt::create_command_list();

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);

  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
            uint64_t(zeCommandQueueSynchronize(nullptr, UINT64_MAX)));

  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

} // namespace
