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

namespace {

class ImageMediaLayoutTests : public ::testing::Test {
protected:
  ImageMediaLayoutTests() {
    command_list = lzt::create_command_list();
    command_queue = lzt::create_command_queue();
    module = lzt::create_module(lzt::zeDevice::get_instance()->get_device(),
                                "image_media_layouts_tests.spv");
    ze_event_pool_desc_t ep_desc = {};
    ep_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    ep_desc.flags = 0;
    ep_desc.count = 10;
    ep = lzt::create_event_pool(ep_desc);
  }

  ~ImageMediaLayoutTests() {
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
    lzt::destroy_event_pool(ep);
    lzt::destroy_module(module);
  }

  ze_image_handle_t create_image_desc_layout(ze_image_format_layout_t layout);
  void run_test(void *inbuff, void *outbuff, std::string kernel_name);
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue;
  ze_module_handle_t module;
  const int image_height = 2;
  const int image_width = 1;
  const int image_size = image_height * image_width;
  ze_event_pool_handle_t ep;
};

ze_image_handle_t ImageMediaLayoutTests::create_image_desc_layout(
    ze_image_format_layout_t layout) {
  ze_image_desc_t image_desc = {};
  image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  ze_image_handle_t image;

  image_desc.pNext = nullptr;
  image_desc.flags = ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED;
  image_desc.type = ZE_IMAGE_TYPE_2D;
  image_desc.format.layout = layout;

  image_desc.format.type = ZE_IMAGE_FORMAT_TYPE_UNORM;
  image_desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_desc.width = image_width;
  image_desc.height = image_height;
  image_desc.depth = 1;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(lzt::get_default_context(),
                          lzt::zeDevice::get_instance()->get_device(),
                          &image_desc, &image));
  EXPECT_NE(nullptr, image);

  return image;
}

TEST_F(ImageMediaLayoutTests,
       GivenImageLayoutRBGWhenConvertingImageToNV12ThenResultIsCorrect) {

  std::string kernel_name = "copy_image";
  ze_kernel_handle_t kernel = lzt::create_function(module, kernel_name);
  size_t buffer_size = image_width * image_size * sizeof(uint32_t);
  auto buffer_in = lzt::allocate_host_memory(buffer_size);
  auto buffer_out = lzt::allocate_host_memory(buffer_size);
  auto image_rgb_in = create_image_desc_layout(ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8);
  auto image_rgb_out = create_image_desc_layout(ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8);
  auto image_n12 = create_image_desc_layout(ZE_IMAGE_FORMAT_LAYOUT_NV12);

  // populate input buffer
  memset(buffer_in, 0x22, buffer_size);

  // copy input buff to image_grb_in
  lzt::append_image_copy_from_mem(command_list, image_rgb_in, buffer_in,
                                  nullptr);
  lzt::append_barrier(command_list);

  // call kernel to copy image_rgb_in -> image_n12
  uint32_t group_size_x, group_size_y, group_size_z;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSuggestGroupSize(kernel, image_width, image_height, 1,
                                     &group_size_x, &group_size_y,
                                     &group_size_z));
  lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeKernelSetArgumentValue(kernel, 0, sizeof(image_rgb_in), &image_rgb_in));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSetArgumentValue(kernel, 1, sizeof(image_n12), &image_n12));

  ze_group_count_t group_dems = {image_width / group_size_x,
                                 image_height / group_size_y, 1};
  lzt::append_launch_function(command_list, kernel, &group_dems, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list);

  // call kernel to copy image_n12 -> image_rgb_out
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSuggestGroupSize(kernel, image_width, image_height, 1,
                                     &group_size_x, &group_size_y,
                                     &group_size_z));
  lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSetArgumentValue(kernel, 0, sizeof(image_n12), &image_n12));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSetArgumentValue(kernel, 1, sizeof(image_rgb_out),
                                     &image_rgb_out));

  group_dems = {image_width / group_size_x, image_height / group_size_y, 1};
  lzt::append_launch_function(command_list, kernel, &group_dems, nullptr, 0,
                              nullptr);

  // finalize, copy to buffer_out
  lzt::append_barrier(command_list);
  lzt::append_image_copy_to_mem(command_list, buffer_out, image_rgb_out,
                                nullptr);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT32_MAX);

  // compare results
  EXPECT_EQ(memcmp(buffer_in, buffer_out, buffer_size), 0);
}

} // namespace