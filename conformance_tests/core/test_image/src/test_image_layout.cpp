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

using lzt::to_f32;
using lzt::to_int;
using lzt::to_u32;
using lzt::to_u8;

enum TestType { IMAGE_OBJECT_ONLY, ONE_KERNEL_ONLY, TWO_KERNEL_CONVERT };

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

  ze_result_t set_kernel_arg(ze_kernel_handle_t hFunction, uint32_t argIndex,
                             size_t argSize, const void *pArgValue) {
    ze_result_t result =
        zeKernelSetArgumentValue(hFunction, argIndex, argSize, pArgValue);
    if (result == ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT) {
      skip_message << "zeKernelSetArgumentValue[" << std::to_string(argIndex)
                   << "] Unsupported image format";
    } else {
      EXPECT_ZE_RESULT_SUCCESS(result);
    }
    return result;
  }

  void run_test(ze_image_type_t image_type,
                ze_image_format_layout_t base_layout,
                ze_image_format_layout_t convert_layout,
                ze_image_format_type_t format_type, enum TestType test,
                bool is_immediate, bool is_shared_system) {
    LOG_INFO << "TYPE - " << image_type << " FORMAT - " << format_type;
    LOG_INFO << "LAYOUT: BASE - " << base_layout << " CONVERT - "
             << convert_layout;
    auto cmd_bundle = lzt::create_command_bundle(is_immediate);
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
    auto buffer_in = lzt::allocate_host_memory_with_allocator_selector(
        buffer_size, is_shared_system);
    auto buffer_out = lzt::allocate_host_memory_with_allocator_selector(
        buffer_size, is_shared_system);

    if (format_type == ZE_IMAGE_FORMAT_TYPE_FLOAT) {
      float *ptr1 = static_cast<float *>(buffer_in);
      float *ptr2 = static_cast<float *>(buffer_out);
      for (size_t i = 0; i < buffer_size / sizeof(float); ++i) {
        ptr1[i] = to_f32((0xff) - (i & 0xff));
        ptr2[i] = 0xff;
      }
    } else if (format_type == ZE_IMAGE_FORMAT_TYPE_SNORM) {
      int *ptr1 = static_cast<int *>(buffer_in);
      int *ptr2 = static_cast<int *>(buffer_out);
      for (size_t i = 0; i < buffer_size / sizeof(int); ++i) {
        ptr1[i] = to_int(std::norm((0xff) - (i & 0xff)));
        ptr2[i] = 0xff;
      }
    } else {
      uint8_t *ptr1 = static_cast<uint8_t *>(buffer_in);
      uint8_t *ptr2 = static_cast<uint8_t *>(buffer_out);
      for (size_t i = 0; i < buffer_size / sizeof(uint8_t); ++i) {
        ptr1[i] = to_u8((0xff) - (i & 0xff));
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

      lzt::suggest_group_size(kernel, to_u32(image_dims.width),
                              image_dims.height, image_dims.depth, group_size_x,
                              group_size_y, group_size_z);
      lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

      if (set_kernel_arg(kernel, 0, sizeof(image_in), &image_in) !=
          ZE_RESULT_SUCCESS) {
        return;
      }
      if (set_kernel_arg(kernel, 1, sizeof(image_convert), &image_convert) !=
          ZE_RESULT_SUCCESS) {
        return;
      }

      ze_group_count_t group_dems = {to_u32(image_dims.width / group_size_x),
                                     image_dims.height / group_size_y,
                                     image_dims.depth / group_size_z};

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
        lzt::suggest_group_size(kernel, to_u32(image_dims.width),
                                image_dims.height, image_dims.depth,
                                group_size_x, group_size_y, group_size_z);
        lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

        if (set_kernel_arg(kernel, 0, sizeof(image_convert), &image_convert) !=
            ZE_RESULT_SUCCESS) {
          return;
        }
        if (set_kernel_arg(kernel, 1, sizeof(image_out), &image_out) !=
            ZE_RESULT_SUCCESS) {
          return;
        }

        group_dems = {to_u32(image_dims.width / group_size_x),
                      image_dims.height / group_size_y,
                      image_dims.depth / group_size_z};

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
    lzt::free_memory_with_allocator_selector(buffer_in, is_shared_system);
    lzt::free_memory_with_allocator_selector(buffer_out, is_shared_system);
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

  ze_module_handle_t module;
  std::vector<ze_image_type_t> supported_image_types;
  Dims image_dims;
  size_t image_size;
  std::stringstream skip_message;
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

class zeImageLayoutOneOrNoKernelTests
    : public ImageLayoutFixture,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_type_t, ze_image_format_type_t,
                     ze_image_format_layout_t, bool>> {
protected:
  void TearDown() override {
    ImageLayoutFixture::TearDown();
    if (!skip_message.str().empty()) {
      GTEST_SKIP() << skip_message.str();
    }
  }
};

LZT_TEST_P(zeImageLayoutOneOrNoKernelTests,
           GivenImageLayoutWhenConvertingImageToMemory) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto format = std::get<1>(GetParam());
  auto layout = std::get<2>(GetParam());
  auto is_immediate = std::get<3>(GetParam());
  run_test(image_type, layout, layout, format, IMAGE_OBJECT_ONLY, is_immediate,
           false);
}

LZT_TEST_P(
    zeImageLayoutOneOrNoKernelTests,
    GivenImageLayoutWhenPassingImageThroughKernelAndConvertingImageToMemory) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto format = std::get<1>(GetParam());
  auto layout = std::get<2>(GetParam());
  auto is_immediate = std::get<3>(GetParam());
  run_test(image_type, layout, layout, format, ONE_KERNEL_ONLY, is_immediate,
           false);
}

LZT_TEST_P(
    zeImageLayoutOneOrNoKernelTests,
    GivenImageLayoutWhenConvertingImageToMemoryWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto format = std::get<1>(GetParam());
  auto layout = std::get<2>(GetParam());
  auto is_immediate = std::get<3>(GetParam());
  run_test(image_type, layout, layout, format, IMAGE_OBJECT_ONLY, is_immediate,
           true);
}

LZT_TEST_P(
    zeImageLayoutOneOrNoKernelTests,
    GivenImageLayoutWhenPassingImageThroughKernelAndConvertingImageToMemoryWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto format = std::get<1>(GetParam());
  auto layout = std::get<2>(GetParam());
  auto is_immediate = std::get<3>(GetParam());
  run_test(image_type, layout, layout, format, ONE_KERNEL_ONLY, is_immediate,
           true);
}

INSTANTIATE_TEST_SUITE_P(
    TestLayoutUIntFormat, zeImageLayoutOneOrNoKernelTests,
    ::testing::Combine(::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_UINT),
                       ::testing::ValuesIn(lzt::image_format_layout_uint),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TestLayoutSIntFormat, zeImageLayoutOneOrNoKernelTests,
    ::testing::Combine(::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_SINT),
                       ::testing::ValuesIn(lzt::image_format_layout_sint),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TestLayoutUNormFormat, zeImageLayoutOneOrNoKernelTests,
    ::testing::Combine(::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_UNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_unorm),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TestLayoutSNormFormat, zeImageLayoutOneOrNoKernelTests,
    ::testing::Combine(::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_SNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_snorm),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    TestLayoutFloatFormat, zeImageLayoutOneOrNoKernelTests,
    ::testing::Combine(::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ZE_IMAGE_FORMAT_TYPE_FLOAT),
                       ::testing::ValuesIn(lzt::image_format_layout_float),
                       ::testing::Bool()));

class zeImageLayoutTwoKernelsTests
    : public ImageLayoutFixture,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_type_t, ze_image_format_type_t, bool>> {
public:
  void run_test_two_kernels(ze_image_type_t image_type,
                            ze_image_format_layout_t base_layout,
                            ze_image_format_layout_t convert_layout,
                            ze_image_format_type_t format_type,
                            enum TestType test, bool is_immediate,
                            bool is_shared_system) {
    run_test(image_type, base_layout, convert_layout, format_type, test,
             is_immediate, is_shared_system);
    if (!skip_message.str().empty()) {
      LOG_INFO << skip_message.str();
    }
    skip_message.str("");
    skip_message.clear();
  }
};

LZT_TEST_P(zeImageLayoutTwoKernelsTests,
           GivenImageLayoutWhenKernelConvertingImage) {
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
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8,
                         ZE_IMAGE_FORMAT_LAYOUT_16, format, TWO_KERNEL_CONVERT,
                         is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8,
                         ZE_IMAGE_FORMAT_LAYOUT_32, format, TWO_KERNEL_CONVERT,
                         is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                         ZE_IMAGE_FORMAT_LAYOUT_16_16, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                         ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
  case ZE_IMAGE_FORMAT_TYPE_FLOAT:
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                         ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_16,
                         ZE_IMAGE_FORMAT_LAYOUT_32, format, TWO_KERNEL_CONVERT,
                         is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_16_16,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                         ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, false);
    break;
  default:
    throw std::runtime_error("Unhandled format");
  }
}

LZT_TEST_P(zeImageLayoutTwoKernelsTests,
           GivenImageLayoutWhenKernelConvertingImageWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
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
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8,
                         ZE_IMAGE_FORMAT_LAYOUT_16, format, TWO_KERNEL_CONVERT,
                         is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8,
                         ZE_IMAGE_FORMAT_LAYOUT_32, format, TWO_KERNEL_CONVERT,
                         is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                         ZE_IMAGE_FORMAT_LAYOUT_16_16, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                         ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
  case ZE_IMAGE_FORMAT_TYPE_FLOAT:
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                         ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_16,
                         ZE_IMAGE_FORMAT_LAYOUT_32, format, TWO_KERNEL_CONVERT,
                         is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_16_16,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                         ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
    run_test_two_kernels(image_type, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                         ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                         TWO_KERNEL_CONVERT, is_immediate, true);
    break;
  default:
    throw std::runtime_error("Unhandled format");
  }
}

INSTANTIATE_TEST_SUITE_P(
    LayoutTwoKernelsTestsParam, zeImageLayoutTwoKernelsTests,
    ::testing::Combine(::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::ValuesIn(lzt::image_format_types),
                       ::testing::Bool()));

} // namespace
