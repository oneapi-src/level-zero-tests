# Copyright (C) 2019-2025 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_copy
  GROUP "/conformance_tests/core"
  SOURCES
    src/main.cpp
    src/test_copy_events.cpp
    src/test_copy_image.cpp
    src/test_copy_remote_madvise.cpp
    src/test_copy.cpp
    src/test_kernel_copy.cpp
    src/test_multicontext_copy.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::image
    level_zero_tests::utils
  MEDIA
    "png/test_input.png"
  KERNELS
    copy_module
)
