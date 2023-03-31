# Copyright (C) 2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_metric_errors
  GROUP "/negative_tests/tools"
  SOURCES
    src/test_metric_errors.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
