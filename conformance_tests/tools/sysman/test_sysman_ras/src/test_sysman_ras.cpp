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
void validate_ras_details(zet_ras_details_t detailedThresholds) {
  EXPECT_LT(detailedThresholds.numCacheErrors, UINT64_MAX);
  EXPECT_LT(detailedThresholds.numComputeErrors, UINT64_MAX);
  EXPECT_LT(detailedThresholds.numDisplayErrors, UINT64_MAX);
  EXPECT_LT(detailedThresholds.numDriverErrors, UINT64_MAX);
  EXPECT_LT(detailedThresholds.numNonComputeErrors, UINT64_MAX);
  EXPECT_LT(detailedThresholds.numProgrammingErrors, UINT64_MAX);
  EXPECT_LT(detailedThresholds.numResets, UINT64_MAX);
  EXPECT_GE(detailedThresholds.numCacheErrors, 0);
  EXPECT_GE(detailedThresholds.numComputeErrors, 0);
  EXPECT_GE(detailedThresholds.numDisplayErrors, 0);
  EXPECT_GE(detailedThresholds.numDriverErrors, 0);
  EXPECT_GE(detailedThresholds.numNonComputeErrors, 0);
  EXPECT_GE(detailedThresholds.numProgrammingErrors, 0);
  EXPECT_GE(detailedThresholds.numResets, 0);
}

void validate_ras_config(zet_ras_config_t rasConfig) {
  EXPECT_LT(rasConfig.totalThreshold, UINT64_MAX);
  EXPECT_GT(rasConfig.totalThreshold, 0);
  EXPECT_GT(rasConfig.processId, 0);
  validate_ras_details(rasConfig.detailedThresholds);
}

void validate_ras_properties(zet_ras_properties_t rasProperties) {
  EXPECT_LE(rasProperties.type, ZET_RAS_ERROR_TYPE_UNCORRECTABLE);
  EXPECT_GE(rasProperties.type, ZET_RAS_ERROR_TYPE_CORRECTABLE);
  if (rasProperties.onSubdevice == true) {
    EXPECT_LT(rasProperties.subdeviceId, UINT32_MAX);
    EXPECT_GE(rasProperties.subdeviceId, 0);
  }
}
class RASOperationsTest : public lzt::SysmanCtsClass {};

TEST_F(RASOperationsTest,
       GivenValidRASHandleWhenRetrievingConfigThenUpdatedConfigIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      zet_ras_config_t rasConfig = lzt::get_ras_config(rasHandle);
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
    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      zet_ras_config_t rasConfigSet;
      zet_ras_config_t rasConfigOriginal = lzt::get_ras_config(rasHandle);
      validate_ras_config(rasConfigOriginal);
      rasConfigSet.totalThreshold = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.numCacheErrors = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.numComputeErrors = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.numDisplayErrors = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.numDriverErrors = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.numNonComputeErrors = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.numProgrammingErrors = (UINT64_MAX - 1);
      rasConfigSet.detailedThresholds.numResets = (UINT64_MAX - 1);
      lzt::set_ras_config(rasHandle, rasConfigSet);
      zet_ras_config_t rasConfigUpdated = lzt::get_ras_config(rasHandle);
      validate_ras_config(rasConfigUpdated);
      EXPECT_EQ(rasConfigSet.totalThreshold, rasConfigUpdated.totalThreshold);
      EXPECT_EQ(rasConfigSet.detailedThresholds.numCacheErrors,
                rasConfigUpdated.detailedThresholds.numCacheErrors);
      EXPECT_EQ(rasConfigSet.detailedThresholds.numComputeErrors,
                rasConfigUpdated.detailedThresholds.numComputeErrors);
      EXPECT_EQ(rasConfigSet.detailedThresholds.numDisplayErrors,
                rasConfigUpdated.detailedThresholds.numDisplayErrors);
      EXPECT_EQ(rasConfigSet.detailedThresholds.numDriverErrors,
                rasConfigUpdated.detailedThresholds.numDriverErrors);
      EXPECT_EQ(rasConfigSet.detailedThresholds.numNonComputeErrors,
                rasConfigUpdated.detailedThresholds.numNonComputeErrors);
      EXPECT_EQ(rasConfigSet.detailedThresholds.numProgrammingErrors,
                rasConfigUpdated.detailedThresholds.numProgrammingErrors);
      EXPECT_EQ(rasConfigSet.detailedThresholds.numResets,
                rasConfigUpdated.detailedThresholds.numResets);
      lzt::set_ras_config(rasHandle, rasConfigOriginal);
    }
  }
}

TEST_F(RASOperationsTest,
       GivenValidRASHandleWhenRetrievingStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      ze_bool_t clear = 0;
      zet_ras_details_t rasDetails = lzt::get_ras_state(rasHandle, clear);
      validate_ras_details(rasDetails);
    }
  }
}

TEST_F(
    RASOperationsTest,
    GivenValidRASHandleWhenRetrievingStateAfterClearThenUpdatedStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      ze_bool_t clear = 0;
      zet_ras_details_t rasDetails = lzt::get_ras_state(rasHandle, clear);
      validate_ras_details(rasDetails);
      uint64_t tErrorsinitial = lzt::sum_of_ras_errors(rasDetails);
      clear = 1;
      zet_ras_details_t rasDetailsAfterClear =
          lzt::get_ras_state(rasHandle, clear);
      validate_ras_details(rasDetailsAfterClear);
      uint64_t tErrorsAfterClear = lzt::sum_of_ras_errors(rasDetailsAfterClear);
      EXPECT_NE(tErrorsinitial, tErrorsAfterClear);
    }
  }
}

TEST_F(
    RASOperationsTest,
    GivenValidRASHandleWhenRetrievingRASPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
      auto propertiesInitial = lzt::get_ras_properties(rasHandle);
      auto propertiesLater = lzt::get_ras_properties(rasHandle);
      validate_ras_properties(propertiesInitial);
      validate_ras_properties(propertiesLater);
      EXPECT_EQ(propertiesInitial.type, propertiesLater.type);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice == true &&
          propertiesLater.onSubdevice == true) {
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
    for (auto rasHandle : rasHandles) {
      ASSERT_NE(nullptr, rasHandle);
    }
  }
}
} // namespace
