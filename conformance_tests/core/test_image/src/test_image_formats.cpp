/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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

class ImageFormatTests : public ::testing::Test {
public:
  ze_image_handle_t create_image_desc_format(ze_image_format_type_t format_type,
                                             bool layout32);
  void run_test(void *inbuff, void *outbuff, std::string kernel_name,
                ze_device_handle_t device, bool is_immediate);

  lzt::zeCommandBundle cmd_bundle;
  ze_image_handle_t img_in, img_out;
  const int image_height = 32;
  const int image_width = 32;
  const int image_size = image_height * image_width;
};

void ImageFormatTests::run_test(void *inbuff, void *outbuff,
                                std::string kernel_name,
                                ze_device_handle_t device, bool is_immediate) {

  uint32_t group_size_x, group_size_y, group_size_z;
  cmd_bundle = lzt::create_command_bundle(device, is_immediate);
  ze_module_handle_t module =
      lzt::create_module(device, "image_formats_tests.spv");
  ze_kernel_handle_t kernel = lzt::create_function(module, kernel_name);
  lzt::append_image_copy_from_mem(cmd_bundle.list, img_in, inbuff, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
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
  lzt::append_launch_function(cmd_bundle.list, kernel, &group_dems, nullptr, 0,
                              nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::append_image_copy_to_mem(cmd_bundle.list, outbuff, img_out, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_bundle(cmd_bundle);
}

ze_image_handle_t
ImageFormatTests::create_image_desc_format(ze_image_format_type_t format_type,
                                           bool layout32) {
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

void RunGivenImageFormatUINTWhenCopyingImageTest(ImageFormatTests &test,
                                                 bool is_immediate) {
  test.img_in = test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_UINT, true);
  test.img_out = test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_UINT, true);
  uint32_t *inbuff =
      (uint32_t *)lzt::allocate_host_memory(test.image_size * sizeof(uint32_t));
  uint32_t *outbuff =
      (uint32_t *)lzt::allocate_host_memory(test.image_size * sizeof(uint32_t));

  for (int i = 0; i < test.image_size; i++) {
    inbuff[i] = 1;
  }

  test.run_test(inbuff, outbuff, "image_format_uint",
                lzt::get_default_device(lzt::get_default_driver()),
                is_immediate);

  for (int i = 0; i < test.image_size; i++) {
    EXPECT_EQ(outbuff[i], 2);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatUINTWhenCopyingImageThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatUINTWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatUINTWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatUINTWhenCopyingImageTest(*this, true);
}

void RunGivenImageFormatINTWhenCopyingImageTest(ImageFormatTests &test,
                                                bool is_immediate) {
  test.img_in = test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_SINT, true);
  test.img_out = test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_SINT, true);
  int32_t *inbuff =
      (int32_t *)lzt::allocate_host_memory(test.image_size * sizeof(int32_t));
  int32_t *outbuff =
      (int32_t *)lzt::allocate_host_memory(test.image_size * sizeof(int32_t));

  for (int i = 0; i < test.image_size; i++) {
    inbuff[i] = -1;
  }

  test.run_test(inbuff, outbuff, "image_format_int",
                lzt::get_default_device(lzt::get_default_driver()),
                is_immediate);

  for (int i = 0; i < test.image_size; i++) {
    EXPECT_EQ(outbuff[i], -2);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatINTWhenCopyingImageThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatINTWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatINTWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatINTWhenCopyingImageTest(*this, true);
}

void RunGivenImageFormatFloatWhenCopyingImageTest(ImageFormatTests &test,
                                                  bool is_immediate) {
  test.img_in = test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_FLOAT, true);
  test.img_out =
      test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_FLOAT, true);
  float *inbuff =
      (float *)lzt::allocate_host_memory(test.image_size * sizeof(float));
  float *outbuff =
      (float *)lzt::allocate_host_memory(test.image_size * sizeof(float));

  for (int i = 0; i < test.image_size; i++) {
    inbuff[i] = 3.5;
  }

  test.run_test(inbuff, outbuff, "image_format_float",
                lzt::get_default_device(lzt::get_default_driver()),
                is_immediate);

  for (int i = 0; i < test.image_size; i++) {
    EXPECT_LT(outbuff[i], 3.5);
    EXPECT_GT(outbuff[i], 3.0);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatFloatWhenCopyingImageThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatFloatWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatFloatWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatFloatWhenCopyingImageTest(*this, true);
}

void RunGivenImageFormatUNORMWhenCopyingImageTest(ImageFormatTests &test,
                                                  bool is_immediate) {
  test.img_in =
      test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_UNORM, false);
  test.img_out =
      test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_UNORM, false);
  uint16_t *inbuff =
      (uint16_t *)lzt::allocate_host_memory(test.image_size * sizeof(uint16_t));
  uint16_t *outbuff =
      (uint16_t *)lzt::allocate_host_memory(test.image_size * sizeof(uint16_t));

  for (int i = 0; i < test.image_size; i++) {
    inbuff[i] = 0;
  }

  test.run_test(inbuff, outbuff, "image_format_unorm",
                lzt::get_default_device(lzt::get_default_driver()),
                is_immediate);

  for (int i = 0; i < test.image_size; i++) {
    EXPECT_EQ(outbuff[i], 65535);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatUNORMWhenCopyingImageThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatUNORMWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatUNORMWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatUNORMWhenCopyingImageTest(*this, true);
}

void RunGivenImageFormatSNORMWhenCopyingImageTest(ImageFormatTests &test,
                                                  bool is_immediate) {
  test.img_in =
      test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_SNORM, false);
  test.img_out =
      test.create_image_desc_format(ZE_IMAGE_FORMAT_TYPE_SNORM, false);
  int16_t *inbuff =
      (int16_t *)lzt::allocate_host_memory(test.image_size * sizeof(int16_t));
  int16_t *outbuff =
      (int16_t *)lzt::allocate_host_memory(test.image_size * sizeof(int16_t));

  for (int i = 0; i < test.image_size; i++) {
    inbuff[i] = -32768;
  }

  test.run_test(inbuff, outbuff, "image_format_snorm",
                lzt::get_default_device(lzt::get_default_driver()),
                is_immediate);

  for (int i = 0; i < test.image_size; i++) {
    EXPECT_EQ(outbuff[i], 32767);
  }

  lzt::free_memory(outbuff);
  lzt::free_memory(inbuff);
}

TEST_F(ImageFormatTests,
       GivenImageFormatSNORMWhenCopyingImageThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatSNORMWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatSNORMWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  RunGivenImageFormatSNORMWhenCopyingImageTest(*this, true);
}

class ImageFormatLayoutTests : public ImageFormatTests,
                               public ::testing::WithParamInterface<
                                   std::tuple<ze_image_format_layout_t, bool>> {
public:
  template <typename T>
  void set_up_buffers(T *&inbuff, T *&outbuff, int size_multiplier = 1);
  template <typename T>
  void verify_buffer(T *&buff, int size_multiplier = 1, bool saturates = true);
  template <typename T>
  void verify_buffer_float(T *&buff, bool saturates = true);
};

template <typename T>
void ImageFormatLayoutTests::set_up_buffers(T *&inbuff, T *&outbuff,
                                            int size_multiplier) {
  inbuff = static_cast<T *>(
      lzt::allocate_host_memory(image_size * sizeof(T) * size_multiplier));
  outbuff = static_cast<T *>(
      lzt::allocate_host_memory(image_size * sizeof(T) * size_multiplier));
  for (int i = 0; i < (image_size * size_multiplier); i++) {
    // set pixel value to 1 less than max for data type so
    // that when the kernel increments it, it saturates
    // or rolls over
    inbuff[i] = ~(0 & inbuff[i]) - 1;
  }
}
template <typename T>
void ImageFormatLayoutTests::verify_buffer(T *&buff, int size_multiplier,
                                           bool saturates) {
  auto max_val = std::numeric_limits<T>::max();
  for (int i = 0; i < (image_size * size_multiplier); i++) {
    if (saturates) {
      ASSERT_EQ(buff[i], max_val);
    } else if (max_val == std::numeric_limits<uint64_t>::max()) {
      // We use 64 bit buffers for the multi-32 bit component types, so
      // adjust the expected value
      ASSERT_EQ(buff[i], std::numeric_limits<uint32_t>::max() + (uint64_t)1);
    } else {
      ASSERT_EQ(buff[i], 0);
    }
  }
}

template <typename T>
void ImageFormatLayoutTests::verify_buffer_float(T *&buff, bool saturates) {
  for (int i = 0; i < image_size; i++) {
    if (saturates) {
      ASSERT_EQ(buff[i], std::numeric_limits<T>::max());
    } else {
      ASSERT_GT(buff[i], 0);
      ASSERT_LT(buff[i], std::numeric_limits<T>::max());
    }
  }
}

TEST_P(ImageFormatLayoutTests,
       GivenImageFormatLayoutWhenCopyingImageThenFormatIsCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  auto driver = lzt::get_default_driver();
  for (auto device : lzt::get_devices(driver)) {

    auto device_image_properties = lzt::get_image_properties(device);
    if (!device_image_properties.maxReadImageArgs ||
        !device_image_properties.maxWriteImageArgs) {
      LOG_WARNING << "Device does not support images";
      continue;
    }

    auto layout = std::get<0>(GetParam());
    auto is_immediate = std::get<1>(GetParam());
    ze_image_desc_t image_descriptor = {};
    image_descriptor.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
    image_descriptor.type = ZE_IMAGE_TYPE_2D;
    image_descriptor.flags =
        (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
    image_descriptor.format.layout = layout;
    image_descriptor.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
    image_descriptor.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
    image_descriptor.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
    image_descriptor.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
    image_descriptor.height = image_height;
    image_descriptor.width = image_width;
    image_descriptor.depth = 1;

    switch (layout) {
    case ZE_IMAGE_FORMAT_LAYOUT_8: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint8_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_one_component", device,
               is_immediate);
      verify_buffer(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_16: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint16_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_one_component", device,
               is_immediate);
      verify_buffer(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    }

    break;
    case ZE_IMAGE_FORMAT_LAYOUT_32: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint32_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_one_component", device,
               is_immediate);
      verify_buffer(outbuff, 1, false);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);

    } break;

    case ZE_IMAGE_FORMAT_LAYOUT_8_8: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint16_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_two_components", device,
               is_immediate);
      verify_buffer(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_16_16: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint32_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_two_components", device,
               is_immediate);
      verify_buffer(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_32_32: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint64_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_two_components", device,
               is_immediate);
      verify_buffer(outbuff, 1, false);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;

    case ZE_IMAGE_FORMAT_LAYOUT_5_6_5: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UNORM;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint16_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_three_components_unorm",
               device, is_immediate);
      verify_buffer_float(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_11_11_10: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_FLOAT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint32_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_three_components_float",
               device, is_immediate);
      verify_buffer_float(outbuff, false);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;

    case ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UNORM;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint16_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_four_components_unorm",
               device, is_immediate);
      verify_buffer_float(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UNORM;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint16_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_four_components_unorm",
               device, is_immediate);
      verify_buffer_float(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint32_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_four_components", device,
               is_immediate);
      verify_buffer(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint32_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_four_components", device,
               is_immediate);
      verify_buffer(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint64_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff);
      run_test(inbuff, outbuff, "image_format_layout_four_components", device,
               is_immediate);
      verify_buffer(outbuff);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32: {
      image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      img_in = lzt::create_ze_image(image_descriptor);
      img_out = lzt::create_ze_image(image_descriptor);
      uint64_t *inbuff, *outbuff;
      set_up_buffers(inbuff, outbuff, 2);
      run_test(inbuff, outbuff, "image_format_layout_four_components", device,
               is_immediate);
      verify_buffer(outbuff, 2, false);
      lzt::free_memory(outbuff);
      lzt::free_memory(inbuff);
    } break;
    default:
      LOG_INFO << "Unhandled image format layout: " << layout;
      break;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    LayoutParameterization, ImageFormatLayoutTests,
    ::testing::Combine(
        ::testing::Values(
            ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_16,
            ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_LAYOUT_8_8,
            ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_LAYOUT_16_16,
            ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,

            ZE_IMAGE_FORMAT_LAYOUT_32_32, ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32,

            ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,

            ZE_IMAGE_FORMAT_LAYOUT_11_11_10,

            ZE_IMAGE_FORMAT_LAYOUT_5_6_5, ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1,
            ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4),
        ::testing::Bool()));

} // namespace
