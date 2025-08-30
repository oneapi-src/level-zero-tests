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
#include "helpers_test_image.hpp"

namespace lzt = level_zero_tests;

namespace {

using lzt::to_u32;

const std::vector<ze_image_type_t> tested_image_types = {
    ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D, ZE_IMAGE_TYPE_1DARRAY,
    ZE_IMAGE_TYPE_2DARRAY};

class ImageFormatFixture : public ::testing::Test {
public:
  void SetUp() override {
    if (!(lzt::image_support())) {
      GTEST_SKIP() << "Device does not support images";
    }
    set_up_module();
    supported_image_types =
        get_supported_image_types(lzt::zeDevice::get_instance()->get_device(),
                                  skip_array_type, skip_buffer_type);
  }

  void TearDown() override {
    if (module != nullptr) {
      lzt::destroy_module(module);
    }
  }

  virtual void set_up_module() = 0;

  virtual ze_image_handle_t
  create_image_desc_format(ze_image_type_t image_type,
                           ze_image_format_type_t format_type,
                           ze_image_format_layout_t layout) = 0;

  void run_test(void (*buffer_setup_f)(ImageFormatFixture &, bool),
                void (*buffer_verify_f)(ImageFormatFixture &),
                ze_image_type_t image_type, ze_image_format_type_t format_type,
                ze_image_format_layout_t layout, bool is_immediate,
                bool is_shared_system);

  virtual void get_kernel(ze_image_type_t image_type,
                          ze_image_format_type_t format_type,
                          ze_image_format_layout_t layout) = 0;

  static const bool skip_array_type = false;
  static const bool skip_buffer_type = true;
  static constexpr float float_pixel_input = 3.5f;

  ze_image_handle_t img_in, img_out;
  ze_module_handle_t module;
  Dims image_dims;
  size_t image_size;
  std::vector<ze_image_type_t> supported_image_types;
  void *inbuff = nullptr, *outbuff = nullptr;
  std::string kernel_name;
};

void ImageFormatFixture::run_test(
    void (*buffer_setup_f)(ImageFormatFixture &, bool),
    void (*buffer_verify_f)(ImageFormatFixture &), ze_image_type_t image_type,
    ze_image_format_type_t format_type, ze_image_format_layout_t layout,
    bool is_immediate, bool is_shared_system) {
  uint32_t group_size_x, group_size_y, group_size_z;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  get_kernel(image_type, format_type, layout);
  ze_kernel_handle_t kernel = lzt::create_function(module, kernel_name);

  image_dims = get_sample_image_dims(image_type);
  image_size = static_cast<size_t>(image_dims.width * image_dims.height *
                                   image_dims.depth);

  img_in = create_image_desc_format(image_type, format_type, layout);
  img_out = create_image_desc_format(image_type, format_type, layout);

  buffer_setup_f(*this, is_shared_system);

  lzt::append_image_copy_from_mem(cmd_bundle.list, img_in, inbuff, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::suggest_group_size(kernel, to_u32(image_dims.width), image_dims.height,
                          image_dims.depth, group_size_x, group_size_y,
                          group_size_z);

  lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

  lzt::set_argument_value(kernel, 0, sizeof(img_in), &img_in);
  lzt::set_argument_value(kernel, 1, sizeof(img_out), &img_out);

  ze_group_count_t group_dems = {to_u32(image_dims.width / group_size_x),
                                 image_dims.height / group_size_y,
                                 image_dims.depth / group_size_z};

  lzt::append_launch_function(cmd_bundle.list, kernel, &group_dems, nullptr, 0,
                              nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::append_image_copy_to_mem(cmd_bundle.list, outbuff, img_out, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  buffer_verify_f(*this);

  lzt::destroy_function(kernel);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(outbuff, is_shared_system);
  lzt::free_memory_with_allocator_selector(inbuff, is_shared_system);
}

class zeImageFormatTypeTests
    : public ImageFormatFixture,
      public ::testing::WithParamInterface<std::tuple<ze_image_type_t, bool>> {
public:
  template <typename T, T value = static_cast<T>(1)>
  static void setup_buffers(ImageFormatFixture &test, bool is_shared_system);

  static void setup_buffers_float(ImageFormatFixture &test,
                                  bool is_shared_system);

  template <typename T, T value = static_cast<T>(1)>
  static void verify_outbuffer(ImageFormatFixture &test);

  static void verify_outbuffer_float(ImageFormatFixture &test);

  void get_kernel(ze_image_type_t image_type,
                  ze_image_format_type_t format_type,
                  ze_image_format_layout_t layout) override;

  ze_image_handle_t
  create_image_desc_format(ze_image_type_t image_type,
                           ze_image_format_type_t format_type,
                           ze_image_format_layout_t layout) override;

  virtual void set_up_module() override {
    module = lzt::create_module(lzt::zeDevice::get_instance()->get_device(),
                                "image_formats_tests.spv");
  }
};

template <typename T, T value>
void zeImageFormatTypeTests::setup_buffers(ImageFormatFixture &test,
                                           bool is_shared_system) {
  test.inbuff = lzt::allocate_host_memory_with_allocator_selector(
      test.image_size * sizeof(T), is_shared_system);
  test.outbuff = lzt::allocate_host_memory_with_allocator_selector(
      test.image_size * sizeof(T), is_shared_system);
  T *in_ptr = static_cast<T *>(test.inbuff);
  for (size_t i = 0U; i < test.image_size; i++) {
    in_ptr[i] = value;
  }
}

void zeImageFormatTypeTests::setup_buffers_float(ImageFormatFixture &test,
                                                 bool is_shared_system) {
  test.inbuff = lzt::allocate_host_memory_with_allocator_selector(
      test.image_size * sizeof(float), is_shared_system);
  test.outbuff = lzt::allocate_host_memory_with_allocator_selector(
      test.image_size * sizeof(float), is_shared_system);
  float *in_ptr = static_cast<float *>(test.inbuff);
  for (size_t i = 0U; i < test.image_size; i++) {
    in_ptr[i] = float_pixel_input;
  }
}

template <typename T, T value>
void zeImageFormatTypeTests::verify_outbuffer(ImageFormatFixture &test) {
  T *out_ptr = static_cast<T *>(test.outbuff);
  for (size_t i = 0U; i < test.image_size; i++) {
    EXPECT_EQ(out_ptr[i], value);
  }
}

void zeImageFormatTypeTests::verify_outbuffer_float(ImageFormatFixture &test) {
  float *out_ptr = static_cast<float *>(test.outbuff);
  for (size_t i = 0U; i < test.image_size; i++) {
    EXPECT_LT(out_ptr[i], 3.5);
    EXPECT_GT(out_ptr[i], 3.0);
  }
}

void zeImageFormatTypeTests::get_kernel(ze_image_type_t image_type,
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
  kernel_name += '_' + shortened_string(image_type);
}

ze_image_handle_t zeImageFormatTypeTests::create_image_desc_format(
    ze_image_type_t image_type, ze_image_format_type_t format_type,
    ze_image_format_layout_t layout) {
  ze_image_desc_t image_desc = {};
  image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;

  uint32_t array_levels = 0;
  if (image_type == ZE_IMAGE_TYPE_1DARRAY) {
    array_levels = image_dims.height;
  }
  if (image_type == ZE_IMAGE_TYPE_2DARRAY) {
    array_levels = image_dims.depth;
  }

  image_desc.pNext = nullptr;
  image_desc.flags = (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
  image_desc.type = image_type;
  image_desc.format.layout = layout;
  image_desc.format.type = format_type;
  image_desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_0;
  image_desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_0;
  image_desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_1;
  image_desc.width = image_dims.width;
  image_desc.height = image_dims.height;
  image_desc.depth = image_dims.depth;
  image_desc.arraylevels = array_levels;
  image_desc.miplevels = 0;

  auto image = lzt::create_ze_image(lzt::get_default_context(),
                                    lzt::zeDevice::get_instance()->get_device(),
                                    image_desc);

  return image;
}

LZT_TEST_P(zeImageFormatTypeTests,
           GivenImageFormatUintWhenCopyingImageThenFormatIsCorrect) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers<uint32_t, 1>,
           zeImageFormatTypeTests::verify_outbuffer<uint32_t, 2>, image_type,
           ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_LAYOUT_32, is_immediate,
           false);
}

LZT_TEST_P(zeImageFormatTypeTests,
           GivenImageFormatIntWhenCopyingImageThenFormatIsCorrect) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers<int32_t, -1>,
           zeImageFormatTypeTests::verify_outbuffer<int32_t, -2>, image_type,
           ZE_IMAGE_FORMAT_TYPE_SINT, ZE_IMAGE_FORMAT_LAYOUT_32, is_immediate,
           false);
}

LZT_TEST_P(zeImageFormatTypeTests,
           GivenImageFormatFloatWhenCopyingImageThenFormatIsCorrect) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers_float,
           zeImageFormatTypeTests::verify_outbuffer_float, image_type,
           ZE_IMAGE_FORMAT_TYPE_FLOAT, ZE_IMAGE_FORMAT_LAYOUT_32, is_immediate,
           false);
}

LZT_TEST_P(zeImageFormatTypeTests,
           GivenImageFormatUnormWhenCopyingImageThenFormatIsCorrect) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers<uint16_t, 0>,
           zeImageFormatTypeTests::verify_outbuffer<uint16_t, 65535>,
           image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, ZE_IMAGE_FORMAT_LAYOUT_16,
           is_immediate, false);
}

LZT_TEST_P(zeImageFormatTypeTests,
           GivenImageFormatSnormWhenCopyingImageThenFormatIsCorrect) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers<int16_t, -32768>,
           zeImageFormatTypeTests::verify_outbuffer<int16_t, 32767>, image_type,
           ZE_IMAGE_FORMAT_TYPE_SNORM, ZE_IMAGE_FORMAT_LAYOUT_16, is_immediate,
           false);
}

LZT_TEST_P(
    zeImageFormatTypeTests,
    GivenImageFormatUintWhenCopyingImageThenFormatIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers<uint32_t, 1>,
           zeImageFormatTypeTests::verify_outbuffer<uint32_t, 2>, image_type,
           ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_LAYOUT_32, is_immediate,
           true);
}

LZT_TEST_P(
    zeImageFormatTypeTests,
    GivenImageFormatIntWhenCopyingImageThenFormatIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers<int32_t, -1>,
           zeImageFormatTypeTests::verify_outbuffer<int32_t, -2>, image_type,
           ZE_IMAGE_FORMAT_TYPE_SINT, ZE_IMAGE_FORMAT_LAYOUT_32, is_immediate,
           true);
}

LZT_TEST_P(
    zeImageFormatTypeTests,
    GivenImageFormatFloatWhenCopyingImageThenFormatIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers_float,
           zeImageFormatTypeTests::verify_outbuffer_float, image_type,
           ZE_IMAGE_FORMAT_TYPE_FLOAT, ZE_IMAGE_FORMAT_LAYOUT_32, is_immediate,
           true);
}

LZT_TEST_P(
    zeImageFormatTypeTests,
    GivenImageFormatUnormWhenCopyingImageThenFormatIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers<uint16_t, 0>,
           zeImageFormatTypeTests::verify_outbuffer<uint16_t, 65535>,
           image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, ZE_IMAGE_FORMAT_LAYOUT_16,
           is_immediate, true);
}

LZT_TEST_P(
    zeImageFormatTypeTests,
    GivenImageFormatSnormWhenCopyingImageThenFormatIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto is_immediate = std::get<1>(GetParam());
  LOG_INFO << "TYPE - " << image_type;
  run_test(zeImageFormatTypeTests::setup_buffers<int16_t, -32768>,
           zeImageFormatTypeTests::verify_outbuffer<int16_t, 32767>, image_type,
           ZE_IMAGE_FORMAT_TYPE_SNORM, ZE_IMAGE_FORMAT_LAYOUT_16, is_immediate,
           true);
}

INSTANTIATE_TEST_SUITE_P(
    FormatTypeTestsParam, zeImageFormatTypeTests,
    ::testing::Combine(::testing::ValuesIn(tested_image_types),
                       ::testing::Bool()));

class zeImageFormatLayoutTests
    : public ImageFormatFixture,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_type_t, ze_image_format_layout_t, bool>> {
public:
  template <typename T, size_t size_multiplier = 1>
  static void set_up_buffers(ImageFormatFixture &test, bool is_shared_system);
  template <typename T, bool saturates = true, size_t size_multiplier = 1>
  static void verify_buffer(ImageFormatFixture &test);
  template <typename T, bool saturates = true>
  static void verify_buffer_float(ImageFormatFixture &test);

  void get_kernel(ze_image_type_t image_type,
                  ze_image_format_type_t format_type,
                  ze_image_format_layout_t layout) override;

  ze_image_handle_t
  create_image_desc_format(ze_image_type_t image_type,
                           ze_image_format_type_t format_type,
                           ze_image_format_layout_t layout) override;

  void set_up_module() override {
    module = lzt::create_module(lzt::zeDevice::get_instance()->get_device(),
                                "image_format_layouts_tests.spv");
  }
};

ze_image_handle_t zeImageFormatLayoutTests::create_image_desc_format(
    ze_image_type_t image_type, ze_image_format_type_t format_type,
    ze_image_format_layout_t layout) {
  ze_image_desc_t image_descriptor = {};

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

  auto image = lzt::create_ze_image(lzt::get_default_context(),
                                    lzt::zeDevice::get_instance()->get_device(),
                                    image_descriptor);

  return image;
}

void zeImageFormatLayoutTests::get_kernel(ze_image_type_t image_type,
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
  kernel_name += '_' + shortened_string(image_type);
}

template <typename T, size_t size_multiplier>
void zeImageFormatLayoutTests::set_up_buffers(ImageFormatFixture &test,
                                              bool is_shared_system) {
  test.inbuff = lzt::allocate_host_memory_with_allocator_selector(
      (test.image_size * sizeof(T) * size_multiplier), is_shared_system);
  test.outbuff = lzt::allocate_host_memory_with_allocator_selector(
      (test.image_size * sizeof(T) * size_multiplier), is_shared_system);
  T *in_ptr = static_cast<T *>(test.inbuff);
  for (size_t i = 0U; i < (test.image_size * size_multiplier); i++) {
    // set pixel value to 1 less than max for data type so
    // that when the kernel increments it, it saturates
    // or rolls over
    in_ptr[i] = ~(0 & in_ptr[i]) - 1;
  }
}

template <typename T, bool saturates, size_t size_multiplier>
void zeImageFormatLayoutTests::verify_buffer(ImageFormatFixture &test) {
  auto max_val = std::numeric_limits<T>::max();
  T *out_ptr = static_cast<T *>(test.outbuff);
  for (size_t i = 0U; i < (test.image_size * size_multiplier); i++) {
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
void zeImageFormatLayoutTests::verify_buffer_float(ImageFormatFixture &test) {
  T *out_ptr = static_cast<T *>(test.outbuff);
  for (size_t i = 0U; i < test.image_size; i++) {
    if (saturates) {
      ASSERT_EQ(out_ptr[i], std::numeric_limits<T>::max());
    } else {
      ASSERT_GT(out_ptr[i], 0);
      ASSERT_LT(out_ptr[i], std::numeric_limits<T>::max());
    }
  }
}

LZT_TEST_P(zeImageFormatLayoutTests,
           GivenImageFormatLayoutWhenCopyingImageThenFormatIsCorrect) {
  auto image_type = std::get<0>(GetParam());
  auto layout = std::get<1>(GetParam());
  auto is_immediate = std::get<2>(GetParam());

  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << image_type;
  }

  LOG_INFO << "TYPE - " << image_type << "LAYOUT - " << layout;

  switch (layout) {
  case ZE_IMAGE_FORMAT_LAYOUT_8:
    run_test(set_up_buffers<uint8_t>, verify_buffer<uint8_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16:
    run_test(set_up_buffers<uint16_t>, verify_buffer<uint16_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32:
    run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t, false>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate,
             false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_8_8:
    run_test(set_up_buffers<uint16_t>, verify_buffer<uint16_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16_16:
    run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32:
    run_test(set_up_buffers<uint64_t>, verify_buffer<uint64_t, false>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate,
             false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_5_6_5:
    run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate,
             false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_11_11_10:
    run_test(set_up_buffers<uint32_t>, verify_buffer_float<uint32_t, false>,
             image_type, ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, is_immediate,
             false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4:
    run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate,
             false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1:
    run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate,
             false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8:
    run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2:
    run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16:
    run_test(set_up_buffers<uint64_t>, verify_buffer<uint64_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, false);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32:
    run_test(set_up_buffers<uint64_t, 2>, verify_buffer<uint64_t, false, 2>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate,
             false);
    break;
  default:
    LOG_INFO << "Unhandled image format layout: " << layout;
    break;
  }
}

LZT_TEST_P(
    zeImageFormatLayoutTests,
    GivenImageFormatLayoutWhenCopyingImageThenFormatIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  auto layout = std::get<1>(GetParam());
  auto is_immediate = std::get<2>(GetParam());

  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << image_type;
  }

  LOG_INFO << "TYPE - " << image_type << "LAYOUT - " << layout;

  switch (layout) {
  case ZE_IMAGE_FORMAT_LAYOUT_8:
    run_test(set_up_buffers<uint8_t>, verify_buffer<uint8_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16:
    run_test(set_up_buffers<uint16_t>, verify_buffer<uint16_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32:
    run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t, false>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_8_8:
    run_test(set_up_buffers<uint16_t>, verify_buffer<uint16_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16_16:
    run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32:
    run_test(set_up_buffers<uint64_t>, verify_buffer<uint64_t, false>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_5_6_5:
    run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate,
             true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_11_11_10:
    run_test(set_up_buffers<uint32_t>, verify_buffer_float<uint32_t, false>,
             image_type, ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, is_immediate,
             true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4:
    run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate,
             true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1:
    run_test(set_up_buffers<uint16_t>, verify_buffer_float<uint16_t>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UNORM, layout, is_immediate,
             true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8:
    run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2:
    run_test(set_up_buffers<uint32_t>, verify_buffer<uint32_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16:
    run_test(set_up_buffers<uint64_t>, verify_buffer<uint64_t>, image_type,
             ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32:
    run_test(set_up_buffers<uint64_t, 2>, verify_buffer<uint64_t, false, 2>,
             image_type, ZE_IMAGE_FORMAT_TYPE_UINT, layout, is_immediate, true);
    break;
  default:
    LOG_INFO << "Unhandled image format layout: " << layout;
    break;
  }
}

INSTANTIATE_TEST_SUITE_P(
    FormatLayoutTestsParam, zeImageFormatLayoutTests,
    ::testing::Combine(
        ::testing::ValuesIn(tested_image_types),
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
