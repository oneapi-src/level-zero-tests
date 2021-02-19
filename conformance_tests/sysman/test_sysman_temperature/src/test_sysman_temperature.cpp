/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;
#include <level_zero/zes_api.h>

namespace {

class TempModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    TempModuleTest,
    GivenComponentCountZeroWhenRetrievingTempHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_temp_handle_count(device);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    TempModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanTempThenNotNullTempHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(tempHandles.size(), count);
    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
    }
  }
}
TEST_F(
    TempModuleTest,
    GivenInvalidComponentCountWhenRetrievingTempHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_temp_handles(device, actualCount);
    if (actualCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t testCount = actualCount + 1;
    lzt::get_temp_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    TempModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarTempHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandlesInitial = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto tempHandle : tempHandlesInitial) {
      ASSERT_NE(nullptr, tempHandle);
    }

    count = 0;
    auto tempHandlesLater = lzt::get_temp_handles(device, count);
    for (auto tempHandle : tempHandlesLater) {
      ASSERT_NE(nullptr, tempHandle);
    }
    EXPECT_EQ(tempHandlesInitial, tempHandlesLater);
  }
}
TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenRetrievingTempPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      auto properties = lzt::get_temp_properties(tempHandle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      EXPECT_GE(properties.type, ZES_TEMP_SENSORS_GLOBAL);
      EXPECT_LE(properties.type, ZES_TEMP_SENSORS_MEMORY_MIN);
    }
  }
}
TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenRetrievingTempPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      auto propertiesInitial = lzt::get_temp_properties(tempHandle);
      auto propertiesLater = lzt::get_temp_properties(tempHandle);
      EXPECT_EQ(propertiesInitial.type, propertiesLater.type);
      if (propertiesInitial.onSubdevice && propertiesLater.onSubdevice) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      EXPECT_EQ(propertiesInitial.isThreshold1Supported,
                propertiesLater.isThreshold1Supported);
      EXPECT_EQ(propertiesInitial.isCriticalTempSupported,
                propertiesLater.isCriticalTempSupported);
      EXPECT_EQ(propertiesInitial.isThreshold2Supported,
                propertiesLater.isThreshold2Supported);
    }
  }
}

TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenRetrievingTempConfigurationThenValidTempConfigurationIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      auto config = lzt::get_temp_config(tempHandle);
      auto properties = lzt::get_temp_properties(tempHandle);
      if (properties.isCriticalTempSupported == false) {
        ASSERT_EQ(config.enableCritical, false);
      }
      if (properties.isThreshold1Supported == false) {
        ASSERT_EQ(config.threshold1.enableLowToHigh, false);
        ASSERT_EQ(config.threshold1.enableHighToLow, false);
      }
      if (properties.isThreshold2Supported == false) {
        ASSERT_EQ(config.threshold2.enableLowToHigh, false);
        ASSERT_EQ(config.threshold2.enableHighToLow, false);
      }
    }
  }
}
TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenSettingTempConfigurationThenExpectzesSysmanTemperatureSetConfigFollowedByzesSysmanTemperatureGetConfigToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      auto properties = lzt::get_temp_properties(tempHandle);
      auto initialConfig = lzt::get_temp_config(tempHandle);
      auto temp = lzt::get_temp_state(tempHandle);
      zes_temp_config_t setConfig = {};
      if (properties.isCriticalTempSupported == true) {
        setConfig.enableCritical = true;
        lzt::set_temp_config(tempHandle, setConfig);
        auto getConfig = lzt::get_temp_config(tempHandle);
        EXPECT_EQ(getConfig.enableCritical, setConfig.enableCritical);
        EXPECT_EQ(getConfig.threshold1.enableLowToHigh, false);
        EXPECT_EQ(getConfig.threshold1.enableHighToLow, false);
        EXPECT_EQ(getConfig.threshold2.enableLowToHigh, false);
        EXPECT_EQ(getConfig.threshold2.enableHighToLow, false);
      }
      if (properties.isThreshold1Supported == true) {
        setConfig.threshold1.enableLowToHigh = true;
        setConfig.threshold1.enableHighToLow = false;
        setConfig.threshold1.threshold = temp;
        lzt::set_temp_config(tempHandle, setConfig);
        auto getConfig = lzt::get_temp_config(tempHandle);
        EXPECT_EQ(getConfig.threshold1.enableLowToHigh,
                  setConfig.threshold1.enableLowToHigh);
        EXPECT_EQ(getConfig.threshold1.enableHighToLow,
                  setConfig.threshold1.enableHighToLow);
        EXPECT_EQ(getConfig.threshold1.threshold,
                  setConfig.threshold1.threshold);
        EXPECT_EQ(getConfig.enableCritical, false);
        EXPECT_EQ(getConfig.threshold2.enableLowToHigh, false);
        EXPECT_EQ(getConfig.threshold2.enableHighToLow, false);
      }
      if (properties.isThreshold1Supported == true) {
        setConfig.threshold2.enableLowToHigh = true;
        setConfig.threshold2.enableHighToLow = false;
        setConfig.threshold2.threshold = temp;
        lzt::set_temp_config(tempHandle, setConfig);
        auto getConfig = lzt::get_temp_config(tempHandle);
        EXPECT_EQ(getConfig.threshold2.enableLowToHigh,
                  setConfig.threshold2.enableLowToHigh);
        EXPECT_EQ(getConfig.threshold2.enableHighToLow,
                  setConfig.threshold2.enableHighToLow);
        EXPECT_EQ(getConfig.threshold2.threshold,
                  setConfig.threshold2.threshold);
        EXPECT_EQ(getConfig.enableCritical, false);
        EXPECT_EQ(getConfig.threshold1.enableLowToHigh, false);
        EXPECT_EQ(getConfig.threshold1.enableHighToLow, false);
      }
      lzt::set_temp_config(tempHandle, initialConfig);
    }
  }
}

TEST_F(TempModuleTest,
       GivenValidTempHandleWhenRetrievingTempStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      auto temp = lzt::get_temp_state(tempHandle);
      EXPECT_GT(temp, 0);
    }
  }
}

} // namespace
