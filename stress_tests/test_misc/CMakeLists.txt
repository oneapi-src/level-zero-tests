# Copyright (C) 2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_stress_misc
  GROUP "/stress_tests"
  SOURCES
    ../common/src/stress_common_func.cpp
    src/test_memory_reservation_57b.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    test_memory_reservation_57b
)
