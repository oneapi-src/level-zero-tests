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

#ifdef USE_ZESINIT
class DiagnosticsZesTest : public lzt::ZesSysmanCtsClass {
public:
  bool is_diag_supported = false;
};
#define DIAGNOSTICS_TEST DiagnosticsZesTest
#else // USE_ZESINIT
class DiagnosticsTest : public lzt::SysmanCtsClass {
public:
  bool is_diag_supported = false;
};
#define DIAGNOSTICS_TEST DiagnosticsTest
#endif // USE_ZESINIT

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenComponentCountZeroWhenRetrievingDiagnosticsHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenComponentCountZeroWhenRetrievingDiagnosticsHandlesThenNotNullDiagnosticsHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
      auto diag_handles = lzt::get_diag_handles(device, count);
      ASSERT_EQ(diag_handles.size(), count);
      for (auto diag_handle : diag_handles) {
        EXPECT_NE(nullptr, diag_handle);
      }
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenInvalidComponentCountWhenRetrievingDiagnosticsHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    actual_count = lzt::get_diag_handle_count(device);
    if (actual_count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
      lzt::get_diag_handles(device, actual_count);
      uint32_t test_count = actual_count + 1;
      lzt::get_diag_handles(device, test_count);
      EXPECT_EQ(test_count, actual_count);
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarDiagHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
      auto diag_handles_initial = lzt::get_diag_handles(device, count);
      for (auto diag_handle : diag_handles_initial) {
        EXPECT_NE(nullptr, diag_handle);
      }
      count = 0;
      auto diag_handles_later = lzt::get_diag_handles(device, count);
      for (auto diag_handle : diag_handles_later) {
        EXPECT_NE(nullptr, diag_handle);
      }
      EXPECT_EQ(diag_handles_initial, diag_handles_later);
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenValidDiagHandleWhenRetrievingDiagPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this devcie! ";
      auto diag_handles = lzt::get_diag_handles(device, count);
      for (auto diag_handle : diag_handles) {
        ASSERT_NE(nullptr, diag_handle);
        auto properties = lzt::get_diag_properties(diag_handle);
        if (properties.onSubdevice) {
          EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
        }
        EXPECT_LT(std::strlen(properties.name), ZES_STRING_PROPERTY_SIZE);
      }
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenValidDiagHandleWhenRetrievingDiagPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
      auto diag_handles = lzt::get_diag_handles(device, count);
      for (auto diag_handle : diag_handles) {
        ASSERT_NE(nullptr, diag_handle);
        auto properties_initial = lzt::get_diag_properties(diag_handle);
        auto properties_later = lzt::get_diag_properties(diag_handle);
        EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
        if (properties_initial.onSubdevice && properties_later.onSubdevice) {
          EXPECT_EQ(properties_initial.subdeviceId,
                    properties_later.subdeviceId);
        }
        EXPECT_TRUE(0 == std::memcmp(properties_initial.name,
                                     properties_later.name,
                                     sizeof(properties_initial.name)));
        EXPECT_EQ(properties_initial.haveTests, properties_later.haveTests);
      }
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenValidDiagHandleWhenRetrievingDiagTestsThenExpectValidTestsToBeReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
      auto diag_handles = lzt::get_diag_handles(device, count);
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
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(DIAGNOSTICS_TEST,
           GivenValidDiagTestsWhenRunningAllDiagTestsThenExpectTestsToRunFine) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
      auto diag_handles = lzt::get_diag_handles(device, count);
      for (auto diag_handle : diag_handles) {
        ASSERT_NE(nullptr, diag_handle);
        auto result = lzt::run_diag_tests(
            diag_handle, ZES_DIAG_FIRST_TEST_INDEX, ZES_DIAG_LAST_TEST_INDEX);
        EXPECT_GE(result, ZES_DIAG_RESULT_NO_ERRORS);
        EXPECT_LE(result, ZES_DIAG_RESULT_REBOOT_FOR_REPAIR);
      }
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenValidDiagTestsWhenRunningIndividualDiagTestsThenExpectTestsToRunFine) {
  for (auto device : devices) {
    uint32_t count = 0;
    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
      auto diag_handles = lzt::get_diag_handles(device, count);
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
    } else {
      LOG_INFO << "No diagnostics handles found for this device! ";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    DIAGNOSTICS_TEST,
    GivenValidDeviceWhenMemoryDiagnosticsIsRunAndMemoryGetStateIsCalledThenExpectSuccessIsReturned) {
  for (auto &device : devices) {
    uint32_t count = 0;
    count = lzt::get_diag_handle_count(device);
    uint32_t mem_count_initial = 0;
    std::vector<zes_mem_handle_t> mem_handles_initial =
        lzt::get_mem_handles(device, mem_count_initial);

    std::vector<zes_mem_health_t> mem_health_initial{};
    for (auto &mem_handle : mem_handles_initial) {
      ASSERT_NE(nullptr, mem_handle);
      auto state = lzt::get_mem_state(mem_handle);
      mem_health_initial.push_back(state.health);
    }

    if (count > 0) {
      is_diag_supported = true;
      LOG_INFO << "Diagnostics handles are available on this device! ";
      auto diag_handles = lzt::get_diag_handles(device, count);
      bool mem_diag_available = false;
      for (auto &diag_handle : diag_handles) {
        ASSERT_NE(nullptr, diag_handle);
        auto properties = lzt::get_diag_properties(diag_handle);
        if (strcmp(properties.name, "MEMORY_PPR") == 0) {
          mem_diag_available = true;
          auto result = lzt::run_diag_tests(
              diag_handle, ZES_DIAG_FIRST_TEST_INDEX, ZES_DIAG_LAST_TEST_INDEX);
          EXPECT_EQ(result, ZES_DIAG_RESULT_NO_ERRORS);
          // get memory state after memory diagnostics
          uint32_t mem_count_later = 0;
          std::vector<zes_mem_handle_t> mem_handles_later =
              lzt::get_mem_handles(device, mem_count_later);
          EXPECT_EQ(mem_count_initial, mem_count_later);

          std::vector<zes_mem_health_t> mem_health_later{};
          for (auto &mem_handle : mem_handles_later) {
            ASSERT_NE(nullptr, mem_handle);
            auto state = lzt::get_mem_state(mem_handle);
            mem_health_later.push_back(state.health);
          }
          EXPECT_TRUE(std::equal(mem_health_initial.begin(),
                                 mem_health_initial.end(),
                                 mem_health_later.begin()));
        }
      }
      if (!mem_diag_available) {
        FAIL() << "No Memory Diagnostics found for this device! ";
      }
    } else {
      LOG_WARNING << "No diagnostics handles found for this device!";
    }
  }
  if (!is_diag_supported) {
    FAIL() << "No diagnostics handles found on any of the devices!";
  }
}

} // namespace
