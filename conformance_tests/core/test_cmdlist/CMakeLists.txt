# Copyright (C) 2019-2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_cmdlist
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_cmdlist.cpp
    src/test_cmdlist_immediate.cpp
    src/test_cmdlist_mixed.cpp
    src/test_cmdlist_and_functions.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    cmdlist_add
    cmdlist_scratch
    cmdlist_verify
    cmdlist_and_functions
  MEDIA
    "png/test_input.png"
)
