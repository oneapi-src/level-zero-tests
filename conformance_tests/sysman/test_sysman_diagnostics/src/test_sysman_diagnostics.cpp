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

void validate_tests(std::vector<zes_diag_test_t> tests) {
  uint32_t prev_index = 0;
  for (auto test : tests) {
    if (test.index != 0) {
      EXPECT_GT(test.index, prev_index);
      prev_index = test.index;
    }
    EXPECT_LE(std::strlen(test.name), ZES_STRING_PROPERTY_SIZE);
  }
}

class DiagnosticsTest : public lzt::SysmanCtsClass {};

TEST_F(
    DiagnosticsTest,
    GivenComponentCountZeroWhenRetrievingDiagnosticsHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    DiagnosticsTest,
    GivenComponentCountZeroWhenRetrievingDiagnosticsHandlesThenNotNullDiagnosticsHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diag_handles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(diag_handles.size(), count);
    for (auto diag_handle : diag_handles) {
      EXPECT_NE(nullptr, diag_handle);
    }
  }
}

TEST_F(
    DiagnosticsTest,
    GivenInvalidComponentCountWhenRetrievingDiagnosticsHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_diag_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_diag_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarDiagHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diag_handles_initial = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diag_handle : diag_handles_initial) {
      EXPECT_NE(nullptr, diag_handle);
    }

    count = 0;
    auto diag_handles_later = lzt::get_diag_handles(device, count);
    for (auto diag_handle : diag_handles_later) {
      EXPECT_NE(nullptr, diag_handle);
    }
    EXPECT_EQ(diag_handles_initial, diag_handles_later);
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidDiagHandleWhenRetrievingDiagPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto diag_handles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diag_handle : diag_handles) {
      ASSERT_NE(nullptr, diag_handle);
      auto properties = lzt::get_diag_properties(diag_handle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      EXPECT_LT(std::strlen(properties.name), ZES_STRING_PROPERTY_SIZE);
    }
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidDiagHandleWhenRetrievingDiagPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diag_handles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diag_handle : diag_handles) {
      ASSERT_NE(nullptr, diag_handle);
      auto properties_initial = lzt::get_diag_properties(diag_handle);
      auto properties_later = lzt::get_diag_properties(diag_handle);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      EXPECT_TRUE(0 == std::memcmp(properties_initial.name,
                                   properties_later.name,
                                   sizeof(properties_initial.name)));
      EXPECT_EQ(properties_initial.haveTests, properties_later.haveTests);
    }
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidDiagHandleWhenRetrievingDiagTestsThenExpectValidTestsToBeReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diag_handles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diag_handle : diag_handles) {
      ASSERT_NE(nullptr, diag_handle);
      auto properties = lzt::get_diag_properties(diag_handle);
      if (properties.haveTests == true) {
        count = 0;
        auto tests = lzt::get_diag_tests(diag_handle, count);
        ASSERT_EQ(tests.size(), 0);
        validate_tests(tests);
      }
    }
  }
}

TEST_F(DiagnosticsTest,
       GivenValidDiagTestsWhenRunningAllDiagTestsThenExpectTestsToRunFine) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diag_handles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diag_handle : diag_handles) {
      ASSERT_NE(nullptr, diag_handle);
      auto result = lzt::run_diag_tests(diag_handle, ZES_DIAG_FIRST_TEST_INDEX,
                                        ZES_DIAG_LAST_TEST_INDEX);
      EXPECT_GE(result, ZES_DIAG_RESULT_NO_ERRORS);
      EXPECT_LE(result, ZES_DIAG_RESULT_REBOOT_FOR_REPAIR);
    }
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidDiagTestsWhenRunningIndividualDiagTestsThenExpectTestsToRunFine) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diag_handles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diag_handle : diag_handles) {
      ASSERT_NE(nullptr, diag_handle);
      auto properties = lzt::get_diag_properties(diag_handle);
      if (properties.haveTests == true) {
        count = 0;
        auto tests = lzt::get_diag_tests(diag_handle, count);
        ASSERT_GT(tests.size(), 0);
        validate_tests(tests);

        for (auto test : tests) {
          auto result =
              lzt::run_diag_tests(diag_handle, test.index, test.index);
          EXPECT_GE(result, ZES_DIAG_RESULT_NO_ERRORS);
          EXPECT_LE(result, ZES_DIAG_RESULT_REBOOT_FOR_REPAIR);
        }
      } else {
        auto result = lzt::run_diag_tests(
            diag_handle, ZES_DIAG_FIRST_TEST_INDEX, ZES_DIAG_LAST_TEST_INDEX);
        EXPECT_GE(result, ZES_DIAG_RESULT_NO_ERRORS);
        EXPECT_LE(result, ZES_DIAG_RESULT_REBOOT_FOR_REPAIR);
      }
    }
  }
}
} // namespace
