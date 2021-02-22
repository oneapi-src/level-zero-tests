/*
 *
 * Copyright (C) 2019 Intel Corporation
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

class zeImmediateCommandListExecutionTests
    : public lzt::zeEventPoolTests,
      public ::testing::WithParamInterface<ze_command_queue_mode_t> {

protected:
  void SetUp() override {
    mode = GetParam();

    if (mode == ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
      timeout = 0;
    }

    ep.InitEventPool(10);
    ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);

    cmdlist_immediate = lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(), 0, mode,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  }

  void TearDown() override {
    ep.destroy_event(event0);
    lzt::destroy_command_list(cmdlist_immediate);
  }
  ze_event_handle_t event0 = nullptr;
  ze_command_list_handle_t cmdlist_immediate = nullptr;
  ze_command_queue_mode_t mode;
  uint64_t timeout = UINT32_MAX - 1;
};

TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendingMemorySetInstructionThenVerifyImmediateExecution) {
  const size_t size = 16;
  const uint8_t one = 1;

  void *buffer = lzt::allocate_shared_memory(size * sizeof(int));
  memset(buffer, 0x0, size * sizeof(int));

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));

  // setting event on following instruction should flush memory
  lzt::append_memory_set(cmdlist_immediate, buffer, &one, size * sizeof(int),
                         event0);
  // command queue execution should be immediate, and so no timeout required for
  // synchronize
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event0, timeout));
  for (size_t i = 0; i < size * sizeof(int); i++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[i], 0x1);
  }

  lzt::free_memory(buffer);
}

TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendLaunchKernelInstructionThenVerifyImmediateExecution) {
  const size_t size = 16;
  const int addval = 10;
  const int addval2 = 15;

  void *buffer = lzt::allocate_shared_memory(size * sizeof(int));
  memset(buffer, 0x0, size * sizeof(int));

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int *p_dev = static_cast<int *>(buffer);
  lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
  ze_group_count_t tg;
  tg.groupCountX = static_cast<uint32_t>(size);
  tg.groupCountY = 1;
  tg.groupCountZ = 1;
  // setting event on following instruction should flush memory
  lzt::append_launch_function(cmdlist_immediate, kernel, &tg, event0, 0,
                              nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event0, timeout));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(event0));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<int *>(buffer)[i], addval);
  }
  lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);
  // setting event on following instruction should flush memory
  lzt::append_launch_function(cmdlist_immediate, kernel, &tg, event0, 0,
                              nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event0, timeout));
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<int *>(buffer)[i], (addval + addval2));
  }
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::free_memory(buffer);
}

TEST_P(zeImmediateCommandListExecutionTests,
       GivenImmediateCommandListWhenAppendMemoryCopyThenVerifyCopyIsCorrect) {
  const size_t size = 4096;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory =
      lzt::allocate_device_memory(lzt::size_in_bytes(host_memory1));

  lzt::write_data_pattern(host_memory1.data(), size, 1);

  // This should execute immediately
  lzt::append_memory_copy(cmdlist_immediate, device_memory, host_memory1.data(),
                          lzt::size_in_bytes(host_memory1), event0);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    zeEventHostSynchronize(event0, timeout);
    zeEventHostReset(event0);
  }
  lzt::append_memory_copy(cmdlist_immediate, host_memory2.data(), device_memory,
                          lzt::size_in_bytes(host_memory2), event0);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    zeEventHostSynchronize(event0, timeout);
  }
  lzt::validate_data_pattern(host_memory2.data(), size, 1);
  lzt::free_memory(device_memory);
}

TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenMultipleImmediateCommandListWhenAppendCommandsToMultipleCommandListThenVerifyResultIsCorrect) {

  auto device_handle = lzt::zeDevice::get_instance()->get_device();

  auto command_queue_group_properties =
      lzt::get_command_queue_group_properties(device_handle);

  uint32_t num_cmdq = 0;
  for (auto properties : command_queue_group_properties) {
    num_cmdq += properties.numQueues;
  }

  auto context_handle = lzt::get_default_context();
  ze_event_pool_desc_t event_pool_desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
      ZE_EVENT_POOL_FLAG_HOST_VISIBLE, num_cmdq};

  auto event_pool_handle =
      lzt::create_event_pool(context_handle, event_pool_desc);

  std::vector<ze_command_list_handle_t> mulcmdlist_immediate(num_cmdq, nullptr);
  std::vector<void *> buffer(num_cmdq, nullptr);
  std::vector<uint8_t> val(num_cmdq, 0);
  const size_t size = 16;

  std::vector<ze_event_handle_t> host_to_dev_event(num_cmdq, nullptr);
  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
                                ZE_COMMAND_QUEUE_PRIORITY_NORMAL};

  for (uint32_t i = 0; i < num_cmdq; i++) {
    mulcmdlist_immediate[i] = lzt::create_immediate_command_list(
        context_handle, device_handle, 0, mode,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

    buffer[i] = lzt::allocate_shared_memory(size);
    val[i] = static_cast<uint8_t>(i + 1);
    host_to_dev_event[i] = lzt::create_event(event_pool_handle, event_desc);

    // This should execute immediately
    lzt::append_memory_set(mulcmdlist_immediate[i], buffer[i], &val[i], size,
                           host_to_dev_event[i]);
    if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
      zeEventHostSynchronize(host_to_dev_event[i], timeout);
    }
  }

  // validate event status and validate buffer copied/set
  for (uint32_t i = 0; i < num_cmdq; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(host_to_dev_event[i]));
    for (size_t j = 0; j < size; j++) {
      EXPECT_EQ(static_cast<uint8_t *>(buffer[i])[j], val[i]);
    }
  }

  for (uint32_t i = 0; i < num_cmdq; i++) {
    lzt::destroy_event(host_to_dev_event[i]);
    lzt::destroy_command_list(mulcmdlist_immediate[i]);
    lzt::free_memory(buffer[i]);
  }
  lzt::destroy_event_pool(event_pool_handle);
}

TEST_P(zeImmediateCommandListExecutionTests,
       GivenImmediateCommandListWhenAppendImageCopyThenVerifyCopyIsCorrect) {

  lzt::zeImageCreateCommon img;
  // dest_host_image_upper is used to validate that the above image copy
  // operation(s) were correct:
  lzt::ImagePNG32Bit dest_host_image_upper(img.dflt_host_image_.width(),
                                           img.dflt_host_image_.height());
  // Scribble a known incorrect data pattern to dest_host_image_upper to
  // ensure we are validating actual data from the L0 functionality:
  lzt::write_image_data_pattern(dest_host_image_upper, -1);

  // First, copy the image from the host to the device:
  lzt::append_image_copy_from_mem(cmdlist_immediate, img.dflt_device_image_2_,
                                  img.dflt_host_image_.raw_data(), event0);
  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    zeEventHostSynchronize(event0, timeout);
    zeEventHostReset(event0);
  }
  // Now, copy the image from the device to the device:
  lzt::append_image_copy(cmdlist_immediate, img.dflt_device_image_,
                         img.dflt_device_image_2_, event0);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    zeEventHostSynchronize(event0, timeout);
    zeEventHostReset(event0);
  }
  // Finally copy the image from the device to the dest_host_image_upper for
  // validation:
  lzt::append_image_copy_to_mem(cmdlist_immediate,
                                dest_host_image_upper.raw_data(),
                                img.dflt_device_image_, event0);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    zeEventHostSynchronize(event0, timeout);
    zeEventHostReset(event0);
  }

  // Validate the result of the above operations:
  // If the operation is a straight image copy, or the second region is null
  // then the result should be the same:
  EXPECT_EQ(0, compare_data_pattern(dest_host_image_upper, img.dflt_host_image_,
                                    0, 0, img.dflt_host_image_.width(),
                                    img.dflt_host_image_.height(), 0, 0,
                                    img.dflt_host_image_.width(),
                                    img.dflt_host_image_.height()));
}

TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendEventResetThenSuccesIsReturnedAndEventIsReset) {
  ze_event_handle_t event1 = nullptr;
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  zeEventHostSignal(event0);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event0));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendEventReset(cmdlist_immediate, event0));
  zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS)
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event1, timeout));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));
  ep.destroy_event(event1);
}

TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendSignalEventThenSuccessIsReturnedAndEventIsSignaled) {
  ze_event_handle_t event1 = nullptr;
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  ASSERT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendSignalEvent(cmdlist_immediate, event0));
  zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS)
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event1, timeout));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event0, timeout));
  ep.destroy_event(event1);
}

TEST_P(zeImmediateCommandListExecutionTests,
       GivenImmediateCommandListWhenAppendWaitOnEventsThenSuccessIsReturned) {
  if (mode == ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS)
    return;
  ze_event_handle_t event1 = nullptr;
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event1));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendWaitOnEvents(cmdlist_immediate, 1, &event0));
  zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
  zeEventHostSignal(event0);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event0, timeout));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event1, timeout));
  ep.destroy_event(event1);
}

TEST_P(zeImmediateCommandListExecutionTests,
       GivenImmediateCommandListWhenAppendMemoryPrefetchThenSuccessIsReturned) {
  size_t size = 4096;
  void *memory = lzt::allocate_shared_memory(size);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendMemoryPrefetch(cmdlist_immediate, memory, size));
  lzt::free_memory(memory);
}

TEST_P(zeImmediateCommandListExecutionTests,
       GivenImmediateCommandListWhenAppendMemAdviseThenSuccessIsReturned) {
  size_t size = 4096;
  void *memory = lzt::allocate_shared_memory(size);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendMemAdvise(
                cmdlist_immediate, lzt::zeDevice::get_instance()->get_device(),
                memory, size, ZE_MEMORY_ADVICE_SET_READ_MOSTLY));
  lzt::free_memory(memory);
}
static ze_image_handle_t create_test_image(int height, int width) {
  ze_image_desc_t image_description = {};
  image_description.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_description.format.layout = ZE_IMAGE_FORMAT_LAYOUT_32;

  image_description.pNext = nullptr;
  image_description.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
  image_description.type = ZE_IMAGE_TYPE_2D;
  image_description.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  image_description.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_description.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_description.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_description.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_description.width = width;
  image_description.height = height;
  image_description.depth = 1;
  ze_image_handle_t image = nullptr;

  image = lzt::create_ze_image(image_description);

  return image;
}
TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenMultipleImmediateCommandListsThatHaveDependenciesThenAllTheCommandListsExecuteSuccessfully) {

  // create 2 images
  lzt::ImagePNG32Bit input("test_input.png");
  int width = input.width();
  int height = input.height();
  lzt::ImagePNG32Bit output(width, height);
  auto input_xeimage = create_test_image(height, width);
  auto output_xeimage = create_test_image(height, width);
  ze_command_list_handle_t cmdlist_immediate = nullptr;
  ze_command_list_handle_t cmdlist_immediate_1 = nullptr;
  ze_command_list_handle_t cmdlist_immediate_2 = nullptr;
  ze_command_list_handle_t cmdlist_immediate_3 = nullptr;
  cmdlist_immediate = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      static_cast<ze_command_queue_flag_t>(0), mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  cmdlist_immediate_1 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      static_cast<ze_command_queue_flag_t>(0), mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  cmdlist_immediate_2 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      static_cast<ze_command_queue_flag_t>(0), mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  cmdlist_immediate_3 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      static_cast<ze_command_queue_flag_t>(0), mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  ze_event_handle_t hEvent1, hEvent2, hEvent3, hEvent4;
  ep.create_event(hEvent1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent2, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent3, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent4, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  // Use ImageCopyFromMemory to upload ImageA
  lzt::append_image_copy_from_mem(cmdlist_immediate, input_xeimage,
                                  input.raw_data(), hEvent1);
  lzt::append_wait_on_events(cmdlist_immediate_1, 1, &hEvent1);
  // use ImageCopy to copy A -> B
  lzt::append_image_copy(cmdlist_immediate_1, output_xeimage, input_xeimage,
                         hEvent2);
  lzt::append_wait_on_events(cmdlist_immediate_2, 1, &hEvent2);
  // use ImageCopyRegion to copy part of A -> B
  ze_image_region_t sr = {0, 0, 0, 1, 1, 1};
  ze_image_region_t dr = {0, 0, 0, 1, 1, 1};
  lzt::append_image_copy_region(cmdlist_immediate_2, output_xeimage,
                                input_xeimage, &dr, &sr, hEvent3);
  lzt::append_wait_on_events(cmdlist_immediate_3, 1, &hEvent3);
  // use ImageCopyToMemory to dowload ImageB
  lzt::append_image_copy_to_mem(cmdlist_immediate_3, output.raw_data(),
                                output_xeimage, hEvent4);
  lzt::event_host_synchronize(hEvent4, UINT64_MAX);

  // Verfy A matches B
  EXPECT_EQ(0,
            memcmp(input.raw_data(), output.raw_data(), input.size_in_bytes()));

  ep.destroy_event(hEvent1);
  ep.destroy_event(hEvent2);
  ep.destroy_event(hEvent3);
  ep.destroy_event(hEvent4);
  lzt::destroy_ze_image(input_xeimage);
  lzt::destroy_ze_image(output_xeimage);
  lzt::destroy_command_list(cmdlist_immediate);
  lzt::destroy_command_list(cmdlist_immediate_1);
  lzt::destroy_command_list(cmdlist_immediate_2);
  lzt::destroy_command_list(cmdlist_immediate_3);
}

INSTANTIATE_TEST_CASE_P(TestCasesforCommandListImmediateCases,
                        zeImmediateCommandListExecutionTests,
                        testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                                        ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                                        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS));

} // namespace
