# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_metric
  GROUP "/conformance_tests/tools/metrics"
  SOURCES
    src/test_metric_utils.cpp
    src/test_metric.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    ze_matrix_multiplication_metrics
)

add_lzt_test_executable(
  NAME test_metric_helper
  GROUP "/conformance_tests/tools/metrics"
  PREFIX "metrics"
  SOURCES
  src/test_metric_utils.cpp
  src/test_metric_helper.cpp
  LINK_LIBRARIES
  level_zero_tests::logging
  level_zero_tests::utils
)
