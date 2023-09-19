# Copyright (C) 2020-2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_sysman_firmware
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_firmware.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    Boost::serialization
)
add_lzt_test(
  NAME test_sysman_firmware_zesinit
  GROUP "/conformance_tests/tools/sysman"
  SOURCES
    src/test_sysman_firmware.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    Boost::serialization
  DEFINES USE_ZESINIT   
)
