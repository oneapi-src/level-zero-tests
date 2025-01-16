/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_IPC_EVENT_HPP
#define TEST_IPC_EVENT_HPP

typedef enum {
  PARENT_TEST_HOST_SIGNALS,
  PARENT_TEST_DEVICE_SIGNALS,
  PARENT_TEST_HOST_LAUNCHES_KERNEL,
  PARENT_TEST_QUERY_EVENT_STATUS
} parent_test_t;

typedef enum {
  CHILD_TEST_HOST_READS,
  CHILD_TEST_DEVICE_READS,
  CHILD_TEST_DEVICE2_READS,
  CHILD_TEST_MULTI_DEVICE_READS,
  CHILD_TEST_HOST_TIMESTAMP_READS,
  CHILD_TEST_DEVICE_TIMESTAMP_READS,
  CHILD_TEST_HOST_MAPPED_TIMESTAMP_READS,
  CHILD_TEST_QUERY_EVENT_STATUS
} child_test_t;

typedef struct {
  parent_test_t parent_type;
  child_test_t child_type;
  bool multi_device;
  bool is_immediate;
  uint64_t start_time;
  uint64_t end_time;
} shared_data_t;

#endif
