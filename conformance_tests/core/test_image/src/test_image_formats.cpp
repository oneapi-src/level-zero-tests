/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <limits>
#include <algorithm>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

namespace {

class ImageFormatTests : public ::testing::Test {
protected:
  void SetUp() override {
    if (!(lzt::image_support())) {
      LOG_INFO << "device does not support images, cannot run test";
      GTEST_SKIP();
    }
    tested_device = lzt::zeDevice::get_instance()->get_device();
    tested_image_types = lzt::get_supported_image_types(
        tested_device, skip_array_type, skip_buffer_type);
  }

  virtual void set_up_module() {
    module = lzt::create_module(tested_device, "image_formats_tests.spv");
  }

  void cleanup() {
    lzt::destroy_command_bundle(cmd_bundle);
    lzt::destroy_module(module);
    lzt::free_memory(outbuff);
    lzt::free_memory(inbuff);
  }

public:
  virtual ze_image_handle_t
  create_image_desc_format(ze_image_type_t image_type,
                           ze_image_format_type_t format_type,
                           ze_image_format_layout_t layout);

  void ImageFormatTests::run_test(void (*buffer_setup_f)(ImageFormatTests &),
                                  void (*buffer_verify_f)(ImageFormatTests &),
                                  ze_image_type_t image_type,
                                  ze_image_format_type_t format_type,
                                  ze_image_format_layout_t layout,
                                  bool is_immediate);

  virtual void get_kernel(ze_image_type_t image_type,
                          ze_image_format_type_t format_type,
                          ze_image_format_layout_t layout);

  template <typename T, T value = static_cast<T>(1)>
  static void setup_buffers(ImageFormatTests &test);

  static void setup_buffers_float(ImageFormatTests &test);

  template <typename T, T value = static_cast<T>(1)>
  static void verify_outbuffer(ImageFormatTests &test);

  static void verify_outbuffer_float(ImageFormatTests &test);

  static const bool skip_array_type = false;
  static const bool skip_buffer_type = true;

  lzt::zeCommandBundle cmd_bundle;
  ze_image_handle_t img_in, img_out;
  ze_device_handle_t tested_device = nullptr;
  ze_module_handle_t module;
  lzt::Dims image_dims;
  size_t image_size;
  std::vector<ze_image_type_t> tested_image_types;
  void *inbuff = nullptr, *outbuff = nullptr;
  std::string kernel_name;
};

template <typename T, T value>
void ImageFormatTests::setup_buffers(ImageFormatTests &test) {
  test.inbuff = lzt::allocate_host_memory(test.image_size * sizeof(T));
  test.outbuff = lzt::allocate_host_memory(test.image_size * sizeof(T));
  T *in_ptr = static_cast<T *>(test.inbuff);
  for (int i = 0; i < test.image_size; i++) {
    in_ptr[i] = value;
  }
}

void ImageFormatTests::setup_buffers_float(ImageFormatTests &test) {
  test.inbuff = lzt::allocate_host_memory(test.image_size * sizeof(float));
  test.outbuff = lzt::allocate_host_memory(test.image_size * sizeof(float));
  float *in_ptr = static_cast<float *>(test.inbuff);
  for (int i = 0; i < test.image_size; i++) {
    in_ptr[i] = 3.5;
  }
}

template <typename T, T value>
void ImageFormatTests::verify_outbuffer(ImageFormatTests &test) {
  T *out_ptr = static_cast<T *>(test.outbuff);
  for (int i = 0; i < test.image_size; i++) {
    EXPECT_EQ(out_ptr[i], value);
  }
}

void ImageFormatTests::verify_outbuffer_float(ImageFormatTests &test) {
  float *out_ptr = static_cast<float *>(test.outbuff);
  for (int i = 0; i < test.image_size; i++) {
    EXPECT_LT(out_ptr[i], 3.5);
    EXPECT_GT(out_ptr[i], 3.0);
  }
}

void ImageFormatTests::get_kernel(ze_image_type_t image_type,
                                  ze_image_format_type_t format_type,
                                  ze_image_format_layout_t layout) {
  kernel_name = "image_format";
  switch (format_type) {
  case ZE_IMAGE_FORMAT_TYPE_UINT:
    kernel_name += "_uint";
    break;
  case ZE_IMAGE_FORMAT_TYPE_SINT:
    kernel_name += "_sint";
    break;
  case ZE_IMAGE_FORMAT_TYPE_FLOAT:
    kernel_name += "_float";
    break;
  case ZE_IMAGE_FORMAT_TYPE_UNORM:
    kernel_name += "_unorm";
    break;
  case ZE_IMAGE_FORMAT_TYPE_SNORM:
    kernel_name += "_snorm";
    break;
  default:
    LOG_WARNING << "UNKNOWN_KERNEL";
    kernel_name = "UNKNOWN_KERNEL";
    return;
  }
  kernel_name += '_' + lzt::shortened_string(image_type);
}

void ImageFormatTests::run_test(void (*buffer_setup_f)(ImageFormatTests &),
                                void (*buffer_verify_f)(ImageFormatTests &),
                                ze_image_type_t image_type,
                                ze_image_format_type_t format_type,
                                ze_image_format_layout_t layout,
                                bool is_immediate) {
  uint32_t group_size_x, group_size_y, group_size_z;
  set_up_module();
  cmd_bundle = lzt::create_command_bundle(tested_device, is_immediate);
  get_kernel(image_type, format_type, layout);
  ze_kernel_handle_t kernel = lzt::create_function(module, kernel_name);

  image_dims = lzt::get_sample_image_dims(image_type);
  image_size = static_cast<size_t>(image_dims.width * image_dims.height *
                                   image_dims.depth);

  img_in = create_image_desc_format(image_type, format_type, layout);
  img_out = create_image_desc_format(image_type, format_type, layout);

  buffer_setup_f(*this);

  lzt::append_image_copy_from_mem(cmd_bundle.list, img_in, inbuff, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  auto result = zeKernelSuggestGroupSize(
      kernel, image_dims.width, image_dims.height, image_dims.depth,
      &group_size_x, &group_size_y, &group_size_z);
  if (lzt::check_unsupported(result)) {
    cleanup();
    return;
  }
  lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

  result = zeKernelSetArgumentValue(kernel, 0, sizeof(img_in), &img_in);
  if (lzt::check_unsupported(result)) {
    cleanup();
    return;
  }

  result = zeKernelSetArgumentValue(kernel, 1, sizeof(img_out), &img_out);
  if (lzt::check_unsupported(result)) {
    cleanup();
    return;
  }

  ze_group_count_t group_dems = {(image_dims.width / group_size_x),
                                 (image_dims.height / group_size_y),
                                 (image_dims.depth / group_size_z)};

  lzt::append_launch_function(cmd_bundle.list, kernel, &group_dems, nullptr, 0,
                              nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::append_image_copy_to_mem(cmd_bundle.list, outbuff, img_out, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  buffer_verify_f(*this);

  lzt::destroy_function(kernel);
  cleanup();
}

ze_image_handle_t
ImageFormatTests::create_image_desc_format(ze_image_type_t image_type,
                                           ze_image_format_type_t format_type,
                                           ze_image_format_layout_t layout) {
  ze_image_desc_t image_desc = {};
  image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  ze_image_handle_t image;

  uint32_t array_levels = 0;
  if (image_type == ZE_IMAGE_TYPE_1DARRAY) {
    array_levels = image_dims.height;
  }
  if (image_type == ZE_IMAGE_TYPE_2DARRAY) {
    array_levels = image_dims.depth;
  }

  image_desc.pNext = nullptr;
  image_desc.flags = (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
  if (image_type != ZE_IMAGE_TYPE_BUFFER) {
    image_desc.type = image_type;
  }
  if (image_type != ZE_IMAGE_TYPE_BUFFER) {
    image_desc.format.layout = layout;
    image_desc.format.type = format_type;
    image_desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
    image_desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_0;
    image_desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_0;
    image_desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_1;
  }
  image_desc.width = image_dims.width;
  image_desc.height = image_dims.height;
  image_desc.depth = image_dims.depth;
  image_desc.arraylevels = array_levels;
  image_desc.miplevels = 0;
  auto result = zeImageCreate(lzt::get_default_context(),
                              lzt::zeDevice::get_instance()->get_device(),
                              &image_desc, &image);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(lzt::get_default_context(),
                          lzt::zeDevice::get_instance()->get_device(),
                          &image_desc, &image));
  EXPECT_NE(nullptr, image);

  return image;
}

void RunGivenImageFormatUINTWhenCopyingImageTest(ImageFormatTests &test,
                                                 bool is_immediate) {
  for (auto image_type : test.tested_image_types) {
    LOG_INFO << "RUN: TYPE - " << std::to_string(image_type); 
    test.run_test(ImageFormatTests::setup_buffers<uint32_t, 1>,
                  ImageFormatTests::verify_outbuffer<uint32_t, 2>, image_type,
                  ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_LAYOUT_32,
                  is_immediate);
  }
}

TEST_F(ImageFormatTests,
       GivenImageFormatUINTWhenCopyingImageThenFormatIsCorrect) {
  RunGivenImageFormatUINTWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatUINTWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  RunGivenImageFormatUINTWhenCopyingImageTest(*this, true);
}

void RunGivenImageFormatINTWhenCopyingImageTest(ImageFormatTests &test,
                                                bool is_immediate) {
  for (auto image_type : test.tested_image_types) {
    LOG_INFO << "RUN: TYPE - " << std::to_string(image_type); 
    test.run_test(ImageFormatTests::setup_buffers<int32_t, -1>,
                  ImageFormatTests::verify_outbuffer<int32_t, -2>, image_type,
                  ZE_IMAGE_FORMAT_TYPE_SINT, ZE_IMAGE_FORMAT_LAYOUT_32,
                  is_immediate);
  }
}

TEST_F(ImageFormatTests,
       GivenImageFormatINTWhenCopyingImageThenFormatIsCorrect) {
  RunGivenImageFormatINTWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatINTWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  RunGivenImageFormatINTWhenCopyingImageTest(*this, true);
}

void RunGivenImageFormatFloatWhenCopyingImageTest(ImageFormatTests &test,
                                                  bool is_immediate) {
  for (auto image_type : test.tested_image_types) {
    LOG_INFO << "RUN: TYPE - " << std::to_string(image_type); 
    test.run_test(ImageFormatTests::setup_buffers_float,
                  ImageFormatTests::verify_outbuffer_float, image_type,
                  ZE_IMAGE_FORMAT_TYPE_FLOAT, ZE_IMAGE_FORMAT_LAYOUT_32,
                  is_immediate);
  }
}

TEST_F(ImageFormatTests,
       GivenImageFormatFloatWhenCopyingImageThenFormatIsCorrect) {
  RunGivenImageFormatFloatWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatFloatWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  RunGivenImageFormatFloatWhenCopyingImageTest(*this, true);
}

void RunGivenImageFormatUNORMWhenCopyingImageTest(ImageFormatTests &test,
                                                  bool is_immediate) {
  for (auto image_type : test.tested_image_types) {
    LOG_INFO << "RUN: TYPE - " << std::to_string(image_type); 
    test.run_test(ImageFormatTests::setup_buffers<uint16_t, 0>,
                  ImageFormatTests::verify_outbuffer<uint16_t, 65535>,
                  image_type, ZE_IMAGE_FORMAT_TYPE_UNORM,
                  ZE_IMAGE_FORMAT_LAYOUT_16, is_immediate);
  }
}

TEST_F(ImageFormatTests,
       GivenImageFormatUNORMWhenCopyingImageThenFormatIsCorrect) {
  RunGivenImageFormatUNORMWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatUNORMWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  RunGivenImageFormatUNORMWhenCopyingImageTest(*this, true);
}

void RunGivenImageFormatSNORMWhenCopyingImageTest(ImageFormatTests &test,
                                                  bool is_immediate) {
  for (auto image_type : test.tested_image_types) {
    LOG_INFO << "RUN: TYPE - " << std::to_string(image_type); 
    test.run_test(ImageFormatTests::setup_buffers<int16_t, -32768>,
                  ImageFormatTests::verify_outbuffer<int16_t, 32767>,
                  image_type, ZE_IMAGE_FORMAT_TYPE_SNORM,
                  ZE_IMAGE_FORMAT_LAYOUT_16, is_immediate);
  }
}

TEST_F(ImageFormatTests,
       GivenImageFormatSNORMWhenCopyingImageThenFormatIsCorrect) {
  RunGivenImageFormatSNORMWhenCopyingImageTest(*this, false);
}

TEST_F(
    ImageFormatTests,
    GivenImageFormatSNORMWhenCopyingImageOnImmediateCmdListThenFormatIsCorrect) {
  RunGivenImageFormatSNORMWhenCopyingImageTest(*this, true);
}

class ImageFormatLayoutTests
    : public ImageFormatTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_type_t, ze_image_format_layout_t, bool>> {
public:
  template <typename T, int size_multiplier = 1>
  static void set_up_buffers(ImageFormatTests &test);
  template <typename T, bool saturates = true, int size_multiplier = 1>
  static void verify_buffer(ImageFormatTests &test);
  template <typename T, bool saturates = true>
  static void verify_buffer_float(ImageFormatTests &test);

  void get_kernel(ze_image_type_t image_type,
                  ze_image_format_type_t format_type,
                  ze_image_format_layout_t layout) override;

  ze_image_handle_t
  create_image_desc_format(ze_image_type_t image_type,
                           ze_image_format_type_t format_type,
                           ze_image_format_layout_t layout) override;

  void set_up_module() override {
    module =
        lzt::create_module(tested_device, "image_format_layouts_tests.spv");
  }
};

ze_image_handle_t ImageFormatLayoutTests::create_image_desc_format(
    ze_image_type_t image_type, ze_image_format_type_t format_type,
    ze_image_format_layout_t layout) {
  ze_image_desc_t image_descriptor = {};
  ze_image_handle_t image;

  uint32_t array_levels = 0;
  if (image_type == ZE_IMAGE_TYPE_1DARRAY) {
    array_levels = image_dims.height;
  }
  if (image_type == ZE_IMAGE_TYPE_2DARRAY) {
    array_levels = image_dims.depth;
  }

  image_descriptor.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_descriptor.type = image_type;
  image_descriptor.flags =
      (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
  image_descriptor.format.type = format_type;
  image_descriptor.format.layout = layout;
  image_descriptor.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_descriptor.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_descriptor.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_descriptor.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_descriptor.width = image_dims.width;
  image_descriptor.height = image_dims.height;
  image_descriptor.depth = image_dims.depth;
  image_descriptor.arraylevels = array_levels;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(lzt::get_default_context(), tested_device,
                          &image_descriptor, &image));
  EXPECT_NE(nullptr, image);

  return image;
}

void ImageFormatLayoutTests::get_kernel(ze_image_type_t image_type,
                                        ze_image_format_type_t format_type,
                                        ze_image_format_layout_t layout) {
  kernel_name = "image_format_layout";
  kernel_name +=
      ('_' + std::to_string(lzt::get_format_component_count(layout)) +
       "_component");
  switch (format_type) {
  case ZE_IMAGE_FORMAT_TYPE_UINT:
    kernel_name += "_uint";
    break;
  case ZE_IMAGE_FORMAT_TYPE_SINT:
    kernel_name += "_sint";
    break;
  case ZE_IMAGE_FORMAT_TYPE_FLOAT:
    kernel_name += "_float";
    break;
  case ZE_IMAGE_FORMAT_TYPE_UNORM:
    kernel_name += "_unorm";
    break;
  case ZE_IMAGE_FORMAT_TYPE_SNORM:
    kernel_name += "_snorm";
    break;
  default:
    kernel_name = "UNKNOWN_KERNEL";
    return;
  }
  kernel_name += '_' + lzt::shortened_string(image_type);
}

template <typename T, int size_multiplier>
void ImageFormatLayoutTests::set_up_buffers(ImageFormatTests &test) {
  test.inbuff =
      lzt::allocate_host_memory(test.image_size * sizeof(T) * size_multiplier);
  test.outbuff =
      lzt::allocate_host_memory(test.image_size * sizeof(T) * size_multiplier);
  T *in_ptr = static_cast<T *>(test.inbuff);
  for (int i = 0; i < (test.image_size * size_multiplier); i++) {
    // set pixel value to 1 less than max for data type so
    // that when the kernel increments it, it saturates
    // or rolls over
    in_ptr[i] = ~(0 & in_ptr[i]) - 1;
  }
}

template <typename T, bool saturates, int size_multiplier>
void ImageFormatLayoutTests::verify_buffer(ImageFormatTests &test) {
  auto max_val = std::numeric_limits<T>::max();
  T *out_ptr = static_cast<T *>(test.outbuff);
  for (int i = 0; i < (test.image_size * size_multiplier); i++) {
    if (saturates) {
      ASSERT_EQ(out_ptr[i], max_val);
    } else if (max_val == std::numeric_limits<uint64_t>::max()) {
      // We use 64 bit buffers for the multi-32 bit component types, so
      // adjust the expected value
      ASSERT_EQ(out_ptr[i], std::numeric_limits<uint32_t>::max() + (uint64_t)1);
    } else {
      ASSERT_EQ(out_ptr[i], 0);
    }
  }
}

template <typename T, bool saturates>
void ImageFormatLayoutTests::verify_buffer_float(ImageFormatTests &test) {
  T *out_ptr = static_cast<T *>(test.outbuff);
  for (int i = 0; i < test.image_size; i++) {
    if (saturates) {
      ASSERT_EQ(out_ptr[i], std::numeric_limits<T>::max());
    } else {
      ASSERT_GT(out_ptr[i], 0);
      ASSERT_LT(out_ptr[i], std::numeric_limits<T>::max());
    }
  }
}

TEST_P(ImageFormatLayoutTests,
       GivenImageFormatLayoutWhenCopyingImageThenFormatIsCorrect) {
  auto image_type = std::get<0>(GetParam());
  auto layout = std::get<1>(GetParam());
  auto is_immediate = std::get<2>(GetParam());

  auto driver = lzt::get_default_driver();
  for (auto device : lzt::get_devices(driver)) {
    tested_device = device;
    auto supported_img_types = lzt::get_supported_image_types(
        tested_device, skip_array_type, skip_buffer_type);
    if (supported_img_types.size() == 0) {
      LOG_WARNING << "Device does not support images";
      continue;
    }

    LOG_INFO << "image type " << image_type;
    LOG_INFO << "image layout " << layout;

    if (std::find(supported_img_types.begin(), supported_img_types.end(),
                  image_type) == supported_img_types.end()) {
      LOG_WARNING << image_type << "unsupported type";
      continue;
    }

    switch (layout) {
    case ZE_IMAGE_FORMAT_LAYOUT_8:
      run_test(set_up_buffers<uint8_t>, verify_buffer<uint8_t>, image_type,
               ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_16:
      run_test(set_up_buffers<uint16_t>, verify_buffer<uint16_t>, image_type,
               ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_32:
      run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t, false>,
               image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_8_8:
      run_test(set_up_buffers<uint16_t>, verify_buffer<uint16_t>, image_type,
               ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_16_16:
      run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
               ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_32_32:
      run_test(set_up_buffers<uint64_t>, verify_buffer<uint64_t, false>,
               image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_5_6_5:
      run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
               image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_11_11_10:
      run_test(set_up_buffers<uint32_t>, verify_buffer_float<uint32_t, false>,
               image_type, ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4:
      run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
               image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1:
      run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
               image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8:
      run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
               ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2:
      run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
               ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16:
      run_test(set_up_buffers<uint64_t>, verify_buffer<uint64_t>, image_type,
               ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32:
      run_test(set_up_buffers<uint64_t, 2>, verify_buffer<uint64_t, false, 2>,
               image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate);
      break;
    default:
      LOG_INFO << "Unhandled image format layout: " << layout;
      break;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    LayoutParameterization, ImageFormatLayoutTests,
    ::testing::Combine(
        ::testing::Values(ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D,
                          ZE_IMAGE_TYPE_1DARRAY, ZE_IMAGE_TYPE_2DARRAY),
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
