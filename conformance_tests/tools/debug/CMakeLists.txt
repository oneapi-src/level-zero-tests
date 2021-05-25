# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_debug
  GROUP "/conformance_tests/tools/debug"
  SOURCES
    src/test_debug.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
