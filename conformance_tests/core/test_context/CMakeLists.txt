# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_context
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_context.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
