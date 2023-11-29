#
#Copyright(C) 2020-2023 Intel Corporation
#
#SPDX - License - Identifier : MIT
#

add_lzt_test(
    NAME test_sysman_device
    GROUP   "/conformance_tests/tools/sysman"
    SOURCES
    src/test_sysman_device.cpp
    src/main.cpp
    LINK_LIBRARIES level_zero_tests::logging level_zero_tests::utils)
add_lzt_test(
    NAME test_sysman_device_zesinit
    GROUP   "/conformance_tests/tools/sysman"
    SOURCES
    src/test_sysman_device.cpp
    src/main.cpp
    LINK_LIBRARIES level_zero_tests::logging level_zero_tests::utils
    DEFINES USE_ZESINIT)    

add_lzt_test_executable(
  NAME test_sysman_device_helper_zesinit
  GROUP "/conformance_tests/tools/sysman"
  PREFIX "sysman_device"  # install to prefix so it's not confused for a test
  SOURCES
      src/test_sysman_device_helper.cpp
  LINK_LIBRARIES
      level_zero_tests::logging
      level_zero_tests::utils
  DEFINES USE_ZESINIT
)

add_lzt_test_executable(
  NAME test_sysman_device_hierarchy_helper
  GROUP "/conformance_tests/tools/sysman"
  PREFIX "sysman_device"  # install to prefix so it's not confused for a test
  SOURCES
      src/test_sysman_device_hierarchy_helper.cpp
  LINK_LIBRARIES
      level_zero_tests::logging
      level_zero_tests::utils
)

add_lzt_test_executable(
  NAME test_sysman_device_hierarchy_helper_zesinit
  GROUP "/conformance_tests/tools/sysman"
  PREFIX "sysman_device"  # install to prefix so it's not confused for a test
  SOURCES
      src/test_sysman_device_hierarchy_helper.cpp
  LINK_LIBRARIES
      level_zero_tests::logging
      level_zero_tests::utils
  DEFINES USE_ZESINIT
)