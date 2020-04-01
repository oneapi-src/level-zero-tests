/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace {

class zeCommandListAppendImageCopyWithSwizzleTests : public ::testing::Test {
protected:
  void test_image_copy_different_swizzles() {
    lzt::ImagePNG32Bit host_image_source(img.dflt_host_image_.width(),
                                         img.dflt_host_image_.height());
    lzt::ImagePNG32Bit host_image_dest(img.dflt_host_image_.width(),
                                       img.dflt_host_image_.height());
    ze_image_desc_t image_desc_source =
                        lzt::zeImageCreateCommon::dflt_ze_image_desc,
                    image_desc_dest;
    ze_image_handle_t hImageSource = nullptr, hImageDest = nullptr;

    image_desc_dest = image_desc_source;

    // Set format of the source image to be RGBA:
    image_desc_source.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
    image_desc_source.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
    image_desc_source.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
    image_desc_source.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;

    // Set format of the dest image to be ABGR:
    image_desc_dest.format.x = ZE_IMAGE_FORMAT_SWIZZLE_A;
    image_desc_dest.format.y = ZE_IMAGE_FORMAT_SWIZZLE_B;
    image_desc_dest.format.z = ZE_IMAGE_FORMAT_SWIZZLE_G;
    image_desc_dest.format.w = ZE_IMAGE_FORMAT_SWIZZLE_R;

    // Create the source and dest images (with different swizzle order):
    lzt::create_ze_image(hImageSource, &image_desc_source);
    lzt::create_ze_image(hImageDest, &image_desc_dest);

    // Write an image with data pattern 1 in RGBA color order to the host image:
    lzt::write_image_data_pattern(host_image_source, 1,
                                  image_desc_source.format);
    // Scribble willy-nilly to the dest host image:
    lzt::write_image_data_pattern(host_image_dest, -1);

    // First, copy the image from the host to the source image on the device:
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendImageCopyFromMemory(
                  cl.command_list_, hImageSource, host_image_source.raw_data(),
                  nullptr, nullptr));
    lzt::append_barrier(cl.command_list_, nullptr, 0, nullptr);
    // Now, copy the image from the device to the device (swizzling the colors):
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendImageCopy(cl.command_list_, hImageDest,
                                           hImageSource, nullptr));
    lzt::append_barrier(cl.command_list_, nullptr, 0, nullptr);
    // Finally copy the image from the device to the host_image_dest for
    // validation:
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendImageCopyToMemory(
                  cl.command_list_, host_image_dest.raw_data(), hImageDest,
                  nullptr, nullptr));
    lzt::append_barrier(cl.command_list_, nullptr, 0, nullptr);
    // Execute all of the commands involving copying of images
    lzt::close_command_list(cl.command_list_);
    lzt::execute_command_lists(cq.command_queue_, 1, &cl.command_list_,
                               nullptr);
    lzt::synchronize(cq.command_queue_, UINT32_MAX);
    // Validate the result of the above operations:
    // If the operation is a straight image copy, or the second region is null
    // then the result should be the same:
    EXPECT_EQ(0, compare_data_pattern(
                     host_image_dest, image_desc_dest.format, host_image_source,
                     image_desc_source.format, 0, 0, host_image_dest.width(),
                     host_image_dest.height(), 0, 0, host_image_source.width(),
                     host_image_source.height()));
  }
  lzt::zeImageCreateCommon img;
  lzt::zeCommandList cl;
  lzt::zeCommandQueue cq;
};

TEST_F(
    zeCommandListAppendImageCopyWithSwizzleTests,
    GivenDeviceImageAndHostImagesWithDifferentSwizzleWhenAppendingImageCopyThenImageIsCorrectAndSuccessIsReturned) {
  test_image_copy_different_swizzles();
}

} // namespace
