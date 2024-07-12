/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

void get_sysman_devices_uuids(std::vector<zes_device_handle_t> sysman_devices,
                              std::vector<zes_uuid_t> &sysman_device_uuids) {
  for (const auto &sysman_device : sysman_devices) {
    zes_device_properties_t properties = {ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    zes_device_ext_properties_t ext_properties = {
        ZES_STRUCTURE_TYPE_DEVICE_EXT_PROPERTIES};
    properties.pNext = &ext_properties;
    ze_result_t result = zesDeviceGetProperties(sysman_device, &properties);
    auto sysman_device_uuid = ext_properties.uuid;
    sysman_device_uuids.push_back(sysman_device_uuid);
  }
}

void get_ze_root_uuids(std::vector<ze_device_handle_t> ze_devices,
                       std::vector<ze_device_uuid_t> &ze_root_uuids,
                       char *device_hierarchy) {
  for (const auto &ze_device : ze_devices) {
    ze_device_handle_t ze_root_device;
    if (strcmp(device_hierarchy, "COMBINED") == 0) {
      ze_root_device = lzt::get_root_device(ze_device);
      if (ze_root_device == nullptr) {
        ze_root_device = ze_device;
      }
    } else if (strcmp(device_hierarchy, "COMPOSITE") == 0) {
      ze_root_device = ze_device;
    } else {
      LOG_WARNING << "Unhandled ZE_FLAT_DEVICE_HIERARCHY mode:"
                  << device_hierarchy;
      continue;
    }
    auto root_device_properties = lzt::get_device_properties(ze_root_device);
    auto root_uuid = root_device_properties.uuid;
    if (std::find(ze_root_uuids.begin(), ze_root_uuids.end(), root_uuid) ==
        ze_root_uuids.end()) {
      ze_root_uuids.push_back(root_uuid);
    }
  }
}

int main(int argc, char **argv) {

  char *device_hierarchy = getenv("ZE_FLAT_DEVICE_HIERARCHY");
  EXPECT_NE(device_hierarchy, nullptr);
  LOG_INFO << "Device Hierarchy : " << device_hierarchy ? device_hierarchy
                                                        : "NULL";

  auto driver = lzt::zeDevice::get_instance()->get_driver();
  std::vector<ze_device_handle_t> ze_devices = lzt::get_ze_devices(driver);
  EXPECT_FALSE(ze_devices.empty());

  LOG_INFO << "Child process started for ZESINIT";
  ze_result_t result = zesInit(0);
  if (result) {
    throw std::runtime_error("Child Process : zesInit failed : " +
                             level_zero_tests::to_string(result));
    exit(1);
  }
  LOG_INFO << "Child Process : Sysman initialized";

  auto sysman_devices = lzt::get_zes_devices();
  EXPECT_FALSE(sysman_devices.empty());

  std::vector<zes_uuid_t> sysman_device_uuids;
  get_sysman_devices_uuids(sysman_devices, sysman_device_uuids);

  std::vector<ze_device_uuid_t> ze_root_uuids;
  get_ze_root_uuids(ze_devices, ze_root_uuids, device_hierarchy);

  for (const auto &ze_root_uuid : ze_root_uuids) {
    bool ze_and_sysman_uuid_equal = false;
    for (const auto &sysman_device_uuid : sysman_device_uuids) {
      if (memcmp(ze_root_uuid.id, sysman_device_uuid.id,
                 ZE_MAX_DEVICE_UUID_SIZE) == false) {
        ze_and_sysman_uuid_equal = true;
        break;
      }
    }
    EXPECT_TRUE(ze_and_sysman_uuid_equal);
  }

  exit(0);
}
