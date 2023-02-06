# Copyright (C) 2021-2023 Intel Corporation
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
    src/test_debug_utils.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    ${debug_libraries}
  KERNELS
    debug_add
    debug_loop
    debug_loop_slm
    debug_transform
)

add_lzt_test_executable(
  NAME test_debug_helper
  GROUP "/conformance_tests/tools/debug"
  PREFIX "debug"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_debug_helper.cpp
    src/test_debug_utils.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    ${debug_libraries}
)

add_lzt_test_executable(
  NAME child_debugger
  GROUP "/conformance_tests/tools/debug"
  PREFIX "debug"  # install to prefix so it's not confused for a test
  SOURCES
    src/child_debugger.cpp
    src/test_debug_utils.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    ${debug_libraries}
)
