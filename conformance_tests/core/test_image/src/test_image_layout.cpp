/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <complex>

namespace lzt = level_zero_tests;

namespace {

enum TestType { IMAGE_OBJECT_ONLY, ONE_KERNEL_ONLY, TWO_KERNEL_CONVERT };

class ImageLayoutTests : public ::testing::Test {
public:
  void SetUp() override {
    if (!(lzt::image_support())) {
      LOG_INFO << "device does not support images, cannot run test";
      GTEST_SKIP();
    }
    module = lzt::create_module(lzt::zeDevice::get_instance()->get_device(),
                                "image_layout_tests.spv");
  }

  void run_test(ze_image_type_t image_type,
                ze_image_format_layout_t base_layout,
                ze_image_format_layout_t convert_layout,
                ze_image_format_type_t format_type, enum TestType test,
                bool is_immediate) {
    LOG_INFO << "RUN_TEST";
    LOG_INFO << "image type " << image_type;
    LOG_INFO << "base layout " << base_layout;
    LOG_INFO << "conver layout " << convert_layout;
    LOG_INFO << "format type " << format_type;
    ze_result_t result;
    cmd_bundle = lzt::create_command_bundle(is_immediate);
    std::string kernel_name = get_kernel(format_type);
    LOG_DEBUG << "kernel_name = " << kernel_name;
    ze_kernel_handle_t kernel = lzt::create_function(module, kernel_name);
    size_t image_width = 26;
    size_t image_height = 11;
    size_t image_depth = 1;
    size_t image_size;
    size_t bytes_per_pixel;
    // Make dimensions large enough to guarantee minimum of 256 bytes
    switch (image_type) {
    case ZE_IMAGE_TYPE_2D:
    case ZE_IMAGE_TYPE_2DARRAY:
      image_width = 26;
      image_height = 11;
      image_depth = 1;
      break;
    case ZE_IMAGE_TYPE_3D:
      image_width = 26;
      image_height = 5;
      image_depth = 3;
      break;
    default:
      image_width = 256;
      image_height = 1;
      image_depth = 1;
      break;
    }
    auto image_in =
        create_image_desc_layout(base_layout, image_type, format_type,
                                 image_width, image_height, image_depth);
    auto image_out =
        create_image_desc_layout(base_layout, image_type, format_type,
                                 image_width, image_height, image_depth);
    auto image_convert =
        create_image_desc_layout(convert_layout, image_type, format_type,
                                 image_width, image_height, image_depth);

    image_size = image_width * image_height * image_depth;
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
      result = zeKernelSuggestGroupSize(kernel, image_width, image_height,
                                        image_depth, &group_size_x,
                                        &group_size_y, &group_size_z);
      if (lzt::check_unsupported(result)) {
        return;
      }
      lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);
      result = zeKernelSetArgumentValue(kernel, 0, sizeof(image_in), &image_in);
      if (lzt::check_unsupported(result)) {
        return;
      }
      result = zeKernelSetArgumentValue(kernel, 1, sizeof(image_convert),
                                        &image_convert);
      if (lzt::check_unsupported(result)) {
        return;
      }

      ze_group_count_t group_dems = {
          static_cast<uint32_t>(image_width / group_size_x),
          static_cast<uint32_t>(image_height / group_size_y),
          static_cast<uint32_t>(image_depth / group_size_z)};

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
        result = zeKernelSuggestGroupSize(kernel, image_width, image_height,
                                          image_depth, &group_size_x,
                                          &group_size_y, &group_size_z);
        if (lzt::check_unsupported(result)) {
          return;
        }
        lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);
        result = zeKernelSetArgumentValue(kernel, 0, sizeof(image_convert),
                                          &image_convert);
        if (lzt::check_unsupported(result)) {
          return;
        }
        result =
            zeKernelSetArgumentValue(kernel, 1, sizeof(image_out), &image_out);
        if (lzt::check_unsupported(result)) {
          return;
        }

        group_dems = {static_cast<uint32_t>(image_width / group_size_x),
                      static_cast<uint32_t>(image_height / group_size_y),
                      static_cast<uint32_t>(image_depth / group_size_z)};

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
    if (lzt::image_support()) {
      lzt::destroy_module(module);
    }
  }

  ze_image_handle_t create_image_desc_layout(ze_image_format_layout_t layout,
                                             ze_image_type_t type,
                                             ze_image_format_type_t format,
                                             size_t image_width,
                                             size_t image_height,
                                             size_t image_depth);
  size_t get_components(ze_image_format_layout_t layout);
  size_t get_pixel_bytes(ze_image_format_layout_t layout);
  std::string get_kernel(ze_image_format_type_t format_type);
  lzt::zeCommandBundle cmd_bundle;
  ze_module_handle_t module;
};

size_t ImageLayoutTests::get_pixel_bytes(ze_image_format_layout_t layout) {
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

size_t ImageLayoutTests::get_components(ze_image_format_layout_t layout) {
  size_t components = 1;
  switch (layout) {
  case ZE_IMAGE_FORMAT_LAYOUT_8:
  case ZE_IMAGE_FORMAT_LAYOUT_16:
  case ZE_IMAGE_FORMAT_LAYOUT_32:
    components = 1;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_8_8:
  case ZE_IMAGE_FORMAT_LAYOUT_16_16:
  case ZE_IMAGE_FORMAT_LAYOUT_32_32:
    components = 2;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_11_11_10:
  case ZE_IMAGE_FORMAT_LAYOUT_5_6_5:
    components = 3;
    break;
  case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8:
  case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16:
  case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32:
  case ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2:
  case ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1:
  case ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4:
    components = 4;
    break;
  default:
    break;
  }
  return components;
}

std::string ImageLayoutTests::get_kernel(ze_image_format_type_t format_type) {
  std::string kernel;
  switch (format_type) {
  case ZE_IMAGE_FORMAT_TYPE_UNORM:
  case ZE_IMAGE_FORMAT_TYPE_SNORM:
  case ZE_IMAGE_FORMAT_TYPE_FLOAT:
    kernel = "copy_float_image";
    break;
  case ZE_IMAGE_FORMAT_TYPE_SINT:
    kernel = "copy_sint_image";
    break;
  default:
    kernel = "copy_uint_image";
    break;
  }
  return kernel;
}

ze_image_handle_t ImageLayoutTests::create_image_desc_layout(
    ze_image_format_layout_t layout, ze_image_type_t type,
    ze_image_format_type_t format, size_t image_width, size_t image_height,
    size_t image_depth) {
  ze_image_desc_t image_desc = {};
  size_t components = get_components(layout);
  image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  ze_image_handle_t image;

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

  image_desc.width = image_width;
  image_desc.height = image_height;
  image_desc.depth = image_depth;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(lzt::get_default_context(),
                          lzt::zeDevice::get_instance()->get_device(),
                          &image_desc, &image));
  EXPECT_NE(nullptr, image);

  return image;
}

void RunGivenImageLayoutWhenConvertingImageToMemoryTest(ImageLayoutTests &test,
                                                        bool is_immediate) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto format_type = {ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_TYPE_SINT,
                      ZE_IMAGE_FORMAT_TYPE_UNORM, ZE_IMAGE_FORMAT_TYPE_SNORM};
  auto layout_type = {
      ZE_IMAGE_FORMAT_LAYOUT_8,           ZE_IMAGE_FORMAT_LAYOUT_8_8,
      ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,     ZE_IMAGE_FORMAT_LAYOUT_16,
      ZE_IMAGE_FORMAT_LAYOUT_16_16,       ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
      ZE_IMAGE_FORMAT_LAYOUT_32,          ZE_IMAGE_FORMAT_LAYOUT_32_32,
      ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2};
  LOG_DEBUG << "IMAGE OBJECT ONLY TESTS: UINT, SINT, UNORM AND SNORM";
  for (auto image : image_type) {
    for (auto format : format_type) {
      for (auto layout : layout_type) {
        test.run_test(image, layout, layout, format, IMAGE_OBJECT_ONLY,
                      is_immediate);
      }
    }
  }
  auto layout_type_unorm = {ZE_IMAGE_FORMAT_LAYOUT_5_6_5,
                            ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1,
                            ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4};
  for (auto image : image_type) {
    for (auto layout : layout_type_unorm) {
      test.run_test(image, layout, layout, ZE_IMAGE_FORMAT_TYPE_UNORM,
                    IMAGE_OBJECT_ONLY, is_immediate);
    }
  }

  auto layout_type_float = {
      ZE_IMAGE_FORMAT_LAYOUT_16,          ZE_IMAGE_FORMAT_LAYOUT_16_16,
      ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_LAYOUT_32,
      ZE_IMAGE_FORMAT_LAYOUT_32_32,       ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32,
      ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,  ZE_IMAGE_FORMAT_LAYOUT_11_11_10};
  LOG_DEBUG << "IMAGE OBJECT ONLY TESTS: FLOAT";
  for (auto image : image_type) {
    for (auto layout : layout_type_float) {
      test.run_test(image, layout, layout, ZE_IMAGE_FORMAT_TYPE_FLOAT,
                    IMAGE_OBJECT_ONLY, is_immediate);
    }
  }
}

TEST_F(ImageLayoutTests,
       GivenImageLayoutWhenConvertingImageToMemoryThenBufferResultIsCorrect) {
  RunGivenImageLayoutWhenConvertingImageToMemoryTest(*this, false);
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenConvertingImageToMemoryOnImmediateCmdListThenBufferResultIsCorrect) {
  RunGivenImageLayoutWhenConvertingImageToMemoryTest(*this, true);
}

void RunGivenImageLayoutWhenPassingIntImageThroughKernelAndConvertingImageToMemoryTest(
    ImageLayoutTests &test, bool is_immediate) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto format_type = {ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_TYPE_SINT};
  auto layout_type = {ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                      ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_LAYOUT_16,
                      ZE_IMAGE_FORMAT_LAYOUT_16_16,
                      ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
                      ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_LAYOUT_32_32,
                      ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32,
                      // may result in hang:
                      ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2};
  LOG_DEBUG << "ONE KERNEL ONLY TESTS: UINT, SINT";
  for (auto image : image_type) {
    for (auto format : format_type) {
      for (auto layout : layout_type) {
        test.run_test(image, layout, layout, format, ONE_KERNEL_ONLY,
                      is_immediate);
      }
    }
  }
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingIntImageThroughKernelAndConvertingImageToMemoryThenBufferResultIsCorrect) {
  RunGivenImageLayoutWhenPassingIntImageThroughKernelAndConvertingImageToMemoryTest(
      *this, false);
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingIntImageThroughKernelAndConvertingImageToMemoryOnImmediateCmdListThenBufferResultIsCorrect) {
  RunGivenImageLayoutWhenPassingIntImageThroughKernelAndConvertingImageToMemoryTest(
      *this, false);
}

void RunGivenImageLayoutWhenPassingNormImageThroughKernelAndConvertingImageToMemoryTest(
    ImageLayoutTests &test, bool is_immediate) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto format_type = {ZE_IMAGE_FORMAT_TYPE_UNORM, ZE_IMAGE_FORMAT_TYPE_SNORM};
  auto layout_type = {ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                      ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_LAYOUT_16,
                      ZE_IMAGE_FORMAT_LAYOUT_16_16,
                      ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
                      // may result in hang:
                      ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_LAYOUT_32_32,
                      ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32};
  LOG_DEBUG << "ONE KERNEL ONLY TESTS: UNORM, SNORM";
  for (auto image : image_type) {
    for (auto format : format_type) {
      for (auto layout : layout_type) {
        test.run_test(image, layout, layout, format, ONE_KERNEL_ONLY,
                      is_immediate);
      }
    }
  }
  auto layout_type_unorm = {ZE_IMAGE_FORMAT_LAYOUT_5_6_5,
                            ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1,
                            ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4};
  for (auto image : image_type) {
    for (auto layout : layout_type_unorm) {
      test.run_test(image, layout, layout, ZE_IMAGE_FORMAT_TYPE_UNORM,
                    ONE_KERNEL_ONLY, is_immediate);
    }
  }
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingNormImageThroughKernelAndConvertingImageToMemoryThenBufferResultIsCorrect) {
  RunGivenImageLayoutWhenPassingNormImageThroughKernelAndConvertingImageToMemoryTest(
      *this, false);
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingNormImageThroughKernelAndConvertingImageToMemoryOnImmediateCmdListThenBufferResultIsCorrect) {
  RunGivenImageLayoutWhenPassingNormImageThroughKernelAndConvertingImageToMemoryTest(
      *this, true);
}

void RunGivenImageLayoutWhenPassingFloatImageThroughKernelAndConvertingImageToMemoryTest(
    ImageLayoutTests &test, bool is_immediate) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto layout_type = {
      ZE_IMAGE_FORMAT_LAYOUT_16,          ZE_IMAGE_FORMAT_LAYOUT_16_16,
      ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_LAYOUT_32,
      ZE_IMAGE_FORMAT_LAYOUT_32_32,       ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32,
      ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,  ZE_IMAGE_FORMAT_LAYOUT_11_11_10};
  LOG_DEBUG << "ONE KERNEL ONLY TESTS: FLOAT";
  for (auto image : image_type) {
    for (auto layout : layout_type) {
      test.run_test(image, layout, layout, ZE_IMAGE_FORMAT_TYPE_FLOAT,
                    ONE_KERNEL_ONLY, is_immediate);
    }
  }
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingFloatImageThroughKernelAndConvertingImageToMemoryThenBufferResultIsCorrect) {
  RunGivenImageLayoutWhenPassingFloatImageThroughKernelAndConvertingImageToMemoryTest(
      *this, false);
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingFloatImageThroughKernelAndConvertingImageToMemoryOnImmediateCmdListThenBufferResultIsCorrect) {
  RunGivenImageLayoutWhenPassingFloatImageThroughKernelAndConvertingImageToMemoryTest(
      *this, true);
}

void RunGivenIntImageLayoutWhenKernelConvertingImageTest(ImageLayoutTests &test,
                                                         bool is_immediate) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto format_type = {ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_TYPE_SINT};
  LOG_DEBUG << "TWO_KERNEL_CONVERT TESTS: UINT AND SINT";
  for (auto image : image_type) {
    for (auto format : format_type) {
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_16,
                    format, TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_32,
                    format, TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_LAYOUT_32,
                    format, TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                    ZE_IMAGE_FORMAT_LAYOUT_16_16, format, TWO_KERNEL_CONVERT,
                    is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                    ZE_IMAGE_FORMAT_LAYOUT_32_32, format, TWO_KERNEL_CONVERT,
                    is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16_16,
                    ZE_IMAGE_FORMAT_LAYOUT_32_32, format, TWO_KERNEL_CONVERT,
                    is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                    ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format,
                    TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                    TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
                    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                    TWO_KERNEL_CONVERT, is_immediate);
    }
  }
  for (auto image : image_type) {
    test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                  ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_UINT,
                  TWO_KERNEL_CONVERT, is_immediate);
    test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                  ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_TYPE_UINT,
                  TWO_KERNEL_CONVERT, is_immediate);
#ifdef DEBUG_HANG
    test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                  ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_SINT,
                  TWO_KERNEL_CONVERT, is_immediate);
    test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
                  ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_TYPE_SINT,
                  TWO_KERNEL_CONVERT, is_immediate);
#endif
  }
}

TEST_F(
    ImageLayoutTests,
    GivenIntImageLayoutWhenKernelConvertingImageThenReverseKernelConvertedResultIsCorrect) {
  RunGivenIntImageLayoutWhenKernelConvertingImageTest(*this, false);
}

TEST_F(
    ImageLayoutTests,
    GivenIntImageLayoutWhenKernelConvertingImageOnImmediateCmdListThenReverseKernelConvertedResultIsCorrect) {
  RunGivenIntImageLayoutWhenKernelConvertingImageTest(*this, true);
}

void RunGivenNormImageLayoutWhenKernelConvertingImageTest(
    ImageLayoutTests &test, bool is_immediate) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto format_type = {ZE_IMAGE_FORMAT_TYPE_UNORM, ZE_IMAGE_FORMAT_TYPE_SNORM};
  LOG_DEBUG << "TWO_KERNEL_CONVERT TESTS: UNORM AND SNORM";
  for (auto image : image_type) {
    for (auto format : format_type) {
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_16,
                    format, TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                    ZE_IMAGE_FORMAT_LAYOUT_16_16, format, TWO_KERNEL_CONVERT,
                    is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                    ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format,
                    TWO_KERNEL_CONVERT, is_immediate);
#ifdef DEBUG_HANG
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_32,
                    format, TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8,
                    ZE_IMAGE_FORMAT_LAYOUT_32_32, format, TWO_KERNEL_CONVERT,
                    is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_LAYOUT_32,
                    format, TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16_16,
                    ZE_IMAGE_FORMAT_LAYOUT_32_32, format, TWO_KERNEL_CONVERT,
                    is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                    TWO_KERNEL_CONVERT, is_immediate);
      test.run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
                    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format,
                    TWO_KERNEL_CONVERT, is_immediate);
#endif
    }
  }
}

TEST_F(
    ImageLayoutTests,
    GivenNormImageLayoutWhenKernelConvertingImageThenReverseKernelConvertedResultIsCorrect) {
  RunGivenNormImageLayoutWhenKernelConvertingImageTest(*this, false);
}

TEST_F(
    ImageLayoutTests,
    GivenNormImageLayoutWhenKernelConvertingImageOnImmediateCmdListThenReverseKernelConvertedResultIsCorrect) {
  RunGivenNormImageLayoutWhenKernelConvertingImageTest(*this, true);
}

} // namespace
