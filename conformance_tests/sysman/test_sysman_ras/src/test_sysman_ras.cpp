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

namespace {
void validate_ras_state(zes_ras_state_t detailedThresholds) {
  EXPECT_LT(detailedThresholds.category[ZES_RAS_ERROR_CAT_RESET], UINT64_MAX);
  EXPECT_LT(detailedThresholds.category[ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS],
            UINT64_MAX);
  EXPECT_LT(detailedThresholds.category[ZES_RAS_ERROR_CAT_DRIVER_ERRORS],
            UINT64_MAX);
  EXPECT_LT(detailedThresholds.category[ZES_RAS_ERROR_CAT_COMPUTE_ERRORS],
            UINT64_MAX);
  EXPECT_LT(detailedThresholds.category[ZES_RAS_ERROR_CAT_NON_COMPUTE_ERRORS],
            UINT64_MAX);
  EXPECT_LT(detailedThresholds.category[ZES_RAS_ERROR_CAT_CACHE_ERRORS],
            UINT64_MAX);
  EXPECT_LT(detailedThresholds.category[ZES_RAS_ERROR_CAT_DISPLAY_ERRORS],
            UINT64_MAX);
  EXPECT_GE(detailedThresholds.category[ZES_RAS_ERROR_CAT_RESET], 0);
  EXPECT_GE(detailedThresholds.category[ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS],
            0);
  EXPECT_GE(detailedThresholds.category[ZES_RAS_ERROR_CAT_DRIVER_ERRORS], 0);
  EXPECT_GE(detailedThresholds.category[ZES_RAS_ERROR_CAT_COMPUTE_ERRORS], 0);
  EXPECT_GE(detailedThresholds.category[ZES_RAS_ERROR_CAT_CACHE_ERRORS], 0);
  EXPECT_GE(detailedThresholds.category[ZES_RAS_ERROR_CAT_DISPLAY_ERRORS], 0);
  EXPECT_GE(detailedThresholds.category[ZES_RAS_ERROR_CAT_RESET], 0);
}

void validate_ras_config(zes_ras_config_t rasConfig) {
  EXPECT_LT(rasConfig.totalThreshold, UINT64_MAX);
  EXPECT_GE(rasConfig.totalThreshold, 0);
  validate_ras_state(rasConfig.detailedThresholds);
}

void validate_ras_properties(zes_ras_properties_t rasProperties,
                             zes_device_properties_t deviceProperties) {
  EXPECT_LE(rasProperties.type, ZES_RAS_ERROR_TYPE_UNCORRECTABLE);
  EXPECT_GE(rasProperties.type, ZES_RAS_ERROR_TYPE_CORRECTABLE);
  if (rasProperties.onSubdevice) {
    EXPECT_LT(rasProperties.subdeviceId, deviceProperties.numSubdevices);
  }
}
class RASOperationsTest : public lzt::SysmanCtsClass {};

TEST_F(RASOperationsTest,
       GivenValidRASHandleWhenRetrievingConfigThenUpdatedConfigIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      zes_ras_config_t rasConfig = lzt::get_ras_config(rasHandle);
      validate_ras_config(rasConfig);
    }
  }
}

TEST_F(
    RASOperationsTest,
    GivenValidRASHandleAndSettingNewConfigWhenRetrievingConfigThenUpdatedConfigIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      zes_ras_config_t rasConfigSet = {ZES_STRUCTURE_TYPE_RAS_CONFIG, nullptr};
      rasConfigSet.detailedThresholds = {ZES_STRUCTURE_TYPE_RAS_STATE, nullptr};
      zes_ras_config_t rasConfigOriginal = lzt::get_ras_config(rasHandle);
      validate_ras_config(rasConfigOriginal);
      rasConfigSet.totalThreshold = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.category[ZES_RAS_ERROR_CAT_RESET] =
          (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds
          .category[ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS] = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds
          .category[ZES_RAS_ERROR_CAT_DRIVER_ERRORS] = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds
          .category[ZES_RAS_ERROR_CAT_COMPUTE_ERRORS] = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds
          .category[ZES_RAS_ERROR_CAT_NON_COMPUTE_ERRORS] = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.category[ZES_RAS_ERROR_CAT_CACHE_ERRORS] =
          (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds
          .category[ZES_RAS_ERROR_CAT_DISPLAY_ERRORS] = (UINT64_MAX - 1);
      lzt::set_ras_config(rasHandle, rasConfigSet);
      zes_ras_config_t rasConfigUpdated = lzt::get_ras_config(rasHandle);
      validate_ras_config(rasConfigUpdated);
      EXPECT_EQ(rasConfigSet.totalThreshold, rasConfigUpdated.totalThreshold);
      EXPECT_EQ(
          rasConfigSet.detailedThresholds.category[ZES_RAS_ERROR_CAT_RESET],
          rasConfigUpdated.detailedThresholds
              .category[ZES_RAS_ERROR_CAT_RESET]);
      EXPECT_EQ(rasConfigSet.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS],
                rasConfigUpdated.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS]);
      EXPECT_EQ(rasConfigSet.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_DRIVER_ERRORS],
                rasConfigUpdated.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_DRIVER_ERRORS]);
      EXPECT_EQ(rasConfigSet.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_COMPUTE_ERRORS],
                rasConfigUpdated.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_COMPUTE_ERRORS]);
      EXPECT_EQ(rasConfigSet.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_NON_COMPUTE_ERRORS],
                rasConfigUpdated.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_NON_COMPUTE_ERRORS]);
      EXPECT_EQ(rasConfigSet.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_CACHE_ERRORS],
                rasConfigUpdated.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_CACHE_ERRORS]);
      EXPECT_EQ(rasConfigSet.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_DISPLAY_ERRORS],
                rasConfigUpdated.detailedThresholds
                    .category[ZES_RAS_ERROR_CAT_DISPLAY_ERRORS]);
      lzt::set_ras_config(rasHandle, rasConfigOriginal);
    }
  }
}

TEST_F(RASOperationsTest,
       GivenValidRASHandleWhenRetrievingStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      ze_bool_t clear = 0;
      zes_ras_state_t rasState = lzt::get_ras_state(rasHandle, clear);
      validate_ras_state(rasState);
    }
  }
}

TEST_F(
    RASOperationsTest,
    GivenValidRASHandleWhenRetrievingStateAfterClearThenUpdatedStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      ze_bool_t clear = 0;
      zes_ras_state_t rasState = lzt::get_ras_state(rasHandle, clear);
      validate_ras_state(rasState);
      uint64_t tErrorsinitial = lzt::sum_of_ras_errors(rasState);
      clear = 1;
      zes_ras_state_t rasStateAfterClear = lzt::get_ras_state(rasHandle, clear);
      validate_ras_state(rasStateAfterClear);
      uint64_t tErrorsAfterClear = lzt::sum_of_ras_errors(rasStateAfterClear);
      EXPECT_GE(tErrorsinitial, tErrorsAfterClear);
    }
  }
}

TEST_F(
    RASOperationsTest,
    GivenValidRASHandleWhenRetrievingRASPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      auto propertiesInitial = lzt::get_ras_properties(rasHandle);
      auto propertiesLater = lzt::get_ras_properties(rasHandle);
      validate_ras_properties(propertiesInitial, deviceProperties);
      validate_ras_properties(propertiesLater, deviceProperties);
      EXPECT_EQ(propertiesInitial.type, propertiesLater.type);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice && propertiesLater.onSubdevice) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
    }
  }
}

TEST_F(
    RASOperationsTest,
    GivenComponentCountZeroWhenRetrievingRASHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
    }
  }
}
} // namespace
