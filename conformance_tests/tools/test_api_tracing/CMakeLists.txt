# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_api_tracing
  GROUP "/conformance_tests/tools/tracing"
  SOURCES
    src/test_api_tracing.cpp
    src/test_api_tracing_threading.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
