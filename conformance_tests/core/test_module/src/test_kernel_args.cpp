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

class KernelArgumentTests : public ::testing::Test {

public:
  KernelArgumentTests() {
    device_ = lzt::zeDevice::get_instance()->get_device();
    module_ = lzt::create_module(device_, "multi_argument_kernel_test.spv");
    if (lzt::image_support()) {
      image_module_ =
          lzt::create_module(device_, "multi_image_argument_kernel_test.spv");
    }
    cmd_list_ = lzt::create_command_list(device_);
    cmd_queue_ = lzt::create_command_queue();
  }

  ~KernelArgumentTests() {
    lzt::destroy_module(module_);
    if (lzt::image_support()) {
      lzt::destroy_module(image_module_);
    }
    lzt::destroy_command_list(cmd_list_);
    lzt::destroy_command_queue(cmd_queue_);
  }

protected:
  void set_image_pixel(ze_image_handle_t image, int x, int y, uint32_t val);
  uint32_t get_image_pixel(ze_image_handle_t image, int x, int y);

  ze_device_handle_t device_;
  ze_module_handle_t module_;
  ze_module_handle_t image_module_;
  ze_command_list_handle_t cmd_list_;
  ze_command_queue_handle_t cmd_queue_;

  const int img_height = 20;
  const int img_width = 20;
};

static ze_image_handle_t create_2d_uint_test_image(int width, int height) {
  ze_image_desc_t image_description = {};
  image_description.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_description.format.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;

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

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(lzt::get_default_context(),
                          lzt::zeDevice::get_instance()->get_device(),
                          &image_description, &image));

  return image;
}

void KernelArgumentTests::set_image_pixel(ze_image_handle_t image, int x, int y,
                                          uint32_t val) {

  lzt::ImagePNG32Bit temp_png(img_height, img_width);
  temp_png.set_pixel(x, y, val);
  lzt::append_image_copy_from_mem(cmd_list_, image, temp_png.raw_data(),
                                  nullptr);
  lzt::close_command_list(cmd_list_);
  lzt::execute_command_lists(cmd_queue_, 1, &cmd_list_, nullptr);
  lzt::synchronize(cmd_queue_, UINT64_MAX);
  lzt::reset_command_list(cmd_list_);
  return;
}

uint32_t KernelArgumentTests::get_image_pixel(ze_image_handle_t image, int x,
                                              int y) {

  lzt::ImagePNG32Bit temp_png(img_height, img_width);
  lzt::append_image_copy_to_mem(cmd_list_, temp_png.raw_data(), image, nullptr);
  lzt::close_command_list(cmd_list_);
  lzt::execute_command_lists(cmd_queue_, 1, &cmd_list_, nullptr);
  lzt::synchronize(cmd_queue_, UINT64_MAX);
  lzt::reset_command_list(cmd_list_);
  return temp_png.get_pixel(x, y);
}

TEST_F(KernelArgumentTests,
       GivenSeveralBuffersWhenPassingToKernelThenCorrectResultIsReturned) {
  std::string kernel_name = "many_buffers";
  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;

  const int num_bufs = 5;
  void *buffers[num_bufs];

  for (int i = 0; i < num_bufs; i++) {
    buffers[i] = lzt::allocate_host_memory(sizeof(int));
    arg.arg_size = sizeof(buffers[i]);
    arg.arg_value = &buffers[i];
    args.push_back(arg);
  }
  // Kernel should set buffers to values 1,2,3,4,5.
  lzt::create_and_execute_function(device_, module_, kernel_name, 1, args);

  for (int i = 0; i < num_bufs; i++) {
    int data = *static_cast<int *>(buffers[i]);
    EXPECT_EQ(data, i + 1);
    lzt::free_memory(buffers[i]);
  }
}

TEST_F(KernelArgumentTests,
       GivenSeveral2DImagesWhenPassingToKernelThenCorrectResultIsReturned) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  std::string kernel_name = "many_2d_images";
  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;
  const int num_images = 5;
  ze_image_handle_t images[num_images];

  for (int i = 0; i < num_images; i++) {
    images[i] = create_2d_uint_test_image(img_width, img_height);
    // set pixel to test value;
    set_image_pixel(images[i], i + 1, i + 1, i + 1);
    arg.arg_size = sizeof(images[i]);
    arg.arg_value = &images[i];
    args.push_back(arg);
  }

  // For each image, pixel value at coord ([imagenum],[imagenum])
  // will be written to coord ([imagenum+10],[imagenum+10])
  lzt::create_and_execute_function(device_, image_module_, kernel_name, 1,
                                   args);

  for (int i = 0; i < num_images; i++) {
    uint32_t pixel = get_image_pixel(images[i], i + 1, i + 1);
    EXPECT_EQ(pixel, i + 1);
    pixel = get_image_pixel(images[i], i + 11, i + 11);
    EXPECT_EQ(pixel, i + 1);
    lzt::destroy_ze_image(images[i]);
  }
}

bool sampler_support() {
  ze_sampler_desc_t descriptor = {};
  descriptor.stype = ZE_STRUCTURE_TYPE_SAMPLER_DESC;
  ze_sampler_handle_t sampler = nullptr;
  if (zeSamplerCreate(lzt::get_default_context(),
                      lzt::zeDevice::get_instance()->get_device(), &descriptor,
                      &sampler) == ZE_RESULT_ERROR_UNINITIALIZED) {
    return false;
  } else {
    return true;
  }
}

TEST_F(KernelArgumentTests,
       GivenSeveralSamplersWhenPassingToKernelThenSuccessIsReturned) {
  if (!(sampler_support())) {
    LOG_INFO << "device does not support sampler, cannot run test";
    GTEST_SKIP();
  }
  std::string kernel_name = "many_samplers";
  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;
  const int num_samplers = 5;
  ze_sampler_handle_t samplers[num_samplers];

  for (int i = 0; i < num_samplers; i++) {
    samplers[i] = lzt::create_sampler(ZE_SAMPLER_ADDRESS_MODE_NONE,
                                      ZE_SAMPLER_FILTER_MODE_LINEAR, true);
    arg.arg_size = sizeof(samplers[i]);
    arg.arg_value = &samplers[i];
    args.push_back(arg);
  }

  // sampler kernel is a noop, nothing to check
  lzt::create_and_execute_function(device_, image_module_, kernel_name, 1,
                                   args);

  for (int i = 0; i < num_samplers; i++) {
    lzt::destroy_sampler(samplers[i]);
  }
}

TEST_F(
    KernelArgumentTests,
    GivenManyArgsOfAllTypesIncludingImageWhenPassingToKernelCorrectResultIsReturned) {
  /*
  Kernel used for this test expects the following args:
  global int *buf1, global int *buf2, local int *local_buf, global int *buf3,
  global int *buf4, image2d_t image1, image2d_t image2, image2d_t image3,
  image2d_t image4, image2d_t image5, sampler_t samp1, sampler_t samp2
  */
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  if (!(sampler_support())) {
    LOG_INFO << "device does not support sampler, cannot run test";
    GTEST_SKIP();
  }
  std::string kernel_name = "many_args_all_types";
  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;
  const int num_global_bufs = 4;
  void *buffers[num_global_bufs];

  for (int i = 0; i < num_global_bufs; i++) {
    buffers[i] = lzt::allocate_host_memory(sizeof(int));
  }
  memset(buffers[0], 0x22, sizeof(int));
  memset(buffers[1], 0x33, sizeof(int));
  memset(buffers[2], 0x00, sizeof(int));
  memset(buffers[3], 0x00, sizeof(int));

  // Add 2 global buffers used for input
  arg.arg_size = sizeof(buffers[0]);
  arg.arg_value = &buffers[0];
  args.push_back(arg);
  arg.arg_size = sizeof(buffers[1]);
  arg.arg_value = &buffers[1];
  args.push_back(arg);

  // add local buff that will be used for copying
  arg.arg_size = sizeof(int);
  arg.arg_value = nullptr;
  args.push_back(arg);

  // Add 2 global buffers used for output
  arg.arg_size = sizeof(buffers[2]);
  arg.arg_value = &buffers[2];
  args.push_back(arg);
  arg.arg_size = sizeof(buffers[3]);
  arg.arg_value = &buffers[3];
  args.push_back(arg);

  // Add 5 images
  const int num_images = 5;
  ze_image_handle_t images[num_images];

  for (int i = 0; i < num_images; i++) {
    images[i] = create_2d_uint_test_image(img_width, img_height);
    // set pixel to test value;
    set_image_pixel(images[i], i + 1, i + 1, i + 1);
    arg.arg_size = sizeof(images[i]);
    arg.arg_value = &images[i];
    args.push_back(arg);
  }

  // Add 2 samplers
  const int num_samplers = 2;
  ze_sampler_handle_t samplers[num_samplers];
  for (int i = 0; i < num_samplers; i++) {
    samplers[i] = lzt::create_sampler(ZE_SAMPLER_ADDRESS_MODE_NONE,
                                      ZE_SAMPLER_FILTER_MODE_LINEAR, true);
    arg.arg_size = sizeof(samplers[i]);
    arg.arg_value = &samplers[i];
    args.push_back(arg);
  }

  // Kernel will perform the following operations:
  // buffers[0] copied to buffers[2] using local mem as staging area
  // buffers[1] copied to buffers[3] using local mem as staging area
  // For each image, pixel value at coord ([imagenum],[imagenum])
  //   will be written to coord ([imagenum+10],[imagenum+10]) using sampler
  lzt::create_and_execute_function(device_, image_module_, kernel_name, 1,
                                   args);

  EXPECT_EQ(*static_cast<int *>(buffers[0]), *static_cast<int *>(buffers[2]));
  EXPECT_EQ(*static_cast<int *>(buffers[1]), *static_cast<int *>(buffers[3]));
  for (int i = 0; i < num_global_bufs; i++) {
    lzt::free_memory(buffers[i]);
  }
  for (int i = 0; i < num_samplers; i++) {
    lzt::destroy_sampler(samplers[i]);
  }
  for (int i = 0; i < num_images; i++) {
    uint32_t pixel = get_image_pixel(images[i], i + 1, i + 1);
    EXPECT_EQ(pixel, i + 1);
    pixel = get_image_pixel(images[i], i + 11, i + 11);
    EXPECT_EQ(pixel, i + 1);
    lzt::destroy_ze_image(images[i]);
  }
}

TEST_F(KernelArgumentTests,
       GivenManyLocalArgsWhenPassingToKernelCorrectResultIsReturned) {
  std::string kernel_name = "many_locals";
  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;
  const int num_locals = 4;
  const int local_size = 256 * sizeof(uint8_t);
  auto buff = lzt::allocate_host_memory(local_size);

  for (int i = 0; i < num_locals; i++) {
    arg.arg_size = local_size;
    arg.arg_value = nullptr;
    args.push_back(arg);
  }

  arg.arg_size = sizeof(buff);
  arg.arg_value = &buff;
  args.push_back(arg);

  // Kernel should set global buffer to value 0x55
  lzt::create_and_execute_function(device_, module_, kernel_name, 1, args);

  // Kernel should set all bytes to 0x55
  uint8_t *data = static_cast<uint8_t *>(buff);
  for (int i = 0; i < local_size; i++) {
    ASSERT_EQ(data[i], 0x55);
  }
  lzt::free_memory(buff);
}

} // namespace
