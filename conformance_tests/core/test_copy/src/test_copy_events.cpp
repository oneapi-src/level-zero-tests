/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeCommandListEventTests : public ::testing::Test {
protected:
  zeCommandListEventTests() {
    ep.create_event(hEvent, ZE_EVENT_SCOPE_FLAG_HOST, 0);
    cmdqueue = lzt::create_command_queue();
    cmdlist = lzt::create_command_list();
  }

  ~zeCommandListEventTests() {
    ep.destroy_event(hEvent);
    lzt::destroy_command_queue(cmdqueue);
    lzt::destroy_command_list(cmdlist);
  }

  ze_command_list_handle_t cmdlist;
  ze_command_queue_handle_t cmdqueue;
  ze_event_handle_t hEvent;
  size_t size = 16;
  lzt::zeEventPool ep;
};

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectly) {

  auto src_buffer = lzt::allocate_shared_memory(size);
  auto dst_buffer = lzt::allocate_shared_memory(size);
  memset(src_buffer, 0x1, size);
  memset(dst_buffer, 0x0, size);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(hEvent, 0));

  // Execute and verify GPU reads event
  lzt::append_memory_copy(cmdlist, dst_buffer, src_buffer, size, hEvent);
  lzt::append_wait_on_events(cmdlist, 1, &hEvent);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);

  // Verify Host Reads Event as set
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent, UINT32_MAX));

  // Verify Memory Copy completed
  EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, size));

  lzt::synchronize(cmdqueue, UINT32_MAX);
  lzt::free_memory(src_buffer);
  lzt::free_memory(dst_buffer);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemorySetThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectly) {

  auto ref_buffer = lzt::allocate_shared_memory(size);
  auto dst_buffer = lzt::allocate_shared_memory(size);
  memset(ref_buffer, 0x1, size);
  memset(dst_buffer, 0x0, size);
  const uint8_t one = 1;

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(hEvent, 0));

  // Execute and verify GPU reads event
  lzt::append_memory_set(cmdlist, dst_buffer, &one, size, hEvent);
  lzt::append_wait_on_events(cmdlist, 1, &hEvent);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);

  // Verify Host Reads Event as set
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent, UINT32_MAX));

  // Verify Memory Set completed
  EXPECT_EQ(0, memcmp(ref_buffer, dst_buffer, size));

  lzt::synchronize(cmdqueue, UINT32_MAX);
  lzt::free_memory(ref_buffer);
  lzt::free_memory(dst_buffer);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectly) {

  uint32_t width = 16;
  uint32_t height = 16;
  size = height * width;
  auto src_buffer = lzt::allocate_shared_memory(size);
  auto dst_buffer = lzt::allocate_shared_memory(size);
  memset(src_buffer, 0x1, size);
  memset(dst_buffer, 0x0, size);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(hEvent, 0));

  // Execute and verify GPU reads event
  ze_copy_region_t sr = {0U, 0U, 0U, width, height, 0U};
  ze_copy_region_t dr = {0U, 0U, 0U, width, height, 0U};
  lzt::append_memory_copy_region(cmdlist, dst_buffer, &dr, width, 0, src_buffer,
                                 &sr, width, 0, hEvent);
  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
  lzt::append_wait_on_events(cmdlist, 1, &hEvent);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);

  // Verify Host Reads Event as set
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent, UINT32_MAX));

  // Verify Memory Set completed
  EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, size));

  lzt::synchronize(cmdqueue, UINT32_MAX);
  lzt::free_memory(src_buffer);
  lzt::free_memory(dst_buffer);
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

TEST_F(
    zeCommandListEventTests,
    GivenImageCopyThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectly) {

  // create 2 images
  lzt::ImagePNG32Bit input("test_input.png");
  int width = input.width();
  int height = input.height();
  lzt::ImagePNG32Bit output(width, height);
  auto input_xeimage = create_test_image(height, width);
  auto output_xeimage = create_test_image(height, width);
  ze_event_handle_t hEvent1, hEvent2, hEvent3, hEvent4;
  ep.create_event(hEvent1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent2, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent3, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent4, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  // Use ImageCopyFromMemory to upload ImageA
  lzt::append_image_copy_from_mem(cmdlist, input_xeimage, input.raw_data(),
                                  hEvent1);
  lzt::append_wait_on_events(cmdlist, 1, &hEvent1);
  // use ImageCopy to copy A -> B
  lzt::append_image_copy(cmdlist, output_xeimage, input_xeimage, hEvent2);
  lzt::append_wait_on_events(cmdlist, 1, &hEvent2);
  // use ImageCopyRegion to copy part of A -> B
  ze_image_region_t sr = {0, 0, 0, 1, 1, 1};
  ze_image_region_t dr = {0, 0, 0, 1, 1, 1};
  lzt::append_image_copy_region(cmdlist, output_xeimage, input_xeimage, &dr,
                                &sr, hEvent3);
  lzt::append_wait_on_events(cmdlist, 1, &hEvent3);
  // use ImageCopyToMemory to dowload ImageB
  lzt::append_image_copy_to_mem(cmdlist, output.raw_data(), output_xeimage,
                                hEvent4);
  lzt::append_wait_on_events(cmdlist, 1, &hEvent4);
  // execute commands
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);

  // Make sure all events signaled from host perspective
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent1, UINT32_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent2, UINT32_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent3, UINT32_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent4, UINT32_MAX));

  // Verfy A matches B
  EXPECT_EQ(0,
            memcmp(input.raw_data(), output.raw_data(), input.size_in_bytes()));

  lzt::synchronize(cmdqueue, UINT32_MAX);
  // cleanup
  ep.destroy_event(hEvent1);
  ep.destroy_event(hEvent2);
  ep.destroy_event(hEvent3);
  ep.destroy_event(hEvent4);
  lzt::destroy_ze_image(input_xeimage);
  lzt::destroy_ze_image(output_xeimage);
}

// Other Test Variants:
// ImageCopy
// ImageCopyRegion
// ImageCopyToMemory
// ImageCopyFromMemory

} // namespace
