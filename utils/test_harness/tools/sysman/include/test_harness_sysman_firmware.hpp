/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FIRMWARE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FIRMWARE_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_firmware_handle_count(zes_device_handle_t device);

std::vector<zes_firmware_handle_t>
get_firmware_handles(zes_device_handle_t device, uint32_t &count);

zes_firmware_properties_t
get_firmware_properties(zes_firmware_handle_t firmwareHandle);
void flash_firmware(zes_firmware_handle_t firmwareHandle, void *fwImage,
                    uint32_t size);

} // namespace level_zero_tests

#endif
