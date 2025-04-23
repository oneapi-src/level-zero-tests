# Copyright (C) 2019-2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_image
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_image.cpp
    src/test_image_formats.cpp
    src/helpers_test_image.cpp
    src/test_image_layout.cpp
    src/test_image_swizzle.cpp
    src/test_image_view.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    image_format_layouts_tests
    image_formats_tests
    image_layout_tests
    image_media_layouts_tests
    image_swizzle_tests
)
