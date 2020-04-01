/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

void ipc_memory_handle_get(ZeApp *benchmark, probe_config_t &probe_setting) {
  void *buffer;
  ze_ipc_mem_handle_t ipc_handle;
  size_t buffer_size = sizeof(uint8_t);

  benchmark->memoryAlloc(buffer_size, &buffer);
  /* Warm up */
  for (int i = 0; i < probe_setting.warm_up_iteration; i++) {
    zeDriverGetMemIpcHandle(benchmark->driver, buffer, &ipc_handle);
  }

  NANO_PROBE(" IPC Handle Get\t", probe_setting, zeDriverGetMemIpcHandle,
             benchmark->driver, buffer, &ipc_handle);

  benchmark->memoryFree(&buffer);
}
