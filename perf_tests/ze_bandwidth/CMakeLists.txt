# Copyright (C) 2020-2026 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME ze_bandwidth
  GROUP "/perf_tests"
  SOURCES
    ../common/src/ze_app.cpp
    src/ze_bandwidth.cpp
    src/kernel_copy.cpp
    src/all_host.cpp
    src/options.cpp
  LINK_LIBRARIES ${OS_SPECIFIC_LIBS}
  KERNELSCUSTOM
    ${CMAKE_CURRENT_SOURCE_DIR}/kernels/ze_bandwidth_copy.cl
)
