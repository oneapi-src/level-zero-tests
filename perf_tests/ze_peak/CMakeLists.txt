# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

if(UNIX)
    set(OS_SPECIFIC_LIBS pthread)
else()
    set(OS_SPECIFIC_LIBS "")
endif()

add_lzt_test(
  NAME ze_peak
  GROUP "/perf_tests"
  SOURCES
    src/common.cpp
    src/options.cpp
    src/ze_peak.cpp
    src/global_bw.cpp
    src/kernel_latency.cpp
    src/hp_compute.cpp
    src/sp_compute.cpp
    src/integer_compute.cpp
    src/dp_compute.cpp
    src/transfer_bw.cpp
  LINK_LIBRARIES ${OS_SPECIFIC_LIBS}
  KERNELS
    ze_global_bw
    ze_hp_compute
    ze_sp_compute
    ze_int_compute
    ze_dp_compute
)
