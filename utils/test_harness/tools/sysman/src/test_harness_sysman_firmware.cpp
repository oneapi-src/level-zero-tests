/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_firmware_handle_count(ze_device_handle_t device) {
  uint32_t count = 0;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFirmwareGet(hSysman, &count, nullptr));
  return count;
}

std::vector<zet_sysman_firmware_handle_t>
get_firmware_handles(ze_device_handle_t device, uint32_t &count) {
  std::vector<zet_sysman_firmware_handle_t> firmwareHandles;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0) {
    count = get_firmware_handle_count(device);
  }
  firmwareHandles.resize(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFirmwareGet(hSysman, &count, firmwareHandles.data()));
  EXPECT_EQ(firmwareHandles.size(), count);
  return firmwareHandles;
}

zet_firmware_properties_t
get_firmware_properties(zet_sysman_firmware_handle_t firmwareHandle) {
  zet_firmware_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFirmwareGetProperties(firmwareHandle, &properties));
  return properties;
}

uint32_t get_firmware_checksum(zet_sysman_firmware_handle_t firmwareHandle) {
  uint32_t cksum;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFirmwareGetChecksum(firmwareHandle, &cksum));
  EXPECT_GE(cksum, 0);
  EXPECT_LE(cksum, UINT32_MAX);
  return cksum;
}
void flash_firmware(zet_sysman_firmware_handle_t firmwareHandle, void *fwImage,
                    uint32_t size) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFirmwareFlash(firmwareHandle, fwImage, size));
}
} // namespace level_zero_tests
