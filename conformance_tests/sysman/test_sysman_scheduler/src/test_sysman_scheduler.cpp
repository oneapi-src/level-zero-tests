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
#include <algorithm>
namespace {

class SchedulerTest : public lzt::SysmanCtsClass {};
TEST_F(
    SchedulerTest,
    GivenComponentCountZeroWhenRetrievingSchedulerHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_scheduler_handle_count(device);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    SchedulerTest,
    GivenComponentCountZeroWhenRetrievingSchedulerHandlesThenNotNullSchedulerHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
    }
  }
}

TEST_F(
    SchedulerTest,
    GivenComponentCountWhenRetrievingSchedulerHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t testCount = pCount + 1;
    lzt::get_scheduler_handles(device, testCount);
    EXPECT_EQ(testCount, pCount);
  }
}

TEST_F(
    SchedulerTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarSchedulerHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pSchedHandlesInitial = lzt::get_scheduler_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandlesInitial) {
      EXPECT_NE(nullptr, pSchedHandle);
    }
    count = 0;
    auto pSchedHandlesLater = lzt::get_scheduler_handles(device, count);
    for (auto pSchedHandle : pSchedHandlesLater) {
      EXPECT_NE(nullptr, pSchedHandle);
    }
    EXPECT_EQ(pSchedHandlesInitial, pSchedHandlesLater);
  }
}

TEST_F(SchedulerTest,
       GivenValidSchedulerHandleWhenRetrievinCurrentModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      ASSERT_GE(cur_mode, ZES_SCHED_MODE_TIMEOUT);
      ASSERT_LE(cur_mode, ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG);
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenRetrievingCurrentModeTwiceThenSameModeIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto cur_mode_initial = lzt::get_scheduler_current_mode(pSchedHandle);
      auto cur_mode_later = lzt::get_scheduler_current_mode(pSchedHandle);
      EXPECT_EQ(cur_mode_initial, cur_mode_later);
    }
  }
}

TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenRetrievingSchedulerTimeOutPropertiesThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      if (cur_mode == ZES_SCHED_MODE_TIMEOUT) {
        auto timeout_properties =
            lzt::get_timeout_properties(pSchedHandle, false);
        EXPECT_GT(timeout_properties.watchdogTimeout, 0);
      }
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenRetrievingSchedulerTimeOutPropertiesTwiceThenSamePropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      if (cur_mode == ZES_SCHED_MODE_TIMEOUT) {
        auto timeout_properties_initial =
            lzt::get_timeout_properties(pSchedHandle, false);
        auto timeout_properties_later =
            lzt::get_timeout_properties(pSchedHandle, false);
        EXPECT_EQ(timeout_properties_initial.watchdogTimeout,
                  timeout_properties_later.watchdogTimeout);
      }
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenRetrievingSchedulerTimeSlicePropertiesThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      if (cur_mode == ZES_SCHED_MODE_TIMESLICE) {
        auto timeslice_properties =
            lzt::get_timeslice_properties(pSchedHandle, false);
        EXPECT_GT(timeslice_properties.interval, 0);
        EXPECT_GE(timeslice_properties.yieldTimeout, 0);
      }
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenRetrievingSchedulerTimeSlicePropertiesTwiceThenSamePropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      if (cur_mode == ZES_SCHED_MODE_TIMESLICE) {
        auto timeslice_properties_initial =
            lzt::get_timeslice_properties(pSchedHandle, false);
        auto timeslice_properties_later =
            lzt::get_timeslice_properties(pSchedHandle, false);
        EXPECT_EQ(timeslice_properties_initial.interval,
                  timeslice_properties_later.interval);
        EXPECT_EQ(timeslice_properties_initial.yieldTimeout,
                  timeslice_properties_later.yieldTimeout);
      }
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenSettingSchedulerTimeOutModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto timeout_default_properties =
          lzt::get_timeout_properties(pSchedHandle, true);
      zes_sched_timeout_properties_t timeout_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMEOUT_PROPERTIES, nullptr};
      timeout_set_properties.watchdogTimeout =
          timeout_default_properties.watchdogTimeout;
      lzt::set_timeout_mode(pSchedHandle, timeout_set_properties);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      ASSERT_EQ(cur_mode, ZES_SCHED_MODE_TIMEOUT);
      auto timeout_get_properties =
          lzt::get_timeout_properties(pSchedHandle, false);
      EXPECT_EQ(timeout_get_properties.watchdogTimeout,
                timeout_set_properties.watchdogTimeout);
    }
  }
}

TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenSettingSchedulerTimeSliceModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto timeslice_default_properties =
          lzt::get_timeslice_properties(pSchedHandle, true);
      zes_sched_timeslice_properties_t timeslice_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMESLICE_PROPERTIES, nullptr};
      timeslice_set_properties.interval = timeslice_default_properties.interval;
      timeslice_set_properties.yieldTimeout =
          timeslice_default_properties.yieldTimeout;
      lzt::set_timeslice_mode(pSchedHandle, timeslice_set_properties);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      ASSERT_EQ(cur_mode, ZES_SCHED_MODE_TIMESLICE);
      auto timeslice_get_properties =
          lzt::get_timeslice_properties(pSchedHandle, false);
      EXPECT_EQ(timeslice_get_properties.interval,
                timeslice_set_properties.interval);
      EXPECT_EQ(timeslice_get_properties.yieldTimeout,
                timeslice_set_properties.yieldTimeout);
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenSettingSchedulerExclusiveModeThenSuccesseReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto timeout_default_properties =
          lzt::get_timeout_properties(pSchedHandle, true);
      zes_sched_timeout_properties_t timeout_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMEOUT_PROPERTIES, nullptr};
      timeout_set_properties.watchdogTimeout =
          timeout_default_properties.watchdogTimeout;
      auto timeslice_default_properties =
          lzt::get_timeslice_properties(pSchedHandle, true);
      zes_sched_timeslice_properties_t timeslice_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMESLICE_PROPERTIES, nullptr};
      timeslice_set_properties.interval = timeslice_default_properties.interval;
      timeslice_set_properties.yieldTimeout =
          timeslice_default_properties.yieldTimeout;
      lzt::set_exclusive_mode(pSchedHandle);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      ASSERT_EQ(cur_mode, ZES_SCHED_MODE_EXCLUSIVE);
      lzt::set_timeout_mode(pSchedHandle, timeout_set_properties);
      lzt::set_timeslice_mode(pSchedHandle, timeslice_set_properties);
    }
  }
}

TEST_F(
    SchedulerTest,
    GivenSchedulerHandleWhenSettingSchedulerComputeUnitDebugModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      lzt::set_compute_unit_debug_mode(pSchedHandle);
      auto cur_mode = lzt::get_scheduler_current_mode(pSchedHandle);
      ASSERT_EQ(cur_mode, ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG);
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenRetrievingSchedulerPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      ASSERT_NE(nullptr, pSchedHandle);
      auto properties = lzt::get_scheduler_properties(pSchedHandle);
      EXPECT_GE(properties.engines, ZES_ENGINE_TYPE_FLAG_OTHER);
      EXPECT_LE(properties.engines, ZES_ENGINE_TYPE_FLAG_DMA);
      EXPECT_GE(properties.supportedModes, 1);
      EXPECT_LE(properties.supportedModes,
                (1 << ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG));
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
    }
  }
}

TEST_F(
    SchedulerTest,
    GivenValidSchedulerHandleWhenRetrievingSchedulerPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pSchedHandles = lzt::get_scheduler_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pSchedHandle : pSchedHandles) {
      EXPECT_NE(nullptr, pSchedHandle);
      auto propertiesInitial = lzt::get_scheduler_properties(pSchedHandle);
      auto propertiesLater = lzt::get_scheduler_properties(pSchedHandle);
      EXPECT_EQ(propertiesInitial.engines, propertiesLater.engines);
      EXPECT_EQ(propertiesInitial.supportedModes,
                propertiesLater.supportedModes);
      EXPECT_EQ(propertiesInitial.canControl, propertiesLater.canControl);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice && propertiesLater.onSubdevice) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
    }
  }
}

} // namespace
