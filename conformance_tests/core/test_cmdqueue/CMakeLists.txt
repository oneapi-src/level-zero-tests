# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_cmdqueue
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_cmdqueue.cpp
    src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_image/src/utils.cpp
  INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_image/include
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    level_zero_tests::image
    level_zero_tests::random
  KERNELS
    cmdqueue_add
)
