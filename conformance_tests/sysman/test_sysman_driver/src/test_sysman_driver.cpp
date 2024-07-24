/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace {

class SysmanDriverZesTest : public lzt::ZesSysmanCtsClass {};
#define SYSMAN_DRIVER_TEST SysmanDriverZesTest

TEST_F(
    SYSMAN_DRIVER_TEST,
    GivenValidDriverHandleWhileRetrievingExtensionPropertiesThenValidExtensionPropertiesIsReturned) {
  zes_driver_handle_t driver = lzt::get_default_zes_driver();
  for (auto device : devices) {
    uint32_t count = 0;
    ze_result_t result = lzt::get_driver_ext_properties(driver, &count);

    if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      LOG_INFO
          << "Skipping test as zesDriverGetExtensionProperties is Unsupported";
      GTEST_SKIP();
    }
    EXPECT_EQ(result, ZE_RESULT_SUCCESS);
    EXPECT_GE(count, 0);

    std::vector<zes_driver_extension_properties_t> ext_properties(count);
    lzt::get_driver_ext_properties(driver, &count, ext_properties);
    for (auto ext_property : ext_properties) {
      EXPECT_LE(strlen(ext_property.name), ZES_MAX_EXTENSION_NAME);
      EXPECT_GT(ext_property.version, 0);
    }
  }
}

TEST_F(
    SYSMAN_DRIVER_TEST,
    GivenValidDriverHandleWhileRetrievingExtensionFunctionAddressThenValidAddressIsReturned) {
  zes_driver_handle_t driver = lzt::get_default_zes_driver();
  std::vector<const char *> ext_functions = {
      "zesPowerSetLimitsExt",
      "zesPowerGetLimitsExt",
      "zesEngineGetActivityExt",
      "zesRasGetStateExp",
      "zesRasClearStateExp",
      "zesFirmwareGetSecurityVersionExp",
      "zesFirmwareSetSecurityVersionExp",
      "zesDriverGetDeviceByUuidExp",
      "zesDeviceGetSubDevicePropertiesExp",
      "zesDeviceEnumActiveVFExp",
      "zesVFManagementGetVFPropertiesExp",
      "zesVFManagementGetVFMemoryUtilizationExp",
      "zesVFManagementGetVFEngineUtilizationExp",
      "zesVFManagementSetVFTelemetryModeExp",
      "zesVFManagementSetVFTelemetrySamplingIntervalExp"};

  for (auto device : devices) {
    for (auto ext_function : ext_functions) {
      lzt::get_driver_ext_function_address(driver, ext_function);
    }
  }
}

} // namespace