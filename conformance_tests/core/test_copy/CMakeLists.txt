# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_copy
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_copy.cpp
    src/test_copy_events.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::image
    level_zero_tests::utils
    level_zero_tests::utils
  MEDIA
    "png/test_input.png"
)
