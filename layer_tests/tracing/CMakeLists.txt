# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_api_ltracing
  GROUP "/layer_tests/ltracing"
  SOURCES
    src/test_api_ltracing_compat.cpp
    src/test_api_ltracing.cpp
    src/test_api_ltracing_threading.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
