/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <array>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

using namespace level_zero_tests;

namespace {

class zeCommandListAppendImageCopyTests : public ::testing::Test {
public:
  zeCommandListAppendImageCopyTests() {
    if (!(lzt::image_support())) {
      LOG_INFO << "device does not support images, cannot run test";
      return;
    }
    png_img_src = ImagePNG32Bit("test_input.png");
    image_width = png_img_src.width();
    image_height = png_img_src.height();
    image_size = image_width * image_height * sizeof(uint32_t);
    png_img_dest = ImagePNG32Bit(image_width, image_height);

    ze_event_pool_desc_t desc = {};
    desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    desc.flags = 0;
    desc.count = 10;
    const ze_context_handle_t context = lzt::get_default_context();
    ep = lzt::create_event_pool(context, desc);

    ze_image_desc_t img_desc = {};
    img_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
    img_desc.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
    img_desc.type = ZE_IMAGE_TYPE_2D;
    img_desc.format = {
        ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_TYPE_UNORM,
        ZE_IMAGE_FORMAT_SWIZZLE_R,      ZE_IMAGE_FORMAT_SWIZZLE_G,
        ZE_IMAGE_FORMAT_SWIZZLE_B,      ZE_IMAGE_FORMAT_SWIZZLE_A};
    img_desc.width = image_width;
    img_desc.height = image_height;
    img_desc.depth = 1;
    img_desc.arraylevels = 0;
    img_desc.miplevels = 0;

    ze_img_src = lzt::create_ze_image(img_desc);
    ze_img_dest = lzt::create_ze_image(img_desc);
  }

  void TearDown() override {
    if (!(lzt::image_support())) {
      return;
    }
    lzt::destroy_event_pool(ep);
    lzt::destroy_ze_image(ze_img_src);
    lzt::destroy_ze_image(ze_img_dest);
  }

  ze_event_pool_handle_t ep;
  uint32_t image_width;
  uint32_t image_height;
  uint32_t image_size;
  ze_image_handle_t ze_img_src, ze_img_dest;
  lzt::ImagePNG32Bit png_img_src, png_img_dest;
  lzt::zeImageCreateCommon *img_ptr = nullptr;

  void test_image_copy(bool is_immediate) {
    auto cmd_bundle = lzt::create_command_bundle(is_immediate);
    img_ptr = new zeImageCreateCommon;
    lzt::append_image_copy_from_mem(cmd_bundle.list, ze_img_src,
                                    png_img_src.raw_data(), nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy(cmd_bundle.list, ze_img_dest, ze_img_src, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy_to_mem(cmd_bundle.list, png_img_dest.raw_data(),
                                  ze_img_dest, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(png_img_src, png_img_dest);
    delete img_ptr;
    lzt::destroy_command_bundle(cmd_bundle);
  }

  void test_image_copy_ext(bool is_immediate) {
    auto cmd_bundle = lzt::create_command_bundle(is_immediate);

    uint32_t *padded_buffer_src =
        (uint32_t *)malloc(image_width * 2 * image_height * sizeof(uint32_t));
    uint32_t *padded_buffer_dst =
        (uint32_t *)malloc(image_width * 2 * image_height * sizeof(uint32_t));
    memset(padded_buffer_src, 0,
           image_width * 2 * image_height * sizeof(uint32_t));
    memset(padded_buffer_dst, 0,
           image_width * 2 * image_height * sizeof(uint32_t));

    copy_raw_image_data_to_padded_buffer(padded_buffer_src,
                                         png_img_src.raw_data(), image_width,
                                         image_height, image_width * 2);

    lzt::append_image_copy_from_mem_ext(cmd_bundle.list, ze_img_src,
                                        padded_buffer_src, image_width * 2, 0,
                                        nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy(cmd_bundle.list, ze_img_dest, ze_img_src, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy_to_mem_ext(cmd_bundle.list, padded_buffer_dst,
                                      ze_img_dest, image_width * 2, 0, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(true,
              std::equal(padded_buffer_src,
                         padded_buffer_src + (image_height * 2 * image_height),
                         padded_buffer_dst));

    free(padded_buffer_dst);
    free(padded_buffer_src);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  void copy_raw_image_data_to_padded_buffer(uint32_t *padded_buffer,
                                            uint32_t *src_image_data,
                                            uint32_t image_width,
                                            uint32_t image_height,
                                            uint32_t row_pitch) {
    if (image_width <= row_pitch) {
      for (int row = 0; row < image_height; row++) {
        std::copy(padded_buffer + (row * row_pitch),
                  padded_buffer + (row * row_pitch) + image_width,
                  src_image_data + (row * image_width));
      }
    }
  }

  void test_image_copy_region(const ze_image_region_t *region,
                              bool is_immediate) {
    auto cmd_bundle = lzt::create_command_bundle(is_immediate);
    img_ptr = new zeImageCreateCommon;
    ze_image_handle_t h_dest_image = nullptr;
    ze_image_desc_t image_desc = zeImageCreateCommon::dflt_ze_image_desc;
    h_dest_image = lzt::create_ze_image(image_desc);
    ze_image_handle_t h_source_image = nullptr;
    h_source_image = lzt::create_ze_image(image_desc);
    // Define the three data patterns:
    const int8_t background_dp = 1;
    const int8_t foreground_dp = 2;
    const int8_t scribble_dp = 3;
    // The background image:
    lzt::ImagePNG32Bit background_image(img_ptr->dflt_host_image_.width(),
                                        img_ptr->dflt_host_image_.height());
    // Initialize background image with background data pattern:
    lzt::write_image_data_pattern(background_image, background_dp);
    // The foreground image:
    lzt::ImagePNG32Bit foreground_image(img_ptr->dflt_host_image_.width(),
                                        img_ptr->dflt_host_image_.height());
    // Initialize foreground image with foreground data pattern:
    lzt::write_image_data_pattern(foreground_image, foreground_dp);
    // new_host_image is used to validate that the image copy region
    // operation(s) were correct:
    lzt::ImagePNG32Bit new_host_image(img_ptr->dflt_host_image_.width(),
                                      img_ptr->dflt_host_image_.height());
    // Scribble a known incorrect data pattern to new_host_image to ensure we
    // are validating actual data from the L0 functionality:
    lzt::write_image_data_pattern(new_host_image, scribble_dp);
    // First, copy the background image from the host to the device:
    // This will serve as the BACKGROUND of the image.
    lzt::append_image_copy_from_mem(cmd_bundle.list, h_dest_image,
                                    background_image.raw_data(), nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    // Next, copy the foreground image from the host to the device:
    // This will serve as the FOREGROUND of the image.
    lzt::append_image_copy_from_mem(cmd_bundle.list, h_source_image,
                                    foreground_image.raw_data(), nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    // Copy the portion of the foreground image correspoding to the region:
    lzt::append_image_copy_region(cmd_bundle.list, h_dest_image, h_source_image,
                                  region, region, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    // Finally, copy the image in hTstImage back to new_host_image for
    // validation:
    lzt::append_image_copy_to_mem(cmd_bundle.list, new_host_image.raw_data(),
                                  h_dest_image, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    // Execute all of the commands involving copying of images
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(0, lzt::compare_data_pattern(new_host_image, region,
                                           foreground_image, background_image));

    lzt::destroy_ze_image(h_dest_image);
    lzt::destroy_ze_image(h_source_image);
    delete img_ptr;
    lzt::destroy_command_bundle(cmd_bundle);
  }

  void test_image_mem_copy_no_regions(void *source_buff, void *dest_buff,
                                      bool is_immediate) {
    auto cmd_bundle = lzt::create_command_bundle(is_immediate);

    // Copies proceeds as follows:
    // png -> source_buff -> image -> dest_buff ->png

    lzt::append_memory_copy(cmd_bundle.list, source_buff,
                            png_img_src.raw_data(), image_size);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy_from_mem(cmd_bundle.list, ze_img_src, source_buff,
                                    nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy_to_mem(cmd_bundle.list, dest_buff, ze_img_src,
                                  nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_memory_copy(cmd_bundle.list, png_img_dest.raw_data(), dest_buff,
                            image_size);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(png_img_src, png_img_dest);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  void test_image_mem_copy_use_regions(void *source_buff_bot,
                                       void *source_buff_top,
                                       void *dest_buff_bot, void *dest_buff_top,
                                       bool is_immediate) {
    auto cmd_bundle = lzt::create_command_bundle(is_immediate);

    ze_image_region_t bot_region = {0, 0, 0, image_width, image_height / 2, 1};
    ze_image_region_t top_region = {0,           image_height / 2, 0,
                                    image_width, image_height / 2, 1};

    // Copies proceeds as follows:
    // png -> source_buff -> image -> dest_buff ->png

    lzt::append_memory_copy(cmd_bundle.list, source_buff_bot,
                            png_img_src.raw_data(), image_size / 2);
    lzt::append_memory_copy(cmd_bundle.list, source_buff_top,
                            png_img_src.raw_data() + image_size / 4 / 2,
                            image_size / 2);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy_from_mem(cmd_bundle.list, ze_img_src,
                                    source_buff_bot, bot_region, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy_from_mem(cmd_bundle.list, ze_img_src,
                                    source_buff_top, top_region, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

    lzt::append_image_copy_to_mem(cmd_bundle.list, dest_buff_bot, ze_img_src,
                                  bot_region, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy_to_mem(cmd_bundle.list, dest_buff_top, ze_img_src,
                                  top_region, nullptr);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_memory_copy(cmd_bundle.list, png_img_dest.raw_data(),
                            dest_buff_bot, image_size / 2);
    lzt::append_memory_copy(cmd_bundle.list,
                            png_img_dest.raw_data() + image_size / 4 / 2,
                            dest_buff_top, image_size / 2);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    for (int i = 0; i < (image_size / 4); i++) {
      ASSERT_EQ(png_img_src.raw_data()[i], png_img_dest.raw_data()[i])
          << "i is " << i;
    }

    EXPECT_EQ(png_img_src, png_img_dest);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  void image_region_copy(const ze_image_region_t &in_region,
                         const ze_image_region_t &out_region,
                         bool is_immediate) {
    auto cmd_bundle = lzt::create_command_bundle(is_immediate);
    // Create and initialize input and output images.
    lzt::ImagePNG32Bit in_image =
        lzt::ImagePNG32Bit(in_region.width, in_region.height);
    lzt::ImagePNG32Bit out_image =
        lzt::ImagePNG32Bit(out_region.width, out_region.height);

    for (auto y = 0; y < in_region.height; y++)
      for (auto x = 0; x < in_region.width; x++)
        in_image.set_pixel(x, y, x + (y * in_region.width));

    for (auto y = 0; y < out_region.height; y++)
      for (auto x = 0; x < out_region.width; x++)
        out_image.set_pixel(x, y, 0xffffffff);

    // Copy from host image to to device image region
    lzt::append_image_copy_from_mem(cmd_bundle.list,
                                    img_ptr->dflt_device_image_,
                                    in_image.raw_data(), in_region, nullptr);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

    // Copy from image region to output host image
    lzt::append_image_copy_to_mem(cmd_bundle.list, out_image.raw_data(),
                                  img_ptr->dflt_device_image_, out_region,
                                  nullptr);

    // Execute
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    // Verify output image matches initial host image.
    // Output image contains input image data shifted by in_region's origin
    // minus out_region's origin.  Some of the original data may not make it
    // to the output due to sizes and offests, and there may be junk data in
    // parts of the output image that don't have coresponding pixels in the
    // input; we will ignore those.
    // We may pass negative origin coordinates to compare_data_pattern; in that
    // case, it will skip over any negative-index pixels.
    EXPECT_EQ(0, lzt::compare_data_pattern(
                     in_image, out_image, 0, 0, in_region.width,
                     in_region.height, in_region.originX - out_region.originX,
                     in_region.originY - out_region.originY, out_region.width,
                     out_region.height));
    lzt::destroy_command_bundle(cmd_bundle);
  }
};

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  test_image_copy(false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyOnImmediateCmdListThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  test_image_copy(true);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyWithCopyExtThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  test_image_copy_ext(false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyWithCopyExtOnImmediateCmdListThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  test_image_copy_ext(true);
}

static inline ze_image_region_t init_region(uint32_t originX, uint32_t originY,
                                            uint32_t originZ, uint32_t width,
                                            uint32_t height, uint32_t depth) {
  ze_image_region_t rv = {originX, originY, originZ, width, height, depth};
  return rv;
}

void RunGivenDeviceImageAndHostImageWhenAppendingImageCopyRegionWithVariousRegionsTest(
    zeCommandListAppendImageCopyTests &test, bool is_immediate) {
  LOG_DEBUG << "Starting test of nullptr region." << std::endl;
  test.test_image_copy_region(nullptr, is_immediate);
  LOG_DEBUG << "Completed test of nullptr region" << std::endl;
  // Aliases to reduce widths of the following region initializers
  const uint32_t width = test.img_ptr->dflt_host_image_.width();
  const uint32_t height = test.img_ptr->dflt_host_image_.height();
  ze_image_region_t regions[] = {
      // Region correspond to the entire image (C 2) (0)
      init_region(0, 0, 0, width, height, 1),

      // Entire image less 1 pixel in height, top (region touches 3 out of 4
      // borders): (C 3) (1)  Does not touch the bottom
      init_region(0, 0, 0, width, height - 1, 1),
      // Entire image less 1 pixel in height, bottom (region touches 3 out of 4
      // borders): (C 4) (2) Does not touch the top
      init_region(0, 1, 0, width, height - 1, 1),
      // Entire image less 1 pixel in width, left (region touches 3 out of 4
      // borders): (C 5) (3) Does not touch the right
      init_region(0, 0, 0, width - 1, height, 1),
      // Entire image less 1 pixel in width, right (region touches 3 out of 4
      // borders): (C 6) (4) Does not touch the left
      init_region(1, 0, 0, width - 1, height, 1),

      // Entire image less (1 pixel in width and 1 pixel in height), top, left
      // (region touches 2 out of 4 borders): (C 7) (5)
      init_region(0, 0, 0, width - 1, height - 1, 1),
      // Entire image less (1 pixel in width and 1 pixel in height), top, right
      // (region touches 2 out of 4 borders): (C 8) (6)
      init_region(1, 0, 0, width - 1, height - 1, 1),
      // Entire image less (1 pixel in width and 1 pixel in height), bottom,
      // right (region touches 2 out of 4 borders): (C 9) (7)
      init_region(1, 1, 0, width - 1, height - 1, 1),
      // Entire image less (1 pixel in width and 1 pixel in height), bottom,
      // left (region touches 2 out of 4 borders): (C 10)
      init_region(0, 1, 0, width - 1, height - 1, 1),
      // Entire image less 2 pixels in width, top, bottom (region touches 2 out
      // of 4 borders): (C 11)
      init_region(0, 1, 0, width - 2, height, 1),
      // Entire image less 2 pixels in height, right, left (region touches 2 out
      // of 4 borders): (C 12)
      init_region(1, 0, 0, width, height - 2, 1),

      // Entire image less (2 pixels in width and 1 pixel in height), touches
      // only top border (region touches 1 out of 4 borders): (C 13)
      init_region(1, 0, 0, width - 2, height - 1, 1),
      // Entire image less (2 pixels in width and 1 pixel in height), touches
      // only bottom border (region touches 1 out of 4 borders): (C 14)
      init_region(1, 1, 0, width - 2, height - 1, 1),
      // Entire image less (1 pixel in width and 2 pixels in height), touches
      // only left border (region touches 1 out of 4 borders): (C 15)
      init_region(0, 1, 0, width - 1, height - 2, 1),
      // Entire image less (1 pixel in width and 2 pixels in height), touches
      // only right border (region touches 1 out of 4 borders): (C 16)
      init_region(1, 1, 0, width - 1, height - 2, 1),

      // Entire image less (2 pixels in width and 2 pixels in height), centered
      // (region touches no borders): (C 17)
      init_region(1, 1, 0, width - 2, height - 2, 1),
      // Column, with width 1, left-most: (C 18)
      init_region(0, 0, 0, 1, height, 1),
      // Column, with width 1, right-most: (C 19)
      init_region(width - 1, 0, 0, 1, height, 1),
      // Column, with width 1, center: (C 20)
      init_region(width / 2, 0, 0, 1, height, 1),
      // Row, with height 1, top: (C 21)
      init_region(0, 0, 0, width, 1, 1),
      // Row, with height 1, bottom: (C 22)
      init_region(0, height - 1, 0, width, 1, 1),
      // Row, with height 1, center: (C 23)
      init_region(0, height / 2, 0, width, 1, 1),

      // One pixel, at center: (C 24)
      init_region(width / 2, height / 2, 1, 1, 1, 1),

      // One pixel, at upper-left: (C 25)
      init_region(0, 0, 1, 1, 1, 1),
      // One pixel, at upper-right: (C 26)
      init_region(width - 1, 0, 1, 1, 1, 1),
      // One pixel, at lower-right: (C 27)
      init_region(width - 1, height - 1, 1, 1, 1, 1),
      // One pixel, at lower-leftt: (C 28)
      init_region(0, height - 1, 1, 1, 1, 1),
  };

  for (size_t i = 0; i < sizeof(regions) / sizeof(regions[0]); i++) {
    LOG_DEBUG << "Starting test of region: " << i << std::endl;
    test.test_image_copy_region(&(regions[i]), is_immediate);
    LOG_DEBUG << "Completed test of region: " << i << std::endl;
  }
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyRegionWithVariousRegionsThenImageIsCorrectAndSuccessIsReturned) {
  //  (C 1)
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndHostImageWhenAppendingImageCopyRegionWithVariousRegionsTest(
      *this, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyRegionWithVariousRegionsOnImmediateCmdListThenImageIsCorrectAndSuccessIsReturned) {
  //  (C 1)
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndHostImageWhenAppendingImageCopyRegionWithVariousRegionsTest(
      *this, true);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingHostMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in_top = lzt::allocate_host_memory(image_size);
  void *buff_out_bot = lzt::allocate_host_memory(image_size);
  void *buff_in_bot = lzt::allocate_host_memory(image_size);
  void *buff_out_top = lzt::allocate_host_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top, false);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryToImmediateCmdListUsingHostMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in_top = lzt::allocate_host_memory(image_size);
  void *buff_out_bot = lzt::allocate_host_memory(image_size);
  void *buff_in_bot = lzt::allocate_host_memory(image_size);
  void *buff_out_top = lzt::allocate_host_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top, true);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingHostMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in = lzt::allocate_host_memory(image_size);
  void *buff_out = lzt::allocate_host_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out, false);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyToImmediateCmdListFromMemoryUsingHostMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in = lzt::allocate_host_memory(image_size);
  void *buff_out = lzt::allocate_host_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out, true);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingDeviceMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in_top = lzt::allocate_device_memory(image_size);
  void *buff_out_bot = lzt::allocate_device_memory(image_size);
  void *buff_in_bot = lzt::allocate_device_memory(image_size);
  void *buff_out_top = lzt::allocate_device_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top, false);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryToImmediateCmdListUsingDeviceMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in_top = lzt::allocate_device_memory(image_size);
  void *buff_out_bot = lzt::allocate_device_memory(image_size);
  void *buff_in_bot = lzt::allocate_device_memory(image_size);
  void *buff_out_top = lzt::allocate_device_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top, true);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingDeviceMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in = lzt::allocate_device_memory(image_size);
  void *buff_out = lzt::allocate_device_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out, false);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryToImmediateCmdListUsingDeviceMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in = lzt::allocate_device_memory(image_size);
  void *buff_out = lzt::allocate_device_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out, true);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingSharedMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in_top = lzt::allocate_shared_memory(image_size);
  void *buff_out_bot = lzt::allocate_shared_memory(image_size);
  void *buff_in_bot = lzt::allocate_shared_memory(image_size);
  void *buff_out_top = lzt::allocate_shared_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top, false);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryToImmediateCmdListUsingSharedMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in_top = lzt::allocate_shared_memory(image_size);
  void *buff_out_bot = lzt::allocate_shared_memory(image_size);
  void *buff_in_bot = lzt::allocate_shared_memory(image_size);
  void *buff_out_top = lzt::allocate_shared_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top, true);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingSharedMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in = lzt::allocate_shared_memory(image_size);
  void *buff_out = lzt::allocate_shared_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out, false);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryToImmediateCmdListUsingSharedMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in = lzt::allocate_shared_memory(image_size);
  void *buff_out = lzt::allocate_shared_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out, true);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingSharedSystemMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in_top = lzt::aligned_malloc(image_size, 1);
  void *buff_out_bot = lzt::aligned_malloc(image_size, 1);
  void *buff_in_bot = lzt::aligned_malloc(image_size, 1);
  void *buff_out_top = lzt::aligned_malloc(image_size, 1);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top, false);
  lzt::aligned_free(buff_in_bot);
  lzt::aligned_free(buff_in_top);
  lzt::aligned_free(buff_out_bot);
  lzt::aligned_free(buff_out_top);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryToImmediateCmdListUsingSharedSystemMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in_top = lzt::aligned_malloc(image_size, 1);
  void *buff_out_bot = lzt::aligned_malloc(image_size, 1);
  void *buff_in_bot = lzt::aligned_malloc(image_size, 1);
  void *buff_out_top = lzt::aligned_malloc(image_size, 1);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top, true);
  lzt::aligned_free(buff_in_bot);
  lzt::aligned_free(buff_in_top);
  lzt::aligned_free(buff_out_bot);
  lzt::aligned_free(buff_out_top);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingSharedSystemMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in = lzt::aligned_malloc(image_size, 1);
  void *buff_out = lzt::aligned_malloc(image_size, 1);
  test_image_mem_copy_no_regions(buff_in, buff_out, false);
  lzt::aligned_free(buff_in);
  lzt::aligned_free(buff_out);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyToImmediateCmdListFromMemoryUsingSharedSystemMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  void *buff_in = lzt::aligned_malloc(image_size, 1);
  void *buff_out = lzt::aligned_malloc(image_size, 1);
  test_image_mem_copy_no_regions(buff_in, buff_out, true);
  lzt::aligned_free(buff_in);
  lzt::aligned_free(buff_out);
}

void RunGivenDeviceImageWhenAppendingImageCopyTest(
    zeCommandListAppendImageCopyTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  lzt::append_image_copy(cmd_bundle.list, test.img_ptr->dflt_device_image_,
                         test.img_ptr->dflt_device_image_2_, nullptr);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  lzt::destroy_command_bundle(cmd_bundle);
  delete test.img_ptr;
}

LZT_TEST_F(zeCommandListAppendImageCopyTests,
           GivenDeviceImageWhenAppendingImageCopyThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyTest(*this, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyToImmediateCmdListThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyTest(*this, true);
}

void RunGivenDeviceImageWhenAppendingImageCopyWithHEventTest(
    zeCommandListAppendImageCopyTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  ze_event_handle_t hEvent = nullptr;

  ze_event_desc_t desc = {};
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  desc.index = 0;
  desc.signal = 0;
  desc.wait = 0;
  auto event = lzt::create_event(test.ep, desc);
  lzt::append_image_copy(cmd_bundle.list, test.img_ptr->dflt_device_image_,
                         test.img_ptr->dflt_device_image_2_, event);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  destroy_event(event);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyWithHEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyWithHEventTest(*this, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyToImmediateCmdListWithHEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyWithHEventTest(*this, true);
}

void RunGivenDeviceImageWhenAppendingImageCopyWithWaitEventTest(
    zeCommandListAppendImageCopyTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  test.img_ptr = new zeImageCreateCommon;
  ze_event_handle_t hEvent = nullptr;

  ze_event_desc_t desc = {};
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  desc.index = 0;
  desc.signal = 0;
  desc.wait = 0;
  auto event = lzt::create_event(test.ep, desc);
  auto event_before = event;
  lzt::append_image_copy(cmd_bundle.list, test.img_ptr->dflt_device_image_,
                         test.img_ptr->dflt_device_image_2_, nullptr, 1,
                         &event);
  ASSERT_EQ(event, event_before);
  if (is_immediate) {
    lzt::signal_event_from_host(event);
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  destroy_event(event);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyWithWaitEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyWithWaitEventTest(*this, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyToImmediateCmdListWithWaitEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyWithWaitEventTest(*this, true);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryAndFromMemoryWithOffsetsImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }

  img_ptr = new zeImageCreateCommon;
  int full_width = img_ptr->dflt_host_image_.width();
  int full_height = img_ptr->dflt_host_image_.height();

  EXPECT_GE(full_width, 10);
  EXPECT_GE(full_height, 10);

  // To verify regions are respected, we use input and output host images
  // that are slightly smaller than the full width of the device image, and
  // small arbitrary offsets of the origin.  These should be chosen to fit
  // within the 10x10 minimum size we've checked for above.
  auto in_region = init_region(2, 5, 0, full_width - 8, full_height - 7, 1);
  auto out_region = init_region(3, 1, 0, full_width - 6, full_height - 2, 1);

  image_region_copy(in_region, out_region, false);
  delete img_ptr;
}

LZT_TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryAndFromMemoryToImmediateCmdListWithOffsetsImageIsCorrectAndSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }

  img_ptr = new zeImageCreateCommon;
  int full_width = img_ptr->dflt_host_image_.width();
  int full_height = img_ptr->dflt_host_image_.height();

  EXPECT_GE(full_width, 10);
  EXPECT_GE(full_height, 10);

  // To verify regions are respected, we use input and output host images
  // that are slightly smaller than the full width of the device image, and
  // small arbitrary offsets of the origin.  These should be chosen to fit
  // within the 10x10 minimum size we've checked for above.
  auto in_region = init_region(2, 5, 0, full_width - 8, full_height - 7, 1);
  auto out_region = init_region(3, 1, 0, full_width - 6, full_height - 2, 1);

  image_region_copy(in_region, out_region, true);
  delete img_ptr;
}

class zeCommandListAppendImageCopyFromMemoryTests : public ::testing::Test {
public:
  zeEventPool ep;
  lzt::zeImageCreateCommon *img_ptr = nullptr;
};

void RunGivenDeviceImageAndHostImageWhenAppendingImageCopyTest(
    zeCommandListAppendImageCopyFromMemoryTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  lzt::append_image_copy_from_mem(
      cmd_bundle.list, test.img_ptr->dflt_device_image_,
      test.img_ptr->dflt_host_image_.raw_data(), nullptr);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  lzt::destroy_command_bundle(cmd_bundle);
  delete test.img_ptr;
}

LZT_TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndHostImageWhenAppendingImageCopyTest(*this, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyToImmediateCmdListThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndHostImageWhenAppendingImageCopyTest(*this, true);
}

void RunGivenDeviceImageAndHostImageWhenAppendingImageCopyWithHEventTest(
    zeCommandListAppendImageCopyFromMemoryTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  test.img_ptr = new zeImageCreateCommon;
  ze_event_handle_t hEvent = nullptr;

  test.ep.create_event(hEvent);
  lzt::append_image_copy_from_mem(
      cmd_bundle.list, test.img_ptr->dflt_device_image_,
      test.img_ptr->dflt_host_image_.raw_data(), hEvent);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  test.ep.destroy_event(hEvent);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyWithHEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndHostImageWhenAppendingImageCopyWithHEventTest(*this,
                                                                      false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyToImmediateCmdListWithHEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndHostImageWhenAppendingImageCopyWithHEventTest(*this,
                                                                      true);
}

void RunGivenDeviceImageAndHostImageWhenAppendingImageCopyWithWaitEventTest(
    zeCommandListAppendImageCopyFromMemoryTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  test.img_ptr = new zeImageCreateCommon;
  ze_event_handle_t hEvent = nullptr;

  test.ep.create_event(hEvent);
  auto hEvent_initial = hEvent;
  lzt::append_image_copy_from_mem(
      cmd_bundle.list, test.img_ptr->dflt_device_image_,
      test.img_ptr->dflt_host_image_.raw_data(), nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_initial);
  if (is_immediate) {
    lzt::signal_event_from_host(hEvent);
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  test.ep.destroy_event(hEvent);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyWithWaitEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndHostImageWhenAppendingImageCopyWithWaitEventTest(*this,
                                                                         false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyToImmediateCmdListWithWaitEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndHostImageWhenAppendingImageCopyWithWaitEventTest(*this,
                                                                         true);
}

class zeCommandListAppendImageCopyRegionTests
    : public zeCommandListAppendImageCopyFromMemoryTests {};

void RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionTest(
    zeCommandListAppendImageCopyRegionTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  ze_image_region_t source_region = {};
  ze_image_region_t dest_region = {};

  dest_region.originX = 0;
  dest_region.originY = 0;
  dest_region.originZ = 0;
  dest_region.width = test.img_ptr->dflt_host_image_.width() / 2;
  dest_region.height = test.img_ptr->dflt_host_image_.height() / 2;
  dest_region.depth = 1;

  source_region.originX = test.img_ptr->dflt_host_image_.width() / 2;
  source_region.originY = test.img_ptr->dflt_host_image_.height() / 2;
  source_region.originZ = 0;
  source_region.width = test.img_ptr->dflt_host_image_.width() / 2;
  source_region.height = test.img_ptr->dflt_host_image_.height() / 2;
  source_region.depth = 1;
  lzt::append_image_copy_region(cmd_bundle.list,
                                test.img_ptr->dflt_device_image_,
                                test.img_ptr->dflt_device_image_2_,
                                &dest_region, &source_region, nullptr);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionTest(*this,
                                                                    false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionToImmediateCmdListThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionTest(*this,
                                                                    true);
}

void RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithHEventTest(
    zeCommandListAppendImageCopyRegionTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  ze_image_region_t source_region = {};
  ze_image_region_t dest_region = {};

  dest_region.originX = 0;
  dest_region.originY = 0;
  dest_region.originZ = 0;
  dest_region.width = test.img_ptr->dflt_host_image_.width() / 2;
  dest_region.height = test.img_ptr->dflt_host_image_.height() / 2;
  dest_region.depth = 1;

  source_region.originX = test.img_ptr->dflt_host_image_.width() / 2;
  source_region.originY = test.img_ptr->dflt_host_image_.height() / 2;
  source_region.originZ = 0;
  source_region.width = test.img_ptr->dflt_host_image_.width() / 2;
  source_region.height = test.img_ptr->dflt_host_image_.height() / 2;
  source_region.depth = 1;
  ze_event_handle_t hEvent = nullptr;

  test.ep.create_event(hEvent);
  lzt::append_image_copy_region(
      cmd_bundle.list, test.img_ptr->dflt_device_image_,
      test.img_ptr->dflt_device_image_2_, &dest_region, &source_region, hEvent);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  test.ep.destroy_event(hEvent);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithHEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithHEventTest(
      *this, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionToImmediateCmdListWithHEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithHEventTest(
      *this, true);
}

void RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithWaitEventTest(
    zeCommandListAppendImageCopyRegionTests &test, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  ze_image_region_t source_region = {};
  ze_image_region_t dest_region = {};

  dest_region.originX = 0;
  dest_region.originY = 0;
  dest_region.originZ = 0;
  dest_region.width = test.img_ptr->dflt_host_image_.width() / 2;
  dest_region.height = test.img_ptr->dflt_host_image_.height() / 2;
  dest_region.depth = 1;

  source_region.originX = test.img_ptr->dflt_host_image_.width() / 2;
  source_region.originY = test.img_ptr->dflt_host_image_.height() / 2;
  source_region.originZ = 0;
  source_region.width = test.img_ptr->dflt_host_image_.width() / 2;
  source_region.height = test.img_ptr->dflt_host_image_.height() / 2;
  source_region.depth = 1;
  ze_event_handle_t hEvent = nullptr;

  test.ep.create_event(hEvent);
  auto hEvent_initial = hEvent;
  lzt::append_image_copy_region(
      cmd_bundle.list, test.img_ptr->dflt_device_image_,
      test.img_ptr->dflt_device_image_2_, &dest_region, &source_region, nullptr,
      1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_initial);
  if (is_immediate) {
    lzt::signal_event_from_host(hEvent);
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  test.ep.destroy_event(hEvent);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithWaitEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithWaitEventTest(
      *this, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionToImmediateCmdListWithWaitEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithWaitEventTest(
      *this, true);
}

class zeCommandListAppendImageCopyToMemoryTests
    : public zeCommandListAppendImageCopyFromMemoryTests {};

void RunGivenDeviceImageWhenAppendingImageCopyToMemoryTest(
    zeCommandListAppendImageCopyToMemoryTests &test, bool is_immediate,
    bool is_shared_system) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  void *device_memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(test.img_ptr->dflt_host_image_), is_shared_system);

  lzt::append_image_copy_to_mem(cmd_bundle.list, device_memory,
                                test.img_ptr->dflt_device_image_, nullptr);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  lzt::free_memory_with_allocator_selector(device_memory, is_shared_system);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryTest(*this, false, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryOnImmediateCmdListThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryTest(*this, true, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryTest(*this, false, true);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryOnImmediateCmdListThenSuccessIsReturnedWithAllocatorWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryTest(*this, true, true);
}

void RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithHEventTest(
    zeCommandListAppendImageCopyToMemoryTests &test, bool is_immediate,
    bool is_shared_system) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  void *device_memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(test.img_ptr->dflt_host_image_), is_shared_system);
  ze_event_handle_t hEvent = nullptr;

  test.ep.create_event(hEvent);
  lzt::append_image_copy_to_mem(cmd_bundle.list, device_memory,
                                test.img_ptr->dflt_device_image_, hEvent);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  test.ep.destroy_event(hEvent);
  lzt::free_memory_with_allocator_selector(device_memory, is_shared_system);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryWithHEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithHEventTest(*this, false,
                                                                  false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryOnImmediateCmdListWithHEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithHEventTest(*this, true,
                                                                  false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithHEventTest(*this, false,
                                                                  true);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryOnImmediateCmdListWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithHEventTest(*this, true,
                                                                  true);
}

void RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithWaitEventTest(
    zeCommandListAppendImageCopyToMemoryTests &test, bool is_immediate,
    bool is_shared_system) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  test.img_ptr = new zeImageCreateCommon;
  void *device_memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(test.img_ptr->dflt_host_image_), is_shared_system);
  ze_event_handle_t hEvent = nullptr;

  test.ep.create_event(hEvent);
  auto hEvent_initial = hEvent;
  lzt::append_image_copy_to_mem(cmd_bundle.list, device_memory,
                                test.img_ptr->dflt_device_image_, nullptr, 1,
                                &hEvent);
  ASSERT_EQ(hEvent, hEvent_initial);
  if (is_immediate) {
    lzt::signal_event_from_host(hEvent);
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  test.ep.destroy_event(hEvent);
  lzt::free_memory_with_allocator_selector(device_memory, is_shared_system);
  delete test.img_ptr;
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryWithWaitEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithWaitEventTest(
      *this, false, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryOnImmediateCmdListWithWaitEventThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithWaitEventTest(
      *this, true, false);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithWaitEventTest(
      *this, false, true);
}

LZT_TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryOnImmediateCmdListWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!(lzt::image_support())) {
    GTEST_SKIP();
  }
  RunGivenDeviceImageWhenAppendingImageCopyToMemoryWithWaitEventTest(
      *this, true, true);
}

} // namespace
