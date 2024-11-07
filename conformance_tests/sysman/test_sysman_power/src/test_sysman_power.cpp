/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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
#ifdef USE_ZESINIT
class PowerModuleZesTest : public lzt::ZesSysmanCtsClass {};
#define POWER_TEST PowerModuleZesTest
#else // USE_ZESINIT
class PowerModuleTest : public lzt::SysmanCtsClass {};
#define POWER_TEST PowerModuleTest
#endif // USE_ZESINIT

TEST_F(
    POWER_TEST,
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
    POWER_TEST,
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
    POWER_TEST,
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
    POWER_TEST,
    GivenSamePowerHandleWhenRequestingPowerPropertiesThenCheckPowerLimitsAreInRange) {
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
      if (pProperties.maxLimit != -1) {
        EXPECT_GT(pProperties.maxLimit, 0);
        EXPECT_GE(pProperties.maxLimit, pProperties.minLimit);
      } else {
        LOG_INFO << "maxLimit unsupported: ";
      }

      if (pProperties.minLimit != -1) {
        EXPECT_GE(pProperties.minLimit, 0);
      } else {
        LOG_INFO << "minlimit unsupported: ";
      }

      if (pProperties.defaultLimit != -1) {
        EXPECT_GT(pProperties.defaultLimit, 0);
      } else {
        LOG_INFO << "defaultLimit unsupported: ";
      }
    }
  }
}

TEST_F(
    POWER_TEST,
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
    POWER_TEST,
    GivenValidPowerHandleWhenRequestingPowerLimitsThenExpectZesSysmanPowerGetLimitsToReturnValidPowerLimits) {
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
      auto status = lzt::get_power_limits(p_power_handle, &pSustained, &pBurst, &pPeak);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
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
    POWER_TEST,
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
      auto status = lzt::get_power_limits(p_power_handle, &pSustainedInitial, &pBurstInitial,
                            &pPeakInitial);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      zes_power_sustained_limit_t pSustainedLater = {};
      zes_power_burst_limit_t pBurstLater = {};
      zes_power_peak_limit_t pPeakLater = {};
      status = lzt::get_power_limits(p_power_handle, &pSustainedLater, &pBurstLater,
                            &pPeakLater);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
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
    POWER_TEST,
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
      auto status = lzt::get_power_limits(p_power_handle, &pSustainedInitial, &pBurstInitial,
                            &pPeakInitial); // get default power values
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
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
      if (pBurstSet.enabled && pSustainedSet.enabled) {
        status = lzt::set_power_limits(p_power_handle, &pSustainedSet, &pBurstSet,
                              &pPeakSet); // Set power values
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        zes_power_sustained_limit_t pSustainedGet = {};
        zes_power_burst_limit_t pBurstGet = {};
        zes_power_peak_limit_t pPeakGet = {};
        status = lzt::get_power_limits(p_power_handle, &pSustainedGet, &pBurstGet,
                              &pPeakGet); // Get power values
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
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
        status = lzt::set_power_limits(p_power_handle, &pSustainedInitial,
                              &pBurstInitial,
                              &pPeakInitial); // Set values to default
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      } else {
        LOG_INFO << "Set limit not supported due to burst and sustained "
                    "enabled flag is false";
      }
    }
  }
}
TEST_F(
    POWER_TEST,
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
    POWER_TEST,
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
      zes_energy_threshold_t pThreshold = {};
      auto status =
          lzt::get_power_energy_threshold(p_power_handle, &pThreshold);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;        
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      ASSERT_GE(pThreshold.threshold, 0);
      if (pThreshold.threshold > 0)
        EXPECT_LT(pThreshold.processId, UINT32_MAX);
      else
        EXPECT_EQ(pThreshold.processId, UINT32_MAX);
    }
  }
}
TEST_F(
    POWER_TEST,
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
      zes_energy_threshold_t pThresholdInitial = {};
      auto status =
          lzt::get_power_energy_threshold(p_power_handle, &pThresholdInitial);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      zes_energy_threshold_t pThresholdLater = {};
      status =
          lzt::get_power_energy_threshold(p_power_handle, &pThresholdLater);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      EXPECT_EQ(pThresholdInitial.enable, pThresholdLater.enable);
      EXPECT_EQ(pThresholdInitial.threshold, pThresholdLater.threshold);
      EXPECT_EQ(pThresholdInitial.processId, pThresholdLater.processId);
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandleWhenSettingEnergyValuesThenExpectZesSysmanPowerSetEnergyThresholdFollowedByZesSysmanPowerGetEnergyThresholdToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      zes_energy_threshold_t pThresholdInitial = {};
      auto status = lzt::get_power_energy_threshold(
          p_power_handle, &pThresholdInitial); // get initial value
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      double threshold = 0;
      lzt::set_power_energy_threshold(p_power_handle, threshold); // set test
                                                                  // value
      zes_energy_threshold_t pThresholdGet = {};
      status = lzt::get_power_energy_threshold(p_power_handle,
                                      &pThresholdGet); // get test value
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      EXPECT_EQ(pThresholdGet.threshold, threshold); // match both the values
      EXPECT_EQ(pThresholdGet.processId, UINT32_MAX);
      lzt::set_power_energy_threshold(
          p_power_handle,
          pThresholdInitial.threshold); // reset to initial value
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandleWhenRequestingPowerLimitsThenExpectZesPowerGetLimitsExtToReturnSameValuesTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      uint32_t count_power = 0;

      zes_power_limit_ext_desc_t power_peak_descriptor_first = {};
      zes_power_limit_ext_desc_t power_burst_descriptor_first = {};
      zes_power_limit_ext_desc_t power_sustained_descriptor_first = {};
      zes_power_limit_ext_desc_t power_instantaneous_descriptor_first = {};

      zes_power_limit_ext_desc_t power_peak_descriptor_second = {};
      zes_power_limit_ext_desc_t power_burst_descriptor_second = {};
      zes_power_limit_ext_desc_t power_sustained_descriptor_second = {};
      zes_power_limit_ext_desc_t power_instantaneous_descriptor_second = {};

      std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_initial;
      auto status = lzt::get_power_limits_ext(p_power_handle, &count_power,
                                              power_limits_descriptors_initial);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      for (auto power_limits_descriptor_initial :
           power_limits_descriptors_initial) {
        if (power_limits_descriptor_initial.level ==
            ZES_POWER_LEVEL_SUSTAINED) {
          power_sustained_descriptor_first = power_limits_descriptor_initial;
        } else if (power_limits_descriptor_initial.level ==
                   ZES_POWER_LEVEL_PEAK) {
          power_peak_descriptor_first = power_limits_descriptor_initial;
        } else if (power_limits_descriptor_initial.level ==
                   ZES_POWER_LEVEL_BURST) {
          power_burst_descriptor_first = power_limits_descriptor_initial;
        } else if (power_limits_descriptor_initial.level ==
                   ZES_POWER_LEVEL_INSTANTANEOUS) {
          power_instantaneous_descriptor_first =
              power_limits_descriptor_initial;
        }
      }

      std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_final;
      status = lzt::get_power_limits_ext(p_power_handle, &count_power,
                                         power_limits_descriptors_final);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      for (auto power_limits_descriptor_final :
           power_limits_descriptors_final) {
        if (power_limits_descriptor_final.level == ZES_POWER_LEVEL_SUSTAINED) {
          power_sustained_descriptor_second = power_limits_descriptor_final;
        } else if (power_limits_descriptor_final.level ==
                   ZES_POWER_LEVEL_PEAK) {
          power_peak_descriptor_second = power_limits_descriptor_final;
        } else if (power_limits_descriptor_final.level ==
                   ZES_POWER_LEVEL_BURST) {
          power_burst_descriptor_second = power_limits_descriptor_final;
        } else if (power_limits_descriptor_final.level ==
                   ZES_POWER_LEVEL_INSTANTANEOUS) {
          power_instantaneous_descriptor_second = power_limits_descriptor_final;
        }
      }

      lzt::compare_power_descriptor_structures(power_peak_descriptor_first,
                                               power_peak_descriptor_second);
      lzt::compare_power_descriptor_structures(power_burst_descriptor_first,
                                               power_burst_descriptor_second);
      lzt::compare_power_descriptor_structures(
          power_sustained_descriptor_first, power_sustained_descriptor_second);
      lzt::compare_power_descriptor_structures(
          power_instantaneous_descriptor_first,
          power_instantaneous_descriptor_second);
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandleWhenSettingPowerValuesForSustainedPowerThenExpectzesPowerSetLimitsExtFollowedByzesPowerGetLimitsExtToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      uint32_t count_power = 0;

      zes_power_limit_ext_desc_t power_sustained_set = {};

      std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors;
      auto status = lzt::get_power_limits_ext(
          p_power_handle, &count_power,
          power_limits_descriptors); // get power limits for all descriptors
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      std::vector<zes_power_limit_ext_desc_t>
          power_limits_descriptors_initial; // preserve initial power limit
                                            // descriptors for restoration
                                            // later

      for (int i = 0; i < power_limits_descriptors.size(); i++) {
        power_limits_descriptors[i] = {ZES_STRUCTURE_TYPE_POWER_LIMIT_EXT_DESC,
                                       nullptr};
        power_limits_descriptors_initial.push_back(power_limits_descriptors[i]);

        if (power_limits_descriptors[i].level == ZES_POWER_LEVEL_SUSTAINED) {
          power_sustained_set = power_limits_descriptors[i];
          power_sustained_set.limit = power_limits_descriptors[i].limit - 1000;
          power_limits_descriptors[i].limit = power_sustained_set.limit;
        }
      }

      if (power_sustained_set.level == ZES_POWER_LEVEL_SUSTAINED) {
        if (power_sustained_set.limitValueLocked == false) {
          status = lzt::set_power_limits_ext(
              p_power_handle, &count_power,
              power_limits_descriptors.data()); // set power limits for all descriptors

          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          zes_power_limit_ext_desc_t power_sustained_get = {};

          std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_get;
          status = lzt::get_power_limits_ext(p_power_handle, &count_power,
                                             power_limits_descriptors_get);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          for (const auto& p_power_limits_descriptor_get :
               power_limits_descriptors_get) {
            if (p_power_limits_descriptor_get.level ==
                ZES_POWER_LEVEL_SUSTAINED) {
              power_sustained_get = p_power_limits_descriptor_get;
            }
          }

          EXPECT_EQ(power_sustained_get.limitValueLocked,
                    power_sustained_set.limitValueLocked);
          EXPECT_EQ(power_sustained_get.interval, power_sustained_set.interval);
          EXPECT_EQ(power_sustained_get.limit, power_sustained_set.limit);

          status = lzt::set_power_limits_ext(p_power_handle, &count_power,
                                    power_limits_descriptors_initial
                                        .data()); // restore initial limits
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        } else {
          LOG_INFO << "Set limit not supported due to sustained "
                      "limitValueLocked flag is true";
        }
      } else {
        LOG_INFO << "Sustained power limit not supported";
      }
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandleWhenSettingPowerValuesForPeakPowerThenExpectzesPowerSetLimitsExtFollowedByzesPowerGetLimitsExtToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      uint32_t count_power = 0;

      zes_power_limit_ext_desc_t power_peak_set = {};

      std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors;
      auto status = lzt::get_power_limits_ext(
          p_power_handle, &count_power,
          power_limits_descriptors); // get power limits for all descriptors
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      std::vector<zes_power_limit_ext_desc_t>
          power_limits_descriptors_initial; // preserve initial power limit
                                            // descriptors for restoration
                                            // later

      for (int i = 0; i < power_limits_descriptors.size(); i++) {
        power_limits_descriptors_initial.push_back(power_limits_descriptors[i]);

        if (power_limits_descriptors[i].level == ZES_POWER_LEVEL_PEAK) {
          power_peak_set = power_limits_descriptors[i];
          power_peak_set.limit = power_limits_descriptors[i].limit - 1000;
          power_limits_descriptors[i].limit = power_peak_set.limit;
        }
      }

      if (power_peak_set.limitValueLocked == false) {
        status = lzt::set_power_limits_ext(
            p_power_handle, &count_power,
            power_limits_descriptors.data()); // set power limits for all descriptors

        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        zes_power_limit_ext_desc_t power_peak_get = {};

        std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_get;
        status = lzt::get_power_limits_ext(p_power_handle, &count_power,
                                           power_limits_descriptors_get);
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        for (const auto& p_power_limits_descriptor_get :
             power_limits_descriptors_get) {
          if (p_power_limits_descriptor_get.level == ZES_POWER_LEVEL_PEAK) {
            power_peak_get = p_power_limits_descriptor_get;
          }
        }

        EXPECT_EQ(power_peak_get.limitValueLocked,
                  power_peak_set.limitValueLocked);
        EXPECT_EQ(power_peak_get.interval, power_peak_set.interval);
        EXPECT_EQ(power_peak_get.limit, power_peak_set.limit);

        status = lzt::set_power_limits_ext(
            p_power_handle, &count_power,
            power_limits_descriptors_initial.data()); // restore initial limits
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      } else {
        LOG_INFO << "Set limit not supported due to peak "
                    "limitValueLocked flag is true";
      }
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandleWhenSettingPowerValuesForBurstPowerThenExpectzesPowerSetLimitsExtFollowedByzesPowerGetLimitsExtToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      uint32_t count_power = 0;

      zes_power_limit_ext_desc_t power_burst_set = {};

      std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors; 
      auto status = lzt::get_power_limits_ext(
          p_power_handle, &count_power,
          power_limits_descriptors); // get power limits for all descriptors
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      std::vector<zes_power_limit_ext_desc_t>
          power_limits_descriptors_initial; // preserve initial power limit
                                            // descriptors for restoration
                                            // later

      for (int i = 0; i < power_limits_descriptors.size(); i++) {
        power_limits_descriptors_initial.push_back(power_limits_descriptors[i]);

        if (power_limits_descriptors[i].level == ZES_POWER_LEVEL_BURST) {
          power_burst_set = power_limits_descriptors[i];
          power_burst_set.limit = power_limits_descriptors[i].limit - 1000;
          power_limits_descriptors[i].limit = power_burst_set.limit;
        }
      }

      if (power_burst_set.limitValueLocked == false) {
        status = lzt::set_power_limits_ext(
            p_power_handle, &count_power,
            power_limits_descriptors.data()); // set power limits for all descriptors

        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        zes_power_limit_ext_desc_t power_burst_get = {};

        std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_get;
        status = lzt::get_power_limits_ext(p_power_handle, &count_power,
                                           power_limits_descriptors_get);
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        for (const auto& p_power_limits_descriptor_get :
             power_limits_descriptors_get) {
          if (p_power_limits_descriptor_get.level == ZES_POWER_LEVEL_BURST) {
            power_burst_get = p_power_limits_descriptor_get;
          }
        }

        EXPECT_EQ(power_burst_get.limitValueLocked,
                  power_burst_set.limitValueLocked);
        EXPECT_EQ(power_burst_get.interval, power_burst_set.interval);
        EXPECT_EQ(power_burst_get.limit, power_burst_set.limit);

        status = lzt::set_power_limits_ext(
            p_power_handle, &count_power,
            power_limits_descriptors_initial.data()); // restore initial limits
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      } else {
        LOG_INFO << "Set limit not supported due to burst "
                    "limitValueLocked flag is true";
      }
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandleWhenSettingPowerValuesForInstantaneousPowerThenExpectzesPowerSetLimitsExtFollowedByzesPowerGetLimitsExtToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto p_power_handle : p_power_handles) {
      EXPECT_NE(nullptr, p_power_handle);
      uint32_t count_power = 0;

      zes_power_limit_ext_desc_t power_instantaneous_set = {};

      std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors;
      auto status = lzt::get_power_limits_ext(
          p_power_handle, &count_power,
          power_limits_descriptors); // get power limits for all descriptors
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      std::vector<zes_power_limit_ext_desc_t>
          power_limits_descriptors_initial; // preserve initial power limit
                                            // descriptors for restoration
                                            // later

      for (int i = 0; i < power_limits_descriptors.size(); i++) {
        power_limits_descriptors_initial.push_back(power_limits_descriptors[i]);

        if (power_limits_descriptors[i].level ==
            ZES_POWER_LEVEL_INSTANTANEOUS) {
          power_instantaneous_set = power_limits_descriptors[i];
          power_instantaneous_set.limit =
              power_limits_descriptors[i].limit - 1000;
          power_limits_descriptors[i].limit = power_instantaneous_set.limit;
        }
      }

      if (power_instantaneous_set.limitValueLocked == false) {
        status = lzt::set_power_limits_ext(
            p_power_handle, &count_power,
            power_limits_descriptors.data()); // set power limits for all descriptors

        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        zes_power_limit_ext_desc_t power_instantaneous_get = {};

        std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_get;
        status = lzt::get_power_limits_ext(p_power_handle, &count_power, power_limits_descriptors_get);
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        for (const auto& p_power_limits_descriptor_get :
             power_limits_descriptors_get) {
          if (p_power_limits_descriptor_get.level ==
              ZES_POWER_LEVEL_INSTANTANEOUS) {
            power_instantaneous_get = p_power_limits_descriptor_get;
          }
        }

        EXPECT_EQ(power_instantaneous_get.limitValueLocked,
                  power_instantaneous_set.limitValueLocked);
        EXPECT_EQ(power_instantaneous_get.interval,
                  power_instantaneous_set.interval);
        EXPECT_EQ(power_instantaneous_get.limit, power_instantaneous_set.limit);

        status = lzt::set_power_limits_ext(
            p_power_handle, &count_power,
            power_limits_descriptors_initial.data()); // restore initial limits
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      } else {
        LOG_INFO << "Set limit not supported due to instantaneous "
                    "limitValueLocked flag is true";
      }
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidCardPowerHandleWhenRequestingPowerPropertiesThenExpectOnSubDeviceToReturnFalse) {
  for (auto device : devices) {
    auto p_power_handle = lzt::get_card_power_handle(device);
    if (p_power_handle == nullptr) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    auto p_properties = lzt::get_power_properties(p_power_handle);
    EXPECT_EQ(false, p_properties.onSubdevice);
  }
}
TEST_F(
    POWER_TEST,
    GivenValidCardPowerHandleWhenRequestingPowerLimitsThenExpectzesPowerGetLimitsExtToReturnSameValuesTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handle = lzt::get_card_power_handle(device);
    if (p_power_handle == nullptr) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    uint32_t count_power = 0;
    zes_power_limit_ext_desc_t power_peak_descriptor_initial = {};
    zes_power_limit_ext_desc_t power_burst_descriptor_initial = {};
    zes_power_limit_ext_desc_t power_sustained_descriptor_initial = {};
    zes_power_limit_ext_desc_t power_instantaneous_descriptor_initial = {};
    zes_power_limit_ext_desc_t power_peak_descriptor_later = {};
    zes_power_limit_ext_desc_t power_burst_descriptor_later = {};
    zes_power_limit_ext_desc_t power_sustained_descriptor_later = {};
    zes_power_limit_ext_desc_t power_instantaneous_descriptor_later = {};
    std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_initial;
    auto status = lzt::get_power_limits_ext(p_power_handle, &count_power,
                                            power_limits_descriptors_initial);
    if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      continue;
    }
    EXPECT_EQ(status, ZE_RESULT_SUCCESS);
    for (auto power_limits_descriptor_initial :
         power_limits_descriptors_initial) {
      if (power_limits_descriptor_initial.level == ZES_POWER_LEVEL_SUSTAINED) {
        power_sustained_descriptor_initial = power_limits_descriptor_initial;
      } else if (power_limits_descriptor_initial.level ==
                 ZES_POWER_LEVEL_PEAK) {
        power_peak_descriptor_initial = power_limits_descriptor_initial;
      } else if (power_limits_descriptor_initial.level ==
                 ZES_POWER_LEVEL_BURST) {
        power_burst_descriptor_initial = power_limits_descriptor_initial;
      } else if (power_limits_descriptor_initial.level ==
                 ZES_POWER_LEVEL_INSTANTANEOUS) {
        power_instantaneous_descriptor_initial =
            power_limits_descriptor_initial;
      }
    }
    std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_later;
    status = lzt::get_power_limits_ext(p_power_handle, &count_power,
                                       power_limits_descriptors_later);
    if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      continue;
    }
    EXPECT_EQ(status, ZE_RESULT_SUCCESS);
    for (auto power_limits_descriptor_later : power_limits_descriptors_later) {
      if (power_limits_descriptor_later.level == ZES_POWER_LEVEL_SUSTAINED) {
        power_sustained_descriptor_later = power_limits_descriptor_later;
      } else if (power_limits_descriptor_later.level == ZES_POWER_LEVEL_PEAK) {
        power_peak_descriptor_later = power_limits_descriptor_later;
      } else if (power_limits_descriptor_later.level == ZES_POWER_LEVEL_BURST) {
        power_burst_descriptor_later = power_limits_descriptor_later;
      } else if (power_limits_descriptor_later.level ==
                 ZES_POWER_LEVEL_INSTANTANEOUS) {
        power_instantaneous_descriptor_later = power_limits_descriptor_later;
      }
    }
    lzt::compare_power_descriptor_structures(power_sustained_descriptor_initial,
                                             power_sustained_descriptor_later);
    lzt::compare_power_descriptor_structures(power_peak_descriptor_initial,
                                             power_peak_descriptor_later);
    lzt::compare_power_descriptor_structures(power_burst_descriptor_initial,
                                             power_burst_descriptor_later);
    lzt::compare_power_descriptor_structures(
        power_instantaneous_descriptor_initial,
        power_instantaneous_descriptor_later);
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandleWhenRequestingEnergyCounterThenExpectEnergyConsumedByCardToBeGreaterThanOrEqualsToEnergyConsumedBySubdevices) {
  for (auto device : devices) {
    auto p_power_handle = lzt::get_card_power_handle(device);
    if (p_power_handle == nullptr) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    zes_power_energy_counter_t p_energy = {};
    lzt::get_power_energy_counter(p_power_handle, &p_energy);
    uint32_t total_subdevice_energy = 0;
    for (auto p_power_handle : p_power_handles) {
      auto pProperties = lzt::get_power_properties(p_power_handle);
      zes_power_energy_counter_t p_energy_subdevice = {};
      lzt::get_power_energy_counter(p_power_handle, &p_energy_subdevice);
      if (pProperties.onSubdevice == true) {
        total_subdevice_energy =
            total_subdevice_energy + p_energy_subdevice.energy;
      }
    }
    EXPECT_GE(p_energy.energy, total_subdevice_energy);
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandlesAfterGettingMaxPowerLimitsWhenSettingValuesForSustainedPowerThenExpectzesPowerGetLimitsExtToReturnPowerLimitsLessThanMaxPowerLimits) {
  for (auto device : devices) {
    auto power_card_handle = lzt::get_card_power_handle(device);
    if (power_card_handle == nullptr) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    uint32_t count_power = 0;
    bool sustained_limit_available = false;
    zes_power_limit_ext_desc_t power_sustained_Max = {};
    zes_power_limit_ext_desc_t power_sustained_Initial = {};
    zes_power_limit_ext_desc_t power_sustained_getMax = {};
    zes_power_limit_ext_desc_t power_sustained_get = {};
    std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors;
    auto status = lzt::get_power_limits_ext(power_card_handle, &count_power,
                                            power_limits_descriptors);
    if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      continue;
    }
    EXPECT_EQ(status, ZE_RESULT_SUCCESS);
    for (int i = 0; i < power_limits_descriptors.size(); i++) {
      if (power_limits_descriptors[i].level == ZES_POWER_LEVEL_SUSTAINED) {
        sustained_limit_available = true;
        power_sustained_Max = power_limits_descriptors[i];
        power_sustained_Initial = power_limits_descriptors[i];
        power_sustained_Max.limit = std::numeric_limits<int>::max();
        power_sustained_Initial.limit *= 2;

        if (power_sustained_Max.limitValueLocked == false) {
          status = lzt::set_power_limits_ext(power_card_handle, &count_power,
                                    &power_sustained_Max);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_getMax;
          status = lzt::get_power_limits_ext(power_card_handle, &count_power,
                                             power_limits_descriptors_getMax);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          for (const auto& p_power_limits_descriptor_get :
               power_limits_descriptors_getMax) {
            if (p_power_limits_descriptor_get.level ==
                ZES_POWER_LEVEL_SUSTAINED) {
              power_sustained_getMax = p_power_limits_descriptor_get;
            }
          }
          status = lzt::set_power_limits_ext(power_card_handle, &count_power,
                                    &power_sustained_Initial);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_get;
          status = lzt::get_power_limits_ext(power_card_handle, &count_power,
                                             power_limits_descriptors_get);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          for (const auto& p_power_limits_descriptor_get :
               power_limits_descriptors_get) {
            if (p_power_limits_descriptor_get.level ==
                ZES_POWER_LEVEL_SUSTAINED) {
              power_sustained_get = p_power_limits_descriptor_get;
            }
          }
          EXPECT_LE(power_sustained_get.limit, power_sustained_getMax.limit);
        } else {
          LOG_INFO << "Set limit not supported due to sustained "
                      "limitValueLocked flag is true";
        }
      }
    }
    if (!sustained_limit_available) {
      LOG_INFO << "Sustained power limit not supported";
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerHandlesAfterGettingMaxPowerLimitWhenSettingValuesForPeakPowerThenExpectzesPowerGetLimitsExtToReturnPowerLimitsLessThanMaxPowerLimits) {
  for (auto device : devices) {
    auto power_card_handle = lzt::get_card_power_handle(device);
    if (power_card_handle == nullptr) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    uint32_t count_power = 0;
    uint32_t single_count = 1;
    bool peak_limit_available = false;
    
    std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors;
    auto status = lzt::get_power_limits_ext(power_card_handle, &count_power,
                                  power_limits_descriptors);
    if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      continue;
    }
    EXPECT_EQ(status, ZE_RESULT_SUCCESS);
    for (int i = 0; i < power_limits_descriptors.size(); i++) {
      zes_power_limit_ext_desc_t power_peak_initial = {};
      zes_power_limit_ext_desc_t power_peak_Max = {};
      zes_power_limit_ext_desc_t power_peak_getMax = {};
      zes_power_limit_ext_desc_t power_peak_get = {};
      if (power_limits_descriptors[i].level == ZES_POWER_LEVEL_PEAK) {
        peak_limit_available = true;
        zes_power_source_t power_source = power_limits_descriptors[i].source;
        power_peak_Max = power_limits_descriptors[i];
        power_peak_initial = power_limits_descriptors[i];
        power_peak_Max.limit = std::numeric_limits<int>::max();
        power_peak_initial.limit *= 2;

        if (power_limits_descriptors[i].limitValueLocked == false) {
          status = lzt::set_power_limits_ext(power_card_handle, &single_count,
                                    &power_peak_Max);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          std::vector<zes_power_limit_ext_desc_t>
              power_limits_descriptors_getMax;
          status = lzt::get_power_limits_ext(power_card_handle, &count_power,
                                             power_limits_descriptors_getMax);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          for (const auto& p_power_limits_descriptor_get :
               power_limits_descriptors_getMax) {
            if (p_power_limits_descriptor_get.level == ZES_POWER_LEVEL_PEAK &&
                p_power_limits_descriptor_get.source == power_source) {
              power_peak_getMax = p_power_limits_descriptor_get;
            }
          }
          status = lzt::set_power_limits_ext(power_card_handle, &single_count,
                                    &power_peak_initial);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors_get;
          status = lzt::get_power_limits_ext(power_card_handle, &count_power,
                                             power_limits_descriptors_get);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          for (const auto& p_power_limits_descriptor_get :
               power_limits_descriptors_get) {
            if (p_power_limits_descriptor_get.level == ZES_POWER_LEVEL_PEAK &&
                p_power_limits_descriptor_get.source == power_source) {
              power_peak_get = p_power_limits_descriptor_get;
            }
          }
          EXPECT_LE(power_peak_get.limit, power_peak_getMax.limit);
        } else {
          LOG_INFO << "Set limit not supported due to peak "
                      "limitValueLocked flag is true";
        }
      }
    }
    if (!peak_limit_available) {
      LOG_INFO << "peak power limit not supported";
    }
  }
}

TEST_F(
    POWER_TEST,
    GivenPowerHandleWhenRequestingExtensionPowerPropertiesThenValidPowerDomainIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto p_power_handle : p_power_handles) {
      zes_power_properties_t pProperties = {ZES_STRUCTURE_TYPE_POWER_PROPERTIES,
                                            nullptr};
      zes_power_ext_properties_t pExtProperties = {
          ZES_STRUCTURE_TYPE_POWER_EXT_PROPERTIES, nullptr};
      pProperties.pNext = &pExtProperties;

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zesPowerGetProperties(p_power_handle, &pProperties));

      EXPECT_GT(pExtProperties.domain, ZES_POWER_DOMAIN_UNKNOWN);
      EXPECT_LT(pExtProperties.domain, ZES_POWER_DOMAIN_FORCE_UINT32);
    }
  }
}

TEST_F(
    POWER_TEST,
    GivenPowerHandleWhenRequestingExtensionPowerPropertiesThenValidDefaultLimitsAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto p_power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto p_power_handle : p_power_handles) {
      zes_power_properties_t pProperties = {ZES_STRUCTURE_TYPE_POWER_PROPERTIES,
                                            nullptr};
      zes_power_ext_properties_t pExtProperties = {
          ZES_STRUCTURE_TYPE_POWER_EXT_PROPERTIES, nullptr};
      zes_power_limit_ext_desc_t default_limits = {};
      pExtProperties.defaultLimit = &default_limits;
      pProperties.pNext = &pExtProperties;
      // query extension properties
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zesPowerGetProperties(p_power_handle, &pProperties));
      // verify default limits
      EXPECT_GE(pExtProperties.defaultLimit->level, ZES_POWER_LEVEL_UNKNOWN);
      EXPECT_LE(pExtProperties.defaultLimit->level,
                ZES_POWER_LEVEL_INSTANTANEOUS);
      EXPECT_GE(pExtProperties.defaultLimit->source, ZES_POWER_SOURCE_ANY);
      EXPECT_LE(pExtProperties.defaultLimit->source, ZES_POWER_SOURCE_BATTERY);
      EXPECT_GE(pExtProperties.defaultLimit->limitUnit, ZES_LIMIT_UNIT_UNKNOWN);
      EXPECT_LE(pExtProperties.defaultLimit->limitUnit, ZES_LIMIT_UNIT_POWER);
      if (!pExtProperties.defaultLimit->intervalValueLocked) {
        EXPECT_GE(pExtProperties.defaultLimit->interval, 0u);
        EXPECT_LE(pExtProperties.defaultLimit->interval, INT32_MAX);
      }
      if (!pExtProperties.defaultLimit->limitValueLocked) {
        EXPECT_GE(pExtProperties.defaultLimit->limit, 0u);
        EXPECT_LE(pExtProperties.defaultLimit->limit, INT32_MAX);
      }
    }
  }
}
TEST_F(
    POWER_TEST,
    GivenValidPowerAndPerformanceHandlesWhenIncreasingPerformanceFactorThenExpectTotalEnergyConsumedToBeIncreased) {
  for (auto device : devices) {
    uint32_t perf_count = 0;
    zes_power_energy_counter_t energy_counter_initial;
    zes_power_energy_counter_t energy_counter_later;
    auto p_power_handle = lzt::get_card_power_handle(device);
    if (p_power_handle == nullptr) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    auto p_performance_handles =
        lzt::get_performance_handles(device, perf_count);
    if (perf_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto p_performance_handle : p_performance_handles) {
      zes_perf_properties_t p_perf_properties =
          lzt::get_performance_properties(p_performance_handle);
      p_perf_properties.engines =
          1; // 1 is the equivalent value for ZES_ENGINE_TYPE_FLAG_OTHER
      if (p_perf_properties.engines == ZES_ENGINE_TYPE_FLAG_OTHER) {
        lzt::set_performance_config(p_performance_handle, 25);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lzt::get_power_energy_counter(p_power_handle, &energy_counter_initial);
        lzt::set_performance_config(p_performance_handle, 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lzt::get_power_energy_counter(p_power_handle, &energy_counter_later);
        EXPECT_GE(energy_counter_later.energy, energy_counter_initial.energy);
      }
    }
  }
}

} // namespace
