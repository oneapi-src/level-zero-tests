# Copyright (C) 2020-2021 Intel Corporation
# SPDX-License-Identifier: MIT

set(ipc_libraries
  level_zero_tests::logging
  level_zero_tests::utils
  Boost::system
)

option(ENABLE_RUNTIME_TRACING "Enable runtime tracing" ON)
if(ENABLE_RUNTIME_TRACING)
  set(defines_runtime_tracing USE_RUNTIME_TRACING)
endif()

if(UNIX)
    list(APPEND ipc_event_libraries rt)
endif()

add_lzt_test(
  NAME test_api_ltracing
  GROUP "/layer_tests/ltracing"
  SOURCES
    src/test_api_ltracing_compat.cpp
    src/test_api_ltracing.cpp
    src/test_api_ltracing_threading.cpp
    src/main.cpp
  LINK_LIBRARIES
    ${ipc_libraries}
    ${ipc_event_libraries}
    level_zero_tests::net
)

add_lzt_test(
  NAME test_api_ltracing_dynamic
  GROUP "/layer_tests/ltracing"
  SOURCES
    src/test_api_ltracing_compat.cpp
    src/test_api_ltracing.cpp
    src/test_api_ltracing_threading.cpp
    src/main.cpp
  LINK_LIBRARIES
    ${ipc_libraries}
    ${ipc_event_libraries}
    level_zero_tests::net
  DEFINES ${defines_runtime_tracing}
)

add_lzt_test_executable(
  NAME test_ltracing_ipc_event_helper
  GROUP "/layer_tests/ltracing"
  PREFIX "tracing"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_ltracing_ipc_event_helper.cpp
  LINK_LIBRARIES
    ${ipc_libraries}
    ${ipc_event_libraries}
    level_zero_tests::net
)

add_lzt_test_executable(
  NAME test_ltracing_ipc_event_helper_dynamic
  GROUP "/layer_tests/ltracing"
  PREFIX "tracing"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_ltracing_ipc_event_helper.cpp
  LINK_LIBRARIES
    ${ipc_libraries}
    ${ipc_event_libraries}
    level_zero_tests::net
  DEFINES ${defines_runtime_tracing}
)

add_lzt_test_executable(
  NAME test_ltracing_compat_ipc_event_helper
  GROUP "/layer_tests/ltracing"
  PREFIX "tracing"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_ltracing_compat_ipc_event_helper.cpp
  LINK_LIBRARIES
    ${ipc_libraries}
    ${ipc_event_libraries}
    level_zero_tests::net
)

add_lzt_test_executable(
  NAME test_ltracing_compat_ipc_event_helper_dynamic
  GROUP "/layer_tests/ltracing"
  PREFIX "tracing"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_ltracing_compat_ipc_event_helper.cpp
  LINK_LIBRARIES
    ${ipc_libraries}
    ${ipc_event_libraries}
    level_zero_tests::net
  DEFINES ${defines_runtime_tracing}
)
