# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_image
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_image.cpp
    src/test_image_copy_swizzle.cpp
    src/test_image_formats.cpp
    src/test_image_layout.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    image_formats_tests
    image_media_layouts_tests
)
