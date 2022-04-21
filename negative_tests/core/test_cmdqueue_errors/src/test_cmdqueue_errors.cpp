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
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueCreate(lzt::get_default_context(), device,
                                 &descriptor, &command_queue));
}

TEST_F(
    CommandQueueCreateNegativeTests,
    GivenInValidDescWhenCreatingCommandQueueThenErrorInvalidNullHandleIsReturned) {

  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_queue_handle_t command_queue = nullptr;

  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeCommandQueueCreate(lzt::get_default_context(), device, nullptr,
                                 &command_queue));
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

  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeCommandQueueCreate(lzt::get_default_context(), device,
                                 &descriptor, nullptr));
}

class CommandQueueDestroyNegativeTests : public ::testing::Test {};

TEST_F(
    CommandQueueDestroyNegativeTests,
    GivenValidDeviceAndNonNullCommandQueueWhenDestroyingCommandQueueThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueDestroy(nullptr));
}

class CommandQueueExecuteCommandListNegativeTests : public ::testing::Test {};

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenInvalidCommandQueueHandleWhenExecutingCommandListsThenInvalidNullHandleErrorIsReturned) {

  ze_command_list_handle_t command_list;

  command_list = lzt::create_command_list();

  EXPECT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeCommandQueueExecuteCommandLists(nullptr, 1, &command_list, nullptr));
  lzt::destroy_command_list(command_list);
}

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenInvalidCommandListPointerWhenExecutingCommandListsThenInvalidNullPointerErrorIsReturned) {
  ze_command_queue_handle_t command_queue = nullptr;
  command_queue = lzt::create_command_queue();

  EXPECT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_POINTER,
      zeCommandQueueExecuteCommandLists(command_queue, 1, nullptr, nullptr));
  lzt::destroy_command_queue(command_queue);
}

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenInvalidNumberOfCommandListWhenExecutingCommandListsThenInvalidSizeErrorIsReturned) {

  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue = nullptr;
  command_queue = lzt::create_command_queue();
  command_list = lzt::create_command_list();

  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_SIZE,
            zeCommandQueueExecuteCommandLists(command_queue, 0, &command_list,
                                              nullptr));
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
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT,
            zeCommandQueueExecuteCommandLists(command_queue, 0, &command_list2,
                                              hFence));
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

  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueSynchronize(nullptr, UINT64_MAX));

  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenLongRunningKernelWhenExecutingCommandQueueSynchronizeThenDeviceLostErrorIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue = nullptr;
  command_queue = lzt::create_command_queue(device);
  command_list = lzt::create_command_list(device);

  auto module = lzt::create_module(device, "gpu_hang.spv");
  auto kernel = lzt::create_function(module, "hang");

  lzt::set_group_size(kernel, 1, 1, 1);

  ze_group_count_t group_count;
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);

  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);

  EXPECT_EQ(ZE_RESULT_ERROR_DEVICE_LOST,
            zeCommandQueueSynchronize(command_queue, UINT64_MAX));

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenLongRunningKernelWhenExecutingEventHostSynchronizeThenDeviceLostErrorIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue = nullptr;
  command_queue = lzt::create_command_queue(device);
  command_list = lzt::create_command_list(device);

  ze_event_handle_t hEvent = nullptr;

  lzt::zeEventPool ep;
  ep.create_event(hEvent);

  auto module = lzt::create_module(device, "gpu_hang.spv");
  auto kernel = lzt::create_function(module, "hang");

  lzt::set_group_size(kernel, 1, 1, 1);

  ze_group_count_t group_count;
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  lzt::append_launch_function(command_list, kernel, &group_count, hEvent, 0,
                              nullptr);

  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);

  EXPECT_EQ(ZE_RESULT_ERROR_DEVICE_LOST,
            zeEventHostSynchronize(hEvent, UINT64_MAX));

  ep.destroy_event(hEvent);

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

TEST_F(
    CommandQueueExecuteCommandListNegativeTests,
    GivenLongRunningKernelWhenExecutingFenceHostSynchronizeThenDeviceLostErrorIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue = nullptr;
  ze_fence_handle_t hFence = nullptr;
  command_queue = lzt::create_command_queue(device);
  command_list = lzt::create_command_list(device);
  hFence = lzt::create_fence(command_queue);

  auto module = lzt::create_module(device, "gpu_hang.spv");
  auto kernel = lzt::create_function(module, "hang");

  lzt::set_group_size(kernel, 1, 1, 1);

  ze_group_count_t group_count;
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);

  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, hFence);

  EXPECT_EQ(ZE_RESULT_ERROR_DEVICE_LOST,
            zeFenceHostSynchronize(hFence, UINT64_MAX));

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_fence(hFence);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

} // namespace
