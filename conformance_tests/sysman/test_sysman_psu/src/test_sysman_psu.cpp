/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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
class PsuModuleZesTest : public lzt::ZesSysmanCtsClass {};
#define PSU_TEST PsuModuleZesTest
#else // USE_ZESINIT
class PsuModuleTest : public lzt::SysmanCtsClass {};
#define PSU_TEST PsuModuleTest
#endif // USE_ZESINIT

LZT_TEST_F(
    PSU_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    lzt::get_psu_handles_count(device);
  }
}

LZT_TEST_F(
    PSU_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullPsuHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto psu_handles = lzt::get_psu_handles(device, count);
    ASSERT_EQ(psu_handles.size(), count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psu_handle : psu_handles) {
      ASSERT_NE(nullptr, psu_handle);
    }
  }
}

LZT_TEST_F(
    PSU_TEST,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_psu_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_psu_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

LZT_TEST_F(
    PSU_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarMemHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto psu_handles_initial = lzt::get_psu_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psu_handle : psu_handles_initial) {
      ASSERT_NE(nullptr, psu_handle);
    }

    count = 0;
    auto psu_handles_later = lzt::get_psu_handles(device, count);
    for (auto psu_handle : psu_handles_later) {
      ASSERT_NE(nullptr, psu_handle);
    }

    EXPECT_EQ(psu_handles_initial, psu_handles_later);
  }
}
LZT_TEST_F(
    PSU_TEST,
    GivenValidPsuHandleWhenRetrievingPsuPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto psu_handles = lzt::get_psu_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psu_handle : psu_handles) {
      ASSERT_NE(nullptr, psu_handle);
      auto properties = lzt::get_psu_properties(psu_handle);
      EXPECT_LE(properties.ampLimit, UINT32_MAX);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
    }
  }
}

LZT_TEST_F(
    PSU_TEST,
    GivenValidPsuHandleWhenRetrievingPsuPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto psu_handles = lzt::get_psu_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psu_handle : psu_handles) {
      ASSERT_NE(nullptr, psu_handle);
      auto properties_initial = lzt::get_psu_properties(psu_handle);
      auto properties_later = lzt::get_psu_properties(psu_handle);
      EXPECT_EQ(properties_initial.haveFan, properties_later.haveFan);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);

      EXPECT_EQ(properties_initial.ampLimit, properties_later.ampLimit);
    }
  }
}
LZT_TEST_F(PSU_TEST,
           GivenValidPsuHandleWhenRetrievingPsuStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto psu_handles = lzt::get_psu_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto psu_handle : psu_handles) {
      ASSERT_NE(nullptr, psu_handle);
      auto state = lzt::get_psu_state(psu_handle);
      EXPECT_GE(state.voltStatus, ZES_PSU_VOLTAGE_STATUS_UNKNOWN);
      EXPECT_LE(state.voltStatus, ZES_PSU_VOLTAGE_STATUS_UNDER);
      EXPECT_LE(state.temperature, UINT32_MAX);
      EXPECT_LE(state.current, UINT32_MAX);
      auto properties = lzt::get_psu_properties(psu_handle);
      EXPECT_LE(state.current, properties.ampLimit);
    }
  }
}
} // namespace
