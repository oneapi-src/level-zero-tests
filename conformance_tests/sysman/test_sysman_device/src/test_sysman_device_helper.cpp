/*
 *
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

typedef std::array<uint8_t, ZE_MAX_DEVICE_UUID_SIZE> UUID;

#define TO_STD_ARRAY(x)                                                        \
  [](const uint8_t(&arr)[ZE_MAX_DEVICE_UUID_SIZE]) {                           \
    UUID uuid;                                                                 \
    std::copy(std::begin(arr), std::end(arr), uuid.begin());                   \
    return uuid;                                                               \
  }(x)

UUID get_sysman_device_uuid(zes_device_handle_t sysman_device) {
  zes_device_properties_t properties = {ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  zes_device_ext_properties_t ext_properties = {
      ZES_STRUCTURE_TYPE_DEVICE_EXT_PROPERTIES};
  properties.pNext = &ext_properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceGetProperties(sysman_device, &properties));
  auto sysman_device_uuid = ext_properties.uuid;
  return TO_STD_ARRAY(sysman_device_uuid.id);
}

void get_sysman_sub_devices_uuids(zes_device_handle_t sysman_device,
                                  std::vector<UUID> &sysman_device_uuids) {
  uint32_t num_sub_devices = 0;
  auto sub_device_properties =
      lzt::get_sysman_subdevice_properties(sysman_device, num_sub_devices);
  for (uint32_t sub_device_index = 0; sub_device_index < num_sub_devices;
       sub_device_index++) {
    sysman_device_uuids.push_back(
        TO_STD_ARRAY(sub_device_properties[sub_device_index].uuid.id));
  }
}

UUID get_ze_device_uuid(ze_device_handle_t ze_device) {
  auto ze_device_properties = lzt::get_device_properties(ze_device);
  auto ze_device_uuid = ze_device_properties.uuid;
  return TO_STD_ARRAY(ze_device_uuid.id);
}

UUID get_ze_root_uuid(ze_device_handle_t ze_device,
                      const char *device_hierarchy) {
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
  }

  return get_ze_device_uuid(ze_root_device);
}

bool compare_core_and_sysman_uuid(std::vector<UUID> core_uuids,
                                  std::vector<UUID> sysman_uuids) {
  std::sort(core_uuids.begin(), core_uuids.end());
  std::sort(sysman_uuids.begin(), sysman_uuids.end());
  return (
      core_uuids.size() == sysman_uuids.size() &&
      std::equal(core_uuids.begin(), core_uuids.end(), sysman_uuids.begin()));
}

int main(int argc, char **argv) {

  const char *device_hierarchy = getenv("ZE_FLAT_DEVICE_HIERARCHY");
  EXPECT_NE(device_hierarchy, nullptr);
  device_hierarchy = device_hierarchy ? device_hierarchy : "NULL";
  LOG_INFO << "Device Hierarchy : " << device_hierarchy;

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
  // We must retrieve the SysMan Drivers on the system, then verify that the
  // driver has valid devices. The first driver with valid devices will be used.
  auto zes_drivers = lzt::get_all_zes_driver_handles();
  zes_driver_handle_t zes_driver_used;
  for (const auto &zes_driver : zes_drivers) {
    auto device_count = lzt::get_zes_device_count(zes_driver);
    if (device_count > 0) {
      zes_driver_used = zes_driver;
      break;
    }
  }
  auto sysman_devices = lzt::get_zes_devices(
      lzt::get_zes_device_count(zes_driver_used), zes_driver_used);
  EXPECT_FALSE(sysman_devices.empty());

  std::vector<UUID> sysman_device_uuids{};
  std::vector<UUID> ze_device_uuids{};

  if (strcmp(device_hierarchy, "FLAT") != 0) { // composite or combined mode
    for (const auto &sysman_device : sysman_devices) {
      sysman_device_uuids.push_back(get_sysman_device_uuid(sysman_device));
    }

    for (const auto &ze_device : ze_devices) {
      auto ze_root_uuid = get_ze_root_uuid(ze_device, device_hierarchy);
      if (std::find(ze_device_uuids.begin(), ze_device_uuids.end(),
                    ze_root_uuid) == ze_device_uuids.end()) {
        ze_device_uuids.push_back(ze_root_uuid);
      }
    }
  } else { // flat mode
    for (const auto &sysman_device : sysman_devices) {
      auto device_properties = lzt::get_sysman_device_properties(sysman_device);
      uint32_t sub_devices_count = device_properties.numSubdevices;
      if (sub_devices_count > 0) {
        get_sysman_sub_devices_uuids(sysman_device, sysman_device_uuids);
      } else {
        // if subdevice doesn't exist for a device, then root device UUID
        // is retrieved to match with core UUID's retrieved from flat mode
        sysman_device_uuids.push_back(get_sysman_device_uuid(sysman_device));
      }
    }

    for (const auto &ze_device : ze_devices) {
      ze_device_uuids.push_back(get_ze_device_uuid(ze_device));
    }
  }

  EXPECT_TRUE(
      compare_core_and_sysman_uuid(ze_device_uuids, sysman_device_uuids));

  exit(0);
}
