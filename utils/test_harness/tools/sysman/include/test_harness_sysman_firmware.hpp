/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FIRMWARE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FIRMWARE_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_firmware_handle_count(ze_device_handle_t device);

std::vector<zet_sysman_firmware_handle_t>
get_firmware_handles(ze_device_handle_t device, uint32_t &count);

zet_firmware_properties_t
get_firmware_properties(zet_sysman_firmware_handle_t firmwareHandle);
uint32_t get_firmware_checksum(zet_sysman_firmware_handle_t firmwareHandle);
void flash_firmware(zet_sysman_firmware_handle_t firmwareHandle, void *fwImage,
                    uint32_t size);

} // namespace level_zero_tests

#endif
