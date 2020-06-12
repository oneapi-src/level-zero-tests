# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_device
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_device.cpp
    src/test_affinity.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)

add_lzt_test_executable(
  NAME test_affinity_helper
  GROUP "/conformance_tests/core"
  PREFIX "device"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_affinity_helper.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
