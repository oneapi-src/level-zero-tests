# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_core_library(utils_string
  SOURCE
  "include/utils/utils_string.hpp"
  "src/utils_string.cpp"
  )
target_link_libraries(utils_string
  PUBLIC
  level_zero_tests::logging
  LevelZero::LevelZero
  )

if(NOT BUILD_ZE_PERF_TESTS_ONLY)
  add_core_library(utils
    SOURCE
    "include/utils/utils.hpp"
    "src/utils.cpp"
  )
  target_link_libraries(utils
    PUBLIC
    level_zero_tests::utils_string
    level_zero_tests::test_harness
  )
  add_core_library_test(utils
    SOURCE
    "test/main.cpp"
    "test/utils_unit_tests.cpp"
    "test/utils_integration_tests.cpp"
  )
endif()
