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

namespace {
class OverclockingTest : public lzt::SysmanCtsClass {};
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingOverclockingCapabilitiesThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      EXPECT_GT(capabilities.maxFactoryDefaultFrequency, 0);
      EXPECT_GT(capabilities.maxFactoryDefaultVoltage, 0);
      EXPECT_GT(capabilities.maxOcFrequency, 0);
      EXPECT_GT(capabilities.maxOcVoltage, 0);
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenCallingApiTwiceThenSimilarOverclockingCapabilitiesAreReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities_initial = lzt::get_oc_capabilities(freqHandle);
      auto capabilities_later = lzt::get_oc_capabilities(freqHandle);
      EXPECT_EQ(capabilities_initial.isOcSupported,
                capabilities_later.isOcSupported);
      EXPECT_DOUBLE_EQ(capabilities_initial.maxFactoryDefaultFrequency,
                       capabilities_later.maxFactoryDefaultFrequency);
      EXPECT_DOUBLE_EQ(capabilities_initial.maxFactoryDefaultVoltage,
                       capabilities_later.maxFactoryDefaultVoltage);
      EXPECT_DOUBLE_EQ(capabilities_initial.maxOcFrequency,
                       capabilities_later.maxOcFrequency);
      EXPECT_DOUBLE_EQ(capabilities_initial.minOcVoltageOffset,
                       capabilities_later.minOcVoltageOffset);
      EXPECT_DOUBLE_EQ(capabilities_initial.maxOcVoltageOffset,
                       capabilities_later.maxOcVoltageOffset);
      EXPECT_DOUBLE_EQ(capabilities_initial.maxOcVoltage,
                       capabilities_later.maxOcVoltage);
      EXPECT_EQ(capabilities_initial.isTjMaxSupported,
                capabilities_later.isTjMaxSupported);
      EXPECT_EQ(capabilities_initial.isIccMaxSupported,
                capabilities_later.isIccMaxSupported);
      EXPECT_EQ(capabilities_initial.isHighVoltModeCapable,
                capabilities_later.isHighVoltModeCapable);
      EXPECT_EQ(capabilities_initial.isHighVoltModeEnabled,
                capabilities_later.isHighVoltModeEnabled);
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingOverclockingConfigurationThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto config = lzt::get_oc_config(freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      EXPECT_GE(config.mode, ZET_OC_MODE_OFF);
      EXPECT_LE(config.mode, ZET_OC_MODE_INTERPOLATIVE);
      if (capabilities.isOcSupported == true) {
        if ((config.mode == ZET_OC_MODE_OVERRIDE) ||
            (config.mode == ZET_OC_MODE_INTERPOLATIVE)) {
          EXPECT_PRED_FORMAT2(::testing::DoubleLE, config.frequency,
                              capabilities.maxOcFrequency);
          EXPECT_PRED_FORMAT2(::testing::DoubleLE, config.voltageTarget,
                              capabilities.maxOcVoltageOffset);
          EXPECT_PRED_FORMAT2(::testing::DoubleLE,
                              capabilities.minOcVoltageOffset,
                              config.voltageOffset);
          EXPECT_PRED_FORMAT2(::testing::DoubleLE, config.voltageOffset,
                              capabilities.maxOcVoltageOffset);
        }
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingOverclockingConfigThenExpectzetSysmanFrequencyOcSetConfigFollowedByzetSysmanFrequencyOcGetConfigToMatch) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto config_initial = lzt::get_oc_config(freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if (capabilities.isOcSupported == true) {
        zet_oc_config_t set_config;
        set_config.mode = ZET_OC_MODE_INTERPOLATIVE;
        set_config.frequency = capabilities.maxOcFrequency;
        if ((capabilities.isHighVoltModeCapable == true) &&
            (capabilities.isHighVoltModeEnabled == true)) {
          set_config.voltageTarget = capabilities.maxOcVoltage;
          set_config.voltageTarget = capabilities.maxOcVoltageOffset;
        }
        ze_bool_t restart = false;
        lzt::set_oc_config(freqHandle, set_config, &restart);
        auto get_config = lzt::get_oc_config(freqHandle);
        EXPECT_EQ(get_config.mode, set_config.mode);
        EXPECT_DOUBLE_EQ(get_config.frequency, set_config.frequency);
        if ((capabilities.isHighVoltModeCapable == true) &&
            (capabilities.isHighVoltModeEnabled == true)) {
          EXPECT_DOUBLE_EQ(get_config.voltageTarget, set_config.voltageTarget);
          EXPECT_DOUBLE_EQ(get_config.voltageOffset, set_config.voltageOffset);
        }
        lzt::set_oc_config(freqHandle, config_initial, &restart);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingOverclockingConfigToOffThenExpectzetSysmanFrequencyOcSetConfigFollowedByzetSysmanFrequencyOcGetConfigToMatch) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if (capabilities.isOcSupported == true) {
        zet_oc_config_t set_config;
        set_config.mode = ZET_OC_MODE_OFF;
        ze_bool_t restart = false;
        lzt::set_oc_config(freqHandle, set_config, &restart);
        double icmax = 0;
        lzt::set_oc_iccmax(freqHandle, icmax);
        double tjmax = 0;
        lzt::set_oc_tjmax(freqHandle, tjmax);
        auto state = lzt::get_freq_state(freqHandle);
        auto get_config = lzt::get_oc_config(freqHandle);
        EXPECT_EQ(get_config.mode, set_config.mode);
        EXPECT_PRED_FORMAT2(::testing::DoubleLE, state.actual,
                            capabilities.maxFactoryDefaultFrequency);
      }
    }
  }
}
TEST_F(OverclockingTest,
       GivenValidFrequencyHandlesWhenGettingCurrentLimitThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isIccMaxSupported == true)) {
        auto icmax = lzt::get_oc_iccmax(freqHandle);
        EXPECT_GT(icmax, 0);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzetSysmanFrequencyOcSetIccMaxFollowedByzetSysmanFrequencyOcGetIccMaxToReturnSuccess) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isIccMaxSupported == true)) {
        auto icmax_initial = lzt::get_oc_iccmax(freqHandle);
        EXPECT_GT(icmax_initial, 0);
        lzt::set_oc_iccmax(freqHandle, icmax_initial);
        auto icmax_final = lzt::get_oc_iccmax(freqHandle);
        EXPECT_DOUBLE_EQ(icmax_initial, icmax_final);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingTemperaturetLimitThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isTjMaxSupported == true)) {
        auto tjmax = lzt::get_oc_tjmax(freqHandle);
        EXPECT_GT(tjmax, 0);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzetSysmanFrequencyOcSetTjMaxFollowedByzetSysmanFrequencyOcGetTjMaxToReturnSuccess) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isTjMaxSupported == true)) {
        auto tjmax_initial = lzt::get_oc_tjmax(freqHandle);
        EXPECT_GT(tjmax_initial, 0);
        lzt::set_oc_tjmax(freqHandle, tjmax_initial);
        auto tjmax_final = lzt::get_oc_tjmax(freqHandle);
        EXPECT_DOUBLE_EQ(tjmax_initial, tjmax_final);
      }
    }
  }
}

} // namespace
