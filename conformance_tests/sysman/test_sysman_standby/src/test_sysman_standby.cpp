/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
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
class StandbyModuleTest : public lzt::SysmanCtsClass {};
TEST_F(
    StandbyModuleTest,
    GivenValidDeviceWhenRetrievingStandbyHandlesThenNonZeroCountAndValidStandbyHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pStandbyHandles = lzt::get_standby_handles(device, count);
    for (auto pStandbyHandle : pStandbyHandles) {
      EXPECT_NE(nullptr, pStandbyHandle);
    }
  }
}
TEST_F(
    StandbyModuleTest,
    GivenValidDeviceWhenRetrievingStandbyHandlesThenSimilarHandlesAreReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pStandbyHandlesInitial = lzt::get_standby_handles(device, count);
    count = 0;
    auto pStandbyHandlesLater = lzt::get_standby_handles(device, count);
    EXPECT_EQ(pStandbyHandlesInitial, pStandbyHandlesLater);
  }
}
TEST_F(
    StandbyModuleTest,
    GivenValidDeviceWhenRetrievingStandbyHandlesThenActualHandleCountIsUpdated) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    lzt::get_standby_handles(device, pCount);
    uint32_t tCount = pCount + 1;
    lzt::get_standby_handles(device, tCount);
    EXPECT_EQ(tCount, pCount);
  }
}
TEST_F(
    StandbyModuleTest,
    GivenValidDeviceWhenRequestingModeThenExpectzesSysmanStandbyGetModeToReturnValidStandbyMode) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pHandles = lzt::get_standby_handles(device, count);
    for (auto pHandle : pHandles) {
      EXPECT_NE(nullptr, pHandle);
      zes_standby_promo_mode_t standByMode = {};
      standByMode = lzt::get_standby_mode(pHandle);
      switch (standByMode) {
      case ZES_STANDBY_PROMO_MODE_DEFAULT:
        SUCCEED();
        break;
      case ZES_STANDBY_PROMO_MODE_NEVER:
        SUCCEED();
        break;
      default:
        FAIL();
      }
    }
  }
}
TEST_F(
    StandbyModuleTest,
    GivenValidDeviceWhenSettingModeThenExpectzesSysmanStandbySetModeFollowedByzesSysmanStandbyGetModeToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pHandles = lzt::get_standby_handles(device, count);
    for (auto pHandle : pHandles) {
      EXPECT_NE(nullptr, pHandle);
      zes_standby_promo_mode_t standByMode = {};
      zes_standby_promo_mode_t standByMode1 = {};
      standByMode = ZES_STANDBY_PROMO_MODE_DEFAULT;
      lzt::set_standby_mode(pHandle, standByMode);
      standByMode1 = lzt::get_standby_mode(pHandle);
      EXPECT_EQ(standByMode, standByMode1);
      standByMode = ZES_STANDBY_PROMO_MODE_NEVER;
      lzt::set_standby_mode(pHandle, standByMode);
      standByMode1 = lzt::get_standby_mode(pHandle);
      EXPECT_EQ(standByMode, standByMode1);
    }
  }
}

TEST_F(
    StandbyModuleTest,
    GivenValidStandbyHandleWhenRetrievingStandbyPropertiesThenValidStandbyPropertiesIsReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto pStandbyHandles = lzt::get_standby_handles(device, count);
    for (auto pStandbyHandle : pStandbyHandles) {
      EXPECT_NE(nullptr, pStandbyHandle);
      auto properties = lzt::get_standby_properties(pStandbyHandle);
      EXPECT_EQ(properties.type, ZES_STANDBY_TYPE_GLOBAL);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
    }
  }
}
TEST_F(
    StandbyModuleTest,
    GivenValidStandbyHandleWhenRetrievingStandbyPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pStandbyHandles = lzt::get_standby_handles(device, count);
    for (auto pStandbyHandle : pStandbyHandles) {
      EXPECT_NE(nullptr, pStandbyHandle);
      auto propertiesInitial = lzt::get_standby_properties(pStandbyHandle);
      auto propertiesLater = lzt::get_standby_properties(pStandbyHandle);
      ASSERT_EQ(propertiesInitial.type, ZES_STANDBY_TYPE_GLOBAL);
      EXPECT_EQ(propertiesInitial.type, propertiesLater.type);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice && propertiesLater.onSubdevice)
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
    }
  }
}
} // namespace
