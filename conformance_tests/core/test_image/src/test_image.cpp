/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

namespace {

void check_image_properties(ze_image_properties_t imageprop) {

  EXPECT_TRUE((static_cast<uint32_t>(imageprop.samplerFilterFlags) == 0) ||
              ((static_cast<uint32_t>(imageprop.samplerFilterFlags) &
                static_cast<uint32_t>(ZE_IMAGE_SAMPLER_FILTER_FLAG_POINT)) ==
               static_cast<uint32_t>(ZE_IMAGE_SAMPLER_FILTER_FLAG_POINT)) ||
              ((static_cast<uint32_t>(imageprop.samplerFilterFlags) &
                static_cast<uint32_t>(ZE_IMAGE_SAMPLER_FILTER_FLAG_LINEAR)) ==
               static_cast<uint32_t>(ZE_IMAGE_SAMPLER_FILTER_FLAG_LINEAR)));
}

enum class ImageSize { min, large };

class zeImageCreateTest
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

    ze_image_format_t format_descriptor = {
        layout,                    // layout
        format_type,               // type
        ZE_IMAGE_FORMAT_SWIZZLE_R, // x
        ZE_IMAGE_FORMAT_SWIZZLE_G, // y
        ZE_IMAGE_FORMAT_SWIZZLE_B, // z
        ZE_IMAGE_FORMAT_SWIZZLE_A  // w
    };
    ze_image_flag_t flags = (ze_image_flag_t)(image_rw_flag | image_cache_flag);
    size_t array_levels = 0;
    if (image_type == ZE_IMAGE_TYPE_1DARRAY) {
      array_levels = 1;
    }
    if (image_type == ZE_IMAGE_TYPE_2DARRAY) {
      array_levels = 2;
    }
    uint64_t width = 1;
    uint32_t height = 1, depth = 1;
    if (image_size == ImageSize::min) {
      width = lzt::image_widths[0];
      height = lzt::image_heights[0];
      depth = lzt::image_depths[0];
    } else if (image_size == ImageSize::large) {
      switch (image_type) {
      case ZE_IMAGE_TYPE_3D:
        depth = lzt::image_depths[1];
      case ZE_IMAGE_TYPE_2D:
      case ZE_IMAGE_TYPE_2DARRAY:
        height = lzt::image_heights[1];
      case ZE_IMAGE_TYPE_1D:
      case ZE_IMAGE_TYPE_1DARRAY:
        width = lzt::image_widths[1];
        break;
      }
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

LZT_TEST_P(zeImageCreateTest,
           GivenValidDescriptorWhenCreatingImageThenNotNullPointerIsReturned) {
  lzt::print_image_descriptor(image_descriptor);
  ze_image_handle_t image;
  SKIP_ZE_RESULT_UNSUPPORTED(
      zeImageCreate(context, device, &image_descriptor, &image));
  if (image) {
    lzt::destroy_ze_image(image);
  }
}

LZT_TEST_P(
    zeImageCreateTest,
    GivenValidDescriptorWhenCheckingImagePropertiesThenNotNullPointerIsReturned) {
  lzt::print_image_descriptor(image_descriptor);
  check_image_properties(lzt::get_ze_image_properties(image_descriptor));
}

INSTANTIATE_TEST_SUITE_P(
    UInt, zeImageCreateTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_UINT),
                       ::testing::ValuesIn(lzt::image_format_layout_uint),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    SInt, zeImageCreateTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_SINT),
                       ::testing::ValuesIn(lzt::image_format_layout_sint),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    UNorm, zeImageCreateTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_UNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_unorm),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    SNorm, zeImageCreateTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_SNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_snorm),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    Float, zeImageCreateTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_FLOAT),
                       ::testing::ValuesIn(lzt::image_format_layout_float),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

INSTANTIATE_TEST_SUITE_P(
    Media, zeImageCreateTest,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_FLOAT),
                       ::testing::ValuesIn(lzt::image_format_media_layouts),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(ImageSize::min, ImageSize::large)));

class zeImagePropertiesExtTests
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

LZT_TEST_P(zeImagePropertiesExtTests,
           GivenValidImageWhenGettingAllocPropertiesThenSuccessIsReturned) {
  ze_image_handle_t img = nullptr;
  SKIP_ZE_RESULT_UNSUPPORTED(zeImageCreate(context, device, &image_desc, &img));

  ze_image_allocation_ext_properties_t img_alloc_ext_properties = {};
  img_alloc_ext_properties.stype =
      ZE_STRUCTURE_TYPE_IMAGE_ALLOCATION_EXT_PROPERTIES;
  EXPECT_ZE_RESULT_SUCCESS(
      zeImageGetAllocPropertiesExt(context, img, &img_alloc_ext_properties));
  lzt::destroy_ze_image(img);
}

LZT_TEST_P(
    zeImagePropertiesExtTests,
    GivenValidImageWhenGettingMemoryPropertiesThenValidMemoryPropertiesIsReturned) {
  ze_image_handle_t img = nullptr;
  SKIP_ZE_RESULT_UNSUPPORTED(zeImageCreate(context, device, &image_desc, &img));

  ze_image_memory_properties_exp_t image_mem_properties = {};
  image_mem_properties.stype = ZE_STRUCTURE_TYPE_IMAGE_MEMORY_EXP_PROPERTIES;
  image_mem_properties.pNext = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(
      zeImageGetMemoryPropertiesExp(img, &image_mem_properties));
  EXPECT_GE(0u, image_mem_properties.rowPitch);
  EXPECT_GE(0u, image_mem_properties.size);
  EXPECT_GE(0u, image_mem_properties.slicePitch);
  lzt::destroy_ze_image(img);
}

INSTANTIATE_TEST_SUITE_P(
    UInt, zeImagePropertiesExtTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_UINT),
                       ::testing::ValuesIn(lzt::image_format_layout_uint),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));

INSTANTIATE_TEST_SUITE_P(
    SInt, zeImagePropertiesExtTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_SINT),
                       ::testing::ValuesIn(lzt::image_format_layout_sint),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));

INSTANTIATE_TEST_SUITE_P(
    UNorm, zeImagePropertiesExtTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_UNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_unorm),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));

INSTANTIATE_TEST_SUITE_P(
    SNorm, zeImagePropertiesExtTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_SNORM),
                       ::testing::ValuesIn(lzt::image_format_layout_snorm),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));

INSTANTIATE_TEST_SUITE_P(
    Float, zeImagePropertiesExtTests,
    ::testing::Combine(::testing::Values(ZE_IMAGE_FORMAT_TYPE_FLOAT),
                       ::testing::ValuesIn(lzt::image_format_layout_float),
                       ::testing::ValuesIn(lzt::image_rw_flags),
                       ::testing::ValuesIn(lzt::image_cache_flags),
                       ::testing::ValuesIn(lzt::image_types)));
} // namespace
