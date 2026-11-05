/*
 *
 * Copyright (C) 2020-2026 Intel Corporation
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
LZT_TEST(
    CommandListCreateNegativeTests,
    GivenInvalidDeviceHandleWhileCreatingCommandListThenInvalidNullHandleIsReturned) {
  ze_command_list_desc_t descriptor = {};
  descriptor.flags = 0;
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;

  descriptor.pNext = nullptr;
  ze_command_list_handle_t cmdlist = nullptr;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListCreate(lzt::get_default_context(), nullptr,
                                &descriptor, &cmdlist));
}
LZT_TEST(
    CommandListCreateNegativeTests,
    GivenInvalidCommandQueueDescriptiorWhileCreatingCommandListThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_handle_t cmdlist = nullptr;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeCommandListCreate(lzt::get_default_context(), device, nullptr,
                                &cmdlist));
}
LZT_TEST(
    CommandListCreateNegativeTests,
    GivenInvalidOutPointerToTheCommandListWhileCreatingCommandListThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_desc_t descriptor = {};
  descriptor.flags = 0;
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;

  descriptor.pNext = nullptr;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeCommandListCreate(lzt::get_default_context(), device, &descriptor,
                                nullptr));
}
LZT_TEST(
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
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListCreateImmediate(lzt::get_default_context(), nullptr,
                                         &descriptor, &cmdlist_immediate));
}
LZT_TEST(
    CommandListCreateImmediateNegativeTests,
    GivenInvalidCommandQueueDescriptionWhileCreatingCommandListImmediateThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_handle_t cmdlist_immediate = nullptr;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeCommandListCreateImmediate(lzt::get_default_context(), device,
                                         nullptr, &cmdlist_immediate));
}
LZT_TEST(
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

  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeCommandListCreateImmediate(lzt::get_default_context(), device,
                                         &descriptor, nullptr));
}

class CommandListGetDescriptorNegativeTests : public ::testing::Test {
protected:
  void SetUp() override {
    if (lzt::get_api_version(lzt::get_default_driver()) < ZE_API_VERSION_1_17) {
      GTEST_SKIP() << "Command list getters require API version 1.17";
    }
    ASSERT_NE(nullptr, get_flags_)
        << "Driver supports API version 1.17 but the loader does not export "
           "zeCommandListGetFlags";
    ASSERT_NE(nullptr, get_immediate_flags_)
        << "Driver supports API version 1.17 but the loader does not export "
           "zeCommandListImmediateGetFlags";
    ASSERT_NE(nullptr, get_immediate_mode_)
        << "Driver supports API version 1.17 but the loader does not export "
           "zeCommandListImmediateGetMode";
    ASSERT_NE(nullptr, get_immediate_priority_)
        << "Driver supports API version 1.17 but the loader does not export "
           "zeCommandListImmediateGetPriority";
    command_list = lzt::create_command_list();
    immediate_command_list = lzt::create_immediate_command_list();
  }

  void TearDown() override {
    if (immediate_command_list != nullptr) {
      lzt::destroy_command_list(immediate_command_list);
    }
    if (command_list != nullptr) {
      lzt::destroy_command_list(command_list);
    }
  }

  const decltype(&zeCommandListGetFlags) get_flags_ =
      LZT_LOADER_FUNCTION(zeCommandListGetFlags);
  const decltype(&zeCommandListImmediateGetFlags) get_immediate_flags_ =
      LZT_LOADER_FUNCTION(zeCommandListImmediateGetFlags);
  const decltype(&zeCommandListImmediateGetMode) get_immediate_mode_ =
      LZT_LOADER_FUNCTION(zeCommandListImmediateGetMode);
  const decltype(&zeCommandListImmediateGetPriority) get_immediate_priority_ =
      LZT_LOADER_FUNCTION(zeCommandListImmediateGetPriority);
  ze_command_list_handle_t command_list = nullptr;
  ze_command_list_handle_t immediate_command_list = nullptr;
};

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenInvalidCommandListHandleWhenGettingFlagsThenInvalidNullHandleErrorIsReturned) {
  ze_command_list_flags_t flags = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE, get_flags_(nullptr, &flags));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenInvalidFlagsPointerWhenGettingFlagsThenInvalidNullPointerErrorIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            get_flags_(command_list, nullptr));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenInvalidCommandListHandleWhenGettingImmediateFlagsThenInvalidNullHandleErrorIsReturned) {
  ze_command_queue_flags_t flags = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            get_immediate_flags_(nullptr, &flags));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenInvalidFlagsPointerWhenGettingImmediateFlagsThenInvalidNullPointerErrorIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            get_immediate_flags_(immediate_command_list, nullptr));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenRegularCommandListWhenGettingImmediateFlagsThenInvalidArgumentErrorIsReturned) {
  ze_command_queue_flags_t flags = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
            get_immediate_flags_(command_list, &flags));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenInvalidCommandListHandleWhenGettingImmediateModeThenInvalidNullHandleErrorIsReturned) {
  ze_command_queue_mode_t mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            get_immediate_mode_(nullptr, &mode));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenInvalidModePointerWhenGettingImmediateModeThenInvalidNullPointerErrorIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            get_immediate_mode_(immediate_command_list, nullptr));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenRegularCommandListWhenGettingImmediateModeThenInvalidArgumentErrorIsReturned) {
  ze_command_queue_mode_t mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
            get_immediate_mode_(command_list, &mode));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenInvalidCommandListHandleWhenGettingImmediatePriorityThenInvalidNullHandleErrorIsReturned) {
  ze_command_queue_priority_t priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            get_immediate_priority_(nullptr, &priority));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenInvalidPriorityPointerWhenGettingImmediatePriorityThenInvalidNullPointerErrorIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            get_immediate_priority_(immediate_command_list, nullptr));
}

LZT_TEST_F(
    CommandListGetDescriptorNegativeTests,
    GivenRegularCommandListWhenGettingImmediatePriorityThenInvalidArgumentErrorIsReturned) {
  ze_command_queue_priority_t priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
            get_immediate_priority_(command_list, &priority));
}

} // namespace
