/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>

namespace {

void verify_sysman_api_return_value(zes_device_handle_t zes_device,
                                    ze_result_t expected_result) {
  uint32_t count = 0;
  ze_bool_t bool_false{};
  zes_device_handle_t device_handle{};
  zes_uuid_t uuid{};
  zes_device_properties_t properties{};
  zes_device_state_t state{};
  zes_pci_properties_t pci_properties{};
  zes_pci_state_t pci_state{};
  zes_pci_stats_t pci_stats{};
  zes_overclock_mode_t overclock_mode = ZES_OVERCLOCK_MODE_MODE_OFF;
  zes_pending_action_t pending_action = ZES_PENDING_ACTION_PENDING_NONE;
  zes_pwr_handle_t power_handle{};
  zes_device_ecc_properties_t ecc_properties{};
  const zes_device_ecc_desc_t ecc_state{};
  zes_fabric_port_handle_t fabric_port{};
  zes_fabric_port_throughput_t *fabric_throughput{};
  std::vector<zes_event_type_flags_t> event_type{};
  ze_result_t result{};

  result = zesDeviceGetProperties(zes_device, &properties);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceGetState(zes_device, &state);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceProcessesGetState(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDevicePciGetProperties(zes_device, &pci_properties);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDevicePciGetState(zes_device, &pci_state);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDevicePciGetBars(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDevicePciGetStats(zes_device, &pci_stats);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceSetOverclockWaiver(zes_device);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceGetOverclockDomains(zes_device, &count);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceGetOverclockControls(zes_device, ZES_OVERCLOCK_DOMAIN_CARD,
                                         &count);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceResetOverclockSettings(zes_device, false);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result =
      zesDeviceReadOverclockState(zes_device, &overclock_mode, &bool_false,
                                  &bool_false, &pending_action, &bool_false);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumOverclockDomains(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumDiagnosticTestSuites(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEccAvailable(zes_device, &bool_false);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEccConfigurable(zes_device, &bool_false);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceGetEccState(zes_device, &ecc_properties);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceSetEccState(zes_device, &ecc_state, &ecc_properties);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumEngineGroups(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result =
      zesDeviceEventRegister(zes_device, ZES_EVENT_TYPE_FLAG_DEVICE_DETACH);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumFabricPorts(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  if (count > 0) {
    result = zesFabricPortGetMultiPortThroughput(
        zes_device, count, &fabric_port, &fabric_throughput);
    EXPECT_TRUE(result == expected_result ||
                result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
  }

  result = zesDeviceEnumFans(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumFirmwares(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumFrequencyDomains(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumLeds(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumMemoryModules(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumPerformanceFactorDomains(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumPowerDomains(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceGetCardPowerDomain(zes_device, &power_handle);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumPsus(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumRasErrorSets(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumSchedulers(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumStandbyDomains(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumTemperatureSensors(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceGetSubDevicePropertiesExp(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);

  result = zesDeviceEnumEnabledVFExp(zes_device, &count, nullptr);
  EXPECT_TRUE(result == expected_result ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
}

TEST(SysmanInitTests,
     GivenZesInitWhenZesDriverGetIsCalledThenSuccessIsReturned) {
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&count, nullptr));
  EXPECT_GT(count, 0);
}

TEST(SysmanInitTests, GivenZesInitWhenZeInitIsCalledThenSuccessIsReturned) {
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t zes_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t ze_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zes_driver_count, nullptr));
  EXPECT_GT(zes_driver_count, 0);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&ze_driver_count, nullptr));
  EXPECT_GT(ze_driver_count, 0);
}

TEST(SysmanInitTests,
     GivenZeInitWithSysmanDisabledWhenZesInitIsCalledThenSuccessIsReturned) {
  auto is_sysman_enabled = getenv("ZES_ENABLE_SYSMAN");
  // Disabling enable_sysman env if it's defaultly enabled
  if (is_sysman_enabled != nullptr && strcmp(is_sysman_enabled, "1") == 0) {
    char disable_sysman_env[] = "ZES_ENABLE_SYSMAN=0";
    putenv(disable_sysman_env);
  }
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t ze_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t zes_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&ze_driver_count, nullptr));
  EXPECT_GT(ze_driver_count, 0);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zes_driver_count, nullptr));
  EXPECT_GT(zes_driver_count, 0);
  if (is_sysman_enabled != nullptr && strcmp(is_sysman_enabled, "1") == 0) {
    char enable_sysman_env[] = "ZES_ENABLE_SYSMAN=1";
    putenv(enable_sysman_env);
  }
}

TEST(
    SysmanInitTests,
    GivenZesInitAndZeInitWithSysmanEnabledWhenSysmanApiIsCalledWithZesDeviceThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<zes_driver_handle_t> zes_drivers =
      lzt::get_all_zes_driver_handles();
  EXPECT_FALSE(zes_drivers.empty());
  std::vector<zes_device_handle_t> zes_devices =
      lzt::get_zes_devices(zes_drivers[0]);
  EXPECT_FALSE(zes_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenZesInitAndZeInitWithSysmanEnabledWhenSysmanApiIsCalledWithZeDeviceThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(
      ZE_RESULT_ERROR_UNINITIALIZED,
      zesDeviceEnumFrequencyDomains(
          static_cast<zes_device_handle_t>(ze_devices[0]), &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenValidDeviceWhenBothZeInitAndZesInitAreCalledThenOnlyOneOfTheInitSucceeds) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  ze_result_t init_result = zesInit(0);

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  ze_result_t enum_result = zesDeviceEnumFrequencyDomains(
      static_cast<zes_device_handle_t>(ze_devices[0]), &count, nullptr);

  if (init_result == ZE_RESULT_SUCCESS) {
    EXPECT_EQ(enum_result, ZE_RESULT_ERROR_UNINITIALIZED);
    std::vector<zes_driver_handle_t> zes_drivers =
        lzt::get_all_zes_driver_handles();
    EXPECT_FALSE(zes_drivers.empty());
    std::vector<zes_device_handle_t> zes_devices =
        lzt::get_zes_devices(zes_drivers[0]);
    EXPECT_FALSE(zes_devices.empty());

    count = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));
    LOG_INFO << "zesInit is Successful... ";
  } else if (init_result == ZE_RESULT_ERROR_UNINITIALIZED) {
    EXPECT_EQ(enum_result, ZE_RESULT_SUCCESS);
    LOG_INFO << "zeInit is Successful... ";
  } else {
    FAIL() << "zesInit failed with error code : " << init_result;
  }
}

TEST(
    SysmanInitTests,
    GivenZesInitAndZeInitWithSysmanEnabledWhenSysmanApiIsCalledWithCoreToSysmanMappedDeviceThenSuccessIsReturned) {

  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(ze_devices[0], &properties));

  zes_uuid_t uuid = {};
  memcpy(uuid.id, properties.uuid.id, ZE_MAX_DEVICE_UUID_SIZE);
  std::vector<zes_driver_handle_t> zes_drivers =
      lzt::get_all_zes_driver_handles();
  EXPECT_FALSE(zes_drivers.empty());

  zes_device_handle_t sysman_device_handle{};
  ze_bool_t on_subdevice = false;
  uint32_t subdevice_id = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetDeviceByUuidExp(
                                   zes_drivers[0], uuid, &sysman_device_handle,
                                   &on_subdevice, &subdevice_id));
  EXPECT_NE(sysman_device_handle, nullptr);

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFrequencyDomains(
                                   sysman_device_handle, &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenZesInitAndZeInitWithSysmanDisabledWhenSysmanApiIsCalledWithZeDeviceThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(
      ZE_RESULT_ERROR_UNINITIALIZED,
      zesDeviceEnumFrequencyDomains(
          static_cast<zes_device_handle_t>(ze_devices[0]), &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenZesInitAndZeInitWithSysmanDisabledWhenSysmanApiIsCalledWithZesDeviceThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<zes_driver_handle_t> zes_drivers =
      lzt::get_all_zes_driver_handles();
  EXPECT_FALSE(zes_drivers.empty());
  std::vector<zes_device_handle_t> zes_devices =
      lzt::get_zes_devices(zes_drivers[0]);
  EXPECT_FALSE(zes_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenZeInitWithSysmanDisabledAndZesInitWhenSysmanApiIsCalledWithZesDeviceThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));

  std::vector<zes_driver_handle_t> zes_drivers =
      lzt::get_all_zes_driver_handles();
  EXPECT_FALSE(zes_drivers.empty());
  std::vector<zes_device_handle_t> zes_devices =
      lzt::get_zes_devices(zes_drivers[0]);
  EXPECT_FALSE(zes_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenZeInitWithSysmanDisabledAndZesInitWhenSysmanApiIsCalledWithZeDeviceThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(
      ZE_RESULT_ERROR_UNINITIALIZED,
      zesDeviceEnumFrequencyDomains(
          static_cast<zes_device_handle_t>(ze_devices[0]), &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenZesInitAndZeInitWithSysmanDisabledWhenSysmanApiIsCalledWithCoreToSysmanMappedDeviceThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(ze_devices[0], &properties));

  zes_uuid_t uuid = {};
  memcpy(uuid.id, properties.uuid.id, ZE_MAX_DEVICE_UUID_SIZE);
  std::vector<zes_driver_handle_t> zes_drivers =
      lzt::get_all_zes_driver_handles();
  EXPECT_FALSE(zes_drivers.empty());

  zes_device_handle_t sysman_device_handle{};
  ze_bool_t on_subdevice = false;
  uint32_t subdevice_id = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetDeviceByUuidExp(
                                   zes_drivers[0], uuid, &sysman_device_handle,
                                   &on_subdevice, &subdevice_id));
  EXPECT_NE(sysman_device_handle, nullptr);

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFrequencyDomains(
                                   sysman_device_handle, &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenZeInitWithSysmanDisabledAndZesInitWhenSysmanApiIsCalledWithCoreToSysmanMappedDeviceThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(ze_devices[0], &properties));

  zes_uuid_t uuid = {};
  memcpy(uuid.id, properties.uuid.id, ZE_MAX_DEVICE_UUID_SIZE);
  std::vector<zes_driver_handle_t> zes_drivers =
      lzt::get_all_zes_driver_handles();
  EXPECT_FALSE(zes_drivers.empty());

  zes_device_handle_t sysman_device_handle{};
  ze_bool_t on_subdevice = false;
  uint32_t subdevice_id = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetDeviceByUuidExp(
                                   zes_drivers[0], uuid, &sysman_device_handle,
                                   &on_subdevice, &subdevice_id));
  EXPECT_NE(sysman_device_handle, nullptr);

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFrequencyDomains(
                                   sysman_device_handle, &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenZesInitAndZeInitWithSysmanEnabledWhenMultipleSysmanApiAreCalledWithZeDeviceThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  verify_sysman_api_return_value(
      static_cast<zes_device_handle_t>(ze_devices[0]),
      ZE_RESULT_ERROR_UNINITIALIZED);
}

TEST(
    SysmanInitTests,
    GivenZesInitAndZeInitWithSysmanEnabledWhenMultipleSysmanApiAreCalledWithCoreToSysmanMappedDeviceThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(ze_devices[0], &properties));

  zes_uuid_t uuid = {};
  memcpy(uuid.id, properties.uuid.id, ZE_MAX_DEVICE_UUID_SIZE);
  std::vector<zes_driver_handle_t> zes_drivers =
      lzt::get_all_zes_driver_handles();
  EXPECT_FALSE(zes_drivers.empty());

  zes_device_handle_t sysman_device_handle{};
  ze_bool_t on_subdevice = false;
  uint32_t subdevice_id = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetDeviceByUuidExp(
                                   zes_drivers[0], uuid, &sysman_device_handle,
                                   &on_subdevice, &subdevice_id));
  EXPECT_NE(sysman_device_handle, nullptr);

  verify_sysman_api_return_value(sysman_device_handle, ZE_RESULT_SUCCESS);
}

TEST(
    SysmanInitTests,
    GivenZeInitWithSysmanEnabledWhenZesInitIsCalledThenSuccessIsReturnedOnNewPlatforms) {
  char sysman_env_enable[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sysman_env_enable);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  ze_device_properties_t properties = {};
  ze_device_ip_version_ext_t ip_version_ext{};
  properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  properties.pNext = &ip_version_ext;
  ip_version_ext.stype = ZE_STRUCTURE_TYPE_DEVICE_IP_VERSION_EXT;
  ip_version_ext.pNext = nullptr;
  uint32_t count{};
  uint32_t new_platform_ip_version = 0x05004000;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(ze_devices[0], &properties));

  bool new_platform = false;
  if (properties.type == ZE_DEVICE_TYPE_GPU && properties.vendorId == 0x8086) {
    ze_device_ip_version_ext_t *ip_version =
        static_cast<ze_device_ip_version_ext_t *>(properties.pNext);
    if (ip_version->ipVersion >= new_platform_ip_version) {
      new_platform = true;
    }
  }

  if (new_platform) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zesInit(0));

    std::vector<zes_driver_handle_t> zes_drivers =
        lzt::get_all_zes_driver_handles();
    EXPECT_FALSE(zes_drivers.empty());
    std::vector<zes_device_handle_t> zes_devices =
        lzt::get_zes_devices(zes_drivers[0]);
    EXPECT_FALSE(zes_devices.empty());

    count = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));

    count = 0;
    EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
              zesDeviceEnumFrequencyDomains(ze_devices[0], &count, nullptr));
    LOG_INFO << "zesInit is Successful... ";
  } else {
    count = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zesDeviceEnumFrequencyDomains(ze_devices[0], &count, nullptr));
    EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED, zesInit(0));
    LOG_INFO << "zeInit is Successful... ";
  }
}

} // namespace
