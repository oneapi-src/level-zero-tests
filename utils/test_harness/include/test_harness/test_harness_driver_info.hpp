/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/ze_api.h>

namespace level_zero_tests {
typedef struct driverInfo {
  ze_driver_handle_t driver_handle;

  uint32_t number_device_handles;
  ze_device_handle_t *device_handles;

  uint32_t number_device_properties;
  ze_device_properties_t *device_properties;

  uint32_t number_device_memory_properties;
  ze_device_memory_properties_t *device_memory_properties;

} driverInfo_t;
driverInfo_t *collect_driver_info(uint32_t &);
void free_driver_info(driverInfo_t *, uint32_t);
}; // namespace level_zero_tests
