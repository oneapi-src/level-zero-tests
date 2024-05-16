# Copyright (C) 2024 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_mutable_cmdlist
  GROUP "/conformance_tests/core"
  SOURCES
    src/main.cpp
    src/test_mutable_cmdlist.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    test_mutable_cmdlist
)
