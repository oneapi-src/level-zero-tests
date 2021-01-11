# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_multithread
  GROUP "/conformance_tests/core"
  SOURCES
    src/test_fence_multithread.cpp
    src/test_device_multithread.cpp
    src/test_image_copy_multithread.cpp
    src/test_memory_multithread.cpp
    src/test_event_multithread.cpp
    src/test_cmd_queue_multithread.cpp
    src/test_cmd_list_multithread.cpp
    src/test_immediate_cmd_list_multithread.cpp
    src/test_kernel_multithread.cpp
    src/test_allocation_residency_multithread.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
  KERNELS
    test_fill_device_memory
    residency_tests)
