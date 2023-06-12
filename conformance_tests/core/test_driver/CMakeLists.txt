# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_driver_init_flag_none
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_driver_init_flag_none.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
)

add_lzt_test(
  NAME test_driver_init_flag_gpu
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_driver_init_flag_gpu.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
)

add_lzt_test(
  NAME test_driver_init_flag_vpu
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_driver_init_flag_vpu.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
)

add_lzt_test(
  NAME test_driver_init_flag_vpu_gpu
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_driver_init_flag_vpu_gpu.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
)

add_lzt_test(
  NAME test_driver
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_driver.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
)
