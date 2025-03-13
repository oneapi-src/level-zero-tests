/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "helpers_test_image.hpp"
#include <complex>

namespace lzt = level_zero_tests;

namespace {

enum TestType { IMAGE_OBJECT_ONLY, ONE_KERNEL_ONLY, TWO_KERNEL_CONVERT };

const std::vector<ze_image_type_t> tested_image_types = {
    ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D, ZE_IMAGE_TYPE_1DARRAY,
    ZE_IMAGE_TYPE_2DARRAY};

class ImageLayoutFixture : public ::testing::Test {
public:
  void SetUp() override {
    if (!(lzt::image_support())) {
      GTEST_SKIP() << "Device does not support images";
    }
    auto device = lzt::zeDevice::get_instance()->get_device();
    module = lzt::create_module(device, "image_layout_tests.spv");
    supported_image_types =
        get_supported_image_types(device, skip_array_type, skip_buffer_type);
  }

  void run_test(ze_image_type_t image_type,
                ze_image_format_layout_t base_layout,
                ze_image_format_layout_t convert_layout,
                ze_image_format_type_t format_type, enum TestType test,
                bool is_immediate) {
    LOG_INFO << "TYPE - " << image_type << " FORMAT - " << format_type;
    LOG_INFO << "LAYOUT: BASE - " << base_layout << " CONVERT - "
             << convert_layout;
    cmd_bundle = lzt::create_command_bundle(is_immediate);
    std::string kernel_name = get_kernel(format_type, image_type);
    LOG_DEBUG << "kernel_name = " << kernel_name;
    ze_kernel_handle_t kernel = lzt::create_function(module, kernel_name);
    image_dims = get_sample_image_dims(image_type);
    image_size = static_cast<size_t>(image_dims.width * image_dims.height *
                                     image_dims.depth);

    auto image_in =
        create_image_desc_layout(base_layout, image_type, format_type);
    auto image_out =
        create_image_desc_layout(base_layout, image_type, format_type);
    auto image_convert =
        create_image_desc_layout(convert_layout, image_type, format_type);

    size_t buffer_size = image_size * get_pixel_bytes(base_layout);
    auto buffer_in = lzt::allocate_host_memory(buffer_size);
    auto buffer_out = lzt::allocate_host_memory(buffer_size);

    if (format_type == ZE_IMAGE_FORMAT_TYPE_FLOAT) {
      float *ptr1 = static_cast<float *>(buffer_in);
      float *ptr2 = static_cast<float *>(buffer_out);
      for (size_t i = 0; i < buffer_size / sizeof(float); ++i) {
        ptr1[i] = static_cast<float>((0xff) - (i & 0xff));
        ptr2[i] = 0xff;
      }
    } else if (format_type == ZE_IMAGE_FORMAT_TYPE_SNORM) {
      int *ptr1 = static_cast<int *>(buffer_in);
      int *ptr2 = static_cast<int *>(buffer_out);
      for (size_t i = 0; i < buffer_size / sizeof(int); ++i) {
        ptr1[i] = std::norm((0xff) - (i & 0xff));
        ptr2[i] = 0xff;
      }
    } else {
      uint8_t *ptr1 = static_cast<uint8_t *>(buffer_in);
      uint8_t *ptr2 = static_cast<uint8_t *>(buffer_out);
      for (size_t i = 0; i < buffer_size / sizeof(uint8_t); ++i) {
        ptr1[i] = static_cast<uint8_t>((0xff) - (i & 0xff));
        ptr2[i] = 0xff;
      }
    }

    // copy input buff to image_in object
    lzt::append_image_copy_from_mem(cmd_bundle.list, image_in, buffer_in,
                                    nullptr);
    lzt::append_barrier(cmd_bundle.list);
    if (test == IMAGE_OBJECT_ONLY) {
      LOG_DEBUG << "IMAGE_OBJECT_ONLY";
      lzt::append_image_copy_to_mem(cmd_bundle.list, buffer_out, image_in,
                                    nullptr);
      lzt::append_barrier(cmd_bundle.list);
    } else {
      // call kernel to copy image_in -> image_convert
      uint32_t group_size_x, group_size_y, group_size_z;

      lzt::suggest_group_size(kernel, image_dims.width, image_dims.height,
                              image_dims.depth, group_size_x, group_size_y,
                              group_size_z);
      lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

      lzt::set_argument_value(kernel, 0, sizeof(image_in), &image_in);
      lzt::set_argument_value(kernel, 1, sizeof(image_convert), &image_convert);

      ze_group_count_t group_dems = {
          (static_cast<uint32_t>(image_dims.width) / group_size_x),
          (image_dims.height / group_size_y),
          (image_dims.depth / group_size_z)};

      lzt::append_launch_function(cmd_bundle.list, kernel, &group_dems, nullptr,
                                  0, nullptr);
      lzt::append_barrier(cmd_bundle.list);

      if (test == ONE_KERNEL_ONLY) {
        LOG_DEBUG << "ONE_KERNEL_ONLY";
        lzt::append_image_copy_to_mem(cmd_bundle.list, buffer_out,
                                      image_convert, nullptr);
      } else {
        LOG_DEBUG << "TWO_KERNEL_CONVERT";
        // call kernel to copy image_convert -> image_out
        lzt::suggest_group_size(kernel, image_dims.width, image_dims.height,
                                image_dims.depth, group_size_x, group_size_y,
                                group_size_z);
        lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

        lzt::set_argument_value(kernel, 0, sizeof(image_convert),
                                &image_convert);
        lzt::set_argument_value(kernel, 1, sizeof(image_out), &image_out);

        group_dems = {(static_cast<uint32_t>(image_dims.width) / group_size_x),
                      (image_dims.height / group_size_y),
                      (image_dims.depth / group_size_z)};

        lzt::append_launch_function(cmd_bundle.list, kernel, &group_dems,
                                    nullptr, 0, nullptr);
        // finalize, copy to buffer_out
        lzt::append_barrier(cmd_bundle.list);
        lzt::append_image_copy_to_mem(cmd_bundle.list, buffer_out, image_out,
                                      nullptr);
      }
      LOG_DEBUG << "group_size_x = " << group_size_x
                << " group_size_y = " << group_size_y
                << " group_size_z = " << group_size_z;
    }
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
    LOG_DEBUG << "buffer size = " << buffer_size;

    // compare results
    EXPECT_EQ(memcmp(buffer_in, buffer_out, buffer_size), 0);
    lzt::free_memory(buffer_in);
    lzt::free_memory(buffer_out);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  void TearDown() override {
    if (module != nullptr) {
      lzt::destroy_module(module);
    }
  }

  ze_image_handle_t create_image_desc_layout(ze_image_format_layout_t layout,
                                             ze_image_type_t type,
                                             ze_image_format_type_t format);
  size_t get_pixel_bytes(ze_image_format_layout_t layout);
  std::string get_kernel(ze_image_format_type_t format_type,
                         ze_image_type_t image_type);

  static const bool skip_array_type = false;
  static const bool skip_buffer_type = true;

  lzt::zeCommandBundle cmd_bundle;
  ze_module_handle_t module;
  std::vector<ze_image_type_t> supported_image_types;
  Dims image_dims;
  size_t image_size;
};

size_t ImageLayoutFixture::get_pixel_bytes(ze_image_format_layout_t layout) {
  size_t bytes_per_pixel;
  switch (layout) {
  case ZE_IMAGE_FORMAT_LAYOUT_8:
    bytes_per_pixel = 1;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_16:
  case ZE_IMAGE_FORMAT_LAYOUT_8_8:
  case ZE_IMAGE_FORMAT_LAYOUT_5_6_5:
  case ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1:
  case ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4:
    bytes_per_pixel = 2;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32:
  case ZE_IMAGE_FORMAT_LAYOUT_16_16:
  case ZE_IMAGE_FORMAT_LAYOUT_11_11_10:
  case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8:
  case ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2:
    bytes_per_pixel = 4;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32:
  case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16:
    bytes_per_pixel = 8;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32:
    bytes_per_pixel = 16;
    break;
  default:
    bytes_per_pixel = 1;
    break;
  }
  return bytes_per_pixel;
}

std::string ImageLayoutFixture::get_kernel(ze_image_format_type_t format_type,
                                           ze_image_type_t image_type) {
  std::string kernel = "copy";
  switch (format_type) {
  case ZE_IMAGE_FORMAT_TYPE_UNORM:
  case ZE_IMAGE_FORMAT_TYPE_SNORM:
  case ZE_IMAGE_FORMAT_TYPE_FLOAT:
    kernel += "_float";
    break;
  case ZE_IMAGE_FORMAT_TYPE_SINT:
    kernel += "_sint";
    break;
  default:
    kernel += "_uint";
    break;
  }
  return kernel + '_' + shortened_string(image_type);
}

ze_image_handle_t
ImageLayoutFixture::create_image_desc_layout(ze_image_format_layout_t layout,
                                             ze_image_type_t type,
                                             ze_image_format_type_t format) {
  ze_image_desc_t image_desc = {};
  size_t components = lzt::get_format_component_count(layout);
  image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;

  image_desc.pNext = nullptr;
  image_desc.flags = ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED;
  image_desc.type = type;
  image_desc.format.layout = layout;
  image_desc.format.type = format;
  if (components == 4) {
    image_desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
    image_desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
    image_desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
    image_desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  } else if (components == 3) {
    image_desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
    image_desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
    image_desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
    image_desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_X;
  } else if (components == 2) {
    image_desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
    image_desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
    image_desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_X;
    image_desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_X;
  } else {
    image_desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
    image_desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_X;
    image_desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_X;
    image_desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_X;
  }

  if (type == ZE_IMAGE_TYPE_1DARRAY) {
    image_desc.arraylevels = image_dims.height;
  }
  if (type == ZE_IMAGE_TYPE_2DARRAY) {
    image_desc.arraylevels = image_dims.depth;
  }

  image_desc.width = image_dims.width;
  image_desc.height = image_dims.height;
  image_desc.depth = image_dims.depth;

  auto image = lzt::create_ze_image(lzt::get_default_context(),
                                    lzt::zeDevice::get_instance()->get_device(),
                                    image_desc);
  EXPECT_NE(nullptr, image);

  return image;
}

class ImageLayoutOneOrNoKernel
    : public ImageLayoutFixture,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_type_t, ze_image_format_type_t,
                     ze_image_format_layout_t, bool>> {};

TEST_P(ImageLayoutOneOrNoKernel, GivenImageLayoutWhenConvertingImageToMemory) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto format = std::get<1>(GetParam());
  auto layout = std::get<2>(GetParam());
  auto is_immediate = std::get<3>(GetParam());
  run_test(image_type, layout, layout, format, IMAGE_OBJECT_ONLY, is_immediate);
}

TEST_P(
    ImageLayoutOneOrNoKernel,
    GivenImageLayoutWhenPassingImageThroughKernelAndConvertingImageToMemory) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto format = std::get<1>(GetParam());
  auto layout = std::get<2>(GetParam());
  auto is_immediate = std::get<3>(GetParam());
  run_test(image_type, layout, layout, format, ONE_KERNEL_ONLY, is_immediate);
}

INSTANTIATE_TEST_SUITE_P(
    TestLayoutUIntFormat, ImageLayoutOneOrNoKernel,
    ::testing::Combine(::testing::ValuesIn(tested_image_types),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_UINT),
                       ::testing::ValuesIn(lzt::image_format_layout_uint),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TestLayoutSIntFormat, ImageLayoutOneOrNoKernel,
    ::testing::Combine(::testing::ValuesIn(tested_image_types),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_SINT),
                       ::testing::ValuesIn(lzt::image_format_layout_sint),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TestLayoutUNormFormat, ImageLayoutOneOrNoKernel,
    ::testing::Combine(::testing::ValuesIn(tested_image_types),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_UNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_unorm),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TestLayoutSNormFormat, ImageLayoutOneOrNoKernel,
    ::testing::Combine(::testing::ValuesIn(tested_image_types),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_SNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_snorm),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TestLayoutFloatFormat, ImageLayoutOneOrNoKernel,
    ::testing::Combine(::testing::ValuesIn(tested_image_types),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_FLOAT),
                       ::testing::ValuesIn(lzt::image_format_layout_float),
                       ::testing::Bool()));

class ImageLayoutTwoKernels
    : public ImageLayoutFixture,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_type_t, ze_image_format_type_t, bool>> {};

TEST_P(ImageLayoutTwoKernels, GivenImageLayoutWhenKernelConvertingImage) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto format = std::get<1>(GetParam());
  auto is_immediate = std::get<2>(GetParam());
  switch (format) {
  case ZE_IMAGE_FORMAT_TYPE_UINT:
  case ZE_IMAGE_FORMAT_TYPE_SINT:
  case ZE_IMAGE_FORMAT_TYPE_UNORM:
  case ZE_IMAGE_FORMAT_TYPE_SNORM:
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_16,
             format, TWO_KERNEL_CONVERT, is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_32,
             format, TWO_KERNEL_CONVERT, is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8,
             ZE_IMAGE_FORMAT_LAYOUT_16_16, format, TWO_KERNEL_CONVERT,
             is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8,
             ZE_IMAGE_FORMAT_LAYOUT_32_32, format, TWO_KERNEL_CONVERT,
             is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
             ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format, TWO_KERNEL_CONVERT,
             is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
             ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format, TWO_KERNEL_CONVERT,
             is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
             ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format, TWO_KERNEL_CONVERT,
             is_immediate);
  case ZE_IMAGE_FORMAT_TYPE_FLOAT:
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
             ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format, TWO_KERNEL_CONVERT,
             is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_LAYOUT_32,
             format, TWO_KERNEL_CONVERT, is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_16_16,
             ZE_IMAGE_FORMAT_LAYOUT_32_32, format, TWO_KERNEL_CONVERT,
             is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
             ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format, TWO_KERNEL_CONVERT,
             is_immediate);
    run_test(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
             ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format, TWO_KERNEL_CONVERT,
             is_immediate);
    break;
  default:
    throw std::runtime_error("Unhandled format");
  }
}

INSTANTIATE_TEST_SUITE_P(
    TestLayoutTwoKernels, ImageLayoutTwoKernels,
    ::testing::Combine(::testing::ValuesIn(tested_image_types),
                       lzt::image_format_types, ::testing::Bool()));

} // namespace
