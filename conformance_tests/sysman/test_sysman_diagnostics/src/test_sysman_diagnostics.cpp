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
  uint32_t prevIndex = 0;
  for (auto test : tests) {
    if (test.index != 0) {
      EXPECT_GT(test.index, prevIndex);
      prevIndex = test.index;
    }
    EXPECT_LE(std::strlen(test.name), ZES_STRING_PROPERTY_SIZE);
  }
}

class DiagnosticsTest : public lzt::SysmanCtsClass {};

TEST_F(
    DiagnosticsTest,
    GivenComponentCountZeroWhenRetrievingDiagnosticsHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    lzt::get_diag_handle_count(device);
  }
}

TEST_F(
    DiagnosticsTest,
    GivenComponentCountZeroWhenRetrievingDiagnosticsHandlesThenNotNullDiagnosticsHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diagHandles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(diagHandles.size(), count);
    for (auto diagHandle : diagHandles) {
      EXPECT_NE(nullptr, diagHandle);
    }
  }
}

TEST_F(
    DiagnosticsTest,
    GivenInvalidComponentCountWhenRetrievingDiagnosticsHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_diag_handles(device, actualCount);
    uint32_t testCount = actualCount + 1;
    lzt::get_diag_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarDiagHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diagHandlesInitial = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diagHandle : diagHandlesInitial) {
      EXPECT_NE(nullptr, diagHandle);
    }

    count = 0;
    auto diagHandlesLater = lzt::get_diag_handles(device, count);
    for (auto diagHandle : diagHandlesLater) {
      EXPECT_NE(nullptr, diagHandle);
    }
    EXPECT_EQ(diagHandlesInitial, diagHandlesLater);
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidDiagHandleWhenRetrievingDiagPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto diagHandles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diagHandle : diagHandles) {
      ASSERT_NE(nullptr, diagHandle);
      auto properties = lzt::get_diag_properties(diagHandle);
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
    auto diagHandles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diagHandle : diagHandles) {
      ASSERT_NE(nullptr, diagHandle);
      auto propertiesInitial = lzt::get_diag_properties(diagHandle);
      auto propertiesLater = lzt::get_diag_properties(diagHandle);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice && propertiesLater.onSubdevice) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      EXPECT_TRUE(0 == std::memcmp(propertiesInitial.name, propertiesLater.name,
                                   sizeof(propertiesInitial.name)));
      EXPECT_EQ(propertiesInitial.haveTests, propertiesLater.haveTests);
    }
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidDiagHandleWhenRetrievingDiagTestsThenExpectValidTestsToBeReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diagHandles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diagHandle : diagHandles) {
      ASSERT_NE(nullptr, diagHandle);
      auto properties = lzt::get_diag_properties(diagHandle);
      if (properties.haveTests == true) {
        count = 0;
        auto tests = lzt::get_diag_tests(diagHandle, count);
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
    auto diagHandles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diagHandle : diagHandles) {
      ASSERT_NE(nullptr, diagHandle);
      auto properties = lzt::get_diag_properties(diagHandle);
      if (properties.haveTests == true) {
        auto result = lzt::run_diag_tests(diagHandle, ZES_DIAG_FIRST_TEST_INDEX,
                                          ZES_DIAG_LAST_TEST_INDEX);
        EXPECT_GE(result, ZES_DIAG_RESULT_NO_ERRORS);
        EXPECT_LE(result, ZES_DIAG_RESULT_REBOOT_FOR_REPAIR);
      }
    }
  }
}

TEST_F(
    DiagnosticsTest,
    GivenValidDiagTestsWhenRunningIndividualDiagTestsThenExpectTestsToRunFine) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto diagHandles = lzt::get_diag_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto diagHandle : diagHandles) {
      ASSERT_NE(nullptr, diagHandle);
      auto properties = lzt::get_diag_properties(diagHandle);
      if (properties.haveTests == true) {
        count = 0;
        auto tests = lzt::get_diag_tests(diagHandle, count);
        ASSERT_GT(tests.size(), 0);
        validate_tests(tests);

        for (auto test : tests) {
          auto result = lzt::run_diag_tests(diagHandle, test.index, test.index);
          EXPECT_GE(result, ZES_DIAG_RESULT_NO_ERRORS);
          EXPECT_LE(result, ZES_DIAG_RESULT_REBOOT_FOR_REPAIR);
        }
      }
    }
  }
}

} // namespace
