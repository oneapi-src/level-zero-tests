/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_IPC_MEMORY_HPP
#define TEST_IPC_MEMORY_HPP

typedef enum {
  TEST_DEVICE_ACCESS,
  TEST_SUBDEVICE_ACCESS
} ipc_mem_access_test_t;

typedef enum { TEST_SOCK, TEST_NONSOCK } ipc_mem_access_test_sock_t;

typedef struct {
  ipc_mem_access_test_t test_type;
  ipc_mem_access_test_sock_t test_sock_type;
  uint32_t size;
  ze_ipc_memory_flags_t flags;
  bool is_immediate;
  ze_ipc_mem_handle_t ipc_handle;
} shared_data_t;

#endif
