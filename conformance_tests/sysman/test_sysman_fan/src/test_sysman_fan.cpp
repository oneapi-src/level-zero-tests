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
    lzt::get_fan_handle_count(device);
  }
}

TEST_F(
    FanModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFanHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fanHandles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(fanHandles.size(), count);
    for (auto fanHandle : fanHandles) {
      ASSERT_NE(nullptr, fanHandle);
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_fan_handles(device, actualCount);
    if (actualCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t testCount = actualCount + 1;
    lzt::get_fan_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    FanModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarFanHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fanHandlesInitial = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fanHandle : fanHandlesInitial) {
      ASSERT_NE(nullptr, fanHandle);
    }

    count = 0;
    auto fanHandlesLater = lzt::get_fan_handles(device, count);
    for (auto fanHandle : fanHandlesLater) {
      ASSERT_NE(nullptr, fanHandle);
    }
    EXPECT_EQ(fanHandlesInitial, fanHandlesLater);
  }
}

TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenRetrievingFanPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto fanHandles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fanHandle : fanHandles) {
      ASSERT_NE(nullptr, fanHandle);
      auto properties = lzt::get_fan_properties(fanHandle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      EXPECT_LT(properties.maxSpeed, UINT32_MAX);
      EXPECT_GT(properties.maxSpeed, 0);
      EXPECT_LT(properties.maxPoints, UINT32_MAX);
      EXPECT_GT(properties.maxPoints, 0);
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenRetrievingFanPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fanHandles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fanHandle : fanHandles) {
      ASSERT_NE(nullptr, fanHandle);
      auto propertiesInitial = lzt::get_fan_properties(fanHandle);
      auto propertiesLater = lzt::get_fan_properties(fanHandle);
      EXPECT_EQ(propertiesInitial.canControl, propertiesLater.canControl);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      EXPECT_EQ(propertiesInitial.maxSpeed, propertiesLater.maxSpeed);
      EXPECT_EQ(propertiesInitial.maxPoints, propertiesLater.maxPoints);
    }
  }
}
TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenRetrievingFanConfigurationThenValidFanConfigurationIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fanHandles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fanHandle : fanHandles) {
      ASSERT_NE(nullptr, fanHandle);
      auto config = lzt::get_fan_configuration(fanHandle);
      EXPECT_GE(config.mode, ZET_FAN_SPEED_MODE_DEFAULT);
      EXPECT_LE(config.mode, ZET_FAN_SPEED_MODE_TABLE);
      auto properties = lzt::get_fan_properties(fanHandle);
      EXPECT_LE(config.speed, properties.maxSpeed);
      EXPECT_GE(config.speedUnits, ZET_FAN_SPEED_UNITS_RPM);
      EXPECT_LE(config.speedUnits, ZET_FAN_SPEED_UNITS_PERCENT);
      EXPECT_LE(config.numPoints, properties.maxPoints);
      for (uint32_t i = 0; i < config.numPoints; i++) {
        EXPECT_LT(config.table[i].temperature, UINT32_MAX);
        EXPECT_GT(config.table[i].temperature, 0);
        EXPECT_LE(config.table[i].speed, properties.maxSpeed);
        EXPECT_GE(config.table[i].units, ZET_FAN_SPEED_UNITS_RPM);
        EXPECT_LE(config.table[i].units, ZET_FAN_SPEED_UNITS_PERCENT);
      }
    }
  }
}

TEST_F(
    FanModuleTest,
    GivenValidFanHandleWhenSettingFanConfigurationThenExpectzetSysmanFanSetConfigFollowedByzetSysmanFanGetConfigToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fanHandles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fanHandle : fanHandles) {
      ASSERT_NE(nullptr, fanHandle);
      auto properties = lzt::get_fan_properties(fanHandle);
      auto initial_config = lzt::get_fan_configuration(fanHandle);
      zet_fan_config_t set_config;
      set_config.mode = ZET_FAN_SPEED_MODE_DEFAULT;
      set_config.speed = properties.maxSpeed;
      set_config.speedUnits = ZET_FAN_SPEED_UNITS_RPM;
      set_config.numPoints = properties.maxPoints;
      for (uint32_t i = 0; i < set_config.numPoints; i++) {
        set_config.table[i].temperature = initial_config.table[i].temperature;
        set_config.table[i].speed = initial_config.table[i].speed;
        set_config.table[i].units = initial_config.table[i].units;
      }
      lzt::set_fan_configuration(fanHandle, set_config);
      auto get_config = lzt::get_fan_configuration(fanHandle);
      EXPECT_EQ(get_config.mode, set_config.mode);
      EXPECT_EQ(get_config.speed, set_config.speed);
      EXPECT_EQ(get_config.speedUnits, set_config.speedUnits);
      EXPECT_EQ(get_config.numPoints, set_config.numPoints);
      for (uint32_t i = 0; i < get_config.numPoints; i++) {
        get_config.table[i].temperature = set_config.table[i].temperature;
        get_config.table[i].speed = set_config.table[i].speed;
        get_config.table[i].units = set_config.table[i].units;
      }
      lzt::set_fan_configuration(fanHandle, initial_config);
    }
  }
}

TEST_F(FanModuleTest,
       GivenValidFanHandleWhenRetrievingFanStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fanHandles = lzt::get_fan_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fanHandle : fanHandles) {
      ASSERT_NE(nullptr, fanHandle);
      zet_fan_speed_units_t units = ZET_FAN_SPEED_UNITS_RPM;
      uint32_t speed;
      lzt::get_fan_state(fanHandle, units, speed);
      auto properties = lzt::get_fan_properties(fanHandle);
      EXPECT_LE(speed, properties.maxSpeed);
      EXPECT_GE(units, ZET_FAN_SPEED_UNITS_RPM);
      EXPECT_LE(units, ZET_FAN_SPEED_UNITS_PERCENT);
    }
  }
}
} // namespace
