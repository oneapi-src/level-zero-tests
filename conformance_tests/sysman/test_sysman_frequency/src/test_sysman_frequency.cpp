/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include <thread>
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "math.h"

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

#ifdef USE_ZESINIT
class FrequencyModuleZesTest : public lzt::ZesSysmanCtsClass {};
#define FREQUENCY_TEST FrequencyModuleZesTest
#else // USE_ZESINIT
class FrequencyModuleTest : public lzt::SysmanCtsClass {};
#define FREQUENCY_TEST FrequencyModuleTest
#endif // USE_ZESINIT

TEST_F(
    FREQUENCY_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    lzt::get_freq_handle_count(device);
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFrequencyHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = p_count + 1;
    lzt::get_freq_handles(device, test_count);
    EXPECT_EQ(test_count, p_count);
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarFrequencyHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles_initial = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles_initial) {
      EXPECT_NE(nullptr, pfreq_handle);
    }
    count = 0;
    auto pfreq_handles_later = lzt::get_freq_handles(device, count);
    for (auto pfreq_handle : pfreq_handles_later) {
      EXPECT_NE(nullptr, pfreq_handle);
    }
    EXPECT_EQ(pfreq_handles_initial, pfreq_handles_later);
  }
}

TEST_F(FREQUENCY_TEST,
       GivenValidDeviceWhenRetrievingFreqStateThenValidFreqStatesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      zes_freq_state_t pState = lzt::get_freq_state(pfreq_handle);
      lzt::validate_freq_state(pfreq_handle, pState);
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFreqRangeWhenRetrievingFreqStateThenValidFreqStatesAreReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      zes_freq_range_t limits = {};
      zes_freq_properties_t properties = {};
      properties = lzt::get_freq_properties(pfreq_handle);
      limits.min = properties.min;
      limits.max = properties.max;
      lzt::set_freq_range(pfreq_handle, limits);
      lzt::idle_check(pfreq_handle);
      zes_freq_state_t state = lzt::get_freq_state(pfreq_handle);
      lzt::validate_freq_state(pfreq_handle, state);
      if (state.actual > 0) {
        EXPECT_LE(state.actual, limits.max);
        EXPECT_GE(state.actual, limits.min);
      } else {
        LOG_INFO << "Actual frequency is unknown";
      }
      if (state.request > 0) {
        EXPECT_LE(state.request, limits.max);
        EXPECT_GE(state.request, limits.min);
      } else {
        LOG_INFO << "Requested frequency is unknown";
      }
    }
  }
}
TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyHandleWhenRetrievingAvailableClocksThenSuccessAndSameValuesAreReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      uint32_t icount = 0;
      uint32_t lcount = 0;
      auto pFrequencyInitial = lzt::get_available_clocks(pfreq_handle, icount);
      auto pFrequencyLater = lzt::get_available_clocks(pfreq_handle, lcount);
      EXPECT_EQ(pFrequencyInitial, pFrequencyLater);
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyHandleWhenRetrievingAvailableClocksThenPositiveAndValidValuesAreReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      uint32_t count = 0;
      zes_freq_properties_t freq_property =
          lzt::get_freq_properties(pfreq_handle);

      auto pFrequency = lzt::get_available_clocks(pfreq_handle, count);

      for (uint32_t i = 0; i < pFrequency.size(); i++) {
        if (pFrequency[i] != -1) {
          EXPECT_GE(pFrequency[i], freq_property.min);
          EXPECT_LE(pFrequency[i], freq_property.max);
        }
        if (i > 0)
          EXPECT_GE(
              pFrequency[i],
              pFrequency[i - 1]); // Each entry in array of pFrequency, should
                                  // be less than or equal to next entry
      }
    }
  }
}
TEST_F(FREQUENCY_TEST,
       GivenClocksCountWhenRetrievingAvailableClocksThenActualCountIsUpdated) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      uint32_t p_count = 0;
      p_count = lzt::get_available_clock_count(pfreq_handle);
      uint32_t tCount = p_count + 1;
      lzt::get_available_clocks(pfreq_handle, tCount);
      EXPECT_EQ(p_count, tCount);
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyHandleWhenRequestingDeviceGPUTypeThenExpectCanControlPropertyToBeTrue) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      zes_freq_properties_t properties = {};
      properties = lzt::get_freq_properties(pfreq_handle);
      if (properties.type == ZES_FREQ_DOMAIN_GPU)
        EXPECT_TRUE(properties.canControl);
      else if (properties.type == ZES_FREQ_DOMAIN_MEMORY)
        EXPECT_FALSE(properties.canControl);
      else if (properties.type == ZES_FREQ_DOMAIN_MEDIA)
        EXPECT_TRUE(properties.canControl);
      else {
        LOG_INFO << "Skipping test as freq handle is of unknown type";
        GTEST_SKIP();
      }
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyHandleWhenRequestingFrequencyPropertiesThenExpectPositiveFrequencyRange) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      zes_freq_properties_t properties = {};
      properties = lzt::get_freq_properties(pfreq_handle);
      if (properties.max != -1) {
        EXPECT_GT(properties.max, 0);
      }
      if (properties.min != -1) {
        EXPECT_GT(properties.min, 0);
      }
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenSameFrequencyHandleWhenRequestingFrequencyPropertiesThenExpectSamePropertiesOnMultipleCalls) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
    }
    std::vector<zes_freq_properties_t> properties(3);

    for (uint32_t i = 0; i < 3; i++)
      properties[i] = lzt::get_freq_properties(pfreq_handles[0]);

    ASSERT_GT(properties.size(), 1);
    for (uint32_t i = 1; i < properties.size(); i++) {
      EXPECT_EQ(properties[0].type, properties[i].type);
      EXPECT_EQ(properties[0].onSubdevice, properties[i].onSubdevice);
      if (properties[0].onSubdevice && properties[i].onSubdevice)
        EXPECT_EQ(properties[0].subdeviceId, properties[i].subdeviceId);
      EXPECT_EQ(properties[0].canControl, properties[i].canControl);
      EXPECT_EQ(properties[0].isThrottleEventSupported,
                properties[i].isThrottleEventSupported);
      EXPECT_EQ(properties[0].max, properties[i].max);
      EXPECT_EQ(properties[0].min, properties[i].min);
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyCountWhenRequestingFrequencyHandleThenExpectzesSysmanFrequencyGetRangeToReturnSuccessOnMultipleCalls) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      zes_freq_range_t freqRange = {};
      for (uint32_t i = 0; i < 3; i++)
        freqRange = lzt::get_freq_range(pfreq_handle);
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenSameFrequencyHandleWhenRequestingFrequencyRangeThenExpectSameRangeOnMultipleCalls) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      std::vector<zes_freq_range_t> freqRangeToCompare;
      for (uint32_t i = 0; i < 3; i++)
        freqRangeToCompare.push_back(lzt::get_freq_range(pfreq_handle));

      for (uint32_t i = 1; i < freqRangeToCompare.size(); i++) {
        EXPECT_EQ(freqRangeToCompare[0].max, freqRangeToCompare[i].max);
        EXPECT_EQ(freqRangeToCompare[0].min, freqRangeToCompare[i].min);
      }
    }
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyCountWhenRequestingFrequencyHandleThenExpectzesSysmanFrequencyGetRangeToReturnValidFrequencyRanges) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    zes_freq_range_t freqRange = {};
    for (auto pfreq_handle : pfreq_handles)
      freqRange = lzt::get_and_validate_freq_range(pfreq_handle);
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyRangeWhenRequestingSetFrequencyThenExpectUpdatedFrequencyInGetFrequencyCall) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);

      zes_freq_range_t freqRange = {};
      zes_freq_range_t freqRangeReset = {};
      uint32_t count = 0;
      auto frequency = lzt::get_available_clocks(pfreq_handle, count);
      ASSERT_GT(frequency.size(), 0);
      if (count == 1) {
        freqRange.min = frequency[0];
        freqRange.max = frequency[0];
        lzt::set_freq_range(pfreq_handle, freqRange);
        freqRangeReset = lzt::get_and_validate_freq_range(pfreq_handle);
        EXPECT_EQ(freqRange.max, freqRangeReset.max);
        EXPECT_EQ(freqRange.min, freqRangeReset.min);
      } else {
        for (uint32_t i = 1; i < count; i++) {
          freqRange.min = frequency[i - 1];
          freqRange.max = frequency[i];
          lzt::set_freq_range(pfreq_handle, freqRange);
          freqRangeReset = lzt::get_and_validate_freq_range(pfreq_handle);
          EXPECT_EQ(freqRange.max, freqRangeReset.max);
          EXPECT_EQ(freqRange.min, freqRangeReset.min);
        }
      }
    }
  }
}

std::string get_freq_domain(zes_freq_domain_t domain) {
  switch (domain) {
  case zes_freq_domain_t::ZES_FREQ_DOMAIN_GPU:
    return "ZES_FREQ_DOMAIN_GPU";
    break;
  case zes_freq_domain_t::ZES_FREQ_DOMAIN_MEMORY:
    return "ZES_FREQ_DOMAIN_MEMORY";
    break;
  case zes_freq_domain_t::ZES_FREQ_DOMAIN_MEDIA:
    return "ZES_FREQ_DOMAIN_MEDIA";
    break;
  default:
    return "ZES_FREQ_DOMAIN_FORCE_UINT32";
  }
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyHandleWhenRequestingSetFrequencyWithInvalidRangeThenExpectMinAndMaxFrequencyAreClampedToHardwareLimits) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);

      zes_freq_range_t invalid_freq_range = {};
      zes_freq_range_t clamped_freq_range = {};

      zes_freq_properties_t freq_property =
          lzt::get_freq_properties(pfreq_handle);

      if (freq_property.canControl) {
        if (freq_property.min - 100 >= 0) {
          invalid_freq_range.min = freq_property.min - 100;
        } else {
          invalid_freq_range.min = 0;
        }
        invalid_freq_range.max = freq_property.max + 100;

        lzt::set_freq_range(pfreq_handle, invalid_freq_range);
        clamped_freq_range = lzt::get_and_validate_freq_range(pfreq_handle);
        EXPECT_EQ(freq_property.min, clamped_freq_range.min);
        EXPECT_EQ(freq_property.max, clamped_freq_range.max);
      } else {
        LOG_WARNING << "User cannot control min/max frequency setting for "
                       "frequency domain "
                    << get_freq_domain(freq_property.type);
      }
    }
  }
}

void load_for_gpu(ze_device_handle_t target_device) {
  int m, k, n;
  m = k = n = 5000;
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);
  const ze_device_handle_t device = target_device;
  void *a_buffer = lzt::allocate_host_memory(m * k * sizeof(float));
  void *b_buffer = lzt::allocate_host_memory(k * n * sizeof(float));
  void *c_buffer = lzt::allocate_host_memory(m * n * sizeof(float));
  ze_module_handle_t module =
      lzt::create_module(device, "sysman_matrix_multiplication.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "sysman_matrix_multiplication");
  lzt::set_group_size(function, 16, 16, 1);
  lzt::set_argument_value(function, 0, sizeof(a_buffer), &a_buffer);
  lzt::set_argument_value(function, 1, sizeof(b_buffer), &b_buffer);
  lzt::set_argument_value(function, 2, sizeof(m), &m);
  lzt::set_argument_value(function, 3, sizeof(k), &k);
  lzt::set_argument_value(function, 4, sizeof(n), &n);
  lzt::set_argument_value(function, 5, sizeof(c_buffer), &c_buffer);
  ze_command_list_handle_t cmd_list = lzt::create_command_list(device);
  std::memcpy(a_buffer, a.data(), a.size() * sizeof(float));
  std::memcpy(b_buffer, b.data(), b.size() * sizeof(float));
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  const int group_count_x = m / 16;
  const int group_count_y = n / 16;
  ze_group_count_t tg;
  tg.groupCountX = group_count_x;
  tg.groupCountY = group_count_y;
  tg.groupCountZ = 1;
  zeCommandListAppendLaunchKernel(cmd_list, function, &tg, nullptr, 0, nullptr);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_list);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(device);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
  lzt::synchronize(cmd_q, UINT64_MAX);
  std::memcpy(c.data(), c_buffer, c.size() * sizeof(float));
  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_function(function);
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);
  lzt::destroy_module(module);
}
void get_throttle_time_init(zes_freq_handle_t pfreq_handle,
                            zes_freq_throttle_time_t &throttletime) {
  EXPECT_EQ(lzt::check_for_throttling(pfreq_handle), true);
  throttletime = lzt::get_throttle_time(pfreq_handle);
  EXPECT_GT(throttletime.throttleTime, 0);
  EXPECT_GT(throttletime.timestamp, 0);
}

#ifdef USE_ZESINIT
bool is_uuid_pair_equal(uint8_t *uuid1, uint8_t *uuid2) {
  for (uint32_t i = 0; i < ZE_MAX_UUID_SIZE; i++) {
    if (uuid1[i] != uuid2[i]) {
      return false;
    }
  }
  return true;
}
ze_device_handle_t get_core_device_by_uuid(uint8_t *uuid) {
  lzt::initialize_core();
  auto driver = lzt::zeDevice::get_instance()->get_driver();
  auto core_devices = lzt::get_ze_devices(driver);
  for (auto device : core_devices) {
    auto device_properties = lzt::get_device_properties(device);
    if (is_uuid_pair_equal(uuid, device_properties.uuid.id)) {
      return device;
    }
  }
  return nullptr;
}
#endif // USE_ZESINIT

TEST_F(FREQUENCY_TEST, GivenValidFrequencyHandleThenCheckForThrottling) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);
      {
        uint32_t p_count = 0;
        auto p_power_handles = lzt::get_power_handles(device, p_count);
        if (p_count == 0) {
          FAIL() << "No handles found: "
                 << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
        }
        for (auto p_power_handle : p_power_handles) {
          EXPECT_NE(nullptr, p_power_handle);
          zes_power_sustained_limit_t power_sustained_limit_Initial;
          auto status = lzt::get_power_limits(p_power_handle, &power_sustained_limit_Initial,
                                nullptr, nullptr);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          zes_power_sustained_limit_t power_sustained_limit_set;
          power_sustained_limit_set.power = 0;
          status = lzt::set_power_limits(p_power_handle, &power_sustained_limit_set,
                                nullptr, nullptr);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
          auto before_throttletime = lzt::get_throttle_time(pfreq_handle);
          zes_freq_throttle_time_t throttletime;
#ifdef USE_ZESINIT
          auto sysman_device_properties =
              lzt::get_sysman_device_properties(device);
          ze_device_handle_t core_device =
              get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
          EXPECT_NE(core_device, nullptr);
          std::thread first(load_for_gpu, core_device);
#else  // USE_ZESINIT
          std::thread first(load_for_gpu, device);
#endif // USE_ZESINIT
          std::thread second(get_throttle_time_init, pfreq_handle,
                             std::ref(throttletime));
          first.join();
          second.join();
          auto after_throttletime = lzt::get_throttle_time(pfreq_handle);
          EXPECT_LT(before_throttletime.throttleTime,
                    after_throttletime.throttleTime);
          EXPECT_NE(before_throttletime.timestamp,
                    after_throttletime.timestamp);
          status = lzt::set_power_limits(p_power_handle, &power_sustained_limit_Initial,
                                nullptr, nullptr);
          if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
            continue;
          }
          EXPECT_EQ(status, ZE_RESULT_SUCCESS);
        }
      }
    }
  }
}

// Function(thread 1) to run frequency checks while GPU workload is running
void checkFreqInLoop(zes_freq_handle_t pfreq_handle) {
  // Loop counter
  int counter = 100;

  // Get current min/max frequency setting
  zes_freq_range_t limits = {};
  limits = lzt::get_freq_range(pfreq_handle);

  // Monitor actual frequency in loop[fixed loop count]
  do {
    counter--;
    zes_freq_state_t state = lzt::get_freq_state(pfreq_handle);
    lzt::validate_freq_state(pfreq_handle, state);
    if (state.actual > 0) {
      EXPECT_LE(state.actual, limits.max);
      EXPECT_GE(state.actual, limits.min);
    } else {
      LOG_INFO << "Actual frequency is unknown";
    }
    // Sleep for some fixed duration before next check
    std::this_thread::sleep_for(
        std::chrono::microseconds(1000 * IDLE_WAIT_TIMESTEP_MSEC));
  } while (counter > 0);
}

// Function(thread 2) to run workload on GPU
void loadForGpuMaxFreqTest(ze_device_handle_t target_device) {
  int m, k, n;
  m = k = n = 10000;
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);
  const ze_device_handle_t device = target_device;
  void *a_buffer = lzt::allocate_host_memory(m * k * sizeof(float));
  void *b_buffer = lzt::allocate_host_memory(k * n * sizeof(float));
  void *c_buffer = lzt::allocate_host_memory(m * n * sizeof(float));
  ze_module_handle_t module =
      lzt::create_module(device, "sysman_matrix_multiplication.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "sysman_matrix_multiplication");
  lzt::set_group_size(function, 16, 16, 1);
  lzt::set_argument_value(function, 0, sizeof(a_buffer), &a_buffer);
  lzt::set_argument_value(function, 1, sizeof(b_buffer), &b_buffer);
  lzt::set_argument_value(function, 2, sizeof(m), &m);
  lzt::set_argument_value(function, 3, sizeof(k), &k);
  lzt::set_argument_value(function, 4, sizeof(n), &n);
  lzt::set_argument_value(function, 5, sizeof(c_buffer), &c_buffer);
  ze_command_list_handle_t cmd_list = lzt::create_command_list(device);
  std::memcpy(a_buffer, a.data(), a.size() * sizeof(float));
  std::memcpy(b_buffer, b.data(), b.size() * sizeof(float));
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  const int group_count_x = m / 16;
  const int group_count_y = n / 16;
  ze_group_count_t tg;
  tg.groupCountX = group_count_x;
  tg.groupCountY = group_count_y;
  tg.groupCountZ = 1;
  zeCommandListAppendLaunchKernel(cmd_list, function, &tg, nullptr, 0, nullptr);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_list);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(device);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
  lzt::synchronize(cmd_q, UINT64_MAX);
  std::memcpy(c.data(), c_buffer, c.size() * sizeof(float));
  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_function(function);
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);
  lzt::destroy_module(module);
}

TEST_F(
    FREQUENCY_TEST,
    GivenValidFrequencyRangeWhenRequestingSetFrequencyThenExpectActualFrequencyStaysInRangeDuringGpuLoad) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto pfreq_handles = lzt::get_freq_handles(device, p_count);
    if (p_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto pfreq_handle : pfreq_handles) {
      EXPECT_NE(nullptr, pfreq_handle);

      // Fetch frequency properties
      zes_freq_properties_t properties = {};
      properties = lzt::get_freq_properties(pfreq_handle);

      if (properties.canControl) {
        // Set values for min and max frequencies
        zes_freq_range_t limits = {};
        limits.min = properties.min + (properties.max - properties.min) / 4;
        limits.max = properties.min + (properties.max - properties.min) / 2;

        if ((fmod(limits.min, 50))) {
          limits.min = limits.min - fmod(limits.min, 50);
        }
        if (fmod(limits.max, 50)) {
          limits.max = limits.max - fmod(limits.max, 50);
        }

        lzt::set_freq_range(pfreq_handle, limits);
        lzt::idle_check(pfreq_handle);
        // Thread to start workload on GPU
#ifdef USE_ZESINIT
        auto sysman_device_properties =
            lzt::get_sysman_device_properties(device);
        ze_device_handle_t core_device =
            get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
        EXPECT_NE(core_device, nullptr);
        std::thread first(loadForGpuMaxFreqTest, core_device);
#else  // USE_ZESINIT
        std::thread first(loadForGpuMaxFreqTest, device);
#endif // USE_ZESINIT
       // Thread to monitor actual frequency
        std::thread second(checkFreqInLoop, pfreq_handle);

        // wait for threads to finish
        first.join();
        second.join();
      } else {
        // User cannot control min,max frequency settings
        LOG_WARNING
            << "User cannot control min/max frequency setting, skipping test";
      }
    }
  }
}

} // namespace
