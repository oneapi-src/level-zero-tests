/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

namespace {

static bool test_extension(ze_driver_handle_t driver, const char *ext_name) {
  uint32_t ext_ver = 0;
  auto ext_props = lzt::get_extension_properties(driver);
  for (auto &prop : ext_props) {
    if (strncmp(prop.name, ext_name, ZE_MAX_EXTENSION_NAME) == 0) {
      ext_ver = prop.version;
      break;
    }
  }
  return (ext_ver != 0);
}

class zeImageViewCreateExtTests : public ::testing::Test {};

TEST(zeImageViewCreateExtTests,
     GivenPlanarNV12ImageThenImageViewCreateExtReturnsSuccess) {
  if (!test_extension(lzt::get_default_driver(), ZE_IMAGE_VIEW_EXT_NAME)) {
    GTEST_SKIP() << "Missing ZE_extension_image_view support\n";
  }
  if (!test_extension(lzt::get_default_driver(),
                      ZE_IMAGE_VIEW_PLANAR_EXT_NAME)) {
    GTEST_SKIP() << "Missing ZE_extension_image_view_planar support\n";
  }

  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t device =
      lzt::get_default_device(lzt::get_default_driver());

  ze_image_format_t img_fmt = {};
  img_fmt.layout = ZE_IMAGE_FORMAT_LAYOUT_NV12;
  img_fmt.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  img_fmt.x = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.y = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.z = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.w = ZE_IMAGE_FORMAT_SWIZZLE_X;

  ze_image_desc_t img_desc = {};
  img_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  img_desc.pNext = nullptr;
  img_desc.flags = 0;
  img_desc.type = ZE_IMAGE_TYPE_2D;
  img_desc.format = img_fmt;
  img_desc.width = 512;
  img_desc.height = 512;
  img_desc.depth = 1;
  img_desc.arraylevels = 0;
  img_desc.miplevels = 0;

  auto img = lzt::create_ze_image(context, device, img_desc);
  EXPECT_TRUE(img != nullptr);

  // Get Y-plane view, same image size, elements are u8's
  ze_image_view_planar_ext_desc_t img_view_desc_Y = {};
  img_view_desc_Y.stype = ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC;
  img_view_desc_Y.pNext = nullptr;
  img_view_desc_Y.planeIndex = 0;

  auto img_fmt_Y = img_fmt;
  img_fmt_Y.layout = ZE_IMAGE_FORMAT_LAYOUT_8;

  auto img_desc_Y = img_desc;
  img_desc_Y.pNext = &img_view_desc_Y;
  img_desc_Y.format = img_fmt_Y;

  ze_image_handle_t img_view_Y = nullptr;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeImageViewCreateExt(context, device, &img_desc_Y, img, &img_view_Y));
  if (img_view_Y == nullptr) {
    lzt::destroy_ze_image(img);
    FAIL();
  }

  // Get UV-plane view, half image size, elements are (u8, u8)'s
  ze_image_view_planar_ext_desc_t img_view_desc_UV = {};
  img_view_desc_UV.stype = ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC;
  img_view_desc_UV.pNext = nullptr;
  img_view_desc_UV.planeIndex = 1;

  auto img_fmt_UV = img_fmt;
  img_fmt_UV.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8;

  auto img_desc_UV = img_desc;
  img_desc_UV.pNext = &img_view_desc_UV;
  img_desc_UV.format = img_fmt_UV;
  img_desc_UV.width /= 2;
  img_desc_UV.height /= 2;

  ze_image_handle_t img_view_UV = nullptr;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeImageViewCreateExt(context, device, &img_desc_UV, img, &img_view_UV));
  if (img_view_UV == nullptr) {
    lzt::destroy_ze_image(img_view_Y);
    lzt::destroy_ze_image(img);
    FAIL();
  }

  lzt::destroy_ze_image(img_view_UV);
  lzt::destroy_ze_image(img_view_Y);
  lzt::destroy_ze_image(img);
}

TEST(zeImageViewCreateExtTests,
     GivenPlanarRGBPImageThenImageViewCreateExtReturnsSuccess) {
  if (!test_extension(lzt::get_default_driver(), ZE_IMAGE_VIEW_EXT_NAME)) {
    GTEST_SKIP() << "Missing ZE_extension_image_view support\n";
  }
  if (!test_extension(lzt::get_default_driver(),
                      ZE_IMAGE_VIEW_PLANAR_EXT_NAME)) {
    GTEST_SKIP() << "Missing ZE_extension_image_view_planar support\n";
  }

  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t device =
      lzt::get_default_device(lzt::get_default_driver());

  ze_image_format_t img_fmt = {};
  img_fmt.layout = ZE_IMAGE_FORMAT_LAYOUT_RGBP;
  img_fmt.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  img_fmt.x = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.y = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.z = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.w = ZE_IMAGE_FORMAT_SWIZZLE_X;

  ze_image_desc_t img_desc = {};
  img_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  img_desc.pNext = nullptr;
  img_desc.flags = 0;
  img_desc.type = ZE_IMAGE_TYPE_2D;
  img_desc.format = img_fmt;
  img_desc.width = 512;
  img_desc.height = 512;
  img_desc.depth = 1;
  img_desc.arraylevels = 0;
  img_desc.miplevels = 0;

  auto img = lzt::create_ze_image(context, device, img_desc);
  EXPECT_TRUE(img != nullptr);

  // Get R-plane view, same image size, elements are u8's
  ze_image_view_planar_ext_desc_t img_view_desc_R = {};
  img_view_desc_R.stype = ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC;
  img_view_desc_R.pNext = nullptr;
  img_view_desc_R.planeIndex = 0;

  auto img_fmt_R = img_fmt;
  img_fmt_R.layout = ZE_IMAGE_FORMAT_LAYOUT_8;

  auto img_desc_R = img_desc;
  img_desc_R.pNext = &img_view_desc_R;
  img_desc_R.format = img_fmt_R;

  ze_image_handle_t img_view_R = nullptr;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeImageViewCreateExt(context, device, &img_desc_R, img, &img_view_R));
  if (img_view_R == nullptr) {
    lzt::destroy_ze_image(img);
    FAIL();
  }

  // Get G-plane view, same image size, elements are u8's
  ze_image_view_planar_ext_desc_t img_view_desc_G = {};
  img_view_desc_G.stype = ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC;
  img_view_desc_G.pNext = nullptr;
  img_view_desc_G.planeIndex = 1;

  auto img_fmt_G = img_fmt;
  img_fmt_G.layout = ZE_IMAGE_FORMAT_LAYOUT_8;

  auto img_desc_G = img_desc;
  img_desc_G.pNext = &img_view_desc_G;
  img_desc_G.format = img_fmt_G;

  ze_image_handle_t img_view_G = nullptr;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeImageViewCreateExt(context, device, &img_desc_G, img, &img_view_G));
  if (img_view_G == nullptr) {
    lzt::destroy_ze_image(img_view_R);
    lzt::destroy_ze_image(img);
    FAIL();
  }

  // Get B-plane view, same image size, elements are u8's
  ze_image_view_planar_ext_desc_t img_view_desc_B = {};
  img_view_desc_B.stype = ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC;
  img_view_desc_B.pNext = nullptr;
  img_view_desc_B.planeIndex = 2;

  auto img_fmt_B = img_fmt;
  img_fmt_B.layout = ZE_IMAGE_FORMAT_LAYOUT_8;

  auto img_desc_B = img_desc;
  img_desc_B.pNext = &img_view_desc_B;
  img_desc_B.format = img_fmt_B;

  ze_image_handle_t img_view_B = nullptr;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeImageViewCreateExt(context, device, &img_desc_B, img, &img_view_B));
  if (img_view_B == nullptr) {
    lzt::destroy_ze_image(img_view_G);
    lzt::destroy_ze_image(img_view_R);
    lzt::destroy_ze_image(img);
    FAIL();
  }

  lzt::destroy_ze_image(img_view_B);
  lzt::destroy_ze_image(img_view_G);
  lzt::destroy_ze_image(img_view_R);
  lzt::destroy_ze_image(img);
}

} // namespace
