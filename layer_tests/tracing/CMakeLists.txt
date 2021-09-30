# Copyright (C) 2020-2021 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_api_ltracing
  GROUP "/layer_tests/ltracing"
  SOURCES
    src/test_api_ltracing_compat.cpp
    src/test_api_ltracing.cpp
    src/test_api_ltracing_threading.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    level_zero_tests::net
)

add_lzt_test_executable(
  NAME test_ltracing_ipc_event_helper
  GROUP "/layer_tests/ltracing"
  PREFIX "tracing"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_ltracing_ipc_event_helper.cpp
  LINK_LIBRARIES
    level_zero_tests::net
)

add_lzt_test_executable(
  NAME test_ltracing_compat_ipc_event_helper
  GROUP "/layer_tests/ltracing"
  PREFIX "tracing"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_ltracing_compat_ipc_event_helper.cpp
  LINK_LIBRARIES
    level_zero_tests::net
)
