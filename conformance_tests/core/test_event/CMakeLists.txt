# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_event
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_event.cpp
    src/main.cpp
    src/test_event_profiling.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
