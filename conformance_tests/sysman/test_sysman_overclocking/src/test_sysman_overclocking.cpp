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
#include <cmath>

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {
#ifdef USE_ZESINIT
class OverclockingZesTest : public lzt::ZesSysmanCtsClass {
public:
  bool is_frequency_supported = false;
};
#define OVERCLOCK_TEST OverclockingZesTest
#else // USE_ZESINIT
class OverclockingTest : public lzt::SysmanCtsClass {
public:
  bool is_frequency_supported = false;
};
#define OVERCLOCK_TEST OverclockingTest
#endif // USE_ZESINIT

LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenGettingOverclockingCapabilitiesThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        EXPECT_GT(capabilities.maxFactoryDefaultFrequency, 0);
        EXPECT_GT(capabilities.maxFactoryDefaultVoltage, 0);
        EXPECT_GT(capabilities.maxOcFrequency, 0);
        EXPECT_GT(capabilities.maxOcVoltage, 0);
      }
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenCallingApiTwiceThenSimilarOverclockingCapabilitiesAreReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
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
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenGettingOverclockingFrequencyTargetThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported) {
          auto freqTarget = lzt::get_oc_freq_target(freq_handle);
          EXPECT_PRED_FORMAT2(::testing::DoubleLE, freqTarget,
                              capabilities.maxOcFrequency);
          if (capabilities.isExtendedModeSupported == false) {
            EXPECT_EQ(fmod(freqTarget, 50), 0);
          }
        }
      }
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenSettingOverclockingFrequencyTargetExpectzesFrequencyOcSetFrequencyTargetFollowedByzesFrequencyOcGetFrequencyTargetToMatch) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported) {
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
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenGettingOverclockingVoltageTargetThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported) {
          if ((capabilities.isHighVoltModeCapable) &&
              (capabilities.isHighVoltModeEnabled)) {
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
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenSettingOverclockingVoltageTargetThenExpectzesFrequencyOcSetVoltageTargetFollowedByzesFrequencyOcGetVoltageTargetToMatch) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported) {
          if ((capabilities.isHighVoltModeCapable) &&
              (capabilities.isHighVoltModeEnabled)) {
            auto capabilities = lzt::get_oc_capabilities(freq_handle);
            auto voltageTargetInitial = lzt::get_oc_voltage_target(freq_handle);
            auto voltageOffsetInitial = lzt::get_oc_voltage_offset(freq_handle);
            double voltageTargetSet = capabilities.maxOcVoltage;
            double voltageOffsetSet = capabilities.maxOcVoltageOffset;
            lzt::set_oc_voltage(freq_handle, voltageTargetSet,
                                voltageOffsetSet);
            auto voltageTargetGet = lzt::get_oc_voltage_target(freq_handle);
            auto voltageOffsetGet = lzt::get_oc_voltage_offset(freq_handle);
            EXPECT_DOUBLE_EQ(voltageTargetSet, voltageTargetGet);
            EXPECT_DOUBLE_EQ(voltageOffsetSet, voltageOffsetGet);
            lzt::set_oc_voltage(freq_handle, voltageTargetInitial,
                                voltageOffsetInitial);
          }
        }
      }
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenGettingOverclockingModeThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported) {
          auto mode = lzt::get_oc_mode(freq_handle);
          EXPECT_GE(mode, ZES_OC_MODE_OFF);
          EXPECT_LE(mode, ZES_OC_MODE_FIXED);
        }
      }
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenSettingOverclockingModeThenExpectzesFrequencyOcSetModeFollowedByzesFrequencyOcGetModeToMatch) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported) {
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
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenGettingCurrentLimitThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported && capabilities.isIccMaxSupported) {
          auto icmax = lzt::get_oc_iccmax(freq_handle);
          EXPECT_GT(icmax, 0);
        }
      }
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzesSysmanFrequencyOcSetIccMaxFollowedByzesSysmanFrequencyOcGetIccMaxToReturnSuccess) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported && capabilities.isIccMaxSupported) {
          auto icmax_initial = lzt::get_oc_iccmax(freq_handle);
          EXPECT_GT(icmax_initial, 0);
          lzt::set_oc_iccmax(freq_handle, icmax_initial);
          auto icmax_later = lzt::get_oc_iccmax(freq_handle);
          EXPECT_DOUBLE_EQ(icmax_initial, icmax_later);
        }
      }
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenGettingTemperaturetLimitThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported && capabilities.isTjMaxSupported) {
          auto tjmax = lzt::get_oc_tjmax(freq_handle);
          EXPECT_GT(tjmax, 0);
        }
      }
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    OVERCLOCK_TEST,
    GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzesSysmanFrequencyOcSetTjMaxFollowedByzesSysmanFrequencyOcGetTjMaxToMatch) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    p_count = lzt::get_freq_handle_count(device);
    if (p_count > 0) {
      is_frequency_supported = true;
      LOG_INFO << "Frequency handles are available on this device! ";
      auto freq_handles = lzt::get_freq_handles(device, p_count);
      for (auto freq_handle : freq_handles) {
        ASSERT_NE(nullptr, freq_handle);
        auto capabilities = lzt::get_oc_capabilities(freq_handle);
        if (capabilities.isOcSupported && capabilities.isTjMaxSupported) {
          auto tjmaxInitial = lzt::get_oc_tjmax(freq_handle);
          EXPECT_GT(tjmaxInitial, 0);
          lzt::set_oc_tjmax(freq_handle, tjmaxInitial);
          auto tjmaxLater = lzt::get_oc_tjmax(freq_handle);
          EXPECT_DOUBLE_EQ(tjmaxInitial, tjmaxLater);
        }
      }
    } else {
      LOG_INFO << "No frequency handles found for this device! ";
    }
  }
  if (!is_frequency_supported) {
    FAIL() << "No frequency handles found on any of the devices! ";
  }
}

} // namespace
