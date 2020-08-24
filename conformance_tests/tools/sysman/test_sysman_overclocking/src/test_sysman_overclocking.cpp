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
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

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
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilitiesInitial = lzt::get_oc_capabilities(freqHandle);
      auto capabilitiesLater = lzt::get_oc_capabilities(freqHandle);
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
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if (capabilities.isOcSupported == true) {
        auto freqTarget = lzt::get_oc_freq_target(freqHandle);
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
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if (capabilities.isOcSupported == true) {
        double initFreqTarget = lzt::get_oc_freq_target(freqHandle);
        double setFreqTarget = 0;
        setFreqTarget = capabilities.maxOcFrequency;
        lzt::set_oc_freq_target(freqHandle, setFreqTarget);
        double getFreqTarget = lzt::get_oc_freq_target(freqHandle);
        if (capabilities.isExtendedModeSupported == false) {
          EXPECT_EQ(fmod(getFreqTarget, 50), 0);
        }
        EXPECT_DOUBLE_EQ(getFreqTarget, setFreqTarget);
        lzt::set_oc_freq_target(freqHandle, initFreqTarget);
      }
    }
  }
}
TEST_F(
    OverclockingTest,
    GivenValidFrequencyHandlesWhenGettingOverclockingVoltageTargetThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if (capabilities.isOcSupported == true) {
        if ((capabilities.isHighVoltModeCapable == true) &&
            (capabilities.isHighVoltModeEnabled == true)) {
          auto voltageTarget = lzt::get_oc_voltage_target(freqHandle);
          auto voltageOffset = lzt::get_oc_voltage_offset(freqHandle);

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
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if (capabilities.isOcSupported == true) {
        if ((capabilities.isHighVoltModeCapable == true) &&
            (capabilities.isHighVoltModeEnabled == true)) {
          auto capabilities = lzt::get_oc_capabilities(freqHandle);
          auto voltageTargetInitial = lzt::get_oc_voltage_target(freqHandle);
          auto voltageOffsetInitial = lzt::get_oc_voltage_offset(freqHandle);
          double voltageTargetSet = capabilities.maxOcVoltage;
          double voltageOffsetSet = capabilities.maxOcVoltageOffset;
          lzt::set_oc_voltage(freqHandle, voltageTargetSet, voltageOffsetSet);
          auto voltageTargetGet = lzt::get_oc_voltage_target(freqHandle);
          auto voltageOffsetGet = lzt::get_oc_voltage_offset(freqHandle);
          EXPECT_DOUBLE_EQ(voltageTargetSet, voltageTargetGet);
          EXPECT_DOUBLE_EQ(voltageOffsetSet, voltageOffsetGet);
          lzt::set_oc_voltage(freqHandle, voltageTargetInitial,
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
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if (capabilities.isOcSupported == true) {
        auto mode = lzt::get_oc_mode(freqHandle);
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
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if (capabilities.isOcSupported == true) {
        auto initialMode = lzt::get_oc_mode(freqHandle);
        zes_oc_mode_t setMode = ZES_OC_MODE_OFF;
        auto getMode = lzt::get_oc_mode(freqHandle);
        EXPECT_EQ(setMode, getMode);
        setMode = ZES_OC_MODE_OVERRIDE;
        getMode = lzt::get_oc_mode(freqHandle);
        EXPECT_EQ(setMode, getMode);
        setMode = ZES_OC_MODE_INTERPOLATIVE;
        getMode = lzt::get_oc_mode(freqHandle);
        EXPECT_EQ(setMode, getMode);
        setMode = ZES_OC_MODE_FIXED;
        getMode = lzt::get_oc_mode(freqHandle);
        EXPECT_EQ(setMode, getMode);
      }
    }
  }
}

TEST_F(OverclockingTest,
       GivenValidFrequencyHandlesWhenGettingCurrentLimitThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

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
    GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzesSysmanFrequencyOcSetIccMaxFollowedByzesSysmanFrequencyOcGetIccMaxToReturnSuccess) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isIccMaxSupported == true)) {
        auto icmaxInitial = lzt::get_oc_iccmax(freqHandle);
        EXPECT_GT(icmaxInitial, 0);
        lzt::set_oc_iccmax(freqHandle, icmaxInitial);
        auto icmaxLater = lzt::get_oc_iccmax(freqHandle);
        EXPECT_DOUBLE_EQ(icmaxInitial, icmaxLater);
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
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

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
    GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzesSysmanFrequencyOcSetTjMaxFollowedByzesSysmanFrequencyOcGetTjMaxToMatch) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto freqHandles = lzt::get_freq_handles(device, pCount);
    if (pCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto freqHandle : freqHandles) {
      ASSERT_NE(nullptr, freqHandle);
      auto capabilities = lzt::get_oc_capabilities(freqHandle);
      if ((capabilities.isOcSupported == true) &&
          (capabilities.isTjMaxSupported == true)) {
        auto tjmaxInitial = lzt::get_oc_tjmax(freqHandle);
        EXPECT_GT(tjmaxInitial, 0);
        lzt::set_oc_tjmax(freqHandle, tjmaxInitial);
        auto tjmaxLater = lzt::get_oc_tjmax(freqHandle);
        EXPECT_DOUBLE_EQ(tjmaxInitial, tjmaxLater);
      }
    }
  }
}

} // namespace
