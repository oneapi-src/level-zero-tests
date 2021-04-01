/*
 *
 * Copyright (C) 2020-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class FanModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    FanModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_fan_handle_count(device);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFanHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fan_handles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(fan_handles.size(), count);
    for (auto fan_handle : fan_handles) {
      ASSERT_NE(nullptr, fan_handle);
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_fan_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_fan_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    FanModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarFanHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fan_handles_initial = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fan_handle : fan_handles_initial) {
      ASSERT_NE(nullptr, fan_handle);
    }

    count = 0;
    auto fan_handles_later = lzt::get_fan_handles(device, count);
    for (auto fan_handle : fan_handles_later) {
      ASSERT_NE(nullptr, fan_handle);
    }
    EXPECT_EQ(fan_handles_initial, fan_handles_later);
  }
}

TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenRetrievingFanPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto fan_handles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fan_handle : fan_handles) {
      ASSERT_NE(nullptr, fan_handle);
      auto properties = lzt::get_fan_properties(fan_handle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      if (properties.maxRPM == -1) {
        LOG_INFO << "maxRPM unsupported: ";
      }
      if (properties.maxRPM != -1) {
        EXPECT_LT(properties.maxRPM, INT32_MAX);
      }
      if (properties.maxPoints == -1) {
        LOG_INFO << "maxPoints unsupported: ";
      }
      if (properties.maxPoints != -1) {
        EXPECT_LT(properties.maxPoints, INT32_MAX);
      }
      EXPECT_GE(properties.supportedModes, 1);
      EXPECT_LE(properties.supportedModes, (1 << ZES_FAN_SPEED_MODE_TABLE));
      EXPECT_GE(properties.supportedUnits, 1);
      EXPECT_LE(properties.supportedUnits, (1 << ZES_FAN_SPEED_UNITS_PERCENT));
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenRetrievingFanPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fan_handles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fan_handle : fan_handles) {
      ASSERT_NE(nullptr, fan_handle);
      auto properties_initial = lzt::get_fan_properties(fan_handle);
      auto properties_later = lzt::get_fan_properties(fan_handle);
      EXPECT_EQ(properties_initial.canControl, properties_later.canControl);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      EXPECT_EQ(properties_initial.maxRPM, properties_later.maxRPM);
      EXPECT_EQ(properties_initial.maxPoints, properties_later.maxPoints);
      EXPECT_EQ(properties_initial.supportedModes,
                properties_later.supportedModes);
      EXPECT_EQ(properties_initial.supportedUnits,
                properties_later.supportedUnits);
    }
  }
}
TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenRetrievingFanConfigurationThenValidFanConfigurationIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fan_handles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fan_handle : fan_handles) {
      ASSERT_NE(nullptr, fan_handle);
      auto config = lzt::get_fan_configuration(fan_handle);
      EXPECT_EQ(config.mode, ZES_FAN_SPEED_MODE_DEFAULT);
      auto properties = lzt::get_fan_properties(fan_handle);
      if (config.speedFixed.speed == -1) {
        LOG_INFO << "No fixed fan speed setting ";
      }
      if (config.speedFixed.units == -1) {
        LOG_INFO << "Units should be ignored";
      }
      if (config.speedFixed.units == ZES_FAN_SPEED_UNITS_RPM) {
        EXPECT_LE(config.speedFixed.speed, properties.maxRPM);
      }
      EXPECT_GE(config.speedFixed.units, ZES_FAN_SPEED_UNITS_RPM);
      EXPECT_LE(config.speedFixed.units, ZES_FAN_SPEED_UNITS_PERCENT);
    }
  }
}
} // namespace

TEST_F(FanModuleTest,
       GivenValidFanHandleWhenRetrievingFanStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fan_handles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fan_handle : fan_handles) {
      ASSERT_NE(nullptr, fan_handle);
      zes_fan_speed_units_t units = {};
      int32_t speed = {};
      lzt::get_fan_state(fan_handle, units, speed);
      auto properties = lzt::get_fan_properties(fan_handle);
      auto config = lzt::get_fan_configuration(fan_handle);
      if (config.speedFixed.units == ZES_FAN_SPEED_UNITS_RPM) {
        EXPECT_LE(config.speedFixed.speed, properties.maxRPM);
      }
      EXPECT_GE(units, ZES_FAN_SPEED_UNITS_RPM);
      EXPECT_LE(units, ZES_FAN_SPEED_UNITS_PERCENT);
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenSettingFanToDefaultModeThenValidFanConfigurationIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fan_handles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fan_handle : fan_handles) {
      ASSERT_NE(nullptr, fan_handle);
      lzt::set_fan_default_mode(fan_handle);
      auto config = lzt::get_fan_configuration(fan_handle);
      EXPECT_EQ(config.mode, ZES_FAN_SPEED_MODE_DEFAULT);
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenSettingFanToFixedSpeedModeThenValidFanConfigurationIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fan_handles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fan_handle : fan_handles) {
      ASSERT_NE(nullptr, fan_handle);
      auto config = lzt::get_fan_configuration(fan_handle);
      zes_fan_speed_t speed;
      speed.speed = 50;
      speed.units = ZES_FAN_SPEED_UNITS_RPM;
      lzt::set_fan_fixed_speed_mode(fan_handle, speed);
      EXPECT_EQ(config.mode, ZES_FAN_SPEED_MODE_FIXED);
      EXPECT_EQ(config.speedFixed.speed, speed.speed);
      EXPECT_EQ(config.speedFixed.units, speed.units);
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenSettingFanToSpeedTableModeThenValidFanConfigurationIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fan_handles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fan_handle : fan_handles) {
      ASSERT_NE(nullptr, fan_handle);
      auto config = lzt::get_fan_configuration(fan_handle);
      zes_fan_speed_table_t speedTable;
      speedTable.numPoints = 1;
      speedTable.table->temperature = 140;
      speedTable.table->speed.speed = 50;
      speedTable.table->speed.units = ZES_FAN_SPEED_UNITS_RPM;
      lzt::set_fan_speed_table_mode(fan_handle, speedTable);
      auto properties = lzt::get_fan_properties(fan_handle);
      EXPECT_EQ(config.mode, ZES_FAN_SPEED_MODE_TABLE);
      EXPECT_LE(speedTable.numPoints, properties.maxPoints);
      for (int32_t i = 0; i < speedTable.numPoints; i++) {
        EXPECT_LT(speedTable.table[i].temperature, UINT32_MAX);
        EXPECT_GT(speedTable.table[i].temperature, 0);
        EXPECT_LE(speedTable.table[i].speed.speed, properties.maxRPM);
        EXPECT_GE(speedTable.table[i].speed.units, ZES_FAN_SPEED_UNITS_RPM);
        EXPECT_LE(speedTable.table[i].speed.units, ZES_FAN_SPEED_UNITS_PERCENT);
        EXPECT_EQ(config.speedTable.table->temperature,
                  speedTable.table->temperature);
        EXPECT_EQ(config.speedTable.table->speed.speed,
                  speedTable.table->speed.speed);
        EXPECT_EQ(config.speedTable.table->speed.units,
                  speedTable.table->speed.units);
      }
    }
  }
} // namespace
