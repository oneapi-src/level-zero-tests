/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include <thread>
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include <chrono>
namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {
class PowerModuleTest : public lzt::SysmanCtsClass {};
TEST_F(
    PowerModuleTest,
    GivenValidDeviceWhenRetrievingPowerHandlesThenNonZeroCountAndValidPowerHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
    }
  }
}
TEST_F(
    PowerModuleTest,
    GivenValidDeviceWhenRetrievingPowerHandlesThenSimilarHandlesAreReturnedTwice) {
  for (auto device : devices) {
    uint32_t icount = 0;
    uint32_t lcount = 0;
    auto p_power_handlesInitial = lzt::get_power_handles(device, icount);
    if (icount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    auto p_power_handlesLater = lzt::get_power_handles(device, lcount);
    EXPECT_EQ(p_power_handlesInitial, p_power_handlesLater);
  }
}
TEST_F(
    PowerModuleTest,
    GivenValidDeviceWhenRetrievingPowerHandlesThenActualHandleCountIsUpdatedAndIfRequestedHandlesAreLessThanActualHandleCountThenDesiredNumberOfHandlesAreReturned) {
  for (auto device : devices) {

    uint32_t p_count = lzt::get_power_handle_count(device);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t tcount = p_count + 1;
    lzt::get_power_handles(device, tcount);
    EXPECT_EQ(tcount, p_count);
    if (p_count > 1) {
      tcount = p_count - 1;
      auto p_power_handles = lzt::get_power_handles(device, tcount);
      EXPECT_EQ(static_cast<uint32_t>(p_power_handles.size()), tcount);
    }
  }
}
TEST_F(
    PowerModuleTest,
    GivenSamePowerHandleWhenRequestingPowerPropertiesThenExpectValidMaxLimit) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      auto pProperties = lzt::get_power_properties(p_power_handle);
      if (pProperties.maxLimit == -1) {
        LOG_INFO << "maxlimit unsupported: ";
      }
      if (pProperties.maxLimit != -1) {
        EXPECT_GT(pProperties.maxLimit, 0);
        EXPECT_LE(pProperties.maxLimit, INT32_MAX);
        EXPECT_GE(pProperties.maxLimit, pProperties.minLimit);
      }
      EXPECT_GE(pProperties.minLimit, 0);
      EXPECT_LT(pProperties.minLimit, INT32_MAX);
    }
  }
}

TEST_F(
    PowerModuleTest,
    GivenSamePowerHandleWhenRequestingPowerPropertiesThenExpectSamePropertiesTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      auto pproperties_initial = lzt::get_power_properties(p_power_handle);
      if (pproperties_initial.maxLimit == -1) {
        LOG_INFO << "maxlimit unsupported: ";
      }
      auto pproperties_later = lzt::get_power_properties(p_power_handle);
      EXPECT_EQ(pproperties_initial.onSubdevice, pproperties_later.onSubdevice);
      EXPECT_EQ(pproperties_initial.subdeviceId, pproperties_later.subdeviceId);
      EXPECT_EQ(pproperties_initial.canControl, pproperties_later.canControl);
      EXPECT_EQ(pproperties_initial.isEnergyThresholdSupported,
                pproperties_later.isEnergyThresholdSupported);
      EXPECT_EQ(pproperties_initial.maxLimit, pproperties_later.maxLimit);
      EXPECT_EQ(pproperties_initial.minLimit, pproperties_later.minLimit);
      EXPECT_EQ(pproperties_initial.defaultLimit,
                pproperties_later.defaultLimit);
    }
  }
}

TEST_F(
    PowerModuleTest,
    GivenValidPowerHandleWhenRequestingPowerLimitsThenExpectzesSysmanPowerGetLimitsToReturnValidPowerLimits) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      zes_power_sustained_limit_t pSustained = {};
      zes_power_burst_limit_t pBurst = {};
      zes_power_peak_limit_t pPeak = {};
      lzt::get_power_limits(p_power_handle, &pSustained, &pBurst, &pPeak);
      auto pProperties = lzt::get_power_properties(p_power_handle);
      if (pProperties.maxLimit == -1) {
        LOG_INFO << "maxlimit unsupported: ";
      }
      if ((pBurst.enabled != 0) && (pSustained.enabled != 0)) {
        EXPECT_LE(pSustained.power, pBurst.power);
      }
      if ((pPeak.powerAC != -1) && (pBurst.enabled != 0)) {
        EXPECT_LE(pBurst.power, pPeak.powerAC);
      }
      if (pProperties.maxLimit != -1) {
        EXPECT_LE(pPeak.powerAC, pProperties.maxLimit);
      }
    }
  }
}
TEST_F(
    PowerModuleTest,
    GivenValidPowerHandleWhenRequestingPowerLimitsThenExpectzesSysmanPowerGetLimitsToReturnSameValuesTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      zes_power_sustained_limit_t pSustainedInitial = {};
      zes_power_burst_limit_t pBurstInitial = {};
      zes_power_peak_limit_t pPeakInitial = {};
      lzt::get_power_limits(p_power_handle, &pSustainedInitial, &pBurstInitial,
                            &pPeakInitial);
      zes_power_sustained_limit_t pSustainedLater = {};
      zes_power_burst_limit_t pBurstLater = {};
      zes_power_peak_limit_t pPeakLater = {};
      lzt::get_power_limits(p_power_handle, &pSustainedLater, &pBurstLater,
                            &pPeakLater);

      EXPECT_EQ(pSustainedInitial.enabled, pSustainedLater.enabled);
      EXPECT_EQ(pSustainedInitial.power, pSustainedLater.power);
      EXPECT_EQ(pSustainedInitial.interval, pSustainedLater.interval);
      EXPECT_EQ(pBurstInitial.enabled, pBurstLater.enabled);
      EXPECT_EQ(pBurstInitial.power, pBurstLater.power);
      EXPECT_EQ(pPeakInitial.powerAC, pPeakLater.powerAC);
      EXPECT_EQ(pPeakInitial.powerDC, pPeakLater.powerDC);
    }
  }
}
TEST_F(
    PowerModuleTest,
    GivenValidPowerHandleWhenSettingPowerValuesThenExpectzesSysmanPowerSetLimitsFollowedByzesSysmanPowerGetLimitsToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      zes_power_sustained_limit_t pSustainedInitial = {};
      zes_power_burst_limit_t pBurstInitial = {};
      zes_power_peak_limit_t pPeakInitial = {};
      lzt::get_power_limits(p_power_handle, &pSustainedInitial, &pBurstInitial,
                            &pPeakInitial); // get default power values
      auto pProperties = lzt::get_power_properties(p_power_handle);
      if (pProperties.maxLimit == -1) {
        LOG_INFO << "maxlimit unsupported:";
      }
      zes_power_sustained_limit_t pSustainedSet = {};
      zes_power_burst_limit_t pBurstSet = {};
      zes_power_peak_limit_t pPeakSet = {};
      if (pSustainedInitial.enabled)
        pSustainedSet.enabled = 1;
      else
        pSustainedSet.enabled = 0;
      pSustainedSet.interval = pSustainedInitial.interval;
      if (pBurstInitial.enabled)
        pBurstSet.enabled = 1;
      else
        pBurstSet.enabled = 0;
      if (pProperties.maxLimit != -1) {
        pSustainedSet.power = pProperties.maxLimit;
        pBurstSet.power = pProperties.maxLimit;
        pPeakSet.powerAC = pProperties.maxLimit;
      }
      pPeakSet.powerDC = pPeakInitial.powerDC;
      lzt::set_power_limits(p_power_handle, &pSustainedSet, &pBurstSet,
                            &pPeakSet); // Set power values
      zes_power_sustained_limit_t pSustainedGet = {};
      zes_power_burst_limit_t pBurstGet = {};
      zes_power_peak_limit_t pPeakGet = {};
      lzt::get_power_limits(p_power_handle, &pSustainedGet, &pBurstGet,
                            &pPeakGet); // Get power values
      EXPECT_EQ(pSustainedGet.enabled, pSustainedSet.enabled);
      EXPECT_EQ(pSustainedGet.power, pSustainedSet.power);
      EXPECT_EQ(pSustainedGet.interval, pSustainedSet.interval);
      EXPECT_EQ(pBurstGet.enabled, pBurstSet.enabled);
      EXPECT_EQ(pBurstGet.power, pBurstSet.power);
      if (pPeakGet.powerAC != -1) {
        EXPECT_EQ(pPeakGet.powerAC, pPeakSet.powerAC);
      }
      if (pPeakGet.powerDC != -1) {
        EXPECT_EQ(pPeakGet.powerDC,
                  pPeakSet.powerDC); // Verify whether values match or not
      }
      lzt::set_power_limits(p_power_handle, &pSustainedInitial, &pBurstInitial,
                            &pPeakInitial); // Set values to default
    }
  }
}
TEST_F(
    PowerModuleTest,
    GivenValidPowerHandleThenExpectzesSysmanPowerGetEnergyCounterToReturnSuccess) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      zes_power_energy_counter_t pEnergyCounter = {};
      lzt::get_power_energy_counter(p_power_handle, &pEnergyCounter);
      uint64_t energy_initial = pEnergyCounter.energy;
      uint64_t timestamp_initial = pEnergyCounter.timestamp;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      lzt::get_power_energy_counter(p_power_handle, &pEnergyCounter);
      uint64_t energy_later = pEnergyCounter.energy;
      uint64_t timestamp_later = pEnergyCounter.timestamp;
      EXPECT_GE(energy_later, energy_initial);
      EXPECT_NE(timestamp_later, timestamp_initial);
    }
  }
}
TEST_F(
    PowerModuleTest,
    GivenValidPowerHandleWhenGettingEnergyThresholdThenSuccessIsReturnedAndParameterValuesAreValid) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      auto pThreshold = lzt::get_power_energy_threshold(p_power_handle);
      ASSERT_GE(pThreshold.threshold, 0);
      if (pThreshold.threshold > 0)
        EXPECT_LT(pThreshold.processId, UINT32_MAX);
      else
        EXPECT_EQ(pThreshold.processId, UINT32_MAX);
    }
  }
}
TEST_F(
    PowerModuleTest,
    GivenValidPowerHandleWhenGettingEnergyThresholdTwiceThenSameValueReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      auto pThresholdInitial = lzt::get_power_energy_threshold(p_power_handle);
      auto pThresholdLater = lzt::get_power_energy_threshold(p_power_handle);
      EXPECT_EQ(pThresholdInitial.enable, pThresholdLater.enable);
      EXPECT_EQ(pThresholdInitial.threshold, pThresholdLater.threshold);
      EXPECT_EQ(pThresholdInitial.processId, pThresholdLater.processId);
    }
  }
}
TEST_F(
    PowerModuleTest,
    GivenValidPowerHandleWhenSettingEnergyValuesThenExpectzesSysmanPowerSetEnergyThresholdFollowedByzesSysmanPowerGetEnergyThresholdToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      auto pThresholdInitial =
          lzt::get_power_energy_threshold(p_power_handle); // get initial value
      double threshold = 0;
      lzt::set_power_energy_threshold(p_power_handle, threshold); // set test
                                                                  // value
      auto pThresholdGet =
          lzt::get_power_energy_threshold(p_power_handle); // get test value
      EXPECT_EQ(pThresholdGet.threshold, threshold); // match both the values
      EXPECT_EQ(pThresholdGet.processId, UINT32_MAX);
      lzt::set_power_energy_threshold(
          p_power_handle,
          pThresholdInitial.threshold); // reset to initial value
    }
  }
}
} // namespace
