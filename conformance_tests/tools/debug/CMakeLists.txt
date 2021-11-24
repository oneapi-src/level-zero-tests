# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: MIT


if (UNIX)
  list(APPEND debug_libraries rt)
endif()

add_lzt_test(
  NAME test_debug
  GROUP "/conformance_tests/tools/debug"
  SOURCES
    src/test_debug.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    ${debug_libraries}
  KERNELS
    debug_add
    debug_loop
)

add_lzt_test_executable(
  NAME test_debug_helper
  GROUP "/conformance_tests/tools/debug"
  PREFIX "debug"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_debug_helper.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    ${debug_libraries}
)
