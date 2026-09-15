# Copyright (C) 2019-2026 Intel Corporation
# SPDX-License-Identifier: MIT

add_core_library(utils_string
  SOURCE
  "include/utils/utils_string.hpp"
  "src/utils_string.cpp"
  )
target_link_libraries(utils_string
  PUBLIC
  level_zero_tests::logging
  ${LZT_LEVEL_ZERO_TARGET}
  )

if(NOT BUILD_ZE_PERF_TESTS_ONLY)
  add_core_library(utils
    SOURCE
    "include/utils/utils.hpp"
    "include/utils/utils_command_bundle.hpp"
    "include/utils/utils_process.hpp"
    "src/utils_process.cpp"
    "src/utils.cpp"
    "src/utils_command_bundle.cpp"
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
  find_package(Threads REQUIRED)
  add_executable(utils_process_test_helper
    "test/utils_process_helper.hpp"
    "test/utils_process_helper.cpp"
  )
  target_link_libraries(utils_process_test_helper PRIVATE Threads::Threads)
  add_executable(utils_process_tests
    "test/main.cpp"
    "test/utils_process_helper.hpp"
    "test/utils_process_tests.cpp"
  )
  target_link_libraries(utils_process_tests PRIVATE
    level_zero_tests::utils
    GTest::GTest
  )
  add_dependencies(utils_process_tests utils_process_test_helper)
  add_test(NAME utils_process_tests COMMAND utils_process_tests)
  set_tests_properties(utils_process_tests PROPERTIES TIMEOUT 30)
endif()
