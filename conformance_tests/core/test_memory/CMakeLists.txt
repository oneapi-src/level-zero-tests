# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

if(UNIX)
    set(OS_SPECIFIC_LIBS rt)
else()
    set(OS_SPECIFIC_LIBS "")
endif()

add_lzt_test(
  NAME test_memory
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_memory.cpp
    src/test_param_tests.cpp
    src/test_memory_export_import.cpp
    src/test_virtual_memory.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::net
    level_zero_tests::utils
    ${OS_SPECIFIC_LIBS}
  KERNELS
    unified_mem_test
    write_memory_pattern
)

add_lzt_test_executable(
  NAME test_import_helper
  GROUP "/conformance_tests/core"
  PREFIX "memory"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_import_helper.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::net
    level_zero_tests::utils
)
