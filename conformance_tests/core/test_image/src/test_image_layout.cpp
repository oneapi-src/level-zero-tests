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

enum TestType { IMAGE_OBJECT_ONLY, ONE_KERNEL_ONLY, TWO_KERNEL_CONVERT };

class ImageLayoutTests : public ::testing::Test {
protected:
  ImageLayoutTests() {
    module = lzt::create_module(lzt::zeDevice::get_instance()->get_device(),
                                "image_layout_tests.spv");
  }

  void run_test(ze_image_type_t image_type,
                ze_image_format_layout_t base_layout,
                ze_image_format_layout_t convert_layout,
                ze_image_format_type_t format_type, enum TestType test) {
    LOG_INFO << "RUN_TEST";
    LOG_INFO << "image type " << image_type;
    LOG_INFO << "base layout " << base_layout;
    LOG_INFO << "conver layout " << convert_layout;
    LOG_INFO << "format type " << format_type;
    ze_result_t result;
    command_list = lzt::create_command_list();
    command_queue = lzt::create_command_queue();
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

    uint8_t *ptr1 = static_cast<uint8_t *>(buffer_in);
    uint8_t *ptr2 = static_cast<uint8_t *>(buffer_out);
    for (size_t i = 0; i < buffer_size; ++i) {
      ptr1[i] = static_cast<uint8_t>((0xff) - (i & 0xff));
      ptr2[i] = 0xff;
    }

    // copy input buff to image_in object
    lzt::append_image_copy_from_mem(command_list, image_in, buffer_in, nullptr);
    lzt::append_barrier(command_list);
    if (test == IMAGE_OBJECT_ONLY) {
      LOG_DEBUG << "IMAGE_OBJECT_ONLY";
      lzt::append_image_copy_to_mem(command_list, buffer_out, image_in,
                                    nullptr);
      lzt::append_barrier(command_list);
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

      lzt::append_launch_function(command_list, kernel, &group_dems, nullptr, 0,
                                  nullptr);
      lzt::append_barrier(command_list);

      if (test == ONE_KERNEL_ONLY) {
        LOG_DEBUG << "ONE_KERNEL_ONLY";
        lzt::append_image_copy_to_mem(command_list, buffer_out, image_convert,
                                      nullptr);
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

        lzt::append_launch_function(command_list, kernel, &group_dems, nullptr,
                                    0, nullptr);
        // finalize, copy to buffer_out
        lzt::append_barrier(command_list);
        lzt::append_image_copy_to_mem(command_list, buffer_out, image_out,
                                      nullptr);
      }
      LOG_DEBUG << "group_size_x = " << group_size_x
                << " group_size_y = " << group_size_y
                << " group_size_z = " << group_size_z;
    }
    lzt::close_command_list(command_list);
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);
    LOG_DEBUG << "buffer size = " << buffer_size;

    // compare results
    EXPECT_EQ(memcmp(buffer_in, buffer_out, buffer_size), 0);
    lzt::free_memory(buffer_in);
    lzt::free_memory(buffer_out);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }

  ~ImageLayoutTests() { lzt::destroy_module(module); }

  ze_image_handle_t create_image_desc_layout(ze_image_format_layout_t layout,
                                             ze_image_type_t type,
                                             ze_image_format_type_t format,
                                             size_t image_width,
                                             size_t image_height,
                                             size_t image_depth);
  size_t get_components(ze_image_format_layout_t layout);
  size_t get_pixel_bytes(ze_image_format_layout_t layout);
  std::string get_kernel(ze_image_format_type_t format_type);
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue;
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

TEST_F(ImageLayoutTests,
       GivenImageLayoutWhenConvertingImageToMemoryThenBufferResultIsCorrect) {
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
        run_test(image, layout, layout, format, IMAGE_OBJECT_ONLY);
      }
    }
  }
  auto layout_type_unorm = {ZE_IMAGE_FORMAT_LAYOUT_5_6_5,
                            ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1,
                            ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4};
  for (auto image : image_type) {
    for (auto layout : layout_type_unorm) {
      run_test(image, layout, layout, ZE_IMAGE_FORMAT_TYPE_UNORM,
               IMAGE_OBJECT_ONLY);
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
      run_test(image, layout, layout, ZE_IMAGE_FORMAT_TYPE_FLOAT,
               IMAGE_OBJECT_ONLY);
    }
  }
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingIntImageThroughKernelAndConvertingImageToMemoryThenBufferResultIsCorrect) {
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
        run_test(image, layout, layout, format, ONE_KERNEL_ONLY);
      }
    }
  }
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingNormImageThroughKernelAndConvertingImageToMemoryThenBufferResultIsCorrect) {
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
        run_test(image, layout, layout, format, ONE_KERNEL_ONLY);
      }
    }
  }
  auto layout_type_unorm = {ZE_IMAGE_FORMAT_LAYOUT_5_6_5,
                            ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1,
                            ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4};
  for (auto image : image_type) {
    for (auto layout : layout_type_unorm) {
      run_test(image, layout, layout, ZE_IMAGE_FORMAT_TYPE_UNORM,
               ONE_KERNEL_ONLY);
    }
  }
}

TEST_F(
    ImageLayoutTests,
    GivenImageLayoutWhenPassingFloatImageThroughKernelAndConvertingImageToMemoryThenBufferResultIsCorrect) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto layout_type = {
      ZE_IMAGE_FORMAT_LAYOUT_16,          ZE_IMAGE_FORMAT_LAYOUT_16_16,
      ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_LAYOUT_32,
      ZE_IMAGE_FORMAT_LAYOUT_32_32,       ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32,
      ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,  ZE_IMAGE_FORMAT_LAYOUT_11_11_10};
  LOG_DEBUG << "ONE KERNEL ONLY TESTS: FLOAT";
  for (auto image : image_type) {
    for (auto layout : layout_type) {
      run_test(image, layout, layout, ZE_IMAGE_FORMAT_TYPE_FLOAT,
               ONE_KERNEL_ONLY);
    }
  }
}

TEST_F(
    ImageLayoutTests,
    GivenIntImageLayoutWhenKernelConvertingImageThenReverseKernelConvertedResultIsCorrect) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto format_type = {ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_TYPE_SINT};
  LOG_DEBUG << "TWO_KERNEL_CONVERT TESTS: UINT AND SINT";
  for (auto image : image_type) {
    for (auto format : format_type) {
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_16,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_32,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_LAYOUT_32,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8, ZE_IMAGE_FORMAT_LAYOUT_16_16,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8, ZE_IMAGE_FORMAT_LAYOUT_32_32,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16_16,
               ZE_IMAGE_FORMAT_LAYOUT_32_32, format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
               ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
               ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
               ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format, TWO_KERNEL_CONVERT);
    }
  }
  for (auto image : image_type) {
    run_test(image, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
             ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_UINT,
             TWO_KERNEL_CONVERT);
    run_test(image, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
             ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_TYPE_UINT,
             TWO_KERNEL_CONVERT);
#ifdef DEBUG_HANG
    run_test(image, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
             ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_SINT,
             TWO_KERNEL_CONVERT);
    run_test(image, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
             ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_TYPE_SINT,
             TWO_KERNEL_CONVERT);
#endif
  }
}

TEST_F(
    ImageLayoutTests,
    GivenNormImageLayoutWhenKernelConvertingImageThenReverseKernelConvertedResultIsCorrect) {
  auto image_type = {ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_2D, ZE_IMAGE_TYPE_3D};
  auto format_type = {ZE_IMAGE_FORMAT_TYPE_UNORM, ZE_IMAGE_FORMAT_TYPE_SNORM};
  LOG_DEBUG << "TWO_KERNEL_CONVERT TESTS: UNORM AND SNORM";
  for (auto image : image_type) {
    for (auto format : format_type) {
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_16,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8, ZE_IMAGE_FORMAT_LAYOUT_16_16,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
               ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, format, TWO_KERNEL_CONVERT);
#ifdef DEBUG_HANG
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8, ZE_IMAGE_FORMAT_LAYOUT_32,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8, ZE_IMAGE_FORMAT_LAYOUT_32_32,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_LAYOUT_32,
               format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16_16,
               ZE_IMAGE_FORMAT_LAYOUT_32_32, format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
               ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format, TWO_KERNEL_CONVERT);
      run_test(image, ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
               ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, format, TWO_KERNEL_CONVERT);
#endif
    }
  }
}

} // namespace