# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_stress_multiprocess
  GROUP "/stress_tests"
  SOURCES
    ../common/src/stress_common_func.cpp
    src/test_multiprocess.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    Boost::system
    $<$<PLATFORM_ID:Linux>:rt>
  KERNELS
    test_multiprocess
)
