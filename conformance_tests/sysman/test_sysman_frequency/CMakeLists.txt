# Copyright (C) 2019-2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_sysman_frequency
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_frequency.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    sysman_matrix_multiplication
)
add_lzt_test(
  NAME test_sysman_frequency_zesinit
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_frequency.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    sysman_matrix_multiplication
  DEFINES USE_ZESINIT  
)
