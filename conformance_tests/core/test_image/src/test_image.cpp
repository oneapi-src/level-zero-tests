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

class zeImageCreateDefaultTests : public ::testing::Test {};

class zeImage1DSwizzleCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImage1DSwizzleGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImageArray1DSwizzleCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImageArray1DSwizzleGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImage2DSwizzleCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImage2DSwizzleGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImageArray2DSwizzleCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImageArray2DSwizzleGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImage3DSwizzleCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImage3DSwizzleGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImageArray3DSwizzleCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};
class zeImageArray3DSwizzleGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

class zeImage4DSwizzleCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};
class zeImage4DSwizzleGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};
class zeImageArray4DSwizzleCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};
class zeImageArray4DSwizzleGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};
class zeImageMediaCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};
class zeImageMediaGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};
class zeImageArrayMediaCreateTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};
class zeImageArrayMediaGetPropertiesTests
    : public lzt::zeImageCreateCommonTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_format_layout_t, ze_image_format_type_t>> {};

TEST_F(
    zeImageCreateDefaultTests,
    GivenDefaultImageDescriptorWhenCreatingImageThenNotNullPointerIsReturned) {
  ze_image_handle_t image;
  lzt::create_ze_image(image);
  lzt::destroy_ze_image(image);
}

TEST_P(
    zeImage1DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating1DImageWith1DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {

        ze_image_handle_t image;
        ze_image_format_desc_t format_descriptor = {
            std::get<0>(GetParam()),   // layout
            std::get<1>(GetParam()),   // type
            swizzle_x,                 // x
            ZE_IMAGE_FORMAT_SWIZZLE_X, // y
            ZE_IMAGE_FORMAT_SWIZZLE_X, // z
            ZE_IMAGE_FORMAT_SWIZZLE_X  // w
        };

        ze_image_desc_t image_descriptor = {
            ZE_IMAGE_DESC_VERSION_CURRENT, // version
            image_create_flags,            // flags
            ZE_IMAGE_TYPE_1D,              // type
            format_descriptor,             // format
            image_width,                   // width
            0,                             // height
            0,                             // depth
            0,                             // arraylevels
            0};                            // miplevels

        lzt::print_image_descriptor(image_descriptor);
        lzt::create_ze_image(image, &image_descriptor);
        lzt::destroy_ze_image(image);
      }
    }
  }
}

TEST_P(
    zeImage1DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating2DImageWith1DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {

          ze_image_handle_t image;
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              swizzle_x,                 // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_2D,              // type
              format_descriptor,             // format
              image_width,                   // width
              image_height,                  // height
              0,                             // depth
              0,                             // arraylevels
              0};                            // miplevels
          lzt::print_image_descriptor(image_descriptor);
          lzt::create_ze_image(image, &image_descriptor);
          lzt::destroy_ze_image(image);
        }
      }
    }
  }
}

TEST_P(
    zeImage1DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating3DImageWith1DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          for (auto swizzle_x : lzt::image_format_swizzles) {

            ze_image_handle_t image;
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),   // layout
                std::get<1>(GetParam()),   // type
                swizzle_x,                 // x
                ZE_IMAGE_FORMAT_SWIZZLE_X, // y
                ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                ZE_IMAGE_FORMAT_SWIZZLE_X  // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_3D,              // type
                format_descriptor,             // format
                image_width,                   // width
                image_height,                  // height
                image_depth,                   // depth
                0,                             // arraylevels
                0};                            // miplevels

            lzt::print_image_descriptor(image_descriptor);
            lzt::create_ze_image(image, &image_descriptor);
            lzt::destroy_ze_image(image);
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(1DSwizzleImageCreationCombinations,
                        zeImage1DSwizzleCreateTests,
                        ::testing::Combine(lzt::image_format_1d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArray1DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating1DArrayImageWith1DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto array_level : lzt::image_array_levels) {

          ze_image_handle_t image;
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              swizzle_x,                 // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_1DARRAY,         // type
              format_descriptor,             // format
              image_width,                   // width
              0,                             // height
              0,                             // depth
              array_level,                   // arraylevels
              0};                            // miplevels
          lzt::print_image_descriptor(image_descriptor);
          lzt::create_ze_image(image, &image_descriptor);
          lzt::destroy_ze_image(image);
        }
      }
    }
  }
}

TEST_P(
    zeImageArray1DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating2DArrayImageWith1DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto array_level : lzt::image_array_levels) {

            ze_image_handle_t image;
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),   // layout
                std::get<1>(GetParam()),   // type
                swizzle_x,                 // x
                ZE_IMAGE_FORMAT_SWIZZLE_X, // y
                ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                ZE_IMAGE_FORMAT_SWIZZLE_X  // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_2DARRAY,         // type
                format_descriptor,             // format
                image_width,                   // width
                image_height,                  // height
                0,                             // depth
                array_level,                   // arraylevels
                0};                            // miplevels
            lzt::print_image_descriptor(image_descriptor);
            lzt::create_ze_image(image, &image_descriptor);
            lzt::destroy_ze_image(image);
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(1DSwizzleArrayImageCreationCombinations,
                        zeImageArray1DSwizzleCreateTests,
                        ::testing::Combine(lzt::image_format_1d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImage2DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating1DImageWith2DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {

          ze_image_handle_t image;
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              swizzle_x,                 // x
              swizzle_y,                 // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_1D,              // type
              format_descriptor,             // format
              image_width,                   // width
              0,                             // height
              0,                             // depth
              0,                             // arraylevels
              0};                            // miplevels

          lzt::print_image_descriptor(image_descriptor);
          lzt::create_ze_image(image, &image_descriptor);
          lzt::destroy_ze_image(image);
        }
      }
    }
  }
}

TEST_P(
    zeImage2DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating2DImageWith2DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {

            ze_image_handle_t image;
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),   // layout
                std::get<1>(GetParam()),   // type
                swizzle_x,                 // x
                swizzle_y,                 // y
                ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                ZE_IMAGE_FORMAT_SWIZZLE_X  // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_2D,              // type
                format_descriptor,             // format
                image_width,                   // width
                image_height,                  // height
                0,                             // depth
                0,                             // arraylevels
                0};                            // miplevels
            lzt::print_image_descriptor(image_descriptor);
            lzt::create_ze_image(image, &image_descriptor);
            lzt::destroy_ze_image(image);
          }
        }
      }
    }
  }
}

TEST_P(
    zeImage2DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating3DImageWith2DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          for (auto swizzle_x : lzt::image_format_swizzles) {
            for (auto swizzle_y : lzt::image_format_swizzles) {

              ze_image_handle_t image;
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()),   // layout
                  std::get<1>(GetParam()),   // type
                  swizzle_x,                 // x
                  swizzle_y,                 // y
                  ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                  ZE_IMAGE_FORMAT_SWIZZLE_X  // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_3D,              // type
                  format_descriptor,             // format
                  image_width,                   // width
                  image_height,                  // height
                  image_depth,                   // depth
                  0,                             // arraylevels
                  0};                            // miplevels

              lzt::print_image_descriptor(image_descriptor);
              lzt::create_ze_image(image, &image_descriptor);
              lzt::destroy_ze_image(image);
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(2DSwizzleImageCreationCombinations,
                        zeImage2DSwizzleCreateTests,
                        ::testing::Combine(lzt::image_format_2d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArray2DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating1DArrayImageWith2DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto array_level : lzt::image_array_levels) {

            ze_image_handle_t image;
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),   // layout
                std::get<1>(GetParam()),   // type
                swizzle_x,                 // x
                swizzle_y,                 // y
                ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                ZE_IMAGE_FORMAT_SWIZZLE_X  // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_1DARRAY,         // type
                format_descriptor,             // format
                image_width,                   // width
                0,                             // height
                0,                             // depth
                array_level,                   // arraylevels
                0};                            // miplevels
            lzt::print_image_descriptor(image_descriptor);
            lzt::create_ze_image(image, &image_descriptor);
            lzt::destroy_ze_image(image);
          }
        }
      }
    }
  }
}

TEST_P(
    zeImageArray2DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating2DArrayImageWith2DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            for (auto array_level : lzt::image_array_levels) {

              ze_image_handle_t image;
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()),   // layout
                  std::get<1>(GetParam()),   // type
                  swizzle_x,                 // x
                  swizzle_y,                 // y
                  ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                  ZE_IMAGE_FORMAT_SWIZZLE_X  // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_2DARRAY,         // type
                  format_descriptor,             // format
                  image_width,                   // width
                  image_height,                  // height
                  0,                             // depth
                  array_level,                   // arraylevels
                  0};                            // miplevels
              lzt::print_image_descriptor(image_descriptor);
              lzt::create_ze_image(image, &image_descriptor);
              lzt::destroy_ze_image(image);
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(2DSwizzleArrayImageCreationCombinations,
                        zeImageArray2DSwizzleCreateTests,
                        ::testing::Combine(lzt::image_format_2d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImage3DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating1DImageWith3DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {

            ze_image_handle_t image;
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),  // layout
                std::get<1>(GetParam()),  // type
                swizzle_x,                // x
                swizzle_y,                // y
                swizzle_z,                // z
                ZE_IMAGE_FORMAT_SWIZZLE_X // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_1D,              // type
                format_descriptor,             // format
                image_width,                   // width
                0,                             // height
                0,                             // depth
                0,                             // arraylevels
                0};                            // miplevels

            lzt::print_image_descriptor(image_descriptor);
            lzt::create_ze_image(image, &image_descriptor);
            lzt::destroy_ze_image(image);
          }
        }
      }
    }
  }
}

TEST_P(
    zeImage3DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating2DImageWith3DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            for (auto swizzle_z : lzt::image_format_swizzles) {

              ze_image_handle_t image;
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()),  // layout
                  std::get<1>(GetParam()),  // type
                  swizzle_x,                // x
                  swizzle_y,                // y
                  swizzle_z,                // z
                  ZE_IMAGE_FORMAT_SWIZZLE_X // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_2D,              // type
                  format_descriptor,             // format
                  image_width,                   // width
                  image_height,                  // height
                  0,                             // depth
                  0,                             // arraylevels
                  0};                            // miplevels
              lzt::print_image_descriptor(image_descriptor);
              lzt::create_ze_image(image, &image_descriptor);
              lzt::destroy_ze_image(image);
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImage3DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating3DImageWith3DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          for (auto swizzle_x : lzt::image_format_swizzles) {
            for (auto swizzle_y : lzt::image_format_swizzles) {
              for (auto swizzle_z : lzt::image_format_swizzles) {

                ze_image_handle_t image;
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()),  // layout
                    std::get<1>(GetParam()),  // type
                    swizzle_x,                // x
                    swizzle_y,                // y
                    swizzle_z,                // z
                    ZE_IMAGE_FORMAT_SWIZZLE_X // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_3D,              // type
                    format_descriptor,             // format
                    image_width,                   // width
                    image_height,                  // height
                    image_depth,                   // depth
                    0,                             // arraylevels
                    0};                            // miplevels

                lzt::print_image_descriptor(image_descriptor);
                lzt::create_ze_image(image, &image_descriptor);
                lzt::destroy_ze_image(image);
              }
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(3DSwizzleImageCreationCombinations,
                        zeImage3DSwizzleCreateTests,
                        ::testing::Combine(lzt::image_format_3d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArray3DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating1DArrayImageWith3DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {
            for (auto array_level : lzt::image_array_levels) {

              ze_image_handle_t image;
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()),  // layout
                  std::get<1>(GetParam()),  // type
                  swizzle_x,                // x
                  swizzle_y,                // y
                  swizzle_z,                // z
                  ZE_IMAGE_FORMAT_SWIZZLE_X // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_1DARRAY,         // type
                  format_descriptor,             // format
                  image_width,                   // width
                  0,                             // height
                  0,                             // depth
                  array_level,                   // arraylevels
                  0};                            // miplevels
              lzt::print_image_descriptor(image_descriptor);
              lzt::create_ze_image(image, &image_descriptor);
              lzt::destroy_ze_image(image);
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImageArray3DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating2DArrayImageWith3DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            for (auto swizzle_z : lzt::image_format_swizzles) {
              for (auto array_level : lzt::image_array_levels) {

                ze_image_handle_t image;
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()),  // layout
                    std::get<1>(GetParam()),  // type
                    swizzle_x,                // x
                    swizzle_y,                // y
                    swizzle_z,                // z
                    ZE_IMAGE_FORMAT_SWIZZLE_X // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_2DARRAY,         // type
                    format_descriptor,             // format
                    image_width,                   // width
                    image_height,                  // height
                    0,                             // depth
                    array_level,                   // arraylevels
                    0};                            // miplevels
                lzt::print_image_descriptor(image_descriptor);
                lzt::create_ze_image(image, &image_descriptor);
                lzt::destroy_ze_image(image);
              }
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(3DSwizzleArrayImageCreationCombinations,
                        zeImageArray3DSwizzleCreateTests,
                        ::testing::Combine(lzt::image_format_3d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImage4DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating1DImageWith4DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {
            for (auto swizzle_w : lzt::image_format_swizzles) {

              ze_image_handle_t image;
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()), // layout
                  std::get<1>(GetParam()), // type
                  swizzle_x,               // x
                  swizzle_y,               // y
                  swizzle_z,               // z
                  swizzle_w                // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_1D,              // type
                  format_descriptor,             // format
                  image_width,                   // width
                  0,                             // height
                  0,                             // depth
                  0,                             // arraylevels
                  0};                            // miplevels

              lzt::print_image_descriptor(image_descriptor);
              lzt::create_ze_image(image, &image_descriptor);
              lzt::destroy_ze_image(image);
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImage4DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating2DImageWith4DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            for (auto swizzle_z : lzt::image_format_swizzles) {
              for (auto swizzle_w : lzt::image_format_swizzles) {

                ze_image_handle_t image;
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()), // layout
                    std::get<1>(GetParam()), // type
                    swizzle_x,               // x
                    swizzle_y,               // y
                    swizzle_z,               // z
                    swizzle_w                // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_2D,              // type
                    format_descriptor,             // format
                    image_width,                   // width
                    image_height,                  // height
                    0,                             // depth
                    0,                             // arraylevels
                    0};                            // miplevels
                lzt::print_image_descriptor(image_descriptor);
                lzt::create_ze_image(image, &image_descriptor);
                lzt::destroy_ze_image(image);
              }
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImage4DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating3DImageWith4DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          for (auto swizzle_x : lzt::image_format_swizzles) {
            for (auto swizzle_y : lzt::image_format_swizzles) {
              for (auto swizzle_z : lzt::image_format_swizzles) {
                for (auto swizzle_w : lzt::image_format_swizzles) {

                  ze_image_handle_t image;
                  ze_image_format_desc_t format_descriptor = {
                      std::get<0>(GetParam()), // layout
                      std::get<1>(GetParam()), // type
                      swizzle_x,               // x
                      swizzle_y,               // y
                      swizzle_z,               // z
                      swizzle_w                // w
                  };

                  ze_image_desc_t image_descriptor = {
                      ZE_IMAGE_DESC_VERSION_CURRENT, // version
                      image_create_flags,            // flags
                      ZE_IMAGE_TYPE_3D,              // type
                      format_descriptor,             // format
                      image_width,                   // width
                      image_height,                  // height
                      image_depth,                   // depth
                      0,                             // arraylevels
                      0};                            // miplevels

                  lzt::print_image_descriptor(image_descriptor);
                  lzt::create_ze_image(image, &image_descriptor);
                  lzt::destroy_ze_image(image);
                }
              }
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(4DSwizzleImageCreationCombinations,
                        zeImage4DSwizzleCreateTests,
                        ::testing::Combine(lzt::image_format_4d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArray4DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating1DArrayImageWith4DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {
            for (auto swizzle_w : lzt::image_format_swizzles) {
              for (auto array_level : lzt::image_array_levels) {

                ze_image_handle_t image;
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()), // layout
                    std::get<1>(GetParam()), // type
                    swizzle_x,               // x
                    swizzle_y,               // y
                    swizzle_z,               // z
                    swizzle_w                // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_1DARRAY,         // type
                    format_descriptor,             // format
                    image_width,                   // width
                    0,                             // height
                    0,                             // depth
                    array_level,                   // arraylevels
                    0};                            // miplevels
                lzt::print_image_descriptor(image_descriptor);
                lzt::create_ze_image(image, &image_descriptor);
                lzt::destroy_ze_image(image);
              }
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImageArray4DSwizzleCreateTests,
    GivenValidDescriptorWhenCreating2DArrayImageWith4DSwizzleFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            for (auto swizzle_z : lzt::image_format_swizzles) {
              for (auto swizzle_w : lzt::image_format_swizzles) {
                for (auto array_level : lzt::image_array_levels) {

                  ze_image_handle_t image;
                  ze_image_format_desc_t format_descriptor = {
                      std::get<0>(GetParam()), // layout
                      std::get<1>(GetParam()), // type
                      swizzle_x,               // x
                      swizzle_y,               // y
                      swizzle_z,               // z
                      swizzle_w                // w
                  };

                  ze_image_desc_t image_descriptor = {
                      ZE_IMAGE_DESC_VERSION_CURRENT, // version
                      image_create_flags,            // flags
                      ZE_IMAGE_TYPE_2DARRAY,         // type
                      format_descriptor,             // format
                      image_width,                   // width
                      image_height,                  // height
                      0,                             // depth
                      array_level,                   // arraylevels
                      0};                            // miplevels
                  lzt::print_image_descriptor(image_descriptor);
                  lzt::create_ze_image(image, &image_descriptor);
                  lzt::destroy_ze_image(image);
                }
              }
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(4DSwizzleArrayImageCreationCombinations,
                        zeImageArray4DSwizzleCreateTests,
                        ::testing::Combine(lzt::image_format_4d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageMediaCreateTests,
    GivenValidDescriptorWhenCreating1DImageWithMediaFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {

      ze_image_handle_t image;
      ze_image_format_desc_t format_descriptor = {
          std::get<0>(GetParam()),   // layout
          std::get<1>(GetParam()),   // type
          ZE_IMAGE_FORMAT_SWIZZLE_X, // x
          ZE_IMAGE_FORMAT_SWIZZLE_X, // y
          ZE_IMAGE_FORMAT_SWIZZLE_X, // z
          ZE_IMAGE_FORMAT_SWIZZLE_X  // w
      };

      ze_image_desc_t image_descriptor = {
          ZE_IMAGE_DESC_VERSION_CURRENT, // version
          image_create_flags,            // flags
          ZE_IMAGE_TYPE_1D,              // type
          format_descriptor,             // format
          image_width,                   // width
          0,                             // height
          0,                             // depth
          0,                             // arraylevels
          0};                            // miplevels
      lzt::print_image_descriptor(image_descriptor);
      lzt::create_ze_image(image, &image_descriptor);
      lzt::destroy_ze_image(image);
    }
  }
}

TEST_P(
    zeImageMediaCreateTests,
    GivenValidDescriptorWhenCreating2DImageWithMediaFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {

        ze_image_handle_t image;
        ze_image_format_desc_t format_descriptor = {
            std::get<0>(GetParam()),   // layout
            std::get<1>(GetParam()),   // type
            ZE_IMAGE_FORMAT_SWIZZLE_X, // x
            ZE_IMAGE_FORMAT_SWIZZLE_X, // y
            ZE_IMAGE_FORMAT_SWIZZLE_X, // z
            ZE_IMAGE_FORMAT_SWIZZLE_X  // w
        };

        ze_image_desc_t image_descriptor = {
            ZE_IMAGE_DESC_VERSION_CURRENT, // version
            image_create_flags,            // flags
            ZE_IMAGE_TYPE_2D,              // type
            format_descriptor,             // format
            image_width,                   // width
            image_height,                  // height
            0,                             // depth
            0,                             // arraylevels
            0};                            // miplevels
        lzt::print_image_descriptor(image_descriptor);
        lzt::create_ze_image(image, &image_descriptor);
        lzt::destroy_ze_image(image);
      }
    }
  }
}

TEST_P(
    zeImageMediaCreateTests,
    GivenValidDescriptorWhenCreating3DImageWithMediaFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {

          ze_image_handle_t image;
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              ZE_IMAGE_FORMAT_SWIZZLE_X, // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_3D,              // type
              format_descriptor,             // format
              image_width,                   // width
              image_height,                  // height
              image_depth,                   // depth
              0,                             // arraylevels
              0};                            // miplevels
          lzt::print_image_descriptor(image_descriptor);
          lzt::create_ze_image(image, &image_descriptor);
          lzt::destroy_ze_image(image);
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(MediaImageCreationCombinations, zeImageMediaCreateTests,
                        ::testing::Combine(lzt::image_format_media_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArrayMediaCreateTests,
    GivenValidDescriptorWhenCreating1DArrayImageWithMediaFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto array_level : lzt::image_array_levels) {

        ze_image_handle_t image;
        ze_image_format_desc_t format_descriptor = {
            std::get<0>(GetParam()),   // layout
            std::get<1>(GetParam()),   // type
            ZE_IMAGE_FORMAT_SWIZZLE_X, // x
            ZE_IMAGE_FORMAT_SWIZZLE_X, // y
            ZE_IMAGE_FORMAT_SWIZZLE_X, // z
            ZE_IMAGE_FORMAT_SWIZZLE_X  // w
        };

        ze_image_desc_t image_descriptor = {
            ZE_IMAGE_DESC_VERSION_CURRENT, // version
            image_create_flags,            // flags
            ZE_IMAGE_TYPE_1DARRAY,         // type
            format_descriptor,             // format
            image_width,                   // width
            0,                             // height
            0,                             // depth
            array_level,                   // arraylevels
            0};                            // miplevels
        lzt::print_image_descriptor(image_descriptor);
        lzt::create_ze_image(image, &image_descriptor);
        lzt::destroy_ze_image(image);
      }
    }
  }
}

TEST_P(
    zeImageArrayMediaCreateTests,
    GivenValidDescriptorWhenCreating2DArrayImageWithMediaFormatThenNotNullPointerIsReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto array_level : lzt::image_array_levels) {

          ze_image_handle_t image;
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              ZE_IMAGE_FORMAT_SWIZZLE_X, // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_2DARRAY,         // type
              format_descriptor,             // format
              image_width,                   // width
              image_height,                  // height
              0,                             // depth
              array_level,                   // arraylevels
              0};                            // miplevels
          lzt::print_image_descriptor(image_descriptor);
          lzt::create_ze_image(image, &image_descriptor);
          lzt::destroy_ze_image(image);
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(MediaArrayImageCreationCombinations,
                        zeImageArrayMediaCreateTests,
                        ::testing::Combine(lzt::image_format_media_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImage1DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageWith1DSwizzleThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        ze_image_format_desc_t format_descriptor = {
            std::get<0>(GetParam()),   // layout
            std::get<1>(GetParam()),   // type
            swizzle_x,                 // x
            ZE_IMAGE_FORMAT_SWIZZLE_X, // y
            ZE_IMAGE_FORMAT_SWIZZLE_X, // z
            ZE_IMAGE_FORMAT_SWIZZLE_X  // w
        };

        ze_image_desc_t image_descriptor = {
            ZE_IMAGE_DESC_VERSION_CURRENT, // version
            image_create_flags,            // flags
            ZE_IMAGE_TYPE_1D,              // type
            format_descriptor,             // format
            image_width,                   // width
            0,                             // height
            0,                             // depth
            0,                             // arraylevels
            0};                            // miplevels

        lzt::print_image_descriptor(image_descriptor);
        lzt::get_ze_image_properties(image_descriptor);
      }
    }
  }
}

TEST_P(
    zeImage1DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageWith1DSwizzleThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              swizzle_x,                 // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_2D,              // type
              format_descriptor,             // format
              image_width,                   // width
              image_height,                  // height
              0,                             // depth
              0,                             // arraylevels
              0};                            // miplevels

          lzt::print_image_descriptor(image_descriptor);
          lzt::get_ze_image_properties(image_descriptor);
        }
      }
    }
  }
}

TEST_P(
    zeImage1DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor3DImageWith1DSwizzleThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          for (auto swizzle_x : lzt::image_format_swizzles) {
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),   // layout
                std::get<1>(GetParam()),   // type
                swizzle_x,                 // x
                ZE_IMAGE_FORMAT_SWIZZLE_X, // y
                ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                ZE_IMAGE_FORMAT_SWIZZLE_X  // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_3D,              // type
                format_descriptor,             // format
                image_width,                   // width
                image_height,                  // height
                image_depth,                   // depth
                0,                             // arraylevels
                0};                            // miplevels

            lzt::print_image_descriptor(image_descriptor);
            lzt::get_ze_image_properties(image_descriptor);
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImage1DSwizzleGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_1d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArray1DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageArrayWith1DSwizzleThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto array_level : lzt::image_array_levels) {
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              swizzle_x,                 // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_1DARRAY,         // type
              format_descriptor,             // format
              image_width,                   // width
              0,                             // height
              0,                             // depth
              array_level,                   // arraylevels
              0};                            // miplevels

          lzt::print_image_descriptor(image_descriptor);
          lzt::get_ze_image_properties(image_descriptor);
        }
      }
    }
  }
}

TEST_P(
    zeImageArray1DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageArrayWith1DSwizzleThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto array_level : lzt::image_array_levels) {
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),   // layout
                std::get<1>(GetParam()),   // type
                swizzle_x,                 // x
                ZE_IMAGE_FORMAT_SWIZZLE_X, // y
                ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                ZE_IMAGE_FORMAT_SWIZZLE_X  // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_2DARRAY,         // type
                format_descriptor,             // format
                image_width,                   // width
                image_height,                  // height
                0,                             // depth
                array_level,                   // arraylevels
                0};                            // miplevels

            lzt::print_image_descriptor(image_descriptor);
            lzt::get_ze_image_properties(image_descriptor);
          }
        }
      }
    }
  }
}
INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImageArray1DSwizzleGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_1d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImage2DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageWith2DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              swizzle_x,                 // x
              swizzle_y,                 // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_1D,              // type
              format_descriptor,             // format
              image_width,                   // width
              0,                             // height
              0,                             // depth
              0,                             // arraylevels
              0};                            // miplevels

          lzt::print_image_descriptor(image_descriptor);
          lzt::get_ze_image_properties(image_descriptor);
        }
      }
    }
  }
}

TEST_P(
    zeImage2DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageWith2DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),   // layout
                std::get<1>(GetParam()),   // type
                swizzle_x,                 // x
                swizzle_y,                 // y
                ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                ZE_IMAGE_FORMAT_SWIZZLE_X  // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_2D,              // type
                format_descriptor,             // format
                image_width,                   // width
                image_height,                  // height
                0,                             // depth
                0,                             // arraylevels
                0};                            // miplevels

            lzt::print_image_descriptor(image_descriptor);
            lzt::get_ze_image_properties(image_descriptor);
          }
        }
      }
    }
  }
}

TEST_P(
    zeImage2DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor3DImageWith2DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          for (auto swizzle_x : lzt::image_format_swizzles) {
            for (auto swizzle_y : lzt::image_format_swizzles) {
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()),   // layout
                  std::get<1>(GetParam()),   // type
                  swizzle_x,                 // x
                  swizzle_y,                 // y
                  ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                  ZE_IMAGE_FORMAT_SWIZZLE_X  // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_3D,              // type
                  format_descriptor,             // format
                  image_width,                   // width
                  image_height,                  // height
                  image_depth,                   // depth
                  0,                             // arraylevels
                  0};                            // miplevels

              lzt::print_image_descriptor(image_descriptor);
              lzt::get_ze_image_properties(image_descriptor);
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImage2DSwizzleGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_1d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArray2DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageArrayWith2DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto array_level : lzt::image_array_levels) {
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              swizzle_x,                 // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_1DARRAY,         // type
              format_descriptor,             // format
              image_width,                   // width
              0,                             // height
              0,                             // depth
              array_level,                   // arraylevels
              0};                            // miplevels

          lzt::print_image_descriptor(image_descriptor);
          lzt::get_ze_image_properties(image_descriptor);
        }
      }
    }
  }
}

TEST_P(
    zeImageArray2DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageArrayWith2DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto array_level : lzt::image_array_levels) {
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),   // layout
                std::get<1>(GetParam()),   // type
                swizzle_x,                 // x
                ZE_IMAGE_FORMAT_SWIZZLE_X, // y
                ZE_IMAGE_FORMAT_SWIZZLE_X, // z
                ZE_IMAGE_FORMAT_SWIZZLE_X  // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_2DARRAY,         // type
                format_descriptor,             // format
                image_width,                   // width
                image_height,                  // height
                0,                             // depth
                array_level,                   // arraylevels
                0};                            // miplevels

            lzt::print_image_descriptor(image_descriptor);
            lzt::get_ze_image_properties(image_descriptor);
          }
        }
      }
    }
  }
}
INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImageArray2DSwizzleGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_1d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImage3DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageWith3DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {
            ze_image_format_desc_t format_descriptor = {
                std::get<0>(GetParam()),  // layout
                std::get<1>(GetParam()),  // type
                swizzle_x,                // x
                swizzle_y,                // y
                swizzle_z,                // z
                ZE_IMAGE_FORMAT_SWIZZLE_X // w
            };

            ze_image_desc_t image_descriptor = {
                ZE_IMAGE_DESC_VERSION_CURRENT, // version
                image_create_flags,            // flags
                ZE_IMAGE_TYPE_3D,              // type
                format_descriptor,             // format
                image_width,                   // width
                0,                             // height
                0,                             // depth
                0,                             // arraylevels
                0};                            // miplevels

            lzt::print_image_descriptor(image_descriptor);
            lzt::get_ze_image_properties(image_descriptor);
          }
        }
      }
    }
  }
}
TEST_P(
    zeImage3DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageWith3DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            for (auto swizzle_z : lzt::image_format_swizzles) {
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()),  // layout
                  std::get<1>(GetParam()),  // type
                  swizzle_x,                // x
                  swizzle_y,                // y
                  swizzle_z,                // z
                  ZE_IMAGE_FORMAT_SWIZZLE_X // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_2D,              // type
                  format_descriptor,             // format
                  image_width,                   // width
                  image_height,                  // height
                  0,                             // depth
                  0,                             // arraylevels
                  0};                            // miplevels

              lzt::print_image_descriptor(image_descriptor);
              lzt::get_ze_image_properties(image_descriptor);
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImage3DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor3DImageWith3DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          for (auto swizzle_x : lzt::image_format_swizzles) {
            for (auto swizzle_y : lzt::image_format_swizzles) {
              for (auto swizzle_z : lzt::image_format_swizzles) {
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()),  // layout
                    std::get<1>(GetParam()),  // type
                    swizzle_x,                // x
                    swizzle_y,                // y
                    swizzle_z,                // z
                    ZE_IMAGE_FORMAT_SWIZZLE_X // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_3D,              // type
                    format_descriptor,             // format
                    image_width,                   // width
                    image_height,                  // height
                    image_depth,                   // depth
                    0,                             // arraylevels
                    0};                            // miplevels

                lzt::print_image_descriptor(image_descriptor);
                lzt::get_ze_image_properties(image_descriptor);
              }
            }
          }
        }
      }
    }
  }
}
INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImage3DSwizzleGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_3d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArray3DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageArrayWith3DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {
            for (auto array_level : lzt::image_array_levels) {
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()),  // layout
                  std::get<1>(GetParam()),  // type
                  swizzle_x,                // x
                  swizzle_y,                // y
                  swizzle_z,                // z
                  ZE_IMAGE_FORMAT_SWIZZLE_X // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_1DARRAY,         // type
                  format_descriptor,             // format
                  image_width,                   // width
                  0,                             // height
                  0,                             // depth
                  array_level,                   // arraylevels
                  0};                            // miplevels

              lzt::print_image_descriptor(image_descriptor);
              lzt::get_ze_image_properties(image_descriptor);
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImageArray3DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageArrayWith3DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            for (auto swizzle_z : lzt::image_format_swizzles) {
              for (auto array_level : lzt::image_array_levels) {
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()),  // layout
                    std::get<1>(GetParam()),  // type
                    swizzle_x,                // x
                    swizzle_y,                // y
                    swizzle_z,                // z
                    ZE_IMAGE_FORMAT_SWIZZLE_X // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_2DARRAY,         // type
                    format_descriptor,             // format
                    image_width,                   // width
                    image_height,                  // height
                    0,                             // depth
                    array_level,                   // arraylevels
                    0};                            // miplevels

                lzt::print_image_descriptor(image_descriptor);
                lzt::get_ze_image_properties(image_descriptor);
              }
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImageArray3DSwizzleGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_3d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImage4DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageWith4DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {
            for (auto swizzle_w : lzt::image_format_swizzles) {
              ze_image_format_desc_t format_descriptor = {
                  std::get<0>(GetParam()), // layout
                  std::get<1>(GetParam()), // type
                  swizzle_x,               // x
                  swizzle_y,               // y
                  swizzle_z,               // z
                  swizzle_w                // w
              };

              ze_image_desc_t image_descriptor = {
                  ZE_IMAGE_DESC_VERSION_CURRENT, // version
                  image_create_flags,            // flags
                  ZE_IMAGE_TYPE_1D,              // type
                  format_descriptor,             // format
                  image_width,                   // width
                  0,                             // height
                  0,                             // depth
                  0,                             // arraylevels
                  0};                            // miplevels

              lzt::print_image_descriptor(image_descriptor);
              lzt::get_ze_image_properties(image_descriptor);
            }
          }
        }
      }
    }
  }
}
TEST_P(
    zeImage4DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageWith4DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto swizzle_x : lzt::image_format_swizzles) {
          for (auto swizzle_y : lzt::image_format_swizzles) {
            for (auto swizzle_z : lzt::image_format_swizzles) {
              for (auto swizzle_w : lzt::image_format_swizzles) {
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()), // layout
                    std::get<1>(GetParam()), // type
                    swizzle_x,               // x
                    swizzle_y,               // y
                    swizzle_z,               // z
                    swizzle_w                // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_2D,              // type
                    format_descriptor,             // format
                    image_width,                   // width
                    image_height,                  // height
                    0,                             // depth
                    0,                             // arraylevels
                    0};                            // miplevels

                lzt::print_image_descriptor(image_descriptor);
                lzt::get_ze_image_properties(image_descriptor);
              }
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImage4DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor3DImageWith4DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          for (auto swizzle_x : lzt::image_format_swizzles) {
            for (auto swizzle_y : lzt::image_format_swizzles) {
              for (auto swizzle_z : lzt::image_format_swizzles) {
                for (auto swizzle_w : lzt::image_format_swizzles) {
                  ze_image_format_desc_t format_descriptor = {
                      std::get<0>(GetParam()), // layout
                      std::get<1>(GetParam()), // type
                      swizzle_x,               // x
                      swizzle_y,               // y
                      swizzle_z,               // z
                      swizzle_w                // w
                  };

                  ze_image_desc_t image_descriptor = {
                      ZE_IMAGE_DESC_VERSION_CURRENT, // version
                      image_create_flags,            // flags
                      ZE_IMAGE_TYPE_3D,              // type
                      format_descriptor,             // format
                      image_width,                   // width
                      image_height,                  // height
                      image_depth,                   // depth
                      0,                             // arraylevels
                      0};                            // miplevels

                  lzt::print_image_descriptor(image_descriptor);
                  lzt::get_ze_image_properties(image_descriptor);
                }
              }
            }
          }
        }
      }
    }
  }
}
INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImage4DSwizzleGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_4d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArray4DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageArrayWith4DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {
            for (auto swizzle_w : lzt::image_format_swizzles) {
              for (auto array_level : lzt::image_array_levels) {
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()), // layout
                    std::get<1>(GetParam()), // type
                    swizzle_x,               // x
                    swizzle_y,               // y
                    swizzle_z,               // z
                    swizzle_w                // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_1DARRAY,         // type
                    format_descriptor,             // format
                    image_width,                   // width
                    0,                             // height
                    0,                             // depth
                    array_level,                   // arraylevels
                    0};                            // miplevels

                lzt::print_image_descriptor(image_descriptor);
                lzt::get_ze_image_properties(image_descriptor);
              }
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeImageArray4DSwizzleGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageArrayWith4DSwizzleThenValidPropertiesReturned) {
  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto swizzle_x : lzt::image_format_swizzles) {
        for (auto swizzle_y : lzt::image_format_swizzles) {
          for (auto swizzle_z : lzt::image_format_swizzles) {
            for (auto swizzle_w : lzt::image_format_swizzles) {
              for (auto array_level : lzt::image_array_levels) {
                ze_image_format_desc_t format_descriptor = {
                    std::get<0>(GetParam()), // layout
                    std::get<1>(GetParam()), // type
                    swizzle_x,               // x
                    swizzle_y,               // y
                    swizzle_z,               // z
                    swizzle_w                // w
                };

                ze_image_desc_t image_descriptor = {
                    ZE_IMAGE_DESC_VERSION_CURRENT, // version
                    image_create_flags,            // flags
                    ZE_IMAGE_TYPE_2DARRAY,         // type
                    format_descriptor,             // format
                    image_width,                   // width
                    0,                             // height
                    0,                             // depth
                    array_level,                   // arraylevels
                    0};                            // miplevels

                lzt::print_image_descriptor(image_descriptor);
                lzt::get_ze_image_properties(image_descriptor);
              }
            }
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImageArray4DSwizzleGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_4d_swizzle_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageMediaGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DImageWithMediaFormatThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      ze_image_format_desc_t format_descriptor = {
          std::get<0>(GetParam()),   // layout
          std::get<1>(GetParam()),   // type
          ZE_IMAGE_FORMAT_SWIZZLE_X, // x
          ZE_IMAGE_FORMAT_SWIZZLE_X, // y
          ZE_IMAGE_FORMAT_SWIZZLE_X, // z
          ZE_IMAGE_FORMAT_SWIZZLE_X  // w
      };

      ze_image_desc_t image_descriptor = {
          ZE_IMAGE_DESC_VERSION_CURRENT, // version
          image_create_flags,            // flags
          ZE_IMAGE_TYPE_1D,              // type
          format_descriptor,             // format
          image_width,                   // width
          0,                             // height
          0,                             // depth
          0,                             // arraylevels
          0};                            // miplevels
      lzt::print_image_descriptor(image_descriptor);
      lzt::get_ze_image_properties(image_descriptor);
    }
  }
}

TEST_P(
    zeImageMediaGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DImageWithMediaFormatThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        ze_image_format_desc_t format_descriptor = {
            std::get<0>(GetParam()),   // layout
            std::get<1>(GetParam()),   // type
            ZE_IMAGE_FORMAT_SWIZZLE_X, // x
            ZE_IMAGE_FORMAT_SWIZZLE_X, // y
            ZE_IMAGE_FORMAT_SWIZZLE_X, // z
            ZE_IMAGE_FORMAT_SWIZZLE_X  // w
        };

        ze_image_desc_t image_descriptor = {
            ZE_IMAGE_DESC_VERSION_CURRENT, // version
            image_create_flags,            // flags
            ZE_IMAGE_TYPE_2D,              // type
            format_descriptor,             // format
            image_width,                   // width
            image_height,                  // height
            0,                             // depth
            0,                             // arraylevels
            0};                            // miplevels
        lzt::print_image_descriptor(image_descriptor);
        lzt::get_ze_image_properties(image_descriptor);
      }
    }
  }
}

TEST_P(
    zeImageMediaGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor3DImageWithMediaFormatThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto image_depth : lzt::image_depths) {
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              ZE_IMAGE_FORMAT_SWIZZLE_X, // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_3D,              // type
              format_descriptor,             // format
              image_width,                   // width
              image_height,                  // height
              image_depth,                   // depth
              0,                             // arraylevels
              0};                            // miplevels

          lzt::print_image_descriptor(image_descriptor);
          lzt::get_ze_image_properties(image_descriptor);
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest, zeImageMediaGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_media_layouts,
                                           lzt::image_format_types));

TEST_P(
    zeImageArrayMediaGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor1DArrayImageWithMediaFormatThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto array_level : lzt::image_array_levels) {
        ze_image_format_desc_t format_descriptor = {
            std::get<0>(GetParam()),   // layout
            std::get<1>(GetParam()),   // type
            ZE_IMAGE_FORMAT_SWIZZLE_X, // x
            ZE_IMAGE_FORMAT_SWIZZLE_X, // y
            ZE_IMAGE_FORMAT_SWIZZLE_X, // z
            ZE_IMAGE_FORMAT_SWIZZLE_X  // w
        };

        ze_image_desc_t image_descriptor = {
            ZE_IMAGE_DESC_VERSION_CURRENT, // version
            image_create_flags,            // flags
            ZE_IMAGE_TYPE_1DARRAY,         // type
            format_descriptor,             // format
            image_width,                   // width
            0,                             // height
            0,                             // depth
            array_level,                   // arraylevels
            0};                            // miplevels
        lzt::print_image_descriptor(image_descriptor);
        lzt::get_ze_image_properties(image_descriptor);
      }
    }
  }
}

TEST_P(
    zeImageArrayMediaGetPropertiesTests,
    GivenValidDescriptorWhenGettingPropertiesFor2DArrayImageWithMediaFormatThenValidPropertiesReturned) {

  for (auto image_create_flags : img.image_creation_flags_list_) {
    for (auto image_width : lzt::image_widths) {
      for (auto image_height : lzt::image_heights) {
        for (auto array_level : lzt::image_array_levels) {
          ze_image_format_desc_t format_descriptor = {
              std::get<0>(GetParam()),   // layout
              std::get<1>(GetParam()),   // type
              ZE_IMAGE_FORMAT_SWIZZLE_X, // x
              ZE_IMAGE_FORMAT_SWIZZLE_X, // y
              ZE_IMAGE_FORMAT_SWIZZLE_X, // z
              ZE_IMAGE_FORMAT_SWIZZLE_X  // w
          };

          ze_image_desc_t image_descriptor = {
              ZE_IMAGE_DESC_VERSION_CURRENT, // version
              image_create_flags,            // flags
              ZE_IMAGE_TYPE_2DARRAY,         // type
              format_descriptor,             // format
              image_width,                   // width
              image_height,                  // height
              0,                             // depth
              array_level,                   // arraylevels
              0};                            // miplevels
          lzt::print_image_descriptor(image_descriptor);
          lzt::get_ze_image_properties(image_descriptor);
        }
      }
    }
  }
}

INSTANTIATE_TEST_CASE_P(ImageGetPropertiesTest,
                        zeImageArrayMediaGetPropertiesTests,
                        ::testing::Combine(lzt::image_format_media_layouts,
                                           lzt::image_format_types));
} // namespace
