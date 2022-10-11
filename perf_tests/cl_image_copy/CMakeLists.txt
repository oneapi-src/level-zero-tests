# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_definitions(-DCL_TARGET_OPENCL_VERSION=210)
add_definitions(-DCL_USE_DEPRECATED_OPENCL_1_2_APIS)

add_lzt_test(
  NAME cl_image_copy
  GROUP "/perf_tests"
  SOURCES
    ../common/src/ze_app.cpp
    src/cl_image_copy.cpp
    src/options.cpp
    src/utils.cpp
  LINK_LIBRARIES
    OpenCL::OpenCL
    Boost::boost
    Boost::program_options
)
