/*
 *
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

namespace {

class zeImageViewCreateTests : public ::testing::Test {
public:
  void SetUp() override {
    supported_img_types = lzt::get_supported_image_types(
        lzt::zeDevice::get_instance()->get_device());
    if (supported_img_types.size() == 0) {
      LOG_INFO << "Device does not support images";
      GTEST_SKIP();
    }
  }

  void validate_view(ze_image_handle_t img, ze_image_handle_t view) {
    if (view == nullptr) {
      lzt::destroy_ze_image(img);
      FAIL();
    }
    lzt::destroy_ze_image(view);
  }

  virtual void check_extensions() = 0;
  virtual ze_image_handle_t
  create_image_view(ze_image_format_layout_t view_layout, uint32_t plane_index,
                    lzt::Dims view_dims = {0, 0, 0}) const = 0;

  ze_image_desc_t create_image_desc_view(ze_image_type_t img_type,
                                         ze_image_format_layout_t layout);

  virtual void run_test(ze_image_type_t img_type,
                        ze_image_format_layout_t layout);

  void
  RunGivenImageLayoutAndType(ze_image_format_layout_t layout,
                             const std::vector<ze_image_type_t> &tested_types) {
    for (const auto &type : tested_types) {
      run_test(type, layout);
    }
  }

  ze_image_desc_t img_desc;
  ze_image_handle_t img;
  std::vector<ze_image_type_t> supported_img_types;
};

ze_image_desc_t zeImageViewCreateTests::create_image_desc_view(
    ze_image_type_t img_type, ze_image_format_layout_t layout) {
  lzt::Dims img_dims = lzt::get_sample_image_dims(img_type);

  uint32_t array_levels = 0;
  if (img_type == ZE_IMAGE_TYPE_1DARRAY) {
    array_levels = img_dims.height;
  }
  if (img_type == ZE_IMAGE_TYPE_2DARRAY) {
    array_levels = img_dims.depth;
  }

  ze_image_format_t img_fmt = {};
  img_fmt.layout = layout;
  img_fmt.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  img_fmt.x = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.y = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.z = ZE_IMAGE_FORMAT_SWIZZLE_X;
  img_fmt.w = ZE_IMAGE_FORMAT_SWIZZLE_X;

  ze_image_desc_t img_desc = {};
  img_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  img_desc.pNext = nullptr;
  img_desc.flags = 0;
  img_desc.type = img_type;
  img_desc.format = img_fmt;
  img_desc.width = img_dims.width;
  img_desc.height = img_dims.height;
  img_desc.depth = img_dims.depth;
  img_desc.arraylevels = array_levels;
  img_desc.miplevels = 0;

  return img_desc;
}

void zeImageViewCreateTests::run_test(ze_image_type_t img_type,
                                      ze_image_format_layout_t layout) {
  LOG_INFO << "RUN";
  LOG_INFO << img_type;
  check_extensions();
  if (std::find(supported_img_types.begin(), supported_img_types.end(),
                img_type) == supported_img_types.end()) {
    LOG_WARNING << img_type << "unsupported type";
    return;
  }
  img_desc = create_image_desc_view(img_type, layout);
  img = lzt::create_ze_image(lzt::get_default_context(),
                             lzt::get_default_device(lzt::get_default_driver()),
                             img_desc);
  EXPECT_TRUE(img != nullptr);

  switch (layout) {
  case ZE_IMAGE_FORMAT_LAYOUT_NV12: {
    // Get Y-plane view, same image size, elements are u8's
    ze_image_handle_t view_Y = create_image_view(ZE_IMAGE_FORMAT_LAYOUT_8, 0);
    validate_view(img, view_Y);

    // Get UV-plane view, half image size, elements are (u8, u8)'s
    ze_image_handle_t view_UV = create_image_view(
        ZE_IMAGE_FORMAT_LAYOUT_8_8, 1,
        {img_desc.width / 2, img_desc.height / 2, img_desc.depth});
    validate_view(img, view_UV);

    lzt::destroy_ze_image(img);
    break;
  }
  case ZE_IMAGE_FORMAT_LAYOUT_RGBP: {
    // Get R, G, B planes views, same image size, elements are u8's
    std::array<uint32_t, 3> planes = {0, 1, 2};
    for (auto &plane : planes) {
      ze_image_handle_t view = create_image_view(ZE_IMAGE_FORMAT_LAYOUT_8, 0);
      validate_view(img, view);
    }
    lzt::destroy_ze_image(img);
    break;
  }
  case ZE_IMAGE_FORMAT_LAYOUT_8: {
    ze_image_handle_t view = create_image_view(ZE_IMAGE_FORMAT_LAYOUT_8, 0);
    validate_view(img, view);
    lzt::destroy_ze_image(img);
    break;
  }
  default:
    LOG_WARNING << "Unhandled format layout: " << layout;
    break;
  }
}

class zeImageViewCreateExtTests : public zeImageViewCreateTests {
public:
  virtual ze_image_handle_t
  create_image_view(ze_image_format_layout_t view_layout, uint32_t plane_index,
                    lzt::Dims view_dims = {0, 0, 0}) const override;

  virtual void check_extensions() override {
    if (!lzt::check_if_extension_supported(lzt::get_default_driver(),
                                           ZE_IMAGE_VIEW_EXT_NAME)) {
      GTEST_SKIP() << "Missing ZE_extension_image_view support\n";
    }
    if (!lzt::check_if_extension_supported(lzt::get_default_driver(),
                                           ZE_IMAGE_VIEW_PLANAR_EXT_NAME)) {
      GTEST_SKIP() << "Missing ZE_extension_image_view_planar support\n";
    }
  }
};

ze_image_handle_t zeImageViewCreateExtTests::create_image_view(
    ze_image_format_layout_t view_layout, uint32_t plane_index,
    lzt::Dims view_dims) const {
  ze_image_view_planar_ext_desc_t img_view_desc = {};
  img_view_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC;
  img_view_desc.pNext = nullptr;
  img_view_desc.planeIndex = plane_index;

  auto img_desc_view = img_desc;
  img_desc_view.pNext = &img_view_desc;
  img_desc_view.format.layout = view_layout;
  if (view_dims.width != 0) {
    img_desc_view.width = view_dims.width;
    img_desc_view.height = view_dims.height;
    img_desc_view.depth = view_dims.depth;
  }

  ze_image_handle_t img_view = nullptr;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeImageViewCreateExt(lzt::get_default_context(),
                           lzt::get_default_device(lzt::get_default_driver()),
                           &img_desc_view, img, &img_view));
  return img_view;
}

TEST_F(zeImageViewCreateExtTests,
       GivenPlanarNV12ImageThenImageViewCreateExtReturnsSuccess) {
  std::vector<ze_image_type_t> tested_types = {ZE_IMAGE_TYPE_2D,
                                               ZE_IMAGE_TYPE_2DARRAY};
  RunGivenImageLayoutAndType(ZE_IMAGE_FORMAT_LAYOUT_NV12, tested_types);
}

TEST_F(zeImageViewCreateExtTests,
       GivenPlanarRGBPImageThenImageViewCreateExtReturnsSuccess) {
  std::vector<ze_image_type_t> tested_types = {ZE_IMAGE_TYPE_2D,
                                               ZE_IMAGE_TYPE_2DARRAY};
  RunGivenImageLayoutAndType(ZE_IMAGE_FORMAT_LAYOUT_RGBP, tested_types);
}

TEST_F(zeImageViewCreateExtTests,
       GivenOneComponent8ByteUintImageThenImageViewCreateExtReturnsSuccess) {
  std::vector<ze_image_type_t> tested_types = {
      ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_1DARRAY, ZE_IMAGE_TYPE_2D,
      ZE_IMAGE_TYPE_2DARRAY, ZE_IMAGE_TYPE_3D};
  RunGivenImageLayoutAndType(ZE_IMAGE_FORMAT_LAYOUT_8, tested_types);
}

class zeImageViewCreateExpTests : public zeImageViewCreateTests {
  virtual void check_extensions() override {
    if (!lzt::check_if_extension_supported(lzt::get_default_driver(),
                                           ZE_IMAGE_VIEW_EXP_NAME)) {
      GTEST_SKIP() << "Missing ZE_experimental_image_view support\n";
    }
    if (!lzt::check_if_extension_supported(lzt::get_default_driver(),
                                           ZE_IMAGE_VIEW_PLANAR_EXP_NAME)) {
      GTEST_SKIP() << "Missing ZE_experimental_image_view_planar support\n";
    }
  }

  virtual ze_image_handle_t
  create_image_view(ze_image_format_layout_t view_layout, uint32_t plane_index,
                    lzt::Dims view_dims = {0, 0, 0}) const override;
};

ze_image_handle_t zeImageViewCreateExpTests::create_image_view(
    ze_image_format_layout_t view_layout, uint32_t plane_index,
    lzt::Dims view_dims) const {
  ze_image_view_planar_ext_desc_t img_view_desc = {};
  img_view_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXP_DESC;
  img_view_desc.pNext = nullptr;
  img_view_desc.planeIndex = plane_index;

  auto img_desc_view = img_desc;
  img_desc_view.pNext = &img_view_desc;
  img_desc_view.format.layout = view_layout;
  if (view_dims.width != 0) {
    img_desc_view.width = view_dims.width;
    img_desc_view.height = view_dims.height;
    img_desc_view.depth = view_dims.depth;
  }

  ze_image_handle_t img_view = nullptr;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeImageViewCreateExp(lzt::get_default_context(),
                           lzt::get_default_device(lzt::get_default_driver()),
                           &img_desc_view, img, &img_view));
  return img_view;
}

TEST_F(zeImageViewCreateExpTests,
       GivenPlanarNV12ImageThenImageViewCreateExpReturnsSuccess) {
  std::vector<ze_image_type_t> tested_types = {ZE_IMAGE_TYPE_2D,
                                               ZE_IMAGE_TYPE_2DARRAY};
  RunGivenImageLayoutAndType(ZE_IMAGE_FORMAT_LAYOUT_NV12, tested_types);
}

TEST_F(zeImageViewCreateExpTests,
       GivenPlanarRGBPImageThenImageViewCreateExpReturnsSuccess) {
  std::vector<ze_image_type_t> tested_types = {ZE_IMAGE_TYPE_2D,
                                               ZE_IMAGE_TYPE_2DARRAY};
  RunGivenImageLayoutAndType(ZE_IMAGE_FORMAT_LAYOUT_RGBP, tested_types);
}

TEST_F(zeImageViewCreateExpTests,
       GivenOneComponent8ByteUintImageThenImageViewCreateExpReturnsSuccess) {
  std::vector<ze_image_type_t> tested_types = {
      ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_1DARRAY, ZE_IMAGE_TYPE_2D,
      ZE_IMAGE_TYPE_2DARRAY, ZE_IMAGE_TYPE_3D};
  RunGivenImageLayoutAndType(ZE_IMAGE_FORMAT_LAYOUT_8, tested_types);
}

} // namespace
