# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_barrier
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_barrier.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    barrier_add
)
