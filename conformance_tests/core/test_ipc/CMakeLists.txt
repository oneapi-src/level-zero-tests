# Copyright (C) 2019 - 2021 Intel Corporation
# SPDX-License-Identifier: MIT

set(ipc_libraries
  level_zero_tests::logging
  level_zero_tests::utils
  Boost::system
)

if(UNIX)
    list(APPEND ipc_event_libraries rt)
endif()

add_lzt_test(
  NAME test_ipc
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_ipc.cpp
    src/test_ipc_event.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::net
    ${ipc_libraries}
    ${ipc_event_libraries}
)

add_lzt_test(
  NAME test_ipc_memory
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_ipc_memory.cpp
  LINK_LIBRARIES
    level_zero_tests::net
    ${ipc_libraries}
)

add_lzt_test(
  NAME test_ipc_multidevice
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_ipc_multidevice.cpp
  LINK_LIBRARIES
    level_zero_tests::net
    ${ipc_libraries}
)

add_lzt_test(
  NAME test_ipc_multisubdevice
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_ipc_multisubdevice.cpp
  LINK_LIBRARIES
    level_zero_tests::net
    ${ipc_libraries}
)

add_lzt_test(
  NAME test_ipc_p2p_memory
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_ipc_p2p_memory.cpp
  LINK_LIBRARIES
    level_zero_tests::net
    ${ipc_libraries}
)

add_lzt_test_executable(
  NAME test_ipc_event_helper
  GROUP "/conformance_tests/core"
  PREFIX "ipc"  # install to prefix so it's not confused for a test
  SOURCES
    src/test_ipc_event_helper.cpp
  LINK_LIBRARIES
    ${ipc_libraries}
    ${ipc_event_libraries}
    level_zero_tests::net
)
