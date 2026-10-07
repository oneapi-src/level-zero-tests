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
    level_zero_tests::random
  KERNELS
    cmdlist_add
    cmdlist_scratch_32
    cmdlist_scratch_48
    cmdlist_scratch_64
    cmdlist_scratch_80
    cmdlist_scratch_96
    cmdlist_scratch_128
    cmdlist_scratch_160
    cmdlist_scratch_192
    cmdlist_scratch_256
    cmdlist_scratch_512
    cmdlist_scratch_768
    cmdlist_scratch_1024
    cmdlist_scratch_1280
    cmdlist_scratch_1536
    cmdlist_verify
    cmdlist_and_functions
  MEDIA
    "png/test_input.png"
)
