/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/ze_api.h>

enum peer_transfer_t { PEER_NONE, PEER_WRITE, PEER_READ };
enum peer_test_t { PEER_BANDWIDTH, PEER_LATENCY };

struct device_environ_t {
  uint32_t device_index;
  ze_command_queue_handle_t command_queue;
  ze_command_list_handle_t command_list;

  device_environ_t(uint32_t device_index)
      : device_index(device_index), command_queue(nullptr),
        command_list(nullptr) {}
};
