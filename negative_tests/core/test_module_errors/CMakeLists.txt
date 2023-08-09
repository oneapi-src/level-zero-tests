# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_module_errors
  GROUP "/negative_tests/core"
  SOURCES
    src/test_module_errors.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    local_memory_argument_kernel
    ze_matrix_multiplication_errors
)
