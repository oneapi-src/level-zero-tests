# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: MIT


if (UNIX)
  list(APPEND debug_libraries rt)
endif()

add_lzt_test(
  NAME test_debug_errors
  GROUP "/negative_tests/tools"
  INCLUDE_DIRECTORIES
    ../../../conformance_tests/tools/debug/src
  SOURCES
    src/test_debug_errors.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    ${debug_libraries}
)
