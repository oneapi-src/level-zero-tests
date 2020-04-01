/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/ze_api.h>

enum peer_transfer_t {
    PEER_NONE,
    PEER_WRITE,
    PEER_READ
};

struct device_context_t {
    ze_device_handle_t device;
    ze_module_handle_t module;
    ze_command_queue_handle_t command_queue;
    ze_command_list_handle_t command_list;
};
