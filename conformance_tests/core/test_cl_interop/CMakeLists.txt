# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_cl_interop
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_cl_interop.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    level_zero_tests::random
    Boost::boost
  KERNELS
    bitonic_sort
)
