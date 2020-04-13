/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

namespace {

class zeImageCreateTests : public ::testing::Test {};

void image_create_test_1d(ze_image_format_type_t format_type,
                          ze_image_format_layout_t layout, bool useArrayImage) {

  for (auto image_rw_flag : lzt::image_rw_flags) {
    for (auto image_cache_flag : lzt::image_cache_flags) {
      for (auto image_width : lzt::image_widths) {

        ze_image_type_t image_type;
        uint32_t array_levels;
        if (useArrayImage) {
          image_type = ZE_IMAGE_TYPE_1DARRAY;
          array_levels = 1;
        } else {
          image_type = ZE_IMAGE_TYPE_1D;
          array_levels = 0;
        }

        ze_image_handle_t image;
        ze_image_format_desc_t format_descriptor = {
            layout,                    // layout
            format_type,               // type
            ZE_IMAGE_FORMAT_SWIZZLE_R, // x
            ZE_IMAGE_FORMAT_SWIZZLE_G, // y
            ZE_IMAGE_FORMAT_SWIZZLE_B, // z
            ZE_IMAGE_FORMAT_SWIZZLE_A  // w
        };
        ze_image_flag_t flags =
            (ze_image_flag_t)(image_rw_flag | image_cache_flag);
        ze_image_desc_t image_descriptor = {
            ZE_IMAGE_DESC_VERSION_CURRENT, // version
            flags,                         // flags
            image_type,                    // type
            format_descriptor,             // format
            image_width,                   // width
            1,                             // height
            1,                             // depth
            array_levels,                  // arraylevels
            0};                            // miplevels

        lzt::print_image_descriptor(image_descriptor);
        lzt::create_ze_image(image, &image_descriptor);
        lzt::get_ze_image_properties(image_descriptor);
        lzt::destroy_ze_image(image);
      }
    }
  }
}

void image_create_test_2d(ze_image_format_type_t format_type,
                          ze_image_format_layout_t layout, bool useArrayImage) {

  for (auto image_rw_flag : lzt::image_rw_flags) {
    for (auto image_cache_flag : lzt::image_cache_flags) {
      for (auto image_width : lzt::image_widths) {
        for (auto image_height : lzt::image_heights) {

          ze_image_type_t image_type;
          uint32_t array_levels;
          if (useArrayImage) {
            image_type = ZE_IMAGE_TYPE_2DARRAY;
            array_levels = 2;
          } else {
            image_type = ZE_IMAGE_TYPE_2D;
            array_levels = 0;
          }

          ze_image_handle_t image;
          ze_image_format_desc_t format_descriptor = {
              layout,                    // layout
              format_type,               // type
              ZE_IMAGE_FORMAT_SWIZZLE_R, // x
              ZE_IMAGE_FORMAT_SWIZZLE_G, // y
              ZE_IMAGE_FORMAT_SWIZZLE_B, // z
              ZE_IMAGE_FORMAT_SWIZZLE_A  // w
          };
          ze_image_flag_t flags =
              (ze_image_flag_t)(image_rw_flag | image_cache_flag);
          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              flags,                         // flags
              image_type,                    // type
              format_descriptor,             // format
              image_width,                   // width
              image_height,                  // height
              1,                             // depth
              array_levels,                  // arraylevels
              0};                            // miplevels

          lzt::print_image_descriptor(image_descriptor);
          lzt::create_ze_image(image, &image_descriptor);
          lzt::get_ze_image_properties(image_descriptor);
          lzt::destroy_ze_image(image);
        }
      }
    }
  }
}

void image_create_test_3d(ze_image_format_type_t format_type,
                          ze_image_format_layout_t layout) {

  for (auto image_rw_flag : lzt::image_rw_flags) {
    for (auto image_cache_flag : lzt::image_cache_flags) {
      for (auto image_width : lzt::image_widths) {
        for (auto image_height : lzt::image_heights) {
          for (auto image_depth : lzt::image_depths) {

            ze_image_type_t image_type = ZE_IMAGE_TYPE_3D;
            ze_image_handle_t image;
            ze_image_format_desc_t format_descriptor = {
                layout,                    // layout
                format_type,               // type
                ZE_IMAGE_FORMAT_SWIZZLE_R, // x
                ZE_IMAGE_FORMAT_SWIZZLE_G, // y
                ZE_IMAGE_FORMAT_SWIZZLE_B, // z
                ZE_IMAGE_FORMAT_SWIZZLE_A  // w
            };
            ze_image_flag_t flags =
                (ze_image_flag_t)(image_rw_flag | image_cache_flag);
            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                flags,                         // flags
                image_type,                    // type
                format_descriptor,             // format
                image_width,                   // width
                image_height,                  // height
                image_depth,                   // depth
                0,                             // arraylevels
                0};                            // miplevels

            lzt::print_image_descriptor(image_descriptor);
            lzt::create_ze_image(image, &image_descriptor);
            lzt::get_ze_image_properties(image_descriptor);
            lzt::destroy_ze_image(image);
          }
        }
      }
    }
  }
}

TEST(zeImageCreateTests,
     GivenValidDescriptorWhenCreatingUINTImageThenNotNullPointerIsReturned) {

  for (auto layout : lzt::image_format_layout_uint) {
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_UINT, layout, true);
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_UINT, layout, false);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_UINT, layout, true);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_UINT, layout, false);
    image_create_test_3d(ZE_IMAGE_FORMAT_TYPE_UINT, layout);
  }
}

TEST(zeImageCreateTests,
     GivenValidDescriptorWhenCreatingSINTImageThenNotNullPointerIsReturned) {

  for (auto layout : lzt::image_format_layout_sint) {
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_SINT, layout, true);
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_SINT, layout, false);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_SINT, layout, true);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_SINT, layout, false);
    image_create_test_3d(ZE_IMAGE_FORMAT_TYPE_SINT, layout);
  }
}

TEST(zeImageCreateTests,
     GivenValidDescriptorWhenCreatingUNORMImageThenNotNullPointerIsReturned) {

  for (auto layout : lzt::image_format_layout_unorm) {
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_UNORM, layout, true);
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_UNORM, layout, false);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_UNORM, layout, true);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_UNORM, layout, false);
    image_create_test_3d(ZE_IMAGE_FORMAT_TYPE_UNORM, layout);
  }
}

TEST(zeImageCreateTests,
     GivenValidDescriptorWhenCreatingSNORMImageThenNotNullPointerIsReturned) {

  for (auto layout : lzt::image_format_layout_snorm) {
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_SNORM, layout, true);
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_SNORM, layout, false);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_SNORM, layout, true);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_SNORM, layout, false);
    image_create_test_3d(ZE_IMAGE_FORMAT_TYPE_SNORM, layout);
  }
}

TEST(zeImageCreateTests,
     GivenValidDescriptorWhenCreatingFLOATImageThenNotNullPointerIsReturned) {

  for (auto layout : lzt::image_format_layout_float) {
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, true);
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, false);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, true);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, false);
    image_create_test_3d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout);
  }
}

TEST(zeImageCreateTests,
     GivenValidDescriptorWhenCreatingMediaImageThenNotNullPointerIsReturned) {

  for (auto layout : lzt::image_format_media_layouts) {
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, true);
    image_create_test_1d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, false);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, true);
    image_create_test_2d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout, false);
    image_create_test_3d(ZE_IMAGE_FORMAT_TYPE_FLOAT, layout);
  }
}

} // namespace
