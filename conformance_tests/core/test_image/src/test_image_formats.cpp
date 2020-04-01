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

class ImageFormatTests : public ::testing::Test {
protected:
  ImageFormatTests() {
    command_list = lzt::create_command_list();
    command_queue = lzt::create_command_queue();
    module = lzt::create_module(lzt::zeDevice::get_instance()->get_device(),
                                "image_formats_tests.spv");
  }

  ~ImageFormatTests() {
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

void ImageFormatTests::run_test(void *inbuff, void *outbuff,
                                std::string kernel_name) {

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
  lzt::append_barrier(command_list, nullptr, 0, nullptr);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT32_MAX);
  lzt::destroy_function(kernel);
}

ze_image_handle_t
ImageFormatTests::create_image_desc_format(ze_image_format_type_t format_type,
                                           bool layout32) {
  ze_image_desc_t image_desc;
  ze_image_handle_t image;
  image_desc.version = ZE_IMAGE_DESC_VERSION_CURRENT;
  image_desc.flags = (ze_image_flag_t)(ZE_IMAGE_FLAG_PROGRAM_WRITE |
                                       ZE_IMAGE_FLAG_PROGRAM_READ |
                                       ZE_IMAGE_FLAG_BIAS_UNCACHED);
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
            zeImageCreate(lzt::zeDevice::get_instance()->get_device(),
                          &image_desc, &image));
  EXPECT_NE(nullptr, image);

  return image;
}

TEST_F(ImageFormatTests,
       GivenImageFormatUINTWhenCopyingImageThenFormatIsCorrect) {
  img_in = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_UINT, true);
  img_out = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_UINT, true);
  uint32_t *inbuff =
      (uint32_t *)lzt::allocate_host_memory(image_size * sizeof(uint32_t));
  uint32_t *outbuff =
      (uint32_t *)lzt::allocate_host_memory(image_size * sizeof(uint32_t));

  for (int i = 0; i < image_size; i++) {
    inbuff[i] = 1;
  }

  run_test(inbuff, outbuff, "image_format_uint");

  for (int i = 0; i < image_size; i++) {
    EXPECT_EQ(outbuff[i], 2);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatINTWhenCopyingImageThenFormatIsCorrect) {
  img_in = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_SINT, true);
  img_out = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_SINT, true);
  int32_t *inbuff =
      (int32_t *)lzt::allocate_host_memory(image_size * sizeof(int32_t));
  int32_t *outbuff =
      (int32_t *)lzt::allocate_host_memory(image_size * sizeof(int32_t));

  for (int i = 0; i < image_size; i++) {
    inbuff[i] = -1;
  }

  run_test(inbuff, outbuff, "image_format_int");

  for (int i = 0; i < image_size; i++) {
    EXPECT_EQ(outbuff[i], -2);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatFloatWhenCopyingImageThenFormatIsCorrect) {
  img_in = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_FLOAT, true);
  img_out = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_FLOAT, true);
  float *inbuff =
      (float *)lzt::allocate_host_memory(image_size * sizeof(float));
  float *outbuff =
      (float *)lzt::allocate_host_memory(image_size * sizeof(float));

  for (int i = 0; i < image_size; i++) {
    inbuff[i] = 3.5;
  }

  run_test(inbuff, outbuff, "image_format_float");

  for (int i = 0; i < image_size; i++) {
    EXPECT_LT(outbuff[i], 3.5);
    EXPECT_GT(outbuff[i], 3.0);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatUNORMWhenCopyingImageThenFormatIsCorrect) {
  img_in = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_UNORM, false);
  img_out = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_UNORM, false);
  uint16_t *inbuff =
      (uint16_t *)lzt::allocate_host_memory(image_size * sizeof(uint16_t));
  uint16_t *outbuff =
      (uint16_t *)lzt::allocate_host_memory(image_size * sizeof(uint16_t));

  for (int i = 0; i < image_size; i++) {
    inbuff[i] = 0;
  }

  run_test(inbuff, outbuff, "image_format_unorm");

  for (int i = 0; i < image_size; i++) {
    EXPECT_EQ(outbuff[i], 65535);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatSNORMWhenCopyingImageThenFormatIsCorrect) {
  img_in = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_SNORM, false);
  img_out = create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_SNORM, false);
  int16_t *inbuff =
      (int16_t *)lzt::allocate_host_memory(image_size * sizeof(int16_t));
  int16_t *outbuff =
      (int16_t *)lzt::allocate_host_memory(image_size * sizeof(int16_t));

  for (int i = 0; i < image_size; i++) {
    inbuff[i] = -32768;
  }

  run_test(inbuff, outbuff, "image_format_snorm");

  for (int i = 0; i < image_size; i++) {
    EXPECT_EQ(outbuff[i], 32767);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

} // namespace
