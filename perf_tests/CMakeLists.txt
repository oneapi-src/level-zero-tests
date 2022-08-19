# Copyright (C) 2019-2020 Intel Corporation
# SPDX-License-Identifier: MIT

if(Boost_FOUND)
  add_subdirectory(ze_nano)
  add_subdirectory(ze_image_copy)
else()
  message(WARNING "Skipping ze_nano and ze_image_copy: requires boost")
endif()
add_subdirectory(ze_peak)
add_subdirectory(ze_pingpong)
add_subdirectory(ze_bandwidth)

if(UNIX)
  add_subdirectory(ze_peer)
else()
  message(WARNING "Skipping ze_peer: test only supported on Linux")
endif()

if(OpenCL_FOUND AND Boost_FOUND)
  add_subdirectory(cl_image_copy)
  add_subdirectory(ze_cabe)
else()
  message(WARNING "Skipping cl_image_copy and ze_cabe: requires opencl and boost")
endif()
