# Copyright (C) 2024 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
	NAME test_sysman_driver_zesinit
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_driver.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)