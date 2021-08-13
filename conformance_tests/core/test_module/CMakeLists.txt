# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_module
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_module.cpp
    src/test_kernel_args.cpp
    src/test_cooperative_kernel.cpp
    src/test_kernel_schedule_hints.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    0kernels
    1kernel
    10kernels
    100kernels
    1000kernels
    cooperative_kernel
    multi_argument_kernel_test
    single_global_variable
    multiple_global_variables
    update_variable_on_device
    module_add
    update_variable_with_spec_constant
)
