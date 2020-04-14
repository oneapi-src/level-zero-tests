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

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include <algorithm>
namespace {

class SchedulerTest : public lzt::SysmanCtsClass {};

TEST_F(SchedulerTest,
       GivenValidSysmanHandleWhenRetrievinCurrentModeThenSuccessIsReturned) {
  for (auto device : devices) {
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    ASSERT_GE(cur_mode, ZET_SCHED_MODE_TIMEOUT);
    ASSERT_LE(cur_mode, ZET_SCHED_MODE_COMPUTE_UNIT_DEBUG);
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSysmanHandleWhenRetrievingCurrentModeTwiceThenSameModeIsReturned) {
  for (auto device : devices) {
    auto cur_mode_initial = lzt::get_scheduler_current_mode(device);
    auto cur_mode_later = lzt::get_scheduler_current_mode(device);
    EXPECT_EQ(cur_mode_initial, cur_mode_later);
  }
}

TEST_F(
    SchedulerTest,
    GivenValidSysmanHandleWhenRetrievingSchedulerTimeOutPropertiesThenSuccessIsReturned) {
  for (auto device : devices) {
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    if (cur_mode == ZET_SCHED_MODE_TIMEOUT) {
      auto timeout_properties = lzt::get_timeout_properties(device, false);
      EXPECT_GT(timeout_properties.watchdogTimeout, 0);
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSysmanHandleWhenRetrievingSchedulerTimeOutPropertiesTwiceThenSamePropertiesAreReturned) {
  for (auto device : devices) {
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    if (cur_mode == ZET_SCHED_MODE_TIMEOUT) {
      auto timeout_properties_initial =
          lzt::get_timeout_properties(device, false);
      auto timeout_properties_later =
          lzt::get_timeout_properties(device, false);
      EXPECT_EQ(timeout_properties_initial.watchdogTimeout,
                timeout_properties_later.watchdogTimeout);
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSysmanHandleWhenRetrievingSchedulerTimeSlicePropertiesThenSuccessIsReturned) {
  for (auto device : devices) {
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    if (cur_mode == ZET_SCHED_MODE_TIMESLICE) {
      auto timeslice_properties = lzt::get_timeslice_properties(device, false);
      EXPECT_GT(timeslice_properties.interval, 0);
      EXPECT_GT(timeslice_properties.yieldTimeout, 0);
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSysmanHandleWhenRetrievingSchedulerTimeSlicePropertiesTwiceThenSamePropertiesAreReturned) {
  for (auto device : devices) {
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    if (cur_mode == ZET_SCHED_MODE_TIMESLICE) {
      auto timeslice_properties_initial =
          lzt::get_timeslice_properties(device, false);
      auto timeslice_properties_later =
          lzt::get_timeslice_properties(device, false);
      EXPECT_EQ(timeslice_properties_initial.interval,
                timeslice_properties_later.interval);
      EXPECT_EQ(timeslice_properties_initial.yieldTimeout,
                timeslice_properties_later.yieldTimeout);
    }
  }
}
TEST_F(
    SchedulerTest,
    GivenValidSysmanHandleWhenSettingSchedulerTimeOutModeThenSuccessIsReturned) {
  for (auto device : devices) {
    auto timeout_default_properties = lzt::get_timeout_properties(device, true);
    zet_sched_timeout_properties_t timeout_set_properties;
    timeout_set_properties.watchdogTimeout =
        timeout_default_properties.watchdogTimeout;
    lzt::set_timeout_mode(device, timeout_set_properties);
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    ASSERT_EQ(cur_mode, ZET_SCHED_MODE_TIMEOUT);
    auto timeout_get_properties = lzt::get_timeout_properties(device, false);
    EXPECT_EQ(timeout_get_properties.watchdogTimeout,
              timeout_set_properties.watchdogTimeout);
  }
}

TEST_F(
    SchedulerTest,
    GivenValidSysmanHandleWhenSettingSchedulerTimeSliceModeThenSuccessIsReturned) {
  for (auto device : devices) {
    auto timeslice_default_properties =
        lzt::get_timeslice_properties(device, true);
    zet_sched_timeslice_properties_t timeslice_set_properties;
    timeslice_set_properties.interval = timeslice_default_properties.interval;
    timeslice_set_properties.yieldTimeout =
        timeslice_default_properties.yieldTimeout;
    lzt::set_timeslice_mode(device, timeslice_set_properties);
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    ASSERT_EQ(cur_mode, ZET_SCHED_MODE_TIMESLICE);
    auto timeslice_get_properties =
        lzt::get_timeslice_properties(device, false);
    EXPECT_EQ(timeslice_get_properties.interval,
              timeslice_set_properties.interval);
    EXPECT_EQ(timeslice_get_properties.yieldTimeout,
              timeslice_set_properties.yieldTimeout);
  }
}

TEST_F(
    SchedulerTest,
    GivenValidSysmanHandleWhenSettingSchedulerExclusiveModeThenSuccesseReturned) {
  for (auto device : devices) {
    lzt::set_exclusive_mode(device);
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    ASSERT_EQ(cur_mode, ZET_SCHED_MODE_EXCLUSIVE);
  }
}

TEST_F(
    SchedulerTest,
    GivenSysmanHandlesWhenSettingSchedulerComputeUnitDebugModeThenSuccessIsReturned) {
  for (auto device : devices) {
    lzt::set_compute_unit_debug_mode(device);
    auto cur_mode = lzt::get_scheduler_current_mode(device);
    ASSERT_EQ(cur_mode, ZET_SCHED_MODE_COMPUTE_UNIT_DEBUG);
  }
}

} // namespace
