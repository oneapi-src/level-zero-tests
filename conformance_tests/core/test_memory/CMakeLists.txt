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
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    level_zero_tests::utils
    ${OS_SPECIFIC_LIBS}
  KERNELS
    unified_mem_test
)
