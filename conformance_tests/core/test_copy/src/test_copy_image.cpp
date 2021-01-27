/*
 *
 * Copyright (C) 2019 Intel Corporation
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
    cl = lzt::create_command_list();
    cq = lzt::create_command_queue();

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

  ~zeCommandListAppendImageCopyTests() {
    lzt::destroy_command_list(cl);
    lzt::destroy_command_queue(cq);
    lzt::destroy_event_pool(ep);
    lzt::destroy_ze_image(ze_img_src);
    lzt::destroy_ze_image(ze_img_dest);
  }

protected:
  ze_event_pool_handle_t ep;
  ze_command_list_handle_t cl;
  ze_command_queue_handle_t cq;
  uint32_t image_width;
  uint32_t image_height;
  uint32_t image_size;
  ze_image_handle_t ze_img_src, ze_img_dest;
  lzt::ImagePNG32Bit png_img_src, png_img_dest;

  zeImageCreateCommon img;

  void test_image_copy() {

    lzt::append_image_copy_from_mem(cl, ze_img_src, png_img_src.raw_data(),
                                    nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_image_copy(cl, ze_img_dest, ze_img_src, nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_image_copy_to_mem(cl, png_img_dest.raw_data(), ze_img_dest,
                                  nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    close_command_list(cl);
    execute_command_lists(cq, 1, &cl, nullptr);
    synchronize(cq, UINT64_MAX);

    EXPECT_EQ(png_img_src, png_img_dest);
  }

  void test_image_copy_region(const ze_image_region_t *region) {
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
    lzt::ImagePNG32Bit background_image(img.dflt_host_image_.width(),
                                        img.dflt_host_image_.height());
    // Initialize background image with background data pattern:
    lzt::write_image_data_pattern(background_image, background_dp);
    // The foreground image:
    lzt::ImagePNG32Bit foreground_image(img.dflt_host_image_.width(),
                                        img.dflt_host_image_.height());
    // Initialize foreground image with foreground data pattern:
    lzt::write_image_data_pattern(foreground_image, foreground_dp);
    // new_host_image is used to validate that the image copy region
    // operation(s) were correct:
    lzt::ImagePNG32Bit new_host_image(img.dflt_host_image_.width(),
                                      img.dflt_host_image_.height());
    // Scribble a known incorrect data pattern to new_host_image to ensure we
    // are validating actual data from the L0 functionality:
    lzt::write_image_data_pattern(new_host_image, scribble_dp);
    // First, copy the background image from the host to the device:
    // This will serve as the BACKGROUND of the image.
    append_image_copy_from_mem(cl, h_dest_image, background_image.raw_data(),
                               nullptr);
    append_barrier(cl, nullptr, 0, nullptr);
    // Next, copy the foreground image from the host to the device:
    // This will serve as the FOREGROUND of the image.
    append_image_copy_from_mem(cl, h_source_image, foreground_image.raw_data(),
                               nullptr);
    append_barrier(cl, nullptr, 0, nullptr);
    // Copy the portion of the foreground image correspoding to the region:
    append_image_copy_region(cl, h_dest_image, h_source_image, region, region,
                             nullptr);
    append_barrier(cl, nullptr, 0, nullptr);
    // Finally, copy the image in hTstImage back to new_host_image for
    // validation:
    append_image_copy_to_mem(cl, new_host_image.raw_data(), h_dest_image,
                             nullptr);
    append_barrier(cl, nullptr, 0, nullptr);
    // Execute all of the commands involving copying of images
    close_command_list(cl);
    execute_command_lists(cq, 1, &cl, nullptr);
    synchronize(cq, UINT64_MAX);

    EXPECT_EQ(0, lzt::compare_data_pattern(new_host_image, region,
                                           foreground_image, background_image));

    destroy_ze_image(h_dest_image);
    destroy_ze_image(h_source_image);
    reset_command_list(cl);
  }

  void test_image_mem_copy_no_regions(void *source_buff, void *dest_buff) {

    // Copies proceeds as follows:
    // png -> source_buff -> image -> dest_buff ->png

    lzt::append_memory_copy(cl, source_buff, png_img_src.raw_data(),
                            image_size);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_image_copy_from_mem(cl, ze_img_src, source_buff, nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_image_copy_to_mem(cl, dest_buff, ze_img_src, nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_memory_copy(cl, png_img_dest.raw_data(), dest_buff, image_size);
    lzt::append_barrier(cl, nullptr, 0, nullptr);

    close_command_list(cl);
    execute_command_lists(cq, 1, &cl, nullptr);
    synchronize(cq, UINT64_MAX);

    EXPECT_EQ(png_img_src, png_img_dest);
  }

  void test_image_mem_copy_use_regions(void *source_buff_bot,
                                       void *source_buff_top,
                                       void *dest_buff_bot,
                                       void *dest_buff_top) {

    ze_image_region_t bot_region = {0, 0, 0, image_width, image_height / 2, 1};
    ze_image_region_t top_region = {0,           image_height / 2, 0,
                                    image_width, image_height / 2, 1};

    // Copies proceeds as follows:
    // png -> source_buff -> image -> dest_buff ->png

    lzt::append_memory_copy(cl, source_buff_bot, png_img_src.raw_data(),
                            image_size / 2);
    lzt::append_memory_copy(cl, source_buff_top,
                            png_img_src.raw_data() + image_size / 4 / 2,
                            image_size / 2);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_image_copy_from_mem(cl, ze_img_src, source_buff_bot, bot_region,
                                    nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_image_copy_from_mem(cl, ze_img_src, source_buff_top, top_region,
                                    nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);

    lzt::append_image_copy_to_mem(cl, dest_buff_bot, ze_img_src, bot_region,
                                  nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_image_copy_to_mem(cl, dest_buff_top, ze_img_src, top_region,
                                  nullptr);

    lzt::append_barrier(cl, nullptr, 0, nullptr);
    lzt::append_memory_copy(cl, png_img_dest.raw_data(), dest_buff_bot,
                            image_size / 2);
    lzt::append_memory_copy(cl, png_img_dest.raw_data() + image_size / 4 / 2,
                            dest_buff_top, image_size / 2);
    lzt::append_barrier(cl, nullptr, 0, nullptr);

    close_command_list(cl);
    execute_command_lists(cq, 1, &cl, nullptr);
    synchronize(cq, UINT64_MAX);

    for (int i = 0; i < (image_size / 4); i++) {
      ASSERT_EQ(png_img_src.raw_data()[i], png_img_dest.raw_data()[i])
          << "i is " << i;
    }

    EXPECT_EQ(png_img_src, png_img_dest);
  }

  void image_region_copy(const ze_image_region_t &in_region,
                         const ze_image_region_t &out_region) {
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
    lzt::append_image_copy_from_mem(cl, img.dflt_device_image_,
                                    in_image.raw_data(), in_region, nullptr);

    append_barrier(cl, nullptr, 0, nullptr);

    // Copy from image region to output host image
    lzt::append_image_copy_to_mem(cl, out_image.raw_data(),
                                  img.dflt_device_image_, out_region, nullptr);

    // Execute
    close_command_list(cl);
    execute_command_lists(cq, 1, &cl, nullptr);
    synchronize(cq, UINT64_MAX);

    // Verify output image matches initial host image.
    // Output image contains input image data shifted by in_region's origin
    // minus out_region's origin.  Some of the original data may not make it
    // to the output due to sizes and offests, and there may be junk data in
    // parts of the output image that don't have coresponding pixels in the
    // input; we will ignore those.
    // We may pass negative origin coordinates to compare_data_pattern; in that
    // case, it will skip over any negative-index pixels.
    EXPECT_EQ(0, compare_data_pattern(in_image, out_image, 0, 0,
                                      in_region.width, in_region.height,
                                      in_region.originX - out_region.originX,
                                      in_region.originY - out_region.originY,
                                      out_region.width, out_region.height));
  }
};

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyThenImageIsCorrectAndSuccessIsReturned) {
  test_image_copy();
}

static inline ze_image_region_t init_region(uint32_t originX, uint32_t originY,
                                            uint32_t originZ, uint32_t width,
                                            uint32_t height, uint32_t depth) {
  ze_image_region_t rv = {originX, originY, originZ, width, height, depth};
  return rv;
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyRegionWithVariousRegionsThenImageIsCorrectAndSuccessIsReturned) {
  //  (C 1)
  LOG_DEBUG << "Starting test of nullptr region." << std::endl;
  test_image_copy_region(nullptr);
  LOG_DEBUG << "Completed test of nullptr region" << std::endl;
  // Aliases to reduce widths of the following region initializers
  const uint32_t width = img.dflt_host_image_.width();
  const uint32_t height = img.dflt_host_image_.height();
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
    test_image_copy_region(&(regions[i]));
    LOG_DEBUG << "Completed test of region: " << i << std::endl;
  }
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingHostMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  void *buff_in_top = lzt::allocate_host_memory(image_size);
  void *buff_out_bot = lzt::allocate_host_memory(image_size);
  void *buff_in_bot = lzt::allocate_host_memory(image_size);
  void *buff_out_top = lzt::allocate_host_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingHostMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  void *buff_in = lzt::allocate_host_memory(image_size);
  void *buff_out = lzt::allocate_host_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingDeviceMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  void *buff_in_top = lzt::allocate_device_memory(image_size);
  void *buff_out_bot = lzt::allocate_device_memory(image_size);
  void *buff_in_bot = lzt::allocate_device_memory(image_size);
  void *buff_out_top = lzt::allocate_device_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingDeviceMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  void *buff_in = lzt::allocate_device_memory(image_size);
  void *buff_out = lzt::allocate_device_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingSharedMemoryWithNonNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  void *buff_in_top = lzt::allocate_shared_memory(image_size);
  void *buff_out_bot = lzt::allocate_shared_memory(image_size);
  void *buff_in_bot = lzt::allocate_shared_memory(image_size);
  void *buff_out_top = lzt::allocate_shared_memory(image_size);
  test_image_mem_copy_use_regions(buff_in_bot, buff_in_top, buff_out_bot,
                                  buff_out_top);
  lzt::free_memory(buff_in_bot);
  lzt::free_memory(buff_in_top);
  lzt::free_memory(buff_out_bot);
  lzt::free_memory(buff_out_top);
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyFromMemoryUsingSharedMemoryWithNullRegionsThenImageIsCorrectAndSuccessIsReturned) {
  void *buff_in = lzt::allocate_shared_memory(image_size);
  void *buff_out = lzt::allocate_shared_memory(image_size);
  test_image_mem_copy_no_regions(buff_in, buff_out);
  lzt::free_memory(buff_in);
  lzt::free_memory(buff_out);
}

TEST_F(zeCommandListAppendImageCopyTests,
       GivenDeviceImageWhenAppendingImageCopyThenSuccessIsReturned) {
  lzt::append_image_copy(cl, img.dflt_device_image_, img.dflt_device_image_2_,
                         nullptr);
}

TEST_F(zeCommandListAppendImageCopyTests,
       GivenDeviceImageWhenAppendingImageCopyWithHEventThenSuccessIsReturned) {
  ze_event_handle_t hEvent = nullptr;

  ze_event_desc_t desc = {};
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  desc.index = 0;
  desc.signal = 0;
  desc.wait = 0;
  auto event = lzt::create_event(ep, desc);
  lzt::append_image_copy(cl, img.dflt_device_image_, img.dflt_device_image_2_,
                         event);
  destroy_event(event);
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyWithWaitEventThenSuccessIsReturned) {
  ze_event_handle_t hEvent = nullptr;

  ze_event_desc_t desc = {};
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  desc.index = 0;
  desc.signal = 0;
  desc.wait = 0;
  auto event = lzt::create_event(ep, desc);
  auto event_before = event;
  lzt::append_image_copy(cl, img.dflt_device_image_, img.dflt_device_image_2_,
                         nullptr, 1, &event);
  ASSERT_EQ(event, event_before);
  destroy_event(event);
}

TEST_F(
    zeCommandListAppendImageCopyTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryAndFromMemoryWithOffsetsImageIsCorrectAndSuccessIsReturned) {

  int full_width = img.dflt_host_image_.width();
  int full_height = img.dflt_host_image_.height();

  EXPECT_GE(full_width, 10);
  EXPECT_GE(full_height, 10);

  // To verify regions are respected, we use input and output host images
  // that are slightly smaller than the full width of the device image, and
  // small arbitrary offsets of the origin.  These should be chosen to fit
  // within the 10x10 minimum size we've checked for above.
  auto in_region = init_region(2, 5, 0, full_width - 8, full_height - 7, 1);
  auto out_region = init_region(3, 1, 0, full_width - 6, full_height - 2, 1);

  image_region_copy(in_region, out_region);
}

class zeCommandListAppendImageCopyFromMemoryTests : public ::testing::Test {
protected:
  zeEventPool ep;
  zeCommandList cl;
  zeImageCreateCommon img;
};

TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyThenSuccessIsReturned) {

  lzt::append_image_copy_from_mem(cl.command_list_, img.dflt_device_image_,
                                  img.dflt_host_image_.raw_data(), nullptr);
}

TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyWithHEventThenSuccessIsReturned) {
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  lzt::append_image_copy_from_mem(cl.command_list_, img.dflt_device_image_,
                                  img.dflt_host_image_.raw_data(), hEvent);
  ep.destroy_event(hEvent);
}

TEST_F(
    zeCommandListAppendImageCopyFromMemoryTests,
    GivenDeviceImageAndHostImageWhenAppendingImageCopyWithWaitEventThenSuccessIsReturned) {
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  auto hEvent_initial = hEvent;
  lzt::append_image_copy_from_mem(cl.command_list_, img.dflt_device_image_,
                                  img.dflt_host_image_.raw_data(), nullptr, 1,
                                  &hEvent);
  ASSERT_EQ(hEvent, hEvent_initial);
  ep.destroy_event(hEvent);
}

class zeCommandListAppendImageCopyRegionTests
    : public zeCommandListAppendImageCopyFromMemoryTests {};

TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionThenSuccessIsReturned) {
  ze_image_region_t source_region = {};
  ze_image_region_t dest_region = {};

  dest_region.originX = 0;
  dest_region.originY = 0;
  dest_region.originZ = 0;
  dest_region.width = img.dflt_host_image_.width() / 2;
  dest_region.height = img.dflt_host_image_.height() / 2;
  dest_region.depth = 1;

  source_region.originX = img.dflt_host_image_.width() / 2;
  source_region.originY = img.dflt_host_image_.height() / 2;
  source_region.originZ = 0;
  source_region.width = img.dflt_host_image_.width() / 2;
  source_region.height = img.dflt_host_image_.height() / 2;
  source_region.depth = 1;
  lzt::append_image_copy_region(cl.command_list_, img.dflt_device_image_,
                                img.dflt_device_image_2_, &dest_region,
                                &source_region, nullptr);
}

TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithHEventThenSuccessIsReturned) {
  ze_image_region_t source_region = {};
  ze_image_region_t dest_region = {};

  dest_region.originX = 0;
  dest_region.originY = 0;
  dest_region.originZ = 0;
  dest_region.width = img.dflt_host_image_.width() / 2;
  dest_region.height = img.dflt_host_image_.height() / 2;
  dest_region.depth = 1;

  source_region.originX = img.dflt_host_image_.width() / 2;
  source_region.originY = img.dflt_host_image_.height() / 2;
  source_region.originZ = 0;
  source_region.width = img.dflt_host_image_.width() / 2;
  source_region.height = img.dflt_host_image_.height() / 2;
  source_region.depth = 1;
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  lzt::append_image_copy_region(cl.command_list_, img.dflt_device_image_,
                                img.dflt_device_image_2_, &dest_region,
                                &source_region, hEvent);
  ep.destroy_event(hEvent);
}

TEST_F(
    zeCommandListAppendImageCopyRegionTests,
    GivenDeviceImageAndDeviceImageWhenAppendingImageCopyRegionWithWaitEventThenSuccessIsReturned) {
  ze_image_region_t source_region = {};
  ze_image_region_t dest_region = {};

  dest_region.originX = 0;
  dest_region.originY = 0;
  dest_region.originZ = 0;
  dest_region.width = img.dflt_host_image_.width() / 2;
  dest_region.height = img.dflt_host_image_.height() / 2;
  dest_region.depth = 1;

  source_region.originX = img.dflt_host_image_.width() / 2;
  source_region.originY = img.dflt_host_image_.height() / 2;
  source_region.originZ = 0;
  source_region.width = img.dflt_host_image_.width() / 2;
  source_region.height = img.dflt_host_image_.height() / 2;
  source_region.depth = 1;
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  auto hEvent_initial = hEvent;
  lzt::append_image_copy_region(cl.command_list_, img.dflt_device_image_,
                                img.dflt_device_image_2_, &dest_region,
                                &source_region, nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_initial);
  ep.destroy_event(hEvent);
}

class zeCommandListAppendImageCopyToMemoryTests
    : public zeCommandListAppendImageCopyFromMemoryTests {};

TEST_F(zeCommandListAppendImageCopyToMemoryTests,
       GivenDeviceImageWhenAppendingImageCopyToMemoryThenSuccessIsReturned) {
  void *device_memory =
      allocate_device_memory(size_in_bytes(img.dflt_host_image_));

  lzt::append_image_copy_to_mem(cl.command_list_, device_memory,
                                img.dflt_device_image_, nullptr);
  free_memory(device_memory);
}

TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryWithHEventThenSuccessIsReturned) {
  void *device_memory =
      allocate_device_memory(size_in_bytes(img.dflt_host_image_));
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  lzt::append_image_copy_to_mem(cl.command_list_, device_memory,
                                img.dflt_device_image_, hEvent);
  ep.destroy_event(hEvent);
  free_memory(device_memory);
}

TEST_F(
    zeCommandListAppendImageCopyToMemoryTests,
    GivenDeviceImageWhenAppendingImageCopyToMemoryWithWaitEventThenSuccessIsReturned) {
  void *device_memory =
      allocate_device_memory(size_in_bytes(img.dflt_host_image_));
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  auto hEvent_initial = hEvent;
  lzt::append_image_copy_to_mem(cl.command_list_, device_memory,
                                img.dflt_device_image_, nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_initial);
  ep.destroy_event(hEvent);
  free_memory(device_memory);
}
} // namespace
