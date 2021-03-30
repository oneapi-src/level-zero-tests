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
    auto temp_handles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(temp_handles.size(), count);
    for (auto temp_handle : temp_handles) {
      ASSERT_NE(nullptr, temp_handle);
    }
  }
}
TEST_F(
    TempModuleTest,
    GivenInvalidComponentCountWhenRetrievingTempHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_temp_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_temp_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    TempModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarTempHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto temp_handlesInitial = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto temp_handle : temp_handlesInitial) {
      ASSERT_NE(nullptr, temp_handle);
    }

    count = 0;
    auto temp_handlesLater = lzt::get_temp_handles(device, count);
    for (auto temp_handle : temp_handlesLater) {
      ASSERT_NE(nullptr, temp_handle);
    }
    EXPECT_EQ(temp_handlesInitial, temp_handlesLater);
  }
}
TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenRetrievingTempPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto temp_handles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto temp_handle : temp_handles) {
      ASSERT_NE(nullptr, temp_handle);
      auto properties = lzt::get_temp_properties(temp_handle);
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
    auto temp_handles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto temp_handle : temp_handles) {
      ASSERT_NE(nullptr, temp_handle);
      auto properties_initial = lzt::get_temp_properties(temp_handle);
      auto properties_later = lzt::get_temp_properties(temp_handle);
      EXPECT_EQ(properties_initial.type, properties_later.type);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      EXPECT_EQ(properties_initial.isThreshold1Supported,
                properties_later.isThreshold1Supported);
      EXPECT_EQ(properties_initial.isCriticalTempSupported,
                properties_later.isCriticalTempSupported);
      EXPECT_EQ(properties_initial.isThreshold2Supported,
                properties_later.isThreshold2Supported);
    }
  }
}

TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenRetrievingTempConfigurationThenValidTempConfigurationIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto temp_handles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto temp_handle : temp_handles) {
      ASSERT_NE(nullptr, temp_handle);
      auto config = lzt::get_temp_config(temp_handle);
      auto properties = lzt::get_temp_properties(temp_handle);
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
    auto temp_handles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto temp_handle : temp_handles) {
      ASSERT_NE(nullptr, temp_handle);
      auto properties = lzt::get_temp_properties(temp_handle);
      auto initial_config = lzt::get_temp_config(temp_handle);
      auto temp = lzt::get_temp_state(temp_handle);
      zes_temp_config_t setConfig = {};
      if (properties.isCriticalTempSupported == true) {
        setConfig.enableCritical = true;
        lzt::set_temp_config(temp_handle, setConfig);
        auto get_config = lzt::get_temp_config(temp_handle);
        EXPECT_EQ(get_config.enableCritical, setConfig.enableCritical);
        EXPECT_EQ(get_config.threshold1.enableLowToHigh, false);
        EXPECT_EQ(get_config.threshold1.enableHighToLow, false);
        EXPECT_EQ(get_config.threshold2.enableLowToHigh, false);
        EXPECT_EQ(get_config.threshold2.enableHighToLow, false);
      }
      if (properties.isThreshold1Supported == true) {
        setConfig.threshold1.enableLowToHigh = true;
        setConfig.threshold1.enableHighToLow = false;
        setConfig.threshold1.threshold = temp;
        lzt::set_temp_config(temp_handle, setConfig);
        auto get_config = lzt::get_temp_config(temp_handle);
        EXPECT_EQ(get_config.threshold1.enableLowToHigh,
                  setConfig.threshold1.enableLowToHigh);
        EXPECT_EQ(get_config.threshold1.enableHighToLow,
                  setConfig.threshold1.enableHighToLow);
        EXPECT_EQ(get_config.threshold1.threshold,
                  setConfig.threshold1.threshold);
        EXPECT_EQ(get_config.enableCritical, false);
        EXPECT_EQ(get_config.threshold2.enableLowToHigh, false);
        EXPECT_EQ(get_config.threshold2.enableHighToLow, false);
      }
      if (properties.isThreshold1Supported == true) {
        setConfig.threshold2.enableLowToHigh = true;
        setConfig.threshold2.enableHighToLow = false;
        setConfig.threshold2.threshold = temp;
        lzt::set_temp_config(temp_handle, setConfig);
        auto get_config = lzt::get_temp_config(temp_handle);
        EXPECT_EQ(get_config.threshold2.enableLowToHigh,
                  setConfig.threshold2.enableLowToHigh);
        EXPECT_EQ(get_config.threshold2.enableHighToLow,
                  setConfig.threshold2.enableHighToLow);
        EXPECT_EQ(get_config.threshold2.threshold,
                  setConfig.threshold2.threshold);
        EXPECT_EQ(get_config.enableCritical, false);
        EXPECT_EQ(get_config.threshold1.enableLowToHigh, false);
        EXPECT_EQ(get_config.threshold1.enableHighToLow, false);
      }
      lzt::set_temp_config(temp_handle, initial_config);
    }
  }
}

TEST_F(TempModuleTest,
       GivenValidTempHandleWhenRetrievingTempStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto temp_handles = lzt::get_temp_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto temp_handle : temp_handles) {
      ASSERT_NE(nullptr, temp_handle);
      auto temp = lzt::get_temp_state(temp_handle);
      EXPECT_GT(temp, 0);
    }
  }
}

} // namespace
