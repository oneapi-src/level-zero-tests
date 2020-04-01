# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

set(OS_SPECIFIC_LIBS "")
set(ZE_NANO_HWCOUNTER_SRC src/hardware_counter/hardware_counter_stub.cpp)
if(UNIX)
    set(PAPI_LIB papi)
    find_library(PAPI_LIB_PATH ${PAPI_LIB})
    if(PAPI_LIB_PATH)
      set(ZE_NANO_HWCOUNTER_SRC src/hardware_counter/hardware_counter_support.cpp)
      set(OS_SPECIFIC_LIBS ${OS_SPECIFIC_LIBS} ${PAPI_LIB})
      add_definitions(-DWITH_PAPI)
      message(STATUS "Found PAPI library: ${PAPI_LIB}")
    else()
      message(STATUS "PAPI library not found, hardware counter support disabled")
    endif()
else()
endif()

add_lzt_test(
  NAME ze_nano
  GROUP "/perf_tests"
  SOURCES
    ../common/src/ze_app.cpp
    src/api_static_probe.cpp
    ${ZE_NANO_HWCOUNTER_SRC}
    src/ze_nano.cpp
    src/benchmark.cpp
  LINK_LIBRARIES
    ${OS_SPECIFIC_LIBS}
    gmock
  KERNELS ze_nano_benchmarks
)
