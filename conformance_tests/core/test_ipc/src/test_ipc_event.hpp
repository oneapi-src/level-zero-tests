/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_IPC_EVENT_HPP
#define TEST_IPC_EVENT_HPP

typedef enum {
  PARENT_TEST_HOST_SIGNALS,
  PARENT_TEST_DEVICE_SIGNALS
} parent_test_t;

typedef enum {
  CHILD_TEST_HOST_READS,
  CHILD_TEST_DEVICE_READS,
  CHILD_TEST_DEVICE2_READS,
  CHILD_TEST_MULTI_DEVICE_READS
} child_test_t;

typedef struct {
  parent_test_t parent_type;
  child_test_t child_type;
  bool multi_device;
} shared_data_t;

#endif