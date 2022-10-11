# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

set(COMMON_SOURCE_FILES
    src/common/timer.cpp
    src/common/timer.hpp
    src/common/utils.cpp
    src/common/utils.hpp
    src/common/workload.cpp
    src/common/workload.hpp
)
source_group("Common" FILES ${COMMON_SOURCE_FILES})


set(OPENCL_SOURCE_FILES
    src/opencl/ocl_blackscholes.cpp
    src/opencl/ocl_blackscholes.hpp
    src/opencl/ocl_mandelbrot.cpp
    src/opencl/ocl_mandelbrot.hpp
    src/opencl/ocl_simpleadd.cpp
    src/opencl/ocl_simpleadd.hpp
    src/opencl/ocl_sobel.cpp
    src/opencl/ocl_sobel.hpp
    src/opencl/ocl_workload.cpp
    src/opencl/ocl_workload.hpp
)
source_group("OpenCL" FILES ${OPENCL_SOURCE_FILES})

set(L0_SOURCE_FILES
    src/level-zero/ze_blackscholes.cpp
    src/level-zero/ze_blackscholes.hpp
    src/level-zero/ze_mandelbrot.cpp
    src/level-zero/ze_mandelbrot.hpp
    src/level-zero/ze_simpleadd.cpp
    src/level-zero/ze_simpleadd.hpp
    src/level-zero/ze_sobel.cpp
    src/level-zero/ze_sobel.hpp
    src/level-zero/ze_workload.cpp
    src/level-zero/ze_workload.hpp
)
source_group("Level-Zero" FILES ${L0_SOURCE_FILES})

set(SOURCE_FILES
    src/ze_cabe.cpp
    "${COMMON_SOURCE_FILES}"
    "${OPENCL_SOURCE_FILES}"
    "${L0_SOURCE_FILES}"
)

add_definitions(-DCL_TARGET_OPENCL_VERSION=210)
add_definitions(-DCL_USE_DEPRECATED_OPENCL_1_2_APIS)

add_lzt_test(
  NAME ze_cabe
  GROUP "/perf_tests"
  SOURCES ${SOURCE_FILES}
  LINK_LIBRARIES
   OpenCL::OpenCL
   level_zero_tests::logging
   level_zero_tests::image
  KERNELS
   ze_cabe_simpleadd
   ze_cabe_mandelbrot
   ze_cabe_sobel
   ze_cabe_blackscholes_fp32
   ze_cabe_blackscholes_fp64   
  MEDIA
   "bmp/lena512.bmp"
)
