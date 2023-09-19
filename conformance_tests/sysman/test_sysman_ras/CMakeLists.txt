# Copyright (C) 2020-2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_sysman_ras
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_ras.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)
add_lzt_test(
  NAME test_sysman_ras_zesinit
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_ras.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  DEFINES USE_ZESINIT  
)
