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

class PsuModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    PsuModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    lzt::get_psu_handles_count(device);
  }
}

TEST_F(
    PsuModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullPsuHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto psuHandles = lzt::get_psu_handles(device, count);
    ASSERT_EQ(psuHandles.size(), count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psuHandle : psuHandles) {
      ASSERT_NE(nullptr, psuHandle);
    }
  }
}

TEST_F(
    PsuModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_psu_handles(device, actualCount);
    if (actualCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t testCount = actualCount + 1;
    lzt::get_psu_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    PsuModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarMemHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto psuHandlesInitial = lzt::get_psu_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psuHandle : psuHandlesInitial) {
      ASSERT_NE(nullptr, psuHandle);
    }

    count = 0;
    auto psuHandlesLater = lzt::get_psu_handles(device, count);
    for (auto psuHandle : psuHandlesLater) {
      ASSERT_NE(nullptr, psuHandle);
    }

    EXPECT_EQ(psuHandlesInitial, psuHandlesLater);
  }
}
TEST_F(
    PsuModuleTest,
    GivenValidPsuHandleWhenRetrievingPsuPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto psuHandles = lzt::get_psu_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psuHandle : psuHandles) {
      ASSERT_NE(nullptr, psuHandle);
      auto properties = lzt::get_psu_properties(psuHandle);
      EXPECT_LE(properties.ampLimit, UINT32_MAX);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
    }
  }
}

TEST_F(
    PsuModuleTest,
    GivenValidPsuHandleWhenRetrievingPsuPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto psuHandles = lzt::get_psu_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psuHandle : psuHandles) {
      ASSERT_NE(nullptr, psuHandle);
      auto propertiesInitial = lzt::get_psu_properties(psuHandle);
      auto propertiesLater = lzt::get_psu_properties(psuHandle);
      EXPECT_EQ(propertiesInitial.haveFan, propertiesLater.haveFan);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);

      EXPECT_EQ(propertiesInitial.ampLimit, propertiesLater.ampLimit);
    }
  }
}
TEST_F(PsuModuleTest,
       GivenValidPsuHandleWhenRetrievingPsuStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto psuHandles = lzt::get_psu_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psuHandle : psuHandles) {
      ASSERT_NE(nullptr, psuHandle);
      auto state = lzt::get_psu_state(psuHandle);
      EXPECT_GE(state.voltStatus, ZES_PSU_VOLTAGE_STATUS_NORMAL);
      EXPECT_LE(state.voltStatus, ZES_PSU_VOLTAGE_STATUS_UNDER);
      EXPECT_LE(state.temperature, UINT32_MAX);
      EXPECT_LE(state.current, UINT32_MAX);
      auto properties = lzt::get_psu_properties(psuHandle);
      EXPECT_LE(state.current, properties.ampLimit);
    }
  }
}
} // namespace
