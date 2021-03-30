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
#include <cmath>

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {
class OverclockingTest : public lzt::SysmanCtsClass {};
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingOverclockingCapabilitiesThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
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
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilitiesInitial = lzt::get_oc_capabilities(freq_handle);
      auto capabilitiesLater = lzt::get_oc_capabilities(freq_handle);
      EXPECT_EQ(capabilitiesInitial.isOcSupported,
                capabilitiesLater.isOcSupported);
      EXPECT_DOUBLE_EQ(capabilitiesInitial.maxFactoryDefaultFrequency,
                       capabilitiesLater.maxFactoryDefaultFrequency);
      EXPECT_DOUBLE_EQ(capabilitiesInitial.maxFactoryDefaultVoltage,
                       capabilitiesLater.maxFactoryDefaultVoltage);
      EXPECT_DOUBLE_EQ(capabilitiesInitial.maxOcFrequency,
                       capabilitiesLater.maxOcFrequency);
      EXPECT_DOUBLE_EQ(capabilitiesInitial.minOcVoltageOffset,
                       capabilitiesLater.minOcVoltageOffset);
      EXPECT_DOUBLE_EQ(capabilitiesInitial.maxOcVoltageOffset,
                       capabilitiesLater.maxOcVoltageOffset);
      EXPECT_DOUBLE_EQ(capabilitiesInitial.maxOcVoltage,
                       capabilitiesLater.maxOcVoltage);
      EXPECT_EQ(capabilitiesInitial.isTjMaxSupported,
                capabilitiesLater.isTjMaxSupported);
      EXPECT_EQ(capabilitiesInitial.isIccMaxSupported,
                capabilitiesLater.isIccMaxSupported);
      EXPECT_EQ(capabilitiesInitial.isHighVoltModeCapable,
                capabilitiesLater.isHighVoltModeCapable);
      EXPECT_EQ(capabilitiesInitial.isHighVoltModeEnabled,
                capabilitiesLater.isHighVoltModeEnabled);
      EXPECT_EQ(capabilitiesInitial.isExtendedModeSupported,
                capabilitiesLater.isExtendedModeSupported);
      EXPECT_EQ(capabilitiesInitial.isFixedModeSupported,
                capabilitiesLater.isFixedModeSupported);
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingOverclockingFrequencyTargetThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if (capabilities.isOcSupported == true) {
        auto freqTarget = lzt::get_oc_freq_target(freq_handle);
        EXPECT_PRED_FORMAT2(::testing::DoubleLE, freqTarget,
                            capabilities.maxOcFrequency);
        if (capabilities.isExtendedModeSupported == false) {
          EXPECT_EQ(fmod(freqTarget, 50), 0);
        }
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingOverclockingFrequencyTargetExpectzesFrequencyOcSetFrequencyTargetFollowedByzesFrequencyOcGetFrequencyTargetToMatch) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if (capabilities.isOcSupported == true) {
        double init_freq_target = lzt::get_oc_freq_target(freq_handle);
        double set_freq_target = 0;
        set_freq_target = capabilities.maxOcFrequency;
        lzt::set_oc_freq_target(freq_handle, set_freq_target);
        double get_freq_target = lzt::get_oc_freq_target(freq_handle);
        if (capabilities.isExtendedModeSupported == false) {
          EXPECT_EQ(fmod(get_freq_target, 50), 0);
        }
        EXPECT_DOUBLE_EQ(get_freq_target, set_freq_target);
        lzt::set_oc_freq_target(freq_handle, init_freq_target);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingOverclockingVoltageTargetThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if (capabilities.isOcSupported == true) {
        if ((capabilities.isHighVoltModeCapable == true) &&
            (capabilities.isHighVoltModeEnabled == true)) {
          auto voltageTarget = lzt::get_oc_voltage_target(freq_handle);
          auto voltageOffset = lzt::get_oc_voltage_offset(freq_handle);

          EXPECT_PRED_FORMAT2(::testing::DoubleLE, voltageTarget,
                              capabilities.maxOcVoltage);
          EXPECT_PRED_FORMAT2(::testing::DoubleLE, voltageOffset,
                              capabilities.maxOcVoltageOffset);
          EXPECT_PRED_FORMAT2(::testing::DoubleLE,
                              capabilities.minOcVoltageOffset, voltageOffset);
        }
      }
    }
  }
}

TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingOverclockingVoltageTargetThenExpectzesFrequencyOcSetVoltageTargetFollowedByzesFrequencyOcGetVoltageTargetToMatch) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if (capabilities.isOcSupported == true) {
        if ((capabilities.isHighVoltModeCapable == true) &&
            (capabilities.isHighVoltModeEnabled == true)) {
          auto capabilities = lzt::get_oc_capabilities(freq_handle);
          auto voltageTargetInitial = lzt::get_oc_voltage_target(freq_handle);
          auto voltageOffsetInitial = lzt::get_oc_voltage_offset(freq_handle);
          double voltageTargetSet = capabilities.maxOcVoltage;
          double voltageOffsetSet = capabilities.maxOcVoltageOffset;
          lzt::set_oc_voltage(freq_handle, voltageTargetSet, voltageOffsetSet);
          auto voltageTargetGet = lzt::get_oc_voltage_target(freq_handle);
          auto voltageOffsetGet = lzt::get_oc_voltage_offset(freq_handle);
          EXPECT_DOUBLE_EQ(voltageTargetSet, voltageTargetGet);
          EXPECT_DOUBLE_EQ(voltageOffsetSet, voltageOffsetGet);
          lzt::set_oc_voltage(freq_handle, voltageTargetInitial,
                              voltageOffsetInitial);
        }
      }
    }
  }
}

TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingOverclockingModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if (capabilities.isOcSupported == true) {
        auto mode = lzt::get_oc_mode(freq_handle);
        EXPECT_GE(mode, ZES_OC_MODE_OFF);
        EXPECT_LE(mode, ZES_OC_MODE_FIXED);
      }
    }
  }
}

TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingOverclockingModeThenExpectzesFrequencyOcSetModeFollowedByzesFrequencyOcGetModeToMatch) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if (capabilities.isOcSupported == true) {
        auto initialMode = lzt::get_oc_mode(freq_handle);
        zes_oc_mode_t setMode = ZES_OC_MODE_OFF;
        auto getMode = lzt::get_oc_mode(freq_handle);
        EXPECT_EQ(setMode, getMode);
        setMode = ZES_OC_MODE_OVERRIDE;
        getMode = lzt::get_oc_mode(freq_handle);
        EXPECT_EQ(setMode, getMode);
        setMode = ZES_OC_MODE_INTERPOLATIVE;
        getMode = lzt::get_oc_mode(freq_handle);
        EXPECT_EQ(setMode, getMode);
        setMode = ZES_OC_MODE_FIXED;
        getMode = lzt::get_oc_mode(freq_handle);
        EXPECT_EQ(setMode, getMode);
      }
    }
  }
}

TEST_F(OverclockingTest,
       GivenValidFrequencyHandlesWhenGettingCurrentLimitThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isIccMaxSupported == true)) {
        auto icmax = lzt::get_oc_iccmax(freq_handle);
        EXPECT_GT(icmax, 0);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzesSysmanFrequencyOcSetIccMaxFollowedByzesSysmanFrequencyOcGetIccMaxToReturnSuccess) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isIccMaxSupported == true)) {
        auto icmax_initial = lzt::get_oc_iccmax(freq_handle);
        EXPECT_GT(icmax_initial, 0);
        lzt::set_oc_iccmax(freq_handle, icmax_initial);
        auto icmax_later = lzt::get_oc_iccmax(freq_handle);
        EXPECT_DOUBLE_EQ(icmax_initial, icmax_later);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingTemperaturetLimitThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isTjMaxSupported == true)) {
        auto tjmax = lzt::get_oc_tjmax(freq_handle);
        EXPECT_GT(tjmax, 0);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzesSysmanFrequencyOcSetTjMaxFollowedByzesSysmanFrequencyOcGetTjMaxToMatch) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto freq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freq_handle : freq_handles) {
      ASSERT_NE(nullptr, freq_handle);
      auto capabilities = lzt::get_oc_capabilities(freq_handle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isTjMaxSupported == true)) {
        auto tjmaxInitial = lzt::get_oc_tjmax(freq_handle);
        EXPECT_GT(tjmaxInitial, 0);
        lzt::set_oc_tjmax(freq_handle, tjmaxInitial);
        auto tjmaxLater = lzt::get_oc_tjmax(freq_handle);
        EXPECT_DOUBLE_EQ(tjmaxInitial, tjmaxLater);
      }
    }
  }
}

} // namespace
