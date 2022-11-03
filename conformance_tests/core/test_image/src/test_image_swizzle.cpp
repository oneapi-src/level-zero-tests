/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <limits>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

namespace {

class zeCommandListAppendImageCopyWithSwizzleTests : public ::testing::Test {
protected:
  zeCommandListAppendImageCopyWithSwizzleTests() {
    command_list = lzt::create_command_list();
    command_queue = lzt::create_command_queue();
    module = lzt::create_module(lzt::zeDevice::get_instance()->get_device(),
                                "image_swizzle_tests.spv");
  }

  ~zeCommandListAppendImageCopyWithSwizzleTests() {
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
    lzt::destroy_module(module);
  }

  ze_image_handle_t create_image_desc_format(ze_image_format_type_t format_type,
                                             bool layout32);
  void run_test(void *inbuff, void *outbuff, std::string kernel_name);

  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue;
  ze_image_handle_t img_in, img_out;
  ze_module_handle_t module;
  const int image_height = 32;
  const int image_width = 32;
  const int image_size = image_height * image_width;
};

void zeCommandListAppendImageCopyWithSwizzleTests::run_test(
    void *inbuff, void *outbuff, std::string kernel_name) {

  uint32_t group_size_x, group_size_y, group_size_z;
  ze_kernel_handle_t kernel = lzt::create_function(module, kernel_name);
  lzt::append_image_copy_from_mem(command_list, img_in, inbuff, nullptr);
  lzt::append_barrier(command_list, nullptr, 0, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSuggestGroupSize(kernel, image_width, image_height, 1,
                                     &group_size_x, &group_size_y,
                                     &group_size_z));
  lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSetArgumentValue(kernel, 0, sizeof(img_in), &img_in));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSetArgumentValue(kernel, 1, sizeof(img_out), &img_out));

  ze_group_count_t group_dems = {image_width / group_size_x,
                                 image_height / group_size_y, 1};
  lzt::append_launch_function(command_list, kernel, &group_dems, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list, nullptr, 0, nullptr);
  lzt::append_image_copy_to_mem(command_list, outbuff, img_out, nullptr);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);
  lzt::destroy_function(kernel);
}

ze_image_handle_t
zeCommandListAppendImageCopyWithSwizzleTests::create_image_desc_format(
    ze_image_format_type_t format_type, bool layout32) {
  ze_image_desc_t image_desc = {};
  image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  ze_image_handle_t image;

  image_desc.pNext = nullptr;
  image_desc.flags = (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
  image_desc.type = ZE_IMAGE_TYPE_2D;
  if (layout32)
    image_desc.format.layout = ZE_IMAGE_FORMAT_LAYOUT_32;
  else
    image_desc.format.layout = ZE_IMAGE_FORMAT_LAYOUT_16;

  image_desc.format.type = format_type;
  image_desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_0;
  image_desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_0;
  image_desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_1;
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

TEST_F(
    zeCommandListAppendImageCopyWithSwizzleTests,
    GivenDeviceImageAndHostImagesWithDifferentSwizzleWhenLaunchingCopyFromKernelThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    return;
  }
  ze_image_desc_t image_descriptor_source = {};
  image_descriptor_source.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_descriptor_source.type = ZE_IMAGE_TYPE_2D;
  image_descriptor_source.flags =
      (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
  image_descriptor_source.format.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
  image_descriptor_source.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_descriptor_source.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_descriptor_source.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_descriptor_source.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_descriptor_source.height = image_height;
  image_descriptor_source.width = image_width;
  image_descriptor_source.depth = 1;
  image_descriptor_source.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  img_in = lzt::create_ze_image(image_descriptor_source);

  ze_image_desc_t image_descriptor_dest = {};
  image_descriptor_dest.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_descriptor_dest.type = ZE_IMAGE_TYPE_2D;
  image_descriptor_dest.flags =
      (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
  image_descriptor_dest.format.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
  image_descriptor_dest.format.x = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_descriptor_dest.format.y = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_descriptor_dest.format.z = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_descriptor_dest.format.w = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_descriptor_dest.height = image_height;
  image_descriptor_dest.width = image_width;
  image_descriptor_dest.depth = 1;
  image_descriptor_dest.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  img_out = lzt::create_ze_image(image_descriptor_dest);

  uint32_t *inbuff =
      (uint32_t *)lzt::allocate_host_memory(image_size * sizeof(uint32_t));
  uint32_t *outbuff =
      (uint32_t *)lzt::allocate_host_memory(image_size * sizeof(uint32_t));

  for (int i = 0; i < image_size; i++) {
    inbuff[i] = 0x12345678;
  }

  run_test(inbuff, outbuff, "image_swizzle_test");

  for (int i = 0; i < image_size; i++) {
    EXPECT_EQ(outbuff[i], 0x78563412); // After swizzle from RGBA to AGBR the
                                       // data format will be reversed from 12
                                       // 34 56 78 -> 78 56 34 12
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

} // namespace
