# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_cmdqueue_errors
  GROUP "/negative_tests/core"
  SOURCES
    src/test_cmdqueue_errors.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
