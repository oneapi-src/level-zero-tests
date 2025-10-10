/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "helpers_test_image.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

namespace {

using lzt::to_u32;

void check_image_properties(ze_image_properties_t imageprop) {

  EXPECT_TRUE((to_u32(imageprop.samplerFilterFlags) == 0) ||
              ((to_u32(imageprop.samplerFilterFlags) &
                to_u32(ZE_IMAGE_SAMPLER_FILTER_FLAG_POINT)) ==
               to_u32(ZE_IMAGE_SAMPLER_FILTER_FLAG_POINT)) ||
              ((to_u32(imageprop.samplerFilterFlags) &
                to_u32(ZE_IMAGE_SAMPLER_FILTER_FLAG_LINEAR)) ==
               to_u32(ZE_IMAGE_SAMPLER_FILTER_FLAG_LINEAR)));
}

enum class ImageSize { min, large };

class zeImageDescriptorTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<
          ze_image_format_type_t, ze_image_format_layout_t, ze_image_flags_t,
          ze_image_flags_t, ze_image_type_t, ImageSize>> {
public:
  void SetUp() override {
    if (!(lzt::image_support())) {
      LOG_INFO << "device does not support images, cannot run test";
      GTEST_SKIP();
    }
    format_type = std::get<0>(GetParam());
    layout = std::get<1>(GetParam());
    image_rw_flag = std::get<2>(GetParam());
    image_cache_flag = std::get<3>(GetParam());
    image_type = std::get<4>(GetParam());
    image_size = std::get<5>(GetParam());
    auto supported_image_types = get_supported_image_types(
        lzt::zeDevice::get_instance()->get_device(), false, false);
    if (std::find(supported_image_types.begin(), supported_image_types.end(),
                  image_type) == supported_image_types.end()) {
      GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
    }

    ze_image_format_t format_descriptor = {
        layout,                    // layout
        format_type,               // type
        ZE_IMAGE_FORMAT_SWIZZLE_R, // x
        ZE_IMAGE_FORMAT_SWIZZLE_G, // y
        ZE_IMAGE_FORMAT_SWIZZLE_B, // z
        ZE_IMAGE_FORMAT_SWIZZLE_A  // w
    };
    ze_image_flag_t flags = (ze_image_flag_t)(image_rw_flag | image_cache_flag);
    uint32_t array_levels = 0;
    if (image_type == ZE_IMAGE_TYPE_1DARRAY) {
      array_levels = 1;
    }
    if (image_type == ZE_IMAGE_TYPE_2DARRAY) {
      array_levels = 2;
    }
    uint64_t width = 1;
    uint32_t height = 1, depth = 1;
    switch (image_type) {
    case ZE_IMAGE_TYPE_3D:
      if (image_size == ImageSize::min) {
        width = lzt::image_widths[0];
        height = lzt::image_heights[0];
        depth = lzt::image_depths[0];
      } else if (image_size == ImageSize::large) {
        width = lzt::image_widths[1];
        height = lzt::image_heights[1];
        depth = lzt::image_depths[1];
      }
      break;
    case ZE_IMAGE_TYPE_2D:
      [[fallthrough]];
    case ZE_IMAGE_TYPE_2DARRAY:
      if (image_size == ImageSize::min) {
        width = lzt::image_widths[0];
        height = lzt::image_heights[0];
      } else if (image_size == ImageSize::large) {
        width = lzt::image_widths[1];
        height = lzt::image_heights[1];
      }
      break;
    case ZE_IMAGE_TYPE_1D:
      [[fallthrough]];
    case ZE_IMAGE_TYPE_1DARRAY:
      if (image_size == ImageSize::min) {
        width = lzt::image_widths[0];
      } else if (image_size == ImageSize::large) {
        width = lzt::image_widths[1];
      }
      break;
    default:
      break;
    }

    image_descriptor.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
    image_descriptor.flags = flags;
    image_descriptor.type = image_type;
    image_descriptor.format = format_descriptor;
    image_descriptor.width = width;
    image_descriptor.height = height;
    image_descriptor.depth = depth;
    image_descriptor.arraylevels = array_levels;
    image_descriptor.miplevels = 0;
  }

  ze_image_format_type_t format_type;
  ze_image_format_layout_t layout;
  ze_image_flags_t image_rw_flag;
  ze_image_flags_t image_cache_flag;
  ze_image_type_t image_type;
  ImageSize image_size;
  ze_image_desc_t image_descriptor = {};
  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t device =
      lzt::get_default_device(lzt::get_default_driver());
};

LZT_TEST_P(zeImageDescriptorTest,
           GivenValidDescriptorWhenCreatingImageThenNotNullPointerIsReturned) {
  lzt::print_image_descriptor(image_descriptor);
  auto image = lzt::create_ze_image(context, device, image_descriptor);
  if (image) {
    lzt::destroy_ze_image(image);
  }
}

LZT_TEST_P(
    zeImageDescriptorTest,
    GivenValidDescriptorWhenCheckingImagePropertiesThenNotNullPointerIsReturned) {
  lzt::print_image_descriptor(image_descriptor);
  check_image_properties(lzt::get_ze_image_properties(image_descriptor));
}

INSTANTIATE_TEST_SUITE_P(
    TestDescriptorUIntFormat, zeImageDescriptorTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_UINT),
                       ::testing::ValuesIn(lzt::image_format_layout_uint),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    TestDescriptorSIntFormat, zeImageDescriptorTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_SINT),
                       ::testing::ValuesIn(lzt::image_format_layout_sint),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    TestDescriptorUNormFormat, zeImageDescriptorTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_UNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_unorm),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    TestDescriptorSNormFormat, zeImageDescriptorTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_SNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_snorm),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    TestDescriptorFloatFormat, zeImageDescriptorTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_FLOAT),
                       ::testing::ValuesIn(lzt::image_format_layout_float),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    TestDescriptorMediaFormat, zeImageDescriptorTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_FLOAT),
                       ::testing::ValuesIn(lzt::image_format_media_layouts),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

class zeImagePropertiesTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_type_t, ze_image_format_layout_t,
                     ze_image_flags_t, ze_image_flags_t, ze_image_type_t>> {
public:
  void SetUp() override {
    if (!(lzt::image_support())) {
      LOG_INFO << "device does not support images, cannot run test";
      GTEST_SKIP();
    }
    format_type = std::get<0>(GetParam());
    layout = std::get<1>(GetParam());
    image_rw_flag = std::get<2>(GetParam());
    image_cache_flag = std::get<3>(GetParam());
    image_type = std::get<4>(GetParam());
    auto supported_image_types = get_supported_image_types(
        lzt::zeDevice::get_instance()->get_device(), false, false);
    if (std::find(supported_image_types.begin(), supported_image_types.end(),
                  image_type) == supported_image_types.end()) {
      GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
    }

    ze_device_image_properties_t device_img_properties = {
        ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES, nullptr};
    EXPECT_ZE_RESULT_SUCCESS(
        zeDeviceGetImageProperties(device, &device_img_properties));

    uint64_t img_width = 1;
    uint32_t img_height = 1;
    uint32_t img_depth = 1;
    uint32_t array_levels = 0;
    if ((image_type == ZE_IMAGE_TYPE_1D) ||
        (image_type == ZE_IMAGE_TYPE_1DARRAY)) {
      img_width = device_img_properties.maxImageDims1D >> 1;
    } else if ((image_type == ZE_IMAGE_TYPE_2D) ||
               (image_type == ZE_IMAGE_TYPE_2DARRAY)) {
      img_width = device_img_properties.maxImageDims2D >> 2;
      img_height = device_img_properties.maxImageDims2D >> 2;
    } else if (image_type == ZE_IMAGE_TYPE_3D) {
      img_width = device_img_properties.maxImageDims3D >> 4;
      img_height = device_img_properties.maxImageDims3D >> 4;
      img_depth = device_img_properties.maxImageDims3D >> 4;
    } else {
      EXPECT_EQ(image_type, ZE_IMAGE_TYPE_BUFFER);
      img_width = device_img_properties.maxImageBufferSize >> 1;
    }

    if (image_type == ZE_IMAGE_TYPE_1DARRAY) {
      array_levels = std::min(1u, device_img_properties.maxImageArraySlices);
    } else if (image_type == ZE_IMAGE_TYPE_2DARRAY) {
      array_levels = std::min(2u, device_img_properties.maxImageArraySlices);
    }

    ze_image_format_t img_fmt = {};
    img_fmt.layout = layout;
    img_fmt.type = format_type;
    img_fmt.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
    img_fmt.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
    img_fmt.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
    img_fmt.w = ZE_IMAGE_FORMAT_SWIZZLE_A;

    image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
    image_desc.pNext = nullptr;
    image_desc.flags = image_rw_flag | image_cache_flag;
    image_desc.type = image_type;
    image_desc.format = img_fmt;
    image_desc.width = img_width;
    image_desc.height = img_height;
    image_desc.depth = img_depth;
    image_desc.arraylevels = array_levels;
    image_desc.miplevels = 0;
  }

protected:
  ze_image_format_type_t format_type;
  ze_image_format_layout_t layout;
  ze_image_flags_t image_rw_flag;
  ze_image_flags_t image_cache_flag;
  ze_image_type_t image_type;
  ze_image_desc_t image_desc;
  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t device =
      lzt::get_default_device(lzt::get_default_driver());
};

LZT_TEST_P(zeImagePropertiesTests,
           GivenValidImageWhenGettingAllocPropertiesThenSuccessIsReturned) {
  auto img = lzt::create_ze_image(context, device, image_desc);

  lzt::get_ze_image_alloc_properties_ext(img);

  lzt::destroy_ze_image(img);
}

LZT_TEST_P(
    zeImagePropertiesTests,
    GivenValidImageWhenGettingMemoryPropertiesThenValidMemoryPropertiesIsReturned) {
  auto img = lzt::create_ze_image(context, device, image_desc);

  auto image_mem_properties = lzt::get_ze_image_mem_properties_exp(img);

  EXPECT_GT(image_mem_properties.rowPitch, 0u);
  EXPECT_GT(image_mem_properties.size, 0u);
  EXPECT_GT(image_mem_properties.slicePitch, 0u);
  lzt::destroy_ze_image(img);
}

INSTANTIATE_TEST_SUITE_P(
    UInt, zeImagePropertiesTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_UINT),
                       ::testing::ValuesIn(lzt::image_format_layout_uint),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));

INSTANTIATE_TEST_SUITE_P(
    SInt, zeImagePropertiesTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_SINT),
                       ::testing::ValuesIn(lzt::image_format_layout_sint),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));

INSTANTIATE_TEST_SUITE_P(
    UNorm, zeImagePropertiesTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_UNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_unorm),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));

INSTANTIATE_TEST_SUITE_P(
    SNorm, zeImagePropertiesTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_SNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_snorm),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));

INSTANTIATE_TEST_SUITE_P(
    Float, zeImagePropertiesTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_FLOAT),
                       ::testing::ValuesIn(lzt::image_format_layout_float),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));
} // namespace
