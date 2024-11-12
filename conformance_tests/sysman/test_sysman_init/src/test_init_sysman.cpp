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

std::vector<ze_driver_handle_t> get_ze_drivers() {
  uint32_t ze_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&ze_driver_count, nullptr));
  EXPECT_GT(ze_driver_count, 0);
  std::vector<ze_driver_handle_t> ze_drivers(ze_driver_count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDriverGet(&ze_driver_count, ze_drivers.data()));
  return ze_drivers;
}

std::vector<ze_device_handle_t> get_ze_devices(ze_driver_handle_t ze_driver) {
  uint32_t ze_device_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGet(ze_driver, &ze_device_count, nullptr));
  EXPECT_GT(ze_device_count, 0);
  std::vector<ze_device_handle_t> ze_devices(ze_device_count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGet(ze_driver, &ze_device_count, ze_devices.data()));
  return ze_devices;
}

std::vector<zes_driver_handle_t> get_zes_drivers() {
  uint32_t zes_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zes_driver_count, nullptr));
  EXPECT_GT(zes_driver_count, 0);
  std::vector<zes_driver_handle_t> zes_drivers(zes_driver_count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDriverGet(&zes_driver_count, zes_drivers.data()));
  return zes_drivers;
}

std::vector<zes_device_handle_t>
get_zes_devices(zes_driver_handle_t zes_driver) {
  uint32_t zes_device_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceGet(zes_driver, &zes_device_count, nullptr));
  EXPECT_GT(zes_device_count, 0);
  std::vector<zes_device_handle_t> zes_devices(zes_device_count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceGet(zes_driver, &zes_device_count, zes_devices.data()));
  return zes_devices;
}

TEST(SysmanInitTests,
     GivenCoreNotInitializedWhenSysmanInitializedThenzesDriverGetWorks) {
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t pCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&pCount, nullptr));
  ASSERT_GT(pCount, 0);
}

TEST(SysmanInitTests, GivenSysmanInitializedThenCallingCoreInitSucceeds) {
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t zesInitCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t zeInitCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zesInitCount, nullptr));
  ASSERT_GT(zesInitCount, 0);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&zeInitCount, nullptr));
  ASSERT_GT(zeInitCount, 0);
}

TEST(
    SysmanInitTests,
    GivenSysmanEnableEnvDisabledAndCoreInitializedFirstWhenSysmanInitializedThenzesDriverGetWorks) {
  auto is_sysman_enabled = getenv("ZES_ENABLE_SYSMAN");
  // Disabling enable_sysman env if it's defaultly enabled
  if (is_sysman_enabled != nullptr && strcmp(is_sysman_enabled, "1") == 0) {
    char disable_sysman_env[] = "ZES_ENABLE_SYSMAN=0";
    putenv(disable_sysman_env);
  }
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t zeInitCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t zesInitCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&zeInitCount, nullptr));
  ASSERT_GT(zeInitCount, 0);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zesInitCount, nullptr));
  ASSERT_GT(zesInitCount, 0);
  if (is_sysman_enabled != nullptr && strcmp(is_sysman_enabled, "1") == 0) {
    char enable_sysman_env[] = "ZES_ENABLE_SYSMAN=1";
    putenv(enable_sysman_env);
  }
}

TEST(
    SysmanInitTests,
    GivenSysmanInitializedViaZesInitAndCoreInitializedWithSysmanEnabledWhenEnumFrequencyDomainsIsCalledWithSysmanHandleThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<zes_driver_handle_t> zes_drivers = get_zes_drivers();
  std::vector<zes_device_handle_t> zes_devices =
      get_zes_devices(zes_drivers[0]);
  ASSERT_FALSE(zes_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenSysmanInitializedViaZesInitAndCoreInitializedWithSysmanEnabledWhenEnumFrequencyDomainsIsCalledWithCoreHandleThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = get_ze_drivers();
  std::vector<ze_device_handle_t> ze_devices = get_ze_devices(ze_drivers[0]);
  ASSERT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(
      ZE_RESULT_ERROR_UNINITIALIZED,
      zesDeviceEnumFrequencyDomains(
          static_cast<zes_device_handle_t>(ze_devices[0]), &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenCoreInitializedWithSysmanEnabledWhenSysmanInitializedViaZesInitThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  ASSERT_EQ(ZE_RESULT_ERROR_UNINITIALIZED, zesInit(0));
}

TEST(
    SysmanInitTests,
    GivenSysmanInitializedViaZesInitAndCoreInitializedWithSysmanEnabledWhenEnumFrequencyDomainsIsCalledWithCoreToSysmanMappedHandleThenSuccessIsReturned) {

  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = get_ze_drivers();
  std::vector<ze_device_handle_t> ze_devices = get_ze_devices(ze_drivers[0]);
  ASSERT_FALSE(ze_devices.empty());

  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(ze_devices[0], &properties));

  zes_uuid_t uuid = {};
  memcpy(uuid.id, properties.uuid.id, ZE_MAX_DEVICE_UUID_SIZE);
  std::vector<zes_driver_handle_t> zes_drivers = get_zes_drivers();

  zes_device_handle_t sysman_device_handle{};
  ze_bool_t on_subdevice = false;
  uint32_t subdevice_id = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetDeviceByUuidExp(
                                   zes_drivers[0], uuid, &sysman_device_handle,
                                   &on_subdevice, &subdevice_id));
  ASSERT_NE(sysman_device_handle, nullptr);

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFrequencyDomains(
                                   sysman_device_handle, &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenSysmanInitializedViaZesInitAndCoreInitializedWithSysmanDisabledWhenEnumFrequencyDomainsIsCalledWithCoreHandleThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = get_ze_drivers();
  std::vector<ze_device_handle_t> ze_devices = get_ze_devices(ze_drivers[0]);
  ASSERT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(
      ZE_RESULT_ERROR_UNINITIALIZED,
      zesDeviceEnumFrequencyDomains(
          static_cast<zes_device_handle_t>(ze_devices[0]), &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenSysmanInitializedViaZesInitAndCoreInitializedWithSysmanDisabledWhenEnumFrequencyDomainsIsCalledWithSysmanHandleThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<zes_driver_handle_t> zes_drivers = get_zes_drivers();
  std::vector<zes_device_handle_t> zes_devices =
      get_zes_devices(zes_drivers[0]);
  ASSERT_FALSE(zes_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenCoreInitializedWithSysmanDisabledAndSysmanInitializedViaZesInitWhenEnumFrequencyDomainsIsCalledWithSysmanHandleThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));

  std::vector<zes_driver_handle_t> zes_drivers = get_zes_drivers();
  std::vector<zes_device_handle_t> zes_devices =
      get_zes_devices(zes_drivers[0]);
  ASSERT_FALSE(zes_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenCoreInitializedWithSysmanDisabledAndSysmanInitializedViaZesInitWhenEnumFrequencyDomainsIsCalledWithCoreHandleThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = get_ze_drivers();
  std::vector<ze_device_handle_t> ze_devices = get_ze_devices(ze_drivers[0]);
  ASSERT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(
      ZE_RESULT_ERROR_UNINITIALIZED,
      zesDeviceEnumFrequencyDomains(
          static_cast<zes_device_handle_t>(ze_devices[0]), &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenSysmanInitializationDoneViaZesInitAndCoreInitializedWithSysmanDisabledWhenEnumFrequencyDomainsIsCalledWithCoreToSysmanMappedHandleThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = get_ze_drivers();
  std::vector<ze_device_handle_t> ze_devices = get_ze_devices(ze_drivers[0]);
  ASSERT_FALSE(ze_devices.empty());

  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(ze_devices[0], &properties));

  zes_uuid_t uuid = {};
  memcpy(uuid.id, properties.uuid.id, ZE_MAX_DEVICE_UUID_SIZE);
  std::vector<zes_driver_handle_t> zes_drivers = get_zes_drivers();

  zes_device_handle_t sysman_device_handle{};
  ze_bool_t on_subdevice = false;
  uint32_t subdevice_id = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetDeviceByUuidExp(
                                   zes_drivers[0], uuid, &sysman_device_handle,
                                   &on_subdevice, &subdevice_id));
  ASSERT_NE(sysman_device_handle, nullptr);

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFrequencyDomains(
                                   sysman_device_handle, &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenCoreInitializedWithSysmanDisabledAndSysmanInitializedViaZesInitWhenEnumFrequencyDomainsIsCalledWithCoreToSysmanMappedHandleThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = get_ze_drivers();
  std::vector<ze_device_handle_t> ze_devices = get_ze_devices(ze_drivers[0]);
  ASSERT_FALSE(ze_devices.empty());

  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetProperties(ze_devices[0], &properties));

  zes_uuid_t uuid = {};
  memcpy(uuid.id, properties.uuid.id, ZE_MAX_DEVICE_UUID_SIZE);
  std::vector<zes_driver_handle_t> zes_drivers = get_zes_drivers();

  zes_device_handle_t sysman_device_handle{};
  ze_bool_t on_subdevice = false;
  uint32_t subdevice_id = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetDeviceByUuidExp(
                                   zes_drivers[0], uuid, &sysman_device_handle,
                                   &on_subdevice, &subdevice_id));
  ASSERT_NE(sysman_device_handle, nullptr);

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFrequencyDomains(
                                   sysman_device_handle, &count, nullptr));
}

TEST(
    SysmanInitTests,
    GivenSysmanInitializedViaZesInitAndCoreInitializedWithSysmanEnabledWhenSysmanApiAreCalledWithCoreHandleThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = get_ze_drivers();
  std::vector<ze_device_handle_t> ze_devices = get_ze_devices(ze_drivers[0]);
  ASSERT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  ze_bool_t bool_false{};
  zes_device_handle_t device_handle{};
  zes_uuid_t uuid{};
  zes_device_properties_t properties{};
  zes_device_state_t state{};
  zes_reset_properties_t reset_properties{};
  zes_pci_properties_t pci_properties{};
  zes_pci_state_t pci_state{};
  zes_pci_stats_t pci_stats{};
  zes_device_ecc_properties_t ecc_properties{};
  const zes_device_ecc_desc_t ecc_state{};
  zes_fabric_port_handle_t fabric_port{};
  zes_fabric_port_throughput_t *fabric_throughput{};
  std::vector<zes_event_type_flags_t> event_type{};

  zes_device_handle_t zes_device =
      static_cast<zes_device_handle_t>(ze_devices[0]);

  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetProperties(zes_device, &properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetState(zes_device, &state));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED, zesDeviceReset(zes_device, false));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceResetExt(zes_device, &reset_properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceProcessesGetState(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDevicePciGetProperties(zes_device, &pci_properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDevicePciGetState(zes_device, &pci_state));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDevicePciGetBars(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDevicePciGetStats(zes_device, &pci_stats));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumDiagnosticTestSuites(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEccAvailable(zes_device, &bool_false));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEccConfigurable(zes_device, &bool_false));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetEccState(zes_device, &ecc_properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceSetEccState(zes_device, &ecc_state, &ecc_properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumEngineGroups(zes_device, &count, nullptr));
  EXPECT_EQ(
      ZE_RESULT_ERROR_UNINITIALIZED,
      zesDeviceEventRegister(zes_device, ZES_EVENT_TYPE_FLAG_DEVICE_DETACH));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFabricPorts(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesFabricPortGetMultiPortThroughput(zes_device, count, &fabric_port,
                                                &fabric_throughput));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFans(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFirmwares(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFrequencyDomains(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumMemoryModules(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumPerformanceFactorDomains(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumPowerDomains(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumRasErrorSets(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumSchedulers(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumStandbyDomains(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumTemperatureSensors(zes_device, &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetSubDevicePropertiesExp(zes_device, &count, nullptr));
}

} // namespace
