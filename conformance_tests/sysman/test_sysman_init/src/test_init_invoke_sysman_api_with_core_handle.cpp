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

TEST(
    SysmanInitTests,
    GivenSysmanInitializedFromZesInitAndCoreInitializedWithSysmanFlagWhenSysmanApiAreCalledWithCoreHandleThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  uint32_t driver_count = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&driver_count, nullptr));
  ASSERT_GT(driver_count, 0);
  std::vector<zes_driver_handle_t> drivers(driver_count);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&driver_count, drivers.data()));

  uint32_t device_count = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDeviceGet(drivers[0], &device_count, nullptr));
  std::vector<zes_device_handle_t> devices(device_count);
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGet(drivers[0], &device_count, devices.data()));

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
  zes_overclock_mode_t overclock_mode = ZES_OVERCLOCK_MODE_MODE_OFF;
  zes_pending_action_t pending_action = ZES_PENDING_ACTION_PENDING_NONE;
  zes_device_ecc_properties_t ecc_properties{};
  const zes_device_ecc_desc_t ecc_state{};
  zes_fabric_port_handle_t fabric_port{};
  zes_fabric_port_throughput_t *fabric_throughput{};
  std::vector<zes_event_type_flags_t> event_type{};

  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGet(drivers[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetProperties(devices[0], &properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetState(devices[0], &state));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED, zesDeviceReset(devices[0], false));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceResetExt(devices[0], &reset_properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceProcessesGetState(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDevicePciGetProperties(devices[0], &pci_properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDevicePciGetState(devices[0], &pci_state));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDevicePciGetBars(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDevicePciGetStats(devices[0], &pci_stats));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceSetOverclockWaiver(devices[0]));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetOverclockDomains(devices[0], &count));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetOverclockControls(devices[0], ZES_OVERCLOCK_DOMAIN_CARD,
                                          &count));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceResetOverclockSettings(devices[0], false));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceReadOverclockState(devices[0], &overclock_mode,
                                        &bool_false, &bool_false,
                                        &pending_action, &bool_false));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumOverclockDomains(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumDiagnosticTestSuites(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEccAvailable(devices[0], &bool_false));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEccConfigurable(devices[0], &bool_false));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetEccState(devices[0], &ecc_properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceSetEccState(devices[0], &ecc_state, &ecc_properties));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumEngineGroups(devices[0], &count, nullptr));
  EXPECT_EQ(
      ZE_RESULT_ERROR_UNINITIALIZED,
      zesDeviceEventRegister(devices[0], ZES_EVENT_TYPE_FLAG_DEVICE_DETACH));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDriverEventListen(drivers[0],
                                 std::numeric_limits<uint32_t>::max(), 1,
                                 &devices[0], &count, event_type.data()));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDriverEventListenEx(drivers[0],
                                   std::numeric_limits<uint64_t>::max(), 1,
                                   &devices[0], &count, event_type.data()));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFabricPorts(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesFabricPortGetMultiPortThroughput(devices[0], count, &fabric_port,
                                                &fabric_throughput));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFans(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFirmwares(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFrequencyDomains(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumLeds(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumMemoryModules(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumPerformanceFactorDomains(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumPowerDomains(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumPsus(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumRasErrorSets(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumSchedulers(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumStandbyDomains(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumTemperatureSensors(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceGetSubDevicePropertiesExp(devices[0], &count, nullptr));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDriverGetDeviceByUuidExp(drivers[0], uuid, &device_handle,
                                        &bool_false, &count));
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumEnabledVFExp(devices[0], &count, nullptr));
}

} // namespace