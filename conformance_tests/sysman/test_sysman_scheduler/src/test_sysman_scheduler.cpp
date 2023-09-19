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
#include <algorithm>
namespace {

#ifdef USE_ZESINIT
class SchedulerZesTest : public lzt::ZesSysmanCtsClass {};
#define SCHEDULER_TEST SchedulerZesTest
#else // USE_ZESINIT
class SchedulerTest : public lzt::SysmanCtsClass {};
#define SCHEDULER_TEST SchedulerTest
#endif // USE_ZESINIT

TEST_F(
    SCHEDULER_TEST,
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
    SCHEDULER_TEST,
    GivenComponentCountZeroWhenRetrievingSchedulerHandlesThenNotNullSchedulerHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
    }
  }
}

TEST_F(
    SCHEDULER_TEST,
    GivenComponentCountWhenRetrievingSchedulerHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = p_count + 1;
    lzt::get_scheduler_handles(device, test_count);
    EXPECT_EQ(test_count, p_count);
  }
}

TEST_F(
    SCHEDULER_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarSchedulerHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_sched_handles_initial = lzt::get_scheduler_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles_initial) {
      EXPECT_NE(nullptr, p_sched_handle);
    }
    count = 0;
    auto p_sched_handles_later = lzt::get_scheduler_handles(device, count);
    for (auto p_sched_handle : p_sched_handles_later) {
      EXPECT_NE(nullptr, p_sched_handle);
    }
    EXPECT_EQ(p_sched_handles_initial, p_sched_handles_later);
  }
}

TEST_F(SCHEDULER_TEST,
       GivenValidSchedulerHandleWhenRetrievinCurrentModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      ASSERT_GE(cur_mode, ZES_SCHED_MODE_TIMEOUT);
      ASSERT_LE(cur_mode, ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG);
    }
  }
}
TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenRetrievingCurrentModeTwiceThenSameModeIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto cur_mode_initial = lzt::get_scheduler_current_mode(p_sched_handle);
      auto cur_mode_later = lzt::get_scheduler_current_mode(p_sched_handle);
      EXPECT_EQ(cur_mode_initial, cur_mode_later);
    }
  }
}

TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenRetrievingSchedulerTimeOutPropertiesThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      if (cur_mode == ZES_SCHED_MODE_TIMEOUT) {
        auto timeout_properties =
            lzt::get_timeout_properties(p_sched_handle, false);
        EXPECT_GT(timeout_properties.watchdogTimeout, 0);
      }
    }
  }
}
TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenRetrievingSchedulerTimeOutPropertiesTwiceThenSamePropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      if (cur_mode == ZES_SCHED_MODE_TIMEOUT) {
        auto timeout_properties_initial =
            lzt::get_timeout_properties(p_sched_handle, false);
        auto timeout_properties_later =
            lzt::get_timeout_properties(p_sched_handle, false);
        EXPECT_EQ(timeout_properties_initial.watchdogTimeout,
                  timeout_properties_later.watchdogTimeout);
      }
    }
  }
}
TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenRetrievingSchedulerTimeSlicePropertiesThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      if (cur_mode == ZES_SCHED_MODE_TIMESLICE) {
        auto timeslice_properties =
            lzt::get_timeslice_properties(p_sched_handle, false);
        EXPECT_GT(timeslice_properties.interval, 0);
        EXPECT_GE(timeslice_properties.yieldTimeout, 0);
      }
    }
  }
}
TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenRetrievingSchedulerTimeSlicePropertiesTwiceThenSamePropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      if (cur_mode == ZES_SCHED_MODE_TIMESLICE) {
        auto timeslice_properties_initial =
            lzt::get_timeslice_properties(p_sched_handle, false);
        auto timeslice_properties_later =
            lzt::get_timeslice_properties(p_sched_handle, false);
        EXPECT_EQ(timeslice_properties_initial.interval,
                  timeslice_properties_later.interval);
        EXPECT_EQ(timeslice_properties_initial.yieldTimeout,
                  timeslice_properties_later.yieldTimeout);
      }
    }
  }
}
TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenSettingSchedulerTimeOutModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto timeout_default_properties =
          lzt::get_timeout_properties(p_sched_handle, true);
      zes_sched_timeout_properties_t timeout_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMEOUT_PROPERTIES, nullptr};
      timeout_set_properties.watchdogTimeout =
          timeout_default_properties.watchdogTimeout;
      lzt::set_timeout_mode(p_sched_handle, timeout_set_properties);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      ASSERT_EQ(cur_mode, ZES_SCHED_MODE_TIMEOUT);
      auto timeout_get_properties =
          lzt::get_timeout_properties(p_sched_handle, false);
      EXPECT_EQ(timeout_get_properties.watchdogTimeout,
                timeout_set_properties.watchdogTimeout);
    }
  }
}

TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenSettingSchedulerTimeSliceModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto timeslice_default_properties =
          lzt::get_timeslice_properties(p_sched_handle, true);
      zes_sched_timeslice_properties_t timeslice_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMESLICE_PROPERTIES, nullptr};
      timeslice_set_properties.interval = timeslice_default_properties.interval;
      timeslice_set_properties.yieldTimeout =
          timeslice_default_properties.yieldTimeout;
      lzt::set_timeslice_mode(p_sched_handle, timeslice_set_properties);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      ASSERT_EQ(cur_mode, ZES_SCHED_MODE_TIMESLICE);
      auto timeslice_get_properties =
          lzt::get_timeslice_properties(p_sched_handle, false);
      EXPECT_EQ(timeslice_get_properties.interval,
                timeslice_set_properties.interval);
      EXPECT_EQ(timeslice_get_properties.yieldTimeout,
                timeslice_set_properties.yieldTimeout);
    }
  }
}
TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenSettingSchedulerExclusiveModeThenSuccesseReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto timeout_default_properties =
          lzt::get_timeout_properties(p_sched_handle, true);
      zes_sched_timeout_properties_t timeout_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMEOUT_PROPERTIES, nullptr};
      timeout_set_properties.watchdogTimeout =
          timeout_default_properties.watchdogTimeout;
      auto timeslice_default_properties =
          lzt::get_timeslice_properties(p_sched_handle, true);
      zes_sched_timeslice_properties_t timeslice_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMESLICE_PROPERTIES, nullptr};
      timeslice_set_properties.interval = timeslice_default_properties.interval;
      timeslice_set_properties.yieldTimeout =
          timeslice_default_properties.yieldTimeout;
      lzt::set_exclusive_mode(p_sched_handle);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      ASSERT_EQ(cur_mode, ZES_SCHED_MODE_EXCLUSIVE);
      lzt::set_timeout_mode(p_sched_handle, timeout_set_properties);
      lzt::set_timeslice_mode(p_sched_handle, timeslice_set_properties);
    }
  }
}

TEST_F(
    SCHEDULER_TEST,
    GivenSchedulerHandleWhenSettingSchedulerComputeUnitDebugModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);

      auto timeout_current_properties =
          lzt::get_timeout_properties(p_sched_handle, false);
      zes_sched_timeout_properties_t timeout_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMEOUT_PROPERTIES, nullptr};
      timeout_set_properties.watchdogTimeout =
          timeout_current_properties.watchdogTimeout;
      auto timeslice_current_properties =
          lzt::get_timeslice_properties(p_sched_handle, false);
      zes_sched_timeslice_properties_t timeslice_set_properties = {
          ZES_STRUCTURE_TYPE_SCHED_TIMESLICE_PROPERTIES, nullptr};
      timeslice_set_properties.interval = timeslice_current_properties.interval;
      timeslice_set_properties.yieldTimeout =
          timeslice_current_properties.yieldTimeout;

      auto current_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      lzt::set_compute_unit_debug_mode(p_sched_handle);
      auto cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
      ASSERT_EQ(cur_mode, ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG);

      switch (current_mode) {
      case ZES_SCHED_MODE_TIMEOUT:
        lzt::set_timeout_mode(p_sched_handle, timeout_set_properties);
        cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
        ASSERT_EQ(cur_mode, ZES_SCHED_MODE_TIMEOUT);
        break;
      case ZES_SCHED_MODE_TIMESLICE:
        lzt::set_timeslice_mode(p_sched_handle, timeslice_set_properties);
        cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
        ASSERT_EQ(cur_mode, ZES_SCHED_MODE_TIMESLICE);
        break;
      case ZES_SCHED_MODE_EXCLUSIVE:
        lzt::set_exclusive_mode(p_sched_handle);
        cur_mode = lzt::get_scheduler_current_mode(p_sched_handle);
        ASSERT_EQ(cur_mode, ZES_SCHED_MODE_EXCLUSIVE);
      }
    }
  }
}
TEST_F(
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenRetrievingSchedulerPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      ASSERT_NE(nullptr, p_sched_handle);
      auto properties = lzt::get_scheduler_properties(p_sched_handle);
      EXPECT_GE(properties.engines, ZES_ENGINE_TYPE_FLAG_OTHER);
      EXPECT_LE(properties.engines, ZES_ENGINE_TYPE_FLAG_RENDER);
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
    SCHEDULER_TEST,
    GivenValidSchedulerHandleWhenRetrievingSchedulerPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto p_sched_handles = lzt::get_scheduler_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_sched_handle : p_sched_handles) {
      EXPECT_NE(nullptr, p_sched_handle);
      auto properties_initial = lzt::get_scheduler_properties(p_sched_handle);
      auto properties_later = lzt::get_scheduler_properties(p_sched_handle);
      EXPECT_EQ(properties_initial.engines, properties_later.engines);
      EXPECT_EQ(properties_initial.supportedModes,
                properties_later.supportedModes);
      EXPECT_EQ(properties_initial.canControl, properties_later.canControl);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
    }
  }
}

} // namespace
