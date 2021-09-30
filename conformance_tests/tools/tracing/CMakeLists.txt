# Copyright (C) 2019-2021 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_api_tracing
  GROUP "/conformance_tests/tools/tracing"
  SOURCES
    src/test_api_tracing.cpp
    src/test_api_tracing_threading.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    level_zero_tests::net
)

add_lzt_test_executable(
  NAME test_tracing_ipc_event_helper
  GROUP "/conformance_tests/tools/tracing"
  PREFIX "tracing"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_tracing_ipc_event_helper.cpp
  LINK_LIBRARIES
    level_zero_tests::net
)
