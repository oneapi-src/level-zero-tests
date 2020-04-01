# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME ze_bandwidth
  GROUP "/perf_tests"
  SOURCES
    ../common/src/ze_app.cpp
    src/ze_bandwidth.cpp
    src/options.cpp
  LINK_LIBRARIES ${OS_SPECIFIC_LIBS}
)
