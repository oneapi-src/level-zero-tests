# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

if(UNIX)
    set(OS_SPECIFIC_LIBS pthread rt)
else()
    set(OS_SPECIFIC_LIBS "")
endif()

add_lzt_test(
  NAME ze_peer
  GROUP "/perf_tests"
  SOURCES
    ../common/src/ze_app.cpp
    src/ze_peer.cpp
  LINK_LIBRARIES ${OS_SPECIFIC_LIBS}
  KERNELS ze_peer_benchmarks
)
