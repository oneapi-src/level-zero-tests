# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

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
