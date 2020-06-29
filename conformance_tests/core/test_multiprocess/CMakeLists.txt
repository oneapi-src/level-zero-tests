# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_multiprocess
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_multi_device_multi_process.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    multi_process_add
)

add_lzt_test_executable(
  NAME test_process_helper
  GROUP "/conformance_tests/core"
  PREFIX "process"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_process_helper.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
