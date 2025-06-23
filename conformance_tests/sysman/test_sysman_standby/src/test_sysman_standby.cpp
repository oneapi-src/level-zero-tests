/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
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

#ifdef USE_ZESINIT
class StandbyModuleZesTest : public lzt::ZesSysmanCtsClass {};
#define STANDBY_TEST StandbyModuleZesTest
#else // USE_ZESINIT
class StandbyModuleTest : public lzt::SysmanCtsClass {};
#define STANDBY_TEST StandbyModuleTest
#endif // USE_ZESINIT

LZT_TEST_F(
    STANDBY_TEST,
    GivenValidDeviceWhenRetrievingStandbyHandlesThenNonZeroCountAndValidStandbyHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_standby_handles = lzt::get_standby_handles(device, count);
    for (auto p_standby_handle : p_standby_handles) {
      EXPECT_NE(nullptr, p_standby_handle);
    }
  }
}
LZT_TEST_F(
    STANDBY_TEST,
    GivenValidDeviceWhenRetrievingStandbyHandlesThenSimilarHandlesAreReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_standby_handles_initial = lzt::get_standby_handles(device, count);
    count = 0;
    auto p_standby_handles_later = lzt::get_standby_handles(device, count);
    EXPECT_EQ(p_standby_handles_initial, p_standby_handles_later);
  }
}
LZT_TEST_F(
    STANDBY_TEST,
    GivenValidDeviceWhenRetrievingStandbyHandlesThenActualHandleCountIsUpdated) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    lzt::get_standby_handles(device, p_count);
    uint32_t tCount = p_count + 1;
    lzt::get_standby_handles(device, tCount);
    EXPECT_EQ(tCount, p_count);
  }
}
LZT_TEST_F(
    STANDBY_TEST,
    GivenValidDeviceWhenRequestingModeThenExpectzesSysmanStandbyGetModeToReturnValidStandbyMode) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_handles = lzt::get_standby_handles(device, count);
    for (auto p_handle : p_handles) {
      EXPECT_NE(nullptr, p_handle);
      zes_standby_promo_mode_t standByMode = {};
      standByMode = lzt::get_standby_mode(p_handle);
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
LZT_TEST_F(
    STANDBY_TEST,
    GivenValidDeviceWhenSettingModeThenExpectzesSysmanStandbySetModeFollowedByzesSysmanStandbyGetModeToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_handles = lzt::get_standby_handles(device, count);
    for (auto p_handle : p_handles) {
      EXPECT_NE(nullptr, p_handle);
      zes_standby_promo_mode_t standByMode = {};
      zes_standby_promo_mode_t standByMode1 = {};
      standByMode = ZES_STANDBY_PROMO_MODE_DEFAULT;
      lzt::set_standby_mode(p_handle, standByMode);
      standByMode1 = lzt::get_standby_mode(p_handle);
      EXPECT_EQ(standByMode, standByMode1);
      standByMode = ZES_STANDBY_PROMO_MODE_NEVER;
      lzt::set_standby_mode(p_handle, standByMode);
      standByMode1 = lzt::get_standby_mode(p_handle);
      EXPECT_EQ(standByMode, standByMode1);
    }
  }
}

LZT_TEST_F(
    STANDBY_TEST,
    GivenValidStandbyHandleWhenRetrievingStandbyPropertiesThenValidStandbyPropertiesIsReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto p_standby_handles = lzt::get_standby_handles(device, count);
    for (auto p_standby_handle : p_standby_handles) {
      EXPECT_NE(nullptr, p_standby_handle);
      auto properties = lzt::get_standby_properties(p_standby_handle);
      EXPECT_EQ(properties.type, ZES_STANDBY_TYPE_GLOBAL);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
    }
  }
}
LZT_TEST_F(
    STANDBY_TEST,
    GivenValidStandbyHandleWhenRetrievingStandbyPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_standby_handles = lzt::get_standby_handles(device, count);
    for (auto p_standby_handle : p_standby_handles) {
      EXPECT_NE(nullptr, p_standby_handle);
      auto properties_initial = lzt::get_standby_properties(p_standby_handle);
      auto properties_later = lzt::get_standby_properties(p_standby_handle);
      ASSERT_EQ(properties_initial.type, ZES_STANDBY_TYPE_GLOBAL);
      EXPECT_EQ(properties_initial.type, properties_later.type);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice)
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
    }
  }
}
} // namespace
