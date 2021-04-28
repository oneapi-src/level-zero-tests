/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_firmware_handle_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFirmwares(device, &count, nullptr));
  return count;
}

std::vector<zes_firmware_handle_t>
get_firmware_handles(zes_device_handle_t device, uint32_t &count) {
  if (count == 0) {
    count = get_firmware_handle_count(device);
  }
  std::vector<zes_firmware_handle_t> firmwareHandles(count, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFirmwares(device, &count, firmwareHandles.data()));
  return firmwareHandles;
}

zes_firmware_properties_t
get_firmware_properties(zes_firmware_handle_t firmwareHandle) {
  zes_firmware_properties_t properties = {
      ZES_STRUCTURE_TYPE_FIRMWARE_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFirmwareGetProperties(firmwareHandle, &properties));
  return properties;
}

void flash_firmware(zes_firmware_handle_t firmwareHandle, void *fwImage,
                    uint32_t size) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFirmwareFlash(firmwareHandle, fwImage, size));
}
} // namespace level_zero_tests
