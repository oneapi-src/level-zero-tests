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
    kernelInstructionSupport = mutableCmdListProps.mutableCommandFlags &
                               ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
  }

  void TearDown() override {
    if (mutableCmdList) {
      lzt::destroy_command_list(mutableCmdList);
      lzt::destroy_module(module);
      lzt::destroy_command_queue(queue);
      lzt::destroy_context(context);
    }
  }

  bool CheckExtensionSupport(ze_mutable_command_list_exp_version_t version) {
    auto driver_extension_properties = lzt::get_extension_properties(driver);
    for (auto &extension : driver_extension_properties) {
      if (strcmp(extension.name, ZE_MUTABLE_COMMAND_LIST_EXP_NAME) == 0) {
        if (version <= extension.version) {
          LOG_INFO << "Detected extension version "
                   << ZE_MAJOR_VERSION(extension.version) << "."
                   << ZE_MINOR_VERSION(extension.version)
                   << " is equal or higher than required.";
          return true;
        } else {
          LOG_ERROR << "Detected extension version "
                    << ZE_MAJOR_VERSION(extension.version) << "."
                    << ZE_MINOR_VERSION(extension.version)
                    << " is lower than required.";
          return false;
        }
        break;
      }
    }
    LOG_ERROR << "Required extension " << ZE_MUTABLE_COMMAND_LIST_EXP_NAME
              << " not found";
    return false;
  }

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
  bool kernelInstructionSupport = false;

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
    GivenMutationOfKernelInstructionWhenCommandListIsClosedThenKernelIsReplaced) {
  if (!CheckExtensionSupport(ZE_MUTABLE_COMMAND_LIST_EXP_VERSION_1_1) ||
      !kernelInstructionSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION not "
                    "supported";
  }
  const int32_t bufferSize = 16384;
  const int32_t initBufferVal = 1111;
  const int32_t scalarVal = 3333;
  int32_t *inOutBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutBuffer[i] = initBufferVal;
  }
  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;

  ze_kernel_handle_t addKernel = lzt::create_function(module, "addValue");
  ze_kernel_handle_t mulKernel = lzt::create_function(module, "mulValue");
  std::vector<ze_kernel_handle_t> kernels{addKernel, mulKernel};

  lzt::suggest_group_size(addKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  lzt::set_group_size(addKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(addKernel, 0, sizeof(void *), &inOutBuffer);
  lzt::set_argument_value(addKernel, 1, sizeof(scalarVal), &scalarVal);

  uint64_t mutableKernelCommandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListGetNextCommandIdWithKernelsExp(
                mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(),
                &mutableKernelCommandId));

  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount, nullptr,
                              0, nullptr);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + scalarVal);
  }

  // Mutate kernel
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListUpdateMutableCommandKernelsExp(
                mutableCmdList, 1, &mutableKernelCommandId, &mulKernel));

  // Mutate all invalidated data
  ze_mutable_group_count_exp_desc_t mutateGroupCount{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC};
  mutateGroupCount.commandId = mutableKernelCommandId;
  mutateGroupCount.pGroupCount = &groupCount;
  ze_mutable_group_size_exp_desc_t mutateGroupSize{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC};
  mutateGroupSize.commandId = mutableKernelCommandId;
  mutateGroupSize.groupSizeX = groupSizeX;
  mutateGroupSize.groupSizeY = groupSizeY;
  mutateGroupSize.groupSizeZ = groupSizeZ;
  mutateGroupSize.pNext = &mutateGroupCount;
  ze_mutable_kernel_argument_exp_desc_t mutateBufferKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateBufferKernelArg.commandId = mutableKernelCommandId;
  mutateBufferKernelArg.argIndex = 0;
  mutateBufferKernelArg.argSize = sizeof(void *);
  mutateBufferKernelArg.pArgValue = &inOutBuffer;
  mutateBufferKernelArg.pNext = &mutateGroupSize;
  ze_mutable_kernel_argument_exp_desc_t mutateScalarKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateScalarKernelArg.commandId = mutableKernelCommandId;
  mutateScalarKernelArg.argIndex = 1;
  mutateScalarKernelArg.argSize = sizeof(scalarVal);
  mutateScalarKernelArg.pArgValue = &scalarVal;
  mutateScalarKernelArg.pNext = &mutateBufferKernelArg;
  mutableCmdDesc.pNext = &mutateScalarKernelArg;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandsExp(
                                   mutableCmdList, &mutableCmdDesc));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], (initBufferVal + scalarVal) * scalarVal);
  }
  lzt::free_memory(inOutBuffer);
  lzt::destroy_function(addKernel);
  lzt::destroy_function(mulKernel);
}

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfKernelInstructionWithSignalEventWhenCommandListIsClosedThenKernelIsReplacedAndEventRemainsUnchanged) {
  if (!CheckExtensionSupport(ZE_MUTABLE_COMMAND_LIST_EXP_VERSION_1_1) ||
      !kernelInstructionSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION not "
                    "supported";
  }
  const int32_t bufferSize = 16384;
  const int32_t initBufferVal = 1111;
  const int32_t scalarVal = 3333;
  int32_t *inOutBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutBuffer[i] = initBufferVal;
  }

  lzt::zeEventPool eventPool;
  const uint32_t eventsNumber = 1;
  std::vector<ze_event_handle_t> events(eventsNumber, nullptr);
  eventPool.InitEventPool(context, eventsNumber,
                          ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  eventPool.create_events(events, eventsNumber);

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;

  ze_kernel_handle_t addKernel = lzt::create_function(module, "addValue");
  ze_kernel_handle_t mulKernel = lzt::create_function(module, "mulValue");
  std::vector<ze_kernel_handle_t> kernels{addKernel, mulKernel};

  lzt::suggest_group_size(addKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  lzt::set_group_size(addKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(addKernel, 0, sizeof(void *), &inOutBuffer);
  lzt::set_argument_value(addKernel, 1, sizeof(scalarVal), &scalarVal);

  uint64_t mutableKernelCommandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListGetNextCommandIdWithKernelsExp(
                mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(),
                &mutableKernelCommandId));

  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount, events[0],
                              0, nullptr);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[0]));
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + scalarVal);
  }
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events[0]));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[0]));

  // Mutate kernel
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListUpdateMutableCommandKernelsExp(
                mutableCmdList, 1, &mutableKernelCommandId, &mulKernel));

  // Mutate all invalidated data
  ze_mutable_group_count_exp_desc_t mutateGroupCount{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC};
  mutateGroupCount.commandId = mutableKernelCommandId;
  mutateGroupCount.pGroupCount = &groupCount;
  ze_mutable_group_size_exp_desc_t mutateGroupSize{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC};
  mutateGroupSize.commandId = mutableKernelCommandId;
  mutateGroupSize.groupSizeX = groupSizeX;
  mutateGroupSize.groupSizeY = groupSizeY;
  mutateGroupSize.groupSizeZ = groupSizeZ;
  mutateGroupSize.pNext = &mutateGroupCount;
  ze_mutable_kernel_argument_exp_desc_t mutateBufferKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateBufferKernelArg.commandId = mutableKernelCommandId;
  mutateBufferKernelArg.argIndex = 0;
  mutateBufferKernelArg.argSize = sizeof(void *);
  mutateBufferKernelArg.pArgValue = &inOutBuffer;
  mutateBufferKernelArg.pNext = &mutateGroupSize;
  ze_mutable_kernel_argument_exp_desc_t mutateScalarKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateScalarKernelArg.commandId = mutableKernelCommandId;
  mutateScalarKernelArg.argIndex = 1;
  mutateScalarKernelArg.argSize = sizeof(scalarVal);
  mutateScalarKernelArg.pArgValue = &scalarVal;
  mutateScalarKernelArg.pNext = &mutateBufferKernelArg;
  mutableCmdDesc.pNext = &mutateScalarKernelArg;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandsExp(
                                   mutableCmdList, &mutableCmdDesc));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[0]));
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], (initBufferVal + scalarVal) * scalarVal);
  }
  eventPool.destroy_events(events);
  lzt::free_memory(inOutBuffer);
  lzt::destroy_function(addKernel);
  lzt::destroy_function(mulKernel);
}

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfKernelInstructionWithWaitListWhenCommandListIsClosedThenKernelIsReplacedAndWaitListRemainsUnchanged) {
  if (!CheckExtensionSupport(ZE_MUTABLE_COMMAND_LIST_EXP_VERSION_1_1) ||
      !kernelInstructionSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION not "
                    "supported";
  }
  const int32_t bufferSize = 16384;
  const int32_t initBufferVal = 123;
  const int32_t addVal = 456;
  const int32_t mulVal = 789;
  const int32_t subVal = 321;

  int32_t *inOutBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutBuffer[i] = initBufferVal;
  }

  lzt::zeEventPool eventPool;
  const uint32_t eventsNumber = 2;
  std::vector<ze_event_handle_t> events(eventsNumber, nullptr);
  eventPool.InitEventPool(context, eventsNumber,
                          ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  eventPool.create_events(events, eventsNumber);

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;

  ze_kernel_handle_t addKernel = lzt::create_function(module, "addValue");
  ze_kernel_handle_t mulKernel = lzt::create_function(module, "mulValue");
  ze_kernel_handle_t subKernel = lzt::create_function(module, "subValue");
  std::vector<ze_kernel_handle_t> kernels{addKernel, mulKernel, subKernel};

  lzt::suggest_group_size(addKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  lzt::set_group_size(addKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(addKernel, 0, sizeof(void *), &inOutBuffer);
  lzt::set_argument_value(addKernel, 1, sizeof(addVal), &addVal);
  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(events[0]));
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount, events[1],
                              0, nullptr);

  lzt::set_group_size(mulKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(mulKernel, 0, sizeof(void *), &inOutBuffer);
  lzt::set_argument_value(mulKernel, 1, sizeof(mulVal), &mulVal);

  uint64_t mutableKernelCommandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListGetNextCommandIdWithKernelsExp(
                mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(),
                &mutableKernelCommandId));

  lzt::append_launch_function(mutableCmdList, mulKernel, &groupCount, nullptr,
                              2, events.data());
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[0]));
  const uint32_t firstResult = (initBufferVal + addVal) * mulVal;
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], firstResult);
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events[1]));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[1]));

  // Mutate kernel
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListUpdateMutableCommandKernelsExp(
                mutableCmdList, 1, &mutableKernelCommandId, &subKernel));

  // Mutate all invalidated data
  ze_mutable_group_count_exp_desc_t mutateGroupCount{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC};
  mutateGroupCount.commandId = mutableKernelCommandId;
  mutateGroupCount.pGroupCount = &groupCount;
  ze_mutable_group_size_exp_desc_t mutateGroupSize{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC};
  mutateGroupSize.commandId = mutableKernelCommandId;
  mutateGroupSize.groupSizeX = groupSizeX;
  mutateGroupSize.groupSizeY = groupSizeY;
  mutateGroupSize.groupSizeZ = groupSizeZ;
  mutateGroupSize.pNext = &mutateGroupCount;
  ze_mutable_kernel_argument_exp_desc_t mutateBufferKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateBufferKernelArg.commandId = mutableKernelCommandId;
  mutateBufferKernelArg.argIndex = 0;
  mutateBufferKernelArg.argSize = sizeof(void *);
  mutateBufferKernelArg.pArgValue = &inOutBuffer;
  mutateBufferKernelArg.pNext = &mutateGroupSize;
  ze_mutable_kernel_argument_exp_desc_t mutateScalarKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateScalarKernelArg.commandId = mutableKernelCommandId;
  mutateScalarKernelArg.argIndex = 1;
  mutateScalarKernelArg.argSize = sizeof(subVal);
  mutateScalarKernelArg.pArgValue = &subVal;
  mutateScalarKernelArg.pNext = &mutateBufferKernelArg;
  mutableCmdDesc.pNext = &mutateScalarKernelArg;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandsExp(
                                   mutableCmdList, &mutableCmdDesc));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[1]));
  const uint32_t secondResult = firstResult + addVal - subVal;
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], secondResult);
  }
  eventPool.destroy_events(events);
  lzt::free_memory(inOutBuffer);
  lzt::destroy_function(addKernel);
  lzt::destroy_function(mulKernel);
  lzt::destroy_function(subKernel);
}

TEST_F(
    zeMutableCommandListTests,
    GivenMutationOfMultipleKernelInstructionsAndEventsWhenCommandListIsClosedThenEverythingIsUpdatedCorrectly) {
  if (!CheckExtensionSupport(ZE_MUTABLE_COMMAND_LIST_EXP_VERSION_1_1) ||
      !signalEventSupport || !waitEventsSupport || !kernelInstructionSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION not "
                    "supported";
  }
  const int32_t bufferSize = 16384;
  const int32_t initBufferVal = 100;
  const int32_t addVal = 20;
  const int32_t mulVal = 30;
  const int32_t subVal = 40;
  const int32_t divVal = 4;

  lzt::zeEventPool eventPool;
  const uint32_t eventsNumber = 4;
  std::vector<ze_event_handle_t> events(eventsNumber, nullptr);
  eventPool.InitEventPool(context, eventsNumber,
                          ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  eventPool.create_events(events, eventsNumber);

  int32_t *inOutBuffer1 = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  int32_t *inOutBuffer2 = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutBuffer1[i] = initBufferVal;
    inOutBuffer2[i] = initBufferVal;
  }

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;

  ze_kernel_handle_t addKernel = lzt::create_function(module, "addValue");
  ze_kernel_handle_t mulKernel = lzt::create_function(module, "mulValue");
  ze_kernel_handle_t subKernel = lzt::create_function(module, "subValue");
  ze_kernel_handle_t divKernel = lzt::create_function(module, "divValue");

  uint64_t kernelCommandId1 = 0;
  uint64_t kernelCommandId2 = 0;
  uint64_t kernelCommandId3 = 0;
  std::vector<ze_kernel_handle_t> kernels{addKernel, mulKernel, subKernel,
                                          divKernel};
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS |
                        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;

  lzt::suggest_group_size(addKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};

  // 1 addKernel
  lzt::set_group_size(addKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(addKernel, 0, sizeof(void *), &inOutBuffer1);
  lzt::set_argument_value(addKernel, 1, sizeof(addVal), &addVal);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListGetNextCommandIdWithKernelsExp(
                mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(),
                &kernelCommandId1));
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount, events[0],
                              0, nullptr);
  // 2 mulKernel
  lzt::set_group_size(mulKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(mulKernel, 0, sizeof(void *), &inOutBuffer1);
  lzt::set_argument_value(mulKernel, 1, sizeof(mulVal), &mulVal);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListGetNextCommandIdWithKernelsExp(
                mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(),
                &kernelCommandId2));
  lzt::append_launch_function(mutableCmdList, mulKernel, &groupCount, events[1],
                              1, &events[0]);
  // 3 subKernel
  lzt::set_group_size(subKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(subKernel, 0, sizeof(void *), &inOutBuffer1);
  lzt::set_argument_value(subKernel, 1, sizeof(subVal), &subVal);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListGetNextCommandIdWithKernelsExp(
                mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(),
                &kernelCommandId3));
  lzt::append_launch_function(mutableCmdList, subKernel, &groupCount, nullptr,
                              2, &events[0]);

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[0]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[1]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events[0]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events[1]));
  const uint32_t firstResult = ((initBufferVal + addVal) * mulVal) - subVal;
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer1[i], firstResult);
  }

  // Update events
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListUpdateMutableCommandSignalEventExp(
                mutableCmdList, kernelCommandId1, events[2]));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListUpdateMutableCommandSignalEventExp(
                mutableCmdList, kernelCommandId2, events[3]));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListUpdateMutableCommandWaitEventsExp(
                mutableCmdList, kernelCommandId2, 1, &events[2]));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListUpdateMutableCommandWaitEventsExp(
                mutableCmdList, kernelCommandId3, 2, &events[2]));

  // Change kernels sequence from add, mul, sub to mul, sub, div
  std::vector<uint64_t> commandIds{kernelCommandId1, kernelCommandId2,
                                   kernelCommandId3};
  std::vector<ze_kernel_handle_t> newSequenceOfKenrels{mulKernel, subKernel,
                                                       divKernel};

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandKernelsExp(
                                   mutableCmdList, 3, commandIds.data(),
                                   newSequenceOfKenrels.data()));

  // Mutate invalidated data for kernel 1
  ze_mutable_group_count_exp_desc_t mutateGroupCount{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC};
  mutateGroupCount.commandId = kernelCommandId1;
  mutateGroupCount.pGroupCount = &groupCount;
  ze_mutable_group_size_exp_desc_t mutateGroupSize{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC};
  mutateGroupSize.commandId = kernelCommandId1;
  mutateGroupSize.groupSizeX = groupSizeX;
  mutateGroupSize.groupSizeY = groupSizeY;
  mutateGroupSize.groupSizeZ = groupSizeZ;
  mutateGroupSize.pNext = &mutateGroupCount;
  ze_mutable_kernel_argument_exp_desc_t mutateBufferKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateBufferKernelArg.commandId = kernelCommandId1;
  mutateBufferKernelArg.argIndex = 0;
  mutateBufferKernelArg.argSize = sizeof(void *);
  mutateBufferKernelArg.pArgValue = &inOutBuffer2;
  mutateBufferKernelArg.pNext = &mutateGroupSize;
  ze_mutable_kernel_argument_exp_desc_t mutateScalarKernelArg{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateScalarKernelArg.commandId = kernelCommandId1;
  mutateScalarKernelArg.argIndex = 1;
  mutateScalarKernelArg.argSize = sizeof(mulVal);
  mutateScalarKernelArg.pArgValue = &mulVal;
  mutateScalarKernelArg.pNext = &mutateBufferKernelArg;

  // Mutate invalidated data for kernel 2
  ze_mutable_group_count_exp_desc_t mutateGroupCount2{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC};
  mutateGroupCount2.commandId = kernelCommandId2;
  mutateGroupCount2.pGroupCount = &groupCount;
  mutateGroupCount2.pNext = &mutateScalarKernelArg;
  ze_mutable_group_size_exp_desc_t mutateGroupSize2{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC};
  mutateGroupSize2.commandId = kernelCommandId2;
  mutateGroupSize2.groupSizeX = groupSizeX;
  mutateGroupSize2.groupSizeY = groupSizeY;
  mutateGroupSize2.groupSizeZ = groupSizeZ;
  mutateGroupSize2.pNext = &mutateGroupCount2;
  ze_mutable_kernel_argument_exp_desc_t mutateBufferKernelArg2{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateBufferKernelArg2.commandId = kernelCommandId2;
  mutateBufferKernelArg2.argIndex = 0;
  mutateBufferKernelArg2.argSize = sizeof(void *);
  mutateBufferKernelArg2.pArgValue = &inOutBuffer2;
  mutateBufferKernelArg2.pNext = &mutateGroupSize2;
  ze_mutable_kernel_argument_exp_desc_t mutateScalarKernelArg2{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateScalarKernelArg2.commandId = kernelCommandId2;
  mutateScalarKernelArg2.argIndex = 1;
  mutateScalarKernelArg2.argSize = sizeof(subVal);
  mutateScalarKernelArg2.pArgValue = &subVal;
  mutateScalarKernelArg2.pNext = &mutateBufferKernelArg2;

  // Mutate invalidated data for kernel 3
  ze_mutable_group_count_exp_desc_t mutateGroupCount3{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC};
  mutateGroupCount3.commandId = kernelCommandId3;
  mutateGroupCount3.pGroupCount = &groupCount;
  mutateGroupCount3.pNext = &mutateScalarKernelArg2;
  ze_mutable_group_size_exp_desc_t mutateGroupSize3{
      ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC};
  mutateGroupSize3.commandId = kernelCommandId3;
  mutateGroupSize3.groupSizeX = groupSizeX;
  mutateGroupSize3.groupSizeY = groupSizeY;
  mutateGroupSize3.groupSizeZ = groupSizeZ;
  mutateGroupSize3.pNext = &mutateGroupCount3;
  ze_mutable_kernel_argument_exp_desc_t mutateBufferKernelArg3{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateBufferKernelArg3.commandId = kernelCommandId3;
  mutateBufferKernelArg3.argIndex = 0;
  mutateBufferKernelArg3.argSize = sizeof(void *);
  mutateBufferKernelArg3.pArgValue = &inOutBuffer2;
  mutateBufferKernelArg3.pNext = &mutateGroupSize3;
  ze_mutable_kernel_argument_exp_desc_t mutateScalarKernelArg3{
      ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC};
  mutateScalarKernelArg3.commandId = kernelCommandId3;
  mutateScalarKernelArg3.argIndex = 1;
  mutateScalarKernelArg3.argSize = sizeof(divVal);
  mutateScalarKernelArg3.pArgValue = &divVal;
  mutateScalarKernelArg3.pNext = &mutateBufferKernelArg3;

  mutableCmdDesc.pNext = &mutateScalarKernelArg3;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandsExp(
                                   mutableCmdList, &mutableCmdDesc));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[0]));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[1]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[2]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[3]));
  const uint32_t secondResult = ((initBufferVal * mulVal) - subVal) / divVal;
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer2[i], secondResult);
  }

  eventPool.destroy_events(events);
  lzt::free_memory(inOutBuffer1);
  lzt::free_memory(inOutBuffer2);
  lzt::destroy_function(addKernel);
  lzt::destroy_function(mulKernel);
  lzt::destroy_function(subKernel);
  lzt::destroy_function(divKernel);
}

class zeMutableCommandListTestsEvents
    : public zeMutableCommandListTests,
      public ::testing::WithParamInterface<ze_event_pool_flags_t> {
protected:
  ze_event_pool_flags_t eventPoolFlags{};
  zeMutableCommandListTestsEvents() : eventPoolFlags(GetParam()) {}

  bool isTimestampEvent(ze_event_pool_flags_t flags) {
    return flags & ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP ||
           flags & ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP;
  }
  void verifyKernelTimestamp(ze_event_handle_t eventHandle) {
    ze_kernel_timestamp_result_t kernelTimestamp{};
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeEventQueryKernelTimestamp(eventHandle, &kernelTimestamp));
    EXPECT_GT(kernelTimestamp.context.kernelEnd,
              kernelTimestamp.context.kernelStart);
  }
};

TEST_P(
    zeMutableCommandListTestsEvents,
    GivenMutationOfSignalEventWhenCommandListIsClosedThenSignalEventIsReplaced) {
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
  const uint32_t eventsNumber = 2;
  std::vector<ze_event_handle_t> events(eventsNumber, nullptr);
  eventPool.InitEventPool(context, eventsNumber, eventPoolFlags);
  eventPool.create_events(events, eventsNumber);

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
  if (isTimestampEvent(eventPoolFlags)) {
    verifyKernelTimestamp(events[0]);
  }
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
  if (isTimestampEvent(eventPoolFlags)) {
    verifyKernelTimestamp(events[1]);
  }
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + addVal + addVal);
  }

  lzt::free_memory(inOutBuffer);
  eventPool.destroy_events(events);
  lzt::destroy_function(addKernel);
}

TEST_P(
    zeMutableCommandListTestsEvents,
    GivenMutationOfWaitListToNullPointerWhenCommandListIsClosedThenWaitListIsRemoved) {
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

  const int32_t eventsNumber = 3;
  lzt::zeEventPool eventPool;
  std::vector<ze_event_handle_t> events(eventsNumber, nullptr);
  eventPool.InitEventPool(context, eventsNumber, eventPoolFlags);
  eventPool.create_events(events, eventsNumber);

  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};
  for (size_t i = 0; i < events.size(); i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(events[i]));
  }

  uint64_t commandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListGetNextCommandIdExp(
                                   mutableCmdList, &commandIdDesc, &commandId));
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount, nullptr,
                              eventsNumber, &events[0]);
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + addVal);
  }
  for (size_t i = 0; i < events.size(); i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events[i]));
  }

  std::vector<ze_event_handle_t> nullEvents(eventsNumber, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListUpdateMutableCommandWaitEventsExp(
                mutableCmdList, commandId, 3, &nullEvents[0]));

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());
  for (size_t i = 0; i < events.size(); i++) {
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[i]));
  }
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutBuffer[i], initBufferVal + addVal + addVal);
  }

  eventPool.destroy_events(events);
  lzt::free_memory(inOutBuffer);
  lzt::destroy_function(addKernel);
}

TEST_P(
    zeMutableCommandListTestsEvents,
    GivenMutationOfWaitListWithEventWhenCommandListIsClosedThenSignalEventInWaitListIsReplaced) {
  if (!waitEventsSupport) {
    GTEST_SKIP() << "ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS not "
                    "supported";
  }

  const int32_t bufferSize = 16384;
  const int32_t initBufferVal = 1;
  const int32_t addVal = 2;
  const int32_t mulVal = 5;
  int32_t *inOutAddBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  int32_t *inOutMulBuffer = reinterpret_cast<int32_t *>(
      lzt::allocate_host_memory(bufferSize * sizeof(int32_t)));
  for (size_t i = 0; i < bufferSize; i++) {
    inOutAddBuffer[i] = initBufferVal;
    inOutMulBuffer[i] = initBufferVal;
  }

  ze_kernel_handle_t addKernel = lzt::create_function(module, "addValue");
  ze_kernel_handle_t mulKernel = lzt::create_function(module, "mulValue");

  uint32_t groupSizeX = 0;
  uint32_t groupSizeY = 0;
  uint32_t groupSizeZ = 0;
  lzt::suggest_group_size(addKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  lzt::set_group_size(addKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(addKernel, 0, sizeof(void *), &inOutAddBuffer);
  lzt::set_argument_value(addKernel, 1, sizeof(addVal), &addVal);

  lzt::suggest_group_size(mulKernel, bufferSize, 1, 1, groupSizeX, groupSizeY,
                          groupSizeZ);
  lzt::set_group_size(mulKernel, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(mulKernel, 0, sizeof(void *), &inOutMulBuffer);
  lzt::set_argument_value(mulKernel, 1, sizeof(mulVal), &mulVal);

  const int32_t eventsNumber = 3;
  lzt::zeEventPool eventPool;
  std::vector<ze_event_handle_t> events(eventsNumber, nullptr);
  eventPool.InitEventPool(context, eventsNumber, eventPoolFlags);
  eventPool.create_events(events, eventsNumber);

  ze_group_count_t groupCount{bufferSize / groupSizeX, 1, 1};

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeEventHostSignal(events[0])); // signal event 0 manually
  lzt::append_launch_function(mutableCmdList, addKernel, &groupCount,
                              events[2], // kernel event 2
                              0, nullptr);
  lzt::append_launch_function(mutableCmdList, mulKernel, &groupCount,
                              events[1], // kernel event 1
                              0, nullptr);
  uint64_t commandId = 0;
  commandIdDesc.flags = ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListGetNextCommandIdExp(
                                   mutableCmdList, &commandIdDesc, &commandId));
  lzt::append_launch_function(mutableCmdList, mulKernel, &groupCount, nullptr,
                              2, &events[0]); // wait for 0 and 1
  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  for (size_t i = 0; i < events.size(); i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[i]));
  }

  int32_t addResult = initBufferVal + addVal;
  int32_t mulResult = initBufferVal * mulVal * mulVal;
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutAddBuffer[i], addResult);
    EXPECT_EQ(inOutMulBuffer[i], mulResult);
  }

  for (size_t i = 0; i < events.size(); i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeEventHostReset(events[i])); // reset all events
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListUpdateMutableCommandWaitEventsExp(
                                   mutableCmdList, commandId, 2,
                                   &events[1])); // wait for 1 and 2 instead

  lzt::close_command_list(mutableCmdList);
  lzt::execute_command_lists(queue, 1, &mutableCmdList, nullptr);
  lzt::synchronize(queue, std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[0]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[1]));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(events[2]));
  for (size_t i = 0; i < bufferSize; i++) {
    EXPECT_EQ(inOutAddBuffer[i], addResult + addVal);
    EXPECT_EQ(inOutMulBuffer[i], mulResult * mulVal * mulVal);
  }

  lzt::free_memory(inOutAddBuffer);
  lzt::free_memory(inOutMulBuffer);
  eventPool.destroy_events(events);
  lzt::destroy_function(addKernel);
  lzt::destroy_function(mulKernel);
}

INSTANTIATE_TEST_SUITE_P(
    zeMutableCommandListTests, zeMutableCommandListTestsEvents,
    testing::Values(ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
                    ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC,
                    ZE_EVENT_POOL_FLAG_HOST_VISIBLE |
                        ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP,
                    ZE_EVENT_POOL_FLAG_HOST_VISIBLE |
                        ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP));

} // namespace
