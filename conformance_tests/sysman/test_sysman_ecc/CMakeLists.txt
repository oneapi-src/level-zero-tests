# Copyright (C) 2022-2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
	NAME test_sysman_ecc
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_ecc.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
add_lzt_test(
	NAME test_sysman_ecc_zesinit
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_ecc.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  DEFINES USE_ZESINIT  
)