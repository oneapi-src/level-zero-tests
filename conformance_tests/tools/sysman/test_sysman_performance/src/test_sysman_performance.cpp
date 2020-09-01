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

class PerformanceModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    PerformanceModuleTest,
    GivenComponentCountZeroWhenRetrievingPerformanceHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanPerformanceThenNotNullPerformanceHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performanceHandles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(performanceHandles.size(), count);
    for (auto performanceHandle : performanceHandles) {
      ASSERT_NE(nullptr, performanceHandle);
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenInvalidComponentCountWhenRetrievingPerformanceHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_performance_handles(device, actualCount);
    if (actualCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t testCount = actualCount + 1;
    lzt::get_performance_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarPerformanceHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performanceHandlesInitial =
        lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performanceHandle : performanceHandlesInitial) {
      ASSERT_NE(nullptr, performanceHandle);
    }

    count = 0;
    auto performanceHandlesLater = lzt::get_performance_handles(device, count);
    for (auto performanceHandle : performanceHandlesLater) {
      ASSERT_NE(nullptr, performanceHandle);
    }
    EXPECT_EQ(performanceHandlesInitial, performanceHandlesLater);
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidPerformanceHandleWhenRetrievingPerformancePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performanceHandles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performanceHandle : performanceHandles) {
      ASSERT_NE(nullptr, performanceHandle);
      auto properties = lzt::get_performance_properties(performanceHandle);
      if (properties.onSubdevice == true) {
        EXPECT_LT(properties.subdeviceId, UINT32_MAX);
      } else {
        EXPECT_EQ(0, properties.subdeviceId);
      }
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidPerformanceHandleWhenRetrievingPerformancePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performanceHandles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performanceHandle : performanceHandles) {
      ASSERT_NE(nullptr, performanceHandle);
      auto propertiesInitial =
          lzt::get_performance_properties(performanceHandle);
      auto propertiesLater = lzt::get_performance_properties(performanceHandle);

      if (propertiesInitial.onSubdevice == true &&
          propertiesLater.onSubdevice == true) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      EXPECT_EQ(propertiesInitial.engines, propertiesLater.engines);
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidPerformanceHandleWhenSettingPerformanceConfigurationThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performanceHandles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performanceHandle : performanceHandles) {
      ASSERT_NE(nullptr, performanceHandle);
      double factor = 50;
      lzt::set_performance_config(performanceHandle, factor);
      auto getFactor = lzt::get_performance_config(performanceHandle);
      EXPECT_GT(getFactor, 0);
      EXPECT_LT(getFactor, 100);
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidPerformanceHandleWhenGettingPerformanceConfigurationThenValidPerformanceFactorIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performanceHandles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performanceHandle : performanceHandles) {
      ASSERT_NE(nullptr, performanceHandle);
      double factor = 40;
      lzt::set_performance_config(performanceHandle, factor);
      auto getFactor = lzt::get_performance_config(performanceHandle);
      EXPECT_EQ(factor, getFactor);
    }
  }
}
} // namespace
