/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_IPC_PUT_HANDLE_HPP
#define TEST_IPC_PUT_HANDLE_HPP

typedef enum {
  TEST_PUT_DEVICE_ACCESS,
  TEST_PUT_SUBDEVICE_ACCESS
} ipc_put_mem_access_test_t;

typedef struct {
  ipc_put_mem_access_test_t test_type;
  int size;
  ze_ipc_memory_flags_t flags;
  bool is_immediate;
} shared_data_t;

#endif
