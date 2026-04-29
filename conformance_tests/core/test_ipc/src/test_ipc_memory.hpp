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
  TEST_SUBDEVICE_ACCESS,
  TEST_MULTIDEVICE_ACCESS,
  TEST_HOST_ACCESS,
  TEST_LOOP_ACCESS // IPC handle loop test (multiple iterations, bulk close)
} ipc_mem_access_test_t;

typedef enum { TEST_SOCK, TEST_NONSOCK } ipc_mem_access_test_sock_t;

typedef struct {
  ipc_mem_access_test_t test_type;
  ipc_mem_access_test_sock_t test_sock_type;
  uint32_t size;
  ze_ipc_memory_flags_t flags;
  bool is_immediate;
  ze_ipc_mem_handle_t ipc_handle;
  uint32_t device_id_parent; // Device index for parent (memory allocation)
  uint32_t device_id_child;  // Device index for child (memory access)
  bool use_copy_engine;      // Use copy-only engine for memory copy
} shared_data_t;

// Number of IPC handle iterations for the loop test
static const uint32_t IPC_LOOP_NUM_ITERATIONS = 400;

// Shared memory layout for the IPC loop test. Each element of ipc_handles[]
// corresponds to a device allocation filled with value equal to its index.
// The child opens all handles simultaneously and closes them in a batch at
// the end, exercising handle-lifecycle robustness.
typedef struct {
  ipc_mem_access_test_t test_type; // always TEST_LOOP_ACCESS
  uint32_t num_iterations;
  uint32_t size;
  ze_ipc_memory_flags_t flags;
  bool is_immediate;
  ze_ipc_mem_handle_t ipc_handles[IPC_LOOP_NUM_ITERATIONS];
} shared_data_loop_t;

#endif
