/*
 *
 * Copyright (C) 2020-2024 Intel Corporation
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

void validate_ras_state_exp(zes_ras_state_exp_t ras_state) {
  std::vector<zes_ras_error_category_exp_t> error_category = {
      ZES_RAS_ERROR_CATEGORY_EXP_RESET,
      ZES_RAS_ERROR_CATEGORY_EXP_PROGRAMMING_ERRORS,
      ZES_RAS_ERROR_CATEGORY_EXP_DRIVER_ERRORS,
      ZES_RAS_ERROR_CATEGORY_EXP_COMPUTE_ERRORS,
      ZES_RAS_ERROR_CATEGORY_EXP_NON_COMPUTE_ERRORS,
      ZES_RAS_ERROR_CATEGORY_EXP_CACHE_ERRORS,
      ZES_RAS_ERROR_CATEGORY_EXP_DISPLAY_ERRORS,
      ZES_RAS_ERROR_CATEGORY_EXP_MEMORY_ERRORS,
      ZES_RAS_ERROR_CATEGORY_EXP_SCALE_ERRORS,
      ZES_RAS_ERROR_CATEGORY_EXP_L3FABRIC_ERRORS};

  EXPECT_NE(error_category.end(),
            std::find(error_category.begin(), error_category.end(),
                      ras_state.category));
  EXPECT_GE(ras_state.errorCounter, 0);
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

#ifdef USE_ZESINIT
class RASOperationsZesTest : public lzt::ZesSysmanCtsClass {};
#define RAS_TEST RASOperationsZesTest
#else // USE_ZESINIT
class RASOperationsTest : public lzt::SysmanCtsClass {};
#define RAS_TEST RASOperationsTest
#endif // USE_ZESINIT

TEST_F(RAS_TEST,
       GivenValidRASHandleWhenRetrievingConfigThenUpdatedConfigIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ras_handle : ras_handles) {
      ASSERT_NE(nullptr, ras_handle);
      zes_ras_config_t rasConfig = lzt::get_ras_config(ras_handle);
      validate_ras_config(rasConfig);
    }
  }
}

TEST_F(
    RAS_TEST,
    GivenValidRASHandleAndSettingNewConfigWhenRetrievingConfigThenUpdatedConfigIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ras_handle : ras_handles) {
      ASSERT_NE(nullptr, ras_handle);
      zes_ras_config_t rasConfigSet = {ZES_STRUCTURE_TYPE_RAS_CONFIG, nullptr};
      rasConfigSet.detailedThresholds = {ZES_STRUCTURE_TYPE_RAS_STATE, nullptr};
      zes_ras_config_t rasConfigOriginal = lzt::get_ras_config(ras_handle);
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
      lzt::set_ras_config(ras_handle, rasConfigSet);
      zes_ras_config_t rasConfigUpdated = lzt::get_ras_config(ras_handle);
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
      lzt::set_ras_config(ras_handle, rasConfigOriginal);
    }
  }
}

TEST_F(RAS_TEST,
       GivenValidRASHandleWhenRetrievingStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ras_handle : ras_handles) {
      ASSERT_NE(nullptr, ras_handle);
      ze_bool_t clear = 0;
      zes_ras_state_t rasState = lzt::get_ras_state(ras_handle, clear);
      validate_ras_state(rasState);
    }
  }
}

TEST_F(
    RAS_TEST,
    GivenValidRASHandleWhenRetrievingStateAfterClearThenUpdatedStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ras_handle : ras_handles) {
      ASSERT_NE(nullptr, ras_handle);
      ze_bool_t clear = 0;
      zes_ras_state_t rasState = lzt::get_ras_state(ras_handle, clear);
      validate_ras_state(rasState);
      uint64_t tErrorsinitial = lzt::sum_of_ras_errors(rasState);
      clear = 1;
      zes_ras_state_t rasStateAfterClear =
          lzt::get_ras_state(ras_handle, clear);
      validate_ras_state(rasStateAfterClear);
      uint64_t tErrorsAfterClear = lzt::sum_of_ras_errors(rasStateAfterClear);
      EXPECT_GE(tErrorsinitial, tErrorsAfterClear);
    }
  }
}

TEST_F(
    RAS_TEST,
    GivenValidRASHandleWhenRetrievingRASPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ras_handle : ras_handles) {
      ASSERT_NE(nullptr, ras_handle);
      auto properties_initial = lzt::get_ras_properties(ras_handle);
      auto properties_later = lzt::get_ras_properties(ras_handle);
      validate_ras_properties(properties_initial, deviceProperties);
      validate_ras_properties(properties_later, deviceProperties);
      EXPECT_EQ(properties_initial.type, properties_later.type);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
    }
  }
}

TEST_F(
    RAS_TEST,
    GivenComponentCountZeroWhenRetrievingRASHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ras_handle : ras_handles) {
      ASSERT_NE(nullptr, ras_handle);
    }
  }
}

TEST_F(RAS_TEST,
       GivenValidRASHandleWhenRetrievingStateExpThenValidStateExpIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ras_handle : ras_handles) {
      ASSERT_NE(nullptr, ras_handle);
      auto ras_states = lzt::ras_get_state_exp(ras_handle);
      for (auto state : ras_states) {
        validate_ras_state_exp(state);
      }
    }
  }
}

TEST_F(
    RAS_TEST,
    GivenValidRASHandleWhenRetrievingStateExpAfterInvokingClearstateExpThenUpdatedStateExpIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ras_handle : ras_handles) {
      ASSERT_NE(nullptr, ras_handle);
      uint32_t initial_errors = 0;
      uint32_t errors_after_clear = 0;
      auto ras_states = lzt::ras_get_state_exp(ras_handle);

      for (auto state : ras_states) {
        validate_ras_state_exp(state);
        initial_errors += state.errorCounter;
        lzt::ras_clear_state_exp(ras_handle, state.category);
      }

      ras_states = lzt::ras_get_state_exp(ras_handle);
      for (auto state : ras_states) {
        validate_ras_state_exp(state);
        errors_after_clear += state.errorCounter;
      }
      EXPECT_LE(errors_after_clear, initial_errors);
    }
  }
}
} // namespace
