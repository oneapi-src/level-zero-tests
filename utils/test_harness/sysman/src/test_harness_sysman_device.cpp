/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

void sysman_device_reset(zes_device_handle_t device) {

  EXPECT_ZE_RESULT_SUCCESS(zesDeviceReset(device, false));
}

zes_device_properties_t
get_sysman_device_properties(zes_device_handle_t device) {
  zes_device_properties_t properties = {ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES,
                                        nullptr};
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceGetProperties(device, &properties));
  return properties;
}

zes_uuid_t get_sysman_device_uuid(zes_device_handle_t device) {
  zes_device_properties_t properties = {ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  zes_device_ext_properties_t ext_properties = {
      ZES_STRUCTURE_TYPE_DEVICE_EXT_PROPERTIES};
  properties.pNext = &ext_properties;
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceGetProperties(device, &properties));
  return ext_properties.uuid;
}

std::vector<zes_subdevice_exp_properties_t>
get_sysman_subdevice_properties(zes_device_handle_t device, uint32_t &count) {
  if (count == 0) {
    EXPECT_ZE_RESULT_SUCCESS(
        zesDeviceGetSubDevicePropertiesExp(device, &count, nullptr));
  }
  std::vector<zes_subdevice_exp_properties_t> sub_device_properties(count);
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceGetSubDevicePropertiesExp(
      device, &count, sub_device_properties.data()));
  return sub_device_properties;
}

zes_device_handle_t get_sysman_device_by_uuid(zes_driver_handle_t driver,
                                              zes_uuid_t uuid,
                                              ze_bool_t &on_sub_device,
                                              uint32_t &sub_device_id) {
  zes_device_handle_t device = {};
  EXPECT_ZE_RESULT_SUCCESS(zesDriverGetDeviceByUuidExp(
      driver, uuid, &device, &on_sub_device, &sub_device_id));
  return device;
}

uint32_t get_processes_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceProcessesGetState(device, &count, nullptr));
  return count;
}

std::vector<zes_process_state_t> get_processes_state(zes_device_handle_t device,
                                                     uint32_t &count) {
  if (count == 0)
    count = get_processes_count(device);
  std::vector<zes_process_state_t> processes(count);
  for (uint32_t i = 0; i < count; i++) {
    processes[i].stype = ZES_STRUCTURE_TYPE_PROCESS_STATE;
    processes[i].pNext = nullptr;
  }
  EXPECT_ZE_RESULT_SUCCESS(
      zesDeviceProcessesGetState(device, &count, processes.data()));
  return processes;
}

zes_device_state_t get_device_state(zes_device_handle_t device) {
  zes_device_state_t state = {ZES_STRUCTURE_TYPE_DEVICE_STATE, nullptr};
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceGetState(device, &state));
  return state;
}

void sysman_device_reset_ext(zes_device_handle_t device, ze_bool_t force,
                             zes_reset_type_t type) {
  zes_reset_properties_t properties{};
  properties.stype = ZES_STRUCTURE_TYPE_RESET_PROPERTIES;
  properties.pNext = nullptr;
  properties.force = force;
  properties.resetType = type;
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceResetExt(device, &properties));
}

bool is_uuid_pair_equal(uint8_t *uuid1, uint8_t *uuid2) {
  for (uint32_t i = 0; i < ZE_MAX_UUID_SIZE; i++) {
    if (uuid1[i] != uuid2[i]) {
      return false;
    }
  }
  return true;
}

ze_device_handle_t get_core_device_by_uuid(uint8_t *uuid) {
  lzt::initialize_core();
  auto driver = lzt::zeDevice::get_instance()->get_driver();
  auto core_devices = lzt::get_ze_devices(driver);
  for (auto device : core_devices) {
    auto device_properties = lzt::get_device_properties(device);
    if (is_uuid_pair_equal(uuid, device_properties.uuid.id)) {
      return device;
    }
  }
  return nullptr;
}
} // namespace level_zero_tests
