/*
 *
 * Copyright (C) 2019 Intel Corporation
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
#include <level_zero/zet_api.h>

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
    GivenValidDeviceWhenRequestingModeThenExpectzetSysmanStandbyGetModeToReturnValidStandbyMode) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pHandles = lzt::get_standby_handles(device, count);
    for (auto pHandle : pHandles) {
      EXPECT_NE(nullptr, pHandle);
      zet_standby_promo_mode_t standByMode;
      standByMode = lzt::get_standby_mode(pHandle);
      switch (standByMode) {
      case ZET_STANDBY_PROMO_MODE_DEFAULT:
        SUCCEED();
        break;
      case ZET_STANDBY_PROMO_MODE_NEVER:
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
    GivenValidDeviceWhenSettingModeThenExpectzetSysmanStandbySetModeFollowedByzetSysmanStandbyGetModeToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pHandles = lzt::get_standby_handles(device, count);
    for (auto pHandle : pHandles) {
      EXPECT_NE(nullptr, pHandle);
      zet_standby_promo_mode_t standByMode;
      zet_standby_promo_mode_t standByMode1;
      standByMode = ZET_STANDBY_PROMO_MODE_DEFAULT;
      lzt::set_standby_mode(pHandle, standByMode);
      standByMode1 = lzt::get_standby_mode(pHandle);
      EXPECT_EQ(standByMode, standByMode1);
      standByMode = ZET_STANDBY_PROMO_MODE_NEVER;
      lzt::set_standby_mode(pHandle, standByMode);
      standByMode1 = lzt::get_standby_mode(pHandle);
      EXPECT_EQ(standByMode, standByMode1);
    }
  }
}

TEST_F(
    StandbyModuleTest,
    GivenValidStandbyHandleWhenRetrievingStandbyPropertiesThenValidStandByPolicyIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pStandbyHandles = lzt::get_standby_handles(device, count);
    for (auto pStandbyHandle : pStandbyHandles) {
      EXPECT_NE(nullptr, pStandbyHandle);
      auto properties = lzt::get_standby_properties(pStandbyHandle);
      EXPECT_EQ(properties.type, ZET_STANDBY_TYPE_GLOBAL);
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
      ASSERT_EQ(propertiesInitial.type, ZET_STANDBY_TYPE_GLOBAL);
      EXPECT_EQ(propertiesInitial.type, propertiesLater.type);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice == true &&
          propertiesLater.onSubdevice == true)
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
    }
  }
}
} // namespace
