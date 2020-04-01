# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_pin
  GROUP "/conformance_tests/tools/pin"
  SOURCES
    src/test_pin.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    0kernels
    1kernel
    10kernels
    100kernels
    1000kernels
)
