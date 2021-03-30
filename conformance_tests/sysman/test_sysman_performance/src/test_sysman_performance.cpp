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
    auto performance_handles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(performance_handles.size(), count);
    for (auto performance_handle : performance_handles) {
      ASSERT_NE(nullptr, performance_handle);
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenInvalidComponentCountWhenRetrievingPerformanceHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_performance_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_performance_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarPerformanceHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performance_handles_initial =
        lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performance_handle : performance_handles_initial) {
      ASSERT_NE(nullptr, performance_handle);
    }

    count = 0;
    auto performance_handles_later =
        lzt::get_performance_handles(device, count);
    for (auto performance_handle : performance_handles_later) {
      ASSERT_NE(nullptr, performance_handle);
    }
    EXPECT_EQ(performance_handles_initial, performance_handles_later);
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidPerformanceHandleWhenRetrievingPerformancePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto performance_handles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performance_handle : performance_handles) {
      ASSERT_NE(nullptr, performance_handle);
      auto properties = lzt::get_performance_properties(performance_handle);

      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      EXPECT_GE(properties.engines, 0);
      EXPECT_LE(properties.engines,
                ZES_ENGINE_TYPE_FLAG_OTHER | ZES_ENGINE_TYPE_FLAG_COMPUTE |
                    ZES_ENGINE_TYPE_FLAG_3D | ZES_ENGINE_TYPE_FLAG_MEDIA |
                    ZES_ENGINE_TYPE_FLAG_DMA);
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidPerformanceHandleWhenRetrievingPerformancePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performance_handles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performance_handle : performance_handles) {
      ASSERT_NE(nullptr, performance_handle);
      auto properties_initial =
          lzt::get_performance_properties(performance_handle);
      auto properties_later =
          lzt::get_performance_properties(performance_handle);

      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      EXPECT_EQ(properties_initial.engines, properties_later.engines);
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidPerformanceHandleWhenSettingPerformanceConfigurationThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performance_handles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performance_handle : performance_handles) {
      ASSERT_NE(nullptr, performance_handle);
      auto initialFactor = lzt::get_performance_config(performance_handle);
      double factor =
          50; // Checking with a random factor value within its range
      lzt::set_performance_config(performance_handle, factor);
      auto getFactor = lzt::get_performance_config(performance_handle);
      EXPECT_GT(getFactor, 0);
      EXPECT_LT(getFactor, 100);
      lzt::set_performance_config(performance_handle, initialFactor);
    }
  }
}

TEST_F(
    PerformanceModuleTest,
    GivenValidPerformanceHandleWhenGettingPerformanceConfigurationThenValidPerformanceFactorIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto performance_handles = lzt::get_performance_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto performance_handle : performance_handles) {
      ASSERT_NE(nullptr, performance_handle);
      auto getFactor = lzt::get_performance_config(performance_handle);
      EXPECT_GT(getFactor, 0);
      EXPECT_LT(getFactor, 100);
    }
  }
}
} // namespace
