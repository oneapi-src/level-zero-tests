# Copyright (C) 2019-2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_subdirectory(ze_nano)
add_subdirectory(ze_peak)
add_subdirectory(ze_pingpong)
add_subdirectory(ze_image_copy)
add_subdirectory(ze_bandwidth)

if(UNIX)
  add_subdirectory(ze_peer)
endif()

if(OPENCL_FOUND)
  add_subdirectory(cl_image_copy)
  add_subdirectory(ze_cabe)
endif()
