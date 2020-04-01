# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

if(UNIX)
    set(OS_SPECIFIC_LIBS pthread)
else()
    set(OS_SPECIFIC_LIBS "")
endif()

add_lzt_test(
  NAME ze_pingpong
  GROUP "/perf_tests"
  SOURCES
    src/ze_pingpong.cpp
  LINK_LIBRARIES ${OS_SPECIFIC_LIBS}
  KERNELS
    ze_pingpong
)
