/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <cmath>
#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;
#include <level_zero/zes_api.h>

namespace {

#ifdef USE_ZESINIT
class PerformanceModuleZesTest : public lzt::ZesSysmanCtsClass {
public:
  bool is_performance_supported = false;
};
#define PERFORMANCE_TEST PerformanceModuleZesTest
#else // USE_ZESINIT
class PerformanceModuleTest : public lzt::SysmanCtsClass {
public:
  bool is_performance_supported = false;
};
#define PERFORMANCE_TEST PerformanceModuleTest
#endif // USE_ZESINIT

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenComponentCountZeroWhenRetrievingPerformanceHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanPerformanceThenNotNullPerformanceHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      auto performance_handles = lzt::get_performance_handles(device, count);
      ASSERT_EQ(performance_handles.size(), count);
      for (auto performance_handle : performance_handles) {
        ASSERT_NE(nullptr, performance_handle);
      }
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenInvalidComponentCountWhenRetrievingPerformanceHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    actual_count = lzt::get_performance_handle_count(device);
    if (actual_count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      lzt::get_performance_handles(device, actual_count);
      uint32_t test_count = actual_count + 1;
      lzt::get_performance_handles(device, test_count);
      EXPECT_EQ(test_count, actual_count);
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarPerformanceHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      auto performance_handles_initial =
          lzt::get_performance_handles(device, count);
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
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenValidPerformanceHandleWhenRetrievingPerformancePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      auto performance_handles = lzt::get_performance_handles(device, count);
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
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenValidPerformanceHandleWhenRetrievingPerformancePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      auto performance_handles = lzt::get_performance_handles(device, count);
      for (auto performance_handle : performance_handles) {
        ASSERT_NE(nullptr, performance_handle);
        auto properties_initial =
            lzt::get_performance_properties(performance_handle);
        auto properties_later =
            lzt::get_performance_properties(performance_handle);
        if (properties_initial.onSubdevice && properties_later.onSubdevice) {
          EXPECT_EQ(properties_initial.subdeviceId,
                    properties_later.subdeviceId);
        }
        EXPECT_EQ(properties_initial.engines, properties_later.engines);
      }
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenValidPerformanceHandleWhenSettingPerformanceConfigurationThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      auto performance_handles = lzt::get_performance_handles(device, count);
      for (auto performance_handle : performance_handles) {
        ASSERT_NE(nullptr, performance_handle);
        auto initialFactor = lzt::get_performance_config(performance_handle);
        double factor = 50; // Check with a random factor value within its range
        lzt::set_performance_config(performance_handle, factor);
        auto getFactor = lzt::get_performance_config(performance_handle);
        EXPECT_GE(getFactor, 0);
        EXPECT_LE(getFactor, 100);
        lzt::set_performance_config(performance_handle, initialFactor);
      }
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

static double set_performance_factor(zes_perf_handle_t pHandle,
                                     double pFactor) {
  lzt::set_performance_config(pHandle, pFactor);
  double getFactor = lzt::get_performance_config(pHandle);
  return getFactor;
}

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenValidPerformanceHandleWhenSettingMultiplePerformanceConfigurationsForMediaThenValidPerformanceFactorIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      auto performance_handles = lzt::get_performance_handles(device, count);
      for (auto performance_handle : performance_handles) {
        ASSERT_NE(nullptr, performance_handle);
        zes_perf_properties_t perfProperties = {};
        perfProperties = lzt::get_performance_properties(performance_handle);
        if (perfProperties.engines & ZES_ENGINE_TYPE_FLAG_MEDIA) {
          auto initialFactor = lzt::get_performance_config(performance_handle);
          // Verify that performance factor is equal to 50 if value set is 1-50
          // and equal to 100 if value set is 51-100
          // value 25
          double pFactor = set_performance_factor(performance_handle, 25);
          EXPECT_EQ(pFactor, 50);
          // value 49
          pFactor = set_performance_factor(performance_handle, 49);
          EXPECT_EQ(pFactor, 50);
          // value 50
          pFactor = set_performance_factor(performance_handle, 50);
          EXPECT_EQ(pFactor, 50);
          // value 51
          pFactor = set_performance_factor(performance_handle, 51);
          EXPECT_EQ(pFactor, 100);
          // value 75
          pFactor = set_performance_factor(performance_handle, 75);
          EXPECT_EQ(pFactor, 100);
          // value 99
          pFactor = set_performance_factor(performance_handle, 99);
          EXPECT_EQ(pFactor, 100);
          // value 100
          pFactor = set_performance_factor(performance_handle, 100);
          EXPECT_EQ(pFactor, 100);
          lzt::set_performance_config(performance_handle, initialFactor);
        }
      }
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    PERFORMANCE_TEST,
    GivenValidPerformanceHandleWhenGettingPerformanceConfigurationThenValidPerformanceFactorIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      auto performance_handles = lzt::get_performance_handles(device, count);
      for (auto performance_handle : performance_handles) {
        ASSERT_NE(nullptr, performance_handle);
        auto getFactor = lzt::get_performance_config(performance_handle);
        EXPECT_GE(getFactor, 0);
        EXPECT_LE(getFactor, 100);
      }
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

#ifdef USE_ZESINIT
class PerformanceModuleParamComputePerformanceFactorZesTest
    : public lzt::ZesSysmanCtsClass,
      public ::testing::WithParamInterface<int> {
public:
  bool is_performance_supported = false;
};
#define PERFORMANCE_COMPUTE_TEST                                               \
  PerformanceModuleParamComputePerformanceFactorZesTest
#else // USE_ZESINIT
class PerformanceModuleParamComputePerformanceFactorTest
    : public lzt::SysmanCtsClass,
      public ::testing::WithParamInterface<int> {
public:
  bool is_performance_supported = false;
};
#define PERFORMANCE_COMPUTE_TEST                                               \
  PerformanceModuleParamComputePerformanceFactorTest
#endif // USE_ZESINIT

LZT_TEST_P(
    PERFORMANCE_COMPUTE_TEST,
    GivenValidPerformanceHandleWhenSettingMultiplePerformanceFactorForComputeThenValidPerformanceFactorIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_performance_handle_count(device);
    if (count > 0) {
      is_performance_supported = true;
      LOG_INFO << "Performance handles are available on this device! ";
      auto performance_handles = lzt::get_performance_handles(device, count);
      for (auto performance_handle : performance_handles) {
        ASSERT_NE(nullptr, performance_handle);
        zes_perf_properties_t perfProperties = {};
        perfProperties = lzt::get_performance_properties(performance_handle);
        if (perfProperties.engines & ZES_ENGINE_TYPE_FLAG_COMPUTE) {
          auto initialFactor = lzt::get_performance_config(performance_handle);
          double input_pfactor = static_cast<double>(GetParam());
          double pFactor =
              set_performance_factor(performance_handle, input_pfactor);
          EXPECT_EQ(std::round(pFactor), input_pfactor);
          lzt::set_performance_config(performance_handle, initialFactor);
        }
      }
    } else {
      LOG_INFO << "No performance handles found for this device! ";
    }
  }
  if (!is_performance_supported) {
    FAIL() << "No performance handles found on any of the devices! ";
  }
}

INSTANTIATE_TEST_CASE_P(TestCasesforComputePerformanceFactorVerification,
                        PERFORMANCE_COMPUTE_TEST,
                        testing::Values(0, 25, 49, 50, 51, 75, 99, 100));

} // namespace
