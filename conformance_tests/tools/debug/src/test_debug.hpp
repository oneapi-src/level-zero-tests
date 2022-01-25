/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_DEBUG_HPP
#define TEST_DEBUG_HPP

constexpr auto device_id_string = "device_id";
constexpr auto use_sub_devices_string = "use_sub_devices";
constexpr auto kernel_string = "kernel";
constexpr auto num_kernels_string = "num_kernels";
constexpr auto test_type_string = "test_type";

const uint16_t eventsTimeoutMS = 30000;
const uint16_t eventsTimeoutS = 30;
const uint16_t timeoutThreshold = 4;

typedef enum {
  BASIC,
  ATTACH_AFTER_MODULE_CREATED,
  MULTIPLE_MODULES_CREATED,
  ATTACH_AFTER_MODULE_DESTROYED,
  LONG_RUNNING_KERNEL_INTERRUPTED,
  KERNEL_RESUME,
  THREAD_STOPPED,
  THREAD_UNAVAILABLE,
  PAGE_FAULT
} debug_test_type_t;

typedef struct {
  bool debugger_signal;
  bool debugee_signal;
} debug_signals_t;

typedef enum { SINGLE_THREAD, GROUP_OF_THREADS, ALL_THREADS } num_threads_t;

#define CLEAN_AND_ASSERT(condition, helper)                                    \
  do {                                                                         \
    if (!condition) {                                                          \
      helper.terminate();                                                      \
      ASSERT_TRUE(false);                                                      \
    }                                                                          \
  } while (0)

#endif // TEST_DEBUG_HPP
