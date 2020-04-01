# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_metric_query
  GROUP "/conformance_tests/tools/metrics"
  SOURCES
    src/test_metric_query.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    ze_matrix_multiplication
)
