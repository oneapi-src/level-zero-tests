/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace {

class zeMutableCommandListTests : public ::testing::Test {
protected:
  void SetUp() override {
    driver = lzt::get_default_driver();
    if (!lzt::check_if_extension_supported(driver,
                                           ZE_MUTABLE_COMMAND_LIST_EXP_NAME)) {
      GTEST_SKIP() << "Extension " << ZE_MUTABLE_COMMAND_LIST_EXP_NAME
                   << " not supported";
    };

    context = lzt::create_context(driver);
    device = lzt::get_default_device(driver);
    queue = lzt::create_command_queue(context, device);
    EXPECT_EQ(
        ZE_RESULT_SUCCESS,
        zeCommandListCreate(context, device, &cmdListDesc, &mutableCmdList));

    module = lzt::create_module(device, "test_mutable_cmdlist.spv");

    deviceProps.pNext = &mutableCmdListProps;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &deviceProps));
    kernelArgumentsSupport = mutableCmdListProps.mutableCommandFlags &
                             ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS;
    groupCountSupport = mutableCmdListProps.mutableCommandFlags &
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT;
    groupSizeSupport = mutableCmdListProps.mutableCommandFlags &
                       ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE;
    globalOffsetSupport = mutableCmdListProps.mutableCommandFlags &
                          ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET;
    signalEventSupport = mutableCmdListProps.mutableCommandFlags &
                         ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT;
    waitEventsSupport = mutableCmdListProps.mutableCommandFlags &
                        ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS;

    ze_result_t ret = ZE_RESULT_SUCCESS;
    void *address = nullptr;
    ret = zeDriverGetExtensionFunctionAddress(
        driver, "zeCommandListGetNextCommandIdExp", &address);
    if ((ret != ZE_RESULT_SUCCESS) || (address == nullptr)) {
      GTEST_FAIL() << "Unable to find zeCommandListGetNextCommandIdExp";
    }
    zeCommandListGetNextCommandIdExp =
        (zeCommandListGetNextCommandIdExpPtr)address;
    ret = zeDriverGetExtensionFunctionAddress(
        driver, "zeCommandListUpdateMutableCommandsExp", &address);
    if ((ret != ZE_RESULT_SUCCESS) || (address == nullptr)) {
      GTEST_FAIL() << "Unable to find zeCommandListUpdateMutableCommandsExp";
    }
    zeCommandListUpdateMutableCommandsExp =
        (zeCommandListUpdateMutableCommandsExpPtr)address;
    ret = zeDriverGetExtensionFunctionAddress(
        driver, "zeCommandListUpdateMutableCommandSignalEventExp", &address);
    if ((ret != ZE_RESULT_SUCCESS) || (address == nullptr)) {
      GTEST_FAIL()
          << "Unable to find zeCommandListUpdateMutableCommandSignalEventExp";
    }
    zeCommandListUpdateMutableCommandSignalEventExp =
        (zeCommandListUpdateMutableCommandSignalEventExpPtr)address;
    ret = zeDriverGetExtensionFunctionAddress(
        driver, "zeCommandListUpdateMutableCommandWaitEventsExp", &address);
    if ((ret != ZE_RESULT_SUCCESS) || (address == nullptr)) {
      GTEST_FAIL()
          << "Unable to find zeCommandListUpdateMutableCommandWaitEventsExp";
    }
    zeCommandListUpdateMutableCommandWaitEventsExp =
        (zeCommandListUpdateMutableCommandWaitEventsExpPtr)address;
  }

  void TearDown() override {
    lzt::destroy_command_list(mutableCmdList);
    lzt::destroy_module(module);
    lzt::destroy_command_queue(queue);
    lzt::destroy_context(context);
  }

  typedef ze_result_t (*zeCommandListGetNextCommandIdExpPtr)(
      ze_command_list_handle_t hCommandList,
      const ze_mutable_command_id_exp_desc_t *desc, uint64_t *pCommandId);
  zeCommandListGetNextCommandIdExpPtr zeCommandListGetNextCommandIdExp;

  typedef ze_result_t (*zeCommandListUpdateMutableCommandsExpPtr)(
      ze_command_list_handle_t hCommandList,
      const ze_mutable_commands_exp_desc_t *desc);
  zeCommandListUpdateMutableCommandsExpPtr
      zeCommandListUpdateMutableCommandsExp;

  typedef ze_result_t (*zeCommandListUpdateMutableCommandSignalEventExpPtr)(
      ze_command_list_handle_t hCommandList, uint64_t commandId,
      ze_event_handle_t hSignalEvent);
  zeCommandListUpdateMutableCommandSignalEventExpPtr
      zeCommandListUpdateMutableCommandSignalEventExp;

  typedef ze_result_t (*zeCommandListUpdateMutableCommandWaitEventsExpPtr)(
      ze_command_list_handle_t hCommandList, uint64_t commandId,
      uint32_t numWaitEvents, ze_event_handle_t *phWaitEvents);
  zeCommandListUpdateMutableCommandWaitEventsExpPtr
      zeCommandListUpdateMutableCommandWaitEventsExp;

  ze_driver_handle_t driver = nullptr;
  ze_context_handle_t context = nullptr;
  ze_device_handle_t device = nullptr;
  ze_module_handle_t module = nullptr;

  ze_command_queue_handle_t queue = nullptr;
  ze_command_list_handle_t mutableCmdList = nullptr;
  ze_mutable_command_list_exp_desc_t mutableCmdListExpDesc{
      ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC, nullptr, 0};
  ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
                                        &mutableCmdListExpDesc, 0};

  bool kernelArgumentsSupport = false;
  bool groupCountSupport = false;
  bool groupSizeSupport = false;
  bool globalOffsetSupport = false;
  bool signalEventSupport = false;
  bool waitEventsSupport = false;

  ze_mutable_command_list_exp_properties_t mutableCmdListProps = {
      ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_PROPERTIES, nullptr, 0, 0};
  ze_device_properties_t deviceProps = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  ze_mutable_commands_exp_desc_t mutableCmdDesc = {
      ZE_STRUCTURE_TYPE_MUTABLE_COMMANDS_EXP_DESC};
  ze_mutable_command_id_exp_desc_t commandIdDesc = {
      ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC};
};

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfKernelArgumentsWhenCommandListIsClosedThenArgumentsWereReplaced) {
  if (!kernelArgumentsSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS not "
                    "supported";
  }

  const int32_t bufferSize = 16384;
  const int32_t initBufferVal = 1111;
  const int32_t mutatedBufferVal = 2222;
  const int32_t initAddVal = 3333;
  const int32_t mutatedAddVal = 4444;
  int32_t *initBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  int32_t *mutatedBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    initBuffer[i] = initBufferVal;
    mutatedBuffer[i] = mutatedBufferVal;
  }

  ze_kernel_handle_t addKernel = lzt::create_function(module, "addValue");

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;
  lzt::suggest_group_size(addKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  lzt::set_group_size(addKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(addKernel, 0, sizeof(void *), &initBuffer);
  lzt::set_argument_value(addKernel, 1, sizeof(initAddVal), &initAddVal);

  uint64_t commandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListGetNextCommandIdExp(
                                   mutableCmdList, &commandIdDesc, &commandId));

  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount, nullptr,
                              0, nullptr);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(initBuffer[i], initBufferVal + initAddVal);
  }

  ze_mutable_kernel_argument_exp_desc_t mutateBufferKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateBufferKernelArg.commandId = commandId;
  mutateBufferKernelArg.argIndex = 0;
  mutateBufferKernelArg.argSize = sizeof(int32_t *);
  mutateBufferKernelArg.pArgValue = &mutatedBuffer;
  ze_mutable_kernel_argument_exp_desc_t mutateScalarKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateScalarKernelArg.pNext = &mutateBufferKernelArg;
  mutateScalarKernelArg.commandId = commandId;
  mutateScalarKernelArg.argIndex = 1;
  mutateScalarKernelArg.argSize = sizeof(mutatedAddVal);
  mutateScalarKernelArg.pArgValue = &mutatedAddVal;
  mutableCmdDesc.pNext = &mutateScalarKernelArg;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandsExp(
                                   mutableCmdList, &mutableCmdDesc));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(mutatedBuffer[i], mutatedBufferVal + mutatedAddVal);
  }

  lzt::free_memory(initBuffer);
  lzt::free_memory(mutatedBuffer);
  lzt::destroy_function(addKernel);
}

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfGroupCountWhenCommandListIsClosedThenGlobalWorkSizeIsUpdated) {
  if (!groupCountSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT not "
                    "supported";
  }

  const int32_t bufferSizeX = 128;
  const int32_t bufferSizeY = 32;
  const int32_t bufferSizeZ = 8;
  const int32_t bufferSize = bufferSizeX * bufferSizeY * bufferSizeZ;
  const int32_t dimensions = 3;

  int32_t *inOutBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutBuffer[i] = 0;
  }
  int32_t *globalSizesBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(dimensions * sizeof(int32_t)));
  for (size_t i = 0; i < dimensions; i++) {
    globalSizesBuffer[i] = 0;
  }

  ze_kernel_handle_t testGlobalSizesKernel =
      lzt::create_function(module, "testGlobalSizes");

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;
  lzt::suggest_group_size(testGlobalSizesKernel, bufferSizeX, bufferSizeY,
                          bufferSizeZ, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_group_size(testGlobalSizesKernel, groupSizeX, groupSizeY,
                      groupSizeZ);
  lzt::set_argument_value(testGlobalSizesKernel, 0, sizeof(void *),
                          &inOutBuffer);
  lzt::set_argument_value(testGlobalSizesKernel, 1, sizeof(void *),
                          &globalSizesBuffer);

  uint64_t commandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListGetNextCommandIdExp(
                                   mutableCmdList, &commandIdDesc, &commandId));

  ze_group_count_t groupCount{bufferSizeX / groupSizeX,
                              bufferSizeY / groupSizeY,
                              bufferSizeZ / groupSizeZ};
  lzt::append_launch_function(mutableCmdList, testGlobalSizesKernel,
                              &groupCount, nullptr, 0, nullptr);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], i);
  }
  EXPECT_EQ(globalSizesBuffer[0], bufferSizeX);
  EXPECT_EQ(globalSizesBuffer[1], bufferSizeY);
  EXPECT_EQ(globalSizesBuffer[2], bufferSizeZ);

  ze_group_count_t mutatedGroupCount{1, 1, 1};
  ze_mutable_group_count_exp_desc_t mutateGroupCount{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC};
  mutateGroupCount.commandId = commandId;
  mutateGroupCount.pGroupCount = &mutatedGroupCount;
  mutableCmdDesc.pNext = &mutateGroupCount;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandsExp(
                                   mutableCmdList, &mutableCmdDesc));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  int32_t smallerBufferSize = groupSizeX * groupSizeY * groupSizeZ;
  for (size_t i = 0; i < smallerBufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], i + i);
  }
  for (size_t i = smallerBufferSize; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], i);
  }
  EXPECT_EQ(globalSizesBuffer[0], groupSizeX);
  EXPECT_EQ(globalSizesBuffer[1], groupSizeY);
  EXPECT_EQ(globalSizesBuffer[2], groupSizeZ);

  lzt::free_memory(inOutBuffer);
  lzt::free_memory(globalSizesBuffer);
  lzt::destroy_function(testGlobalSizesKernel);
}

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfGroupSizeWhenCommandListIsClosedThenLocalWorkSizeIsUpdated) {
  if (!groupSizeSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE not "
                    "supported";
  }

  const int32_t bufferSizeX = 128;
  const int32_t bufferSizeY = 32;
  const int32_t bufferSizeZ = 8;
  const int32_t bufferSize = bufferSizeX * bufferSizeY * bufferSizeZ;
  const int32_t dimensions = 3;

  int32_t *inOutBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutBuffer[i] = 0;
  }
  int32_t *localSizesBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(dimensions * sizeof(int32_t)));
  for (size_t i = 0; i < dimensions; i++) {
    localSizesBuffer[i] = 0;
  }

  ze_kernel_handle_t testLocalSizesKernel =
      lzt::create_function(module, "testLocalSizes");

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;
  lzt::suggest_group_size(testLocalSizesKernel, bufferSizeX, bufferSizeY,
                          bufferSizeZ, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_group_size(testLocalSizesKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(testLocalSizesKernel, 0, sizeof(void *),
                          &inOutBuffer);
  lzt::set_argument_value(testLocalSizesKernel, 1, sizeof(void *),
                          &localSizesBuffer);

  uint64_t commandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListGetNextCommandIdExp(
                                   mutableCmdList, &commandIdDesc, &commandId));

  ze_group_count_t groupCount{bufferSizeX / groupSizeX,
                              bufferSizeY / groupSizeY,
                              bufferSizeZ / groupSizeZ};
  lzt::append_launch_function(mutableCmdList, testLocalSizesKernel, &groupCount,
                              nullptr, 0, nullptr);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], i);
  }
  EXPECT_EQ(localSizesBuffer[0], groupSizeX);
  EXPECT_EQ(localSizesBuffer[1], groupSizeY);
  EXPECT_EQ(localSizesBuffer[2], groupSizeZ);

  ze_mutable_group_size_exp_desc_t mutateGroupSize = {
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC};
  mutateGroupSize.commandId = commandId;
  mutateGroupSize.groupSizeX = groupSizeX / 2;
  mutateGroupSize.groupSizeY = groupSizeY;
  mutateGroupSize.groupSizeZ = groupSizeZ;
  mutableCmdDesc.pNext = &mutateGroupSize;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandsExp(
                                   mutableCmdList, &mutableCmdDesc));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < bufferSize / 2; i++) {
    EXPECT_EQ(inOutBuffer[i], i + i);
  }
  for (size_t i = bufferSize / 2; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], i);
  }
  EXPECT_EQ(localSizesBuffer[0], groupSizeX / 2);
  EXPECT_EQ(localSizesBuffer[1], groupSizeY);
  EXPECT_EQ(localSizesBuffer[2], groupSizeZ);

  lzt::free_memory(inOutBuffer);
  lzt::free_memory(localSizesBuffer);
  lzt::destroy_function(testLocalSizesKernel);
}

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfGlobalOffsetWhenCommandListIsClosedThenGlobalOffsetIsUpdated) {
  if (!globalOffsetSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET not "
                    "supported";
  }

  const int32_t dimensions = 3;
  int32_t *globalOffsetsBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(dimensions * sizeof(int32_t)));
  for (size_t i = 0; i < dimensions; i++) {
    globalOffsetsBuffer[i] = 0;
  }

  ze_kernel_handle_t testGlobalOffsetKernel =
      lzt::create_function(module, "testGlobalOffset");

  uint32_t offsetX = 1;
  uint32_t offsetY = 2;
  uint32_t offsetZ = 3;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSetGlobalOffsetExp(testGlobalOffsetKernel, offsetX, offsetY,
                                       offsetZ));
  lzt::set_group_size(testGlobalOffsetKernel, 1, 1, 1);
  lzt::set_argument_value(testGlobalOffsetKernel, 0, sizeof(void *),
                          &globalOffsetsBuffer);

  uint64_t commandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListGetNextCommandIdExp(
                                   mutableCmdList, &commandIdDesc, &commandId));

  ze_group_count_t groupCount{1, 1, 1};
  lzt::append_launch_function(mutableCmdList, testGlobalOffsetKernel,
                              &groupCount, nullptr, 0, nullptr);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(globalOffsetsBuffer[0], offsetX);
  EXPECT_EQ(globalOffsetsBuffer[1], offsetY);
  EXPECT_EQ(globalOffsetsBuffer[2], offsetZ);

  uint32_t mutatedOffsetX = 4;
  uint32_t mutatedOffsetY = 5;
  uint32_t mutatedOffsetZ = 6;
  ze_mutable_global_offset_exp_desc_t mutateGlobalOffset = {
      ZE_STRUCTURE_TYPE_MUTABLE_GLOBAL_OFFSET_EXP_DESC};
  mutateGlobalOffset.commandId = commandId;
  mutateGlobalOffset.offsetX = mutatedOffsetX;
  mutateGlobalOffset.offsetY = mutatedOffsetY;
  mutateGlobalOffset.offsetZ = mutatedOffsetZ;
  mutableCmdDesc.pNext = &mutateGlobalOffset;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandsExp(
                                   mutableCmdList, &mutableCmdDesc));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(globalOffsetsBuffer[0], offsetX + mutatedOffsetX);
  EXPECT_EQ(globalOffsetsBuffer[1], offsetY + mutatedOffsetY);
  EXPECT_EQ(globalOffsetsBuffer[2], offsetZ + mutatedOffsetZ);

  lzt::free_memory(globalOffsetsBuffer);
  lzt::destroy_function(testGlobalOffsetKernel);
}

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfHostSignalEventWhenCommandListIsClosedThenSignalEventIsReplaced) {
  if (!signalEventSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT not "
                    "supported";
  }

  const int32_t bufferSize = 16384;
  const int32_t initBufferVal = 1;
  const int32_t addVal = 2;
  int32_t *inOutBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutBuffer[i] = initBufferVal;
  }

  ze_kernel_handle_t addKernel = lzt::create_function(module, "addValue");

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;
  lzt::suggest_group_size(addKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  lzt::set_group_size(addKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(addKernel, 0, sizeof(void *), &inOutBuffer);
  lzt::set_argument_value(addKernel, 1, sizeof(addVal), &addVal);

  lzt::zeEventPool eventPool;
  const uint32_t eventNumber = 2;
  std::vector<ze_event_handle_t> events(eventNumber, nullptr);
  eventPool.InitEventPool(context, eventNumber);
  eventPool.create_events(events, eventNumber);

  uint64_t commandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListGetNextCommandIdExp(
                                   mutableCmdList, &commandIdDesc, &commandId));

  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount, events[0],
                              0, nullptr);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[0]));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[1]));
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + addVal);
  }

  for (size_t i = 0; i < events.size(); i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events[i]));
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandSignalEventExp(
                                   mutableCmdList, commandId, events[1]));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[0]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[1]));
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + addVal + addVal);
  }

  lzt::free_memory(inOutBuffer);
  eventPool.destroy_events(events);
  lzt::destroy_function(addKernel);
}

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfWaitListWithHostSignalEventWhenCommandListIsClosedThenSignalEventInWaitListIsReplaced) {
  if (!waitEventsSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS not "
                    "supported";
  }

  const int32_t bufferSize = 16384;
  const int32_t initBufferVal = 1;
  const int32_t addVal = 2;
  int32_t *inOutBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutBuffer[i] = initBufferVal;
  }

  ze_kernel_handle_t addKernel = lzt::create_function(module, "addValue");

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;
  lzt::suggest_group_size(addKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  lzt::set_group_size(addKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(addKernel, 0, sizeof(void *), &inOutBuffer);
  lzt::set_argument_value(addKernel, 1, sizeof(addVal), &addVal);

  lzt::zeEventPool eventPool;
  std::vector<ze_event_handle_t> events(2, nullptr);
  eventPool.InitEventPool(context, 2);
  eventPool.create_events(events, 2);

  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeEventHostSignal(events[0])); // Signal event 0 manually
  lzt::append_signal_event(mutableCmdList,
                           events[1]); // Signal event 1 by cmdList

  uint64_t commandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListGetNextCommandIdExp(
                                   mutableCmdList, &commandIdDesc, &commandId));
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount, nullptr,
                              1, &events[0]);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[0]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[1]));
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + addVal);
  }

  for (size_t i = 0; i < events.size(); i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeEventHostReset(events[i])); // Reset both events
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandWaitEventsExp(
                                   mutableCmdList, commandId, 1,
                                   &events[1])); // Change to wait for event 1

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[0]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[1]));
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + addVal + addVal);
  }

  lzt::free_memory(inOutBuffer);
  eventPool.destroy_events(events);
  lzt::destroy_function(addKernel);
}

} // namespace
