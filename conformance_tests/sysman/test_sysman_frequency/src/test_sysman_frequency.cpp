/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include <thread>
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {
class FrequencyModuleTest : public lzt::SysmanCtsClass {};
TEST_F(
    FrequencyModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    lzt::get_freq_handle_count(device);
  }
}

TEST_F(
    FrequencyModuleTest,
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
    FrequencyModuleTest,
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
    FrequencyModuleTest,
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

TEST_F(FrequencyModuleTest,
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
    FrequencyModuleTest,
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
    FrequencyModuleTest,
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
    FrequencyModuleTest,
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
      auto pFrequency = lzt::get_available_clocks(pfreq_handle, count);

      for (uint32_t i = 0; i < pFrequency.size(); i++) {
        EXPECT_GT(pFrequency[i], 0);
        if (i > 0)
          EXPECT_GE(
              pFrequency[i],
              pFrequency[i - 1]); // Each entry in array of pFrequency, should
                                  // be less than or equal to next entry
      }
    }
  }
}
TEST_F(FrequencyModuleTest,
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
    FrequencyModuleTest,
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
      else
        FAIL();
    }
  }
}

TEST_F(
    FrequencyModuleTest,
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
      EXPECT_GT(properties.max, 0);
      EXPECT_GT(properties.min, 0);
    }
  }
}

TEST_F(
    FrequencyModuleTest,
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
    FrequencyModuleTest,
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
    FrequencyModuleTest,
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
    FrequencyModuleTest,
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
    FrequencyModuleTest,
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
      auto frequency = lzt::get_available_clocks(*pfreq_handles.data(), count);
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
void load_for_gpu() {
  int m, k, n;
  m = k = n = 5000;
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  void *a_buffer = lzt::allocate_host_memory(m * k * sizeof(float));
  void *b_buffer = lzt::allocate_host_memory(k * n * sizeof(float));
  void *c_buffer = lzt::allocate_host_memory(m * n * sizeof(float));
  ze_module_handle_t module =
      lzt::create_module(device, "ze_matrix_multiplication.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "ze_matrix_multiplication");
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

TEST_F(FrequencyModuleTest, GivenValidFrequencyHandleThenCheckForThrottling) {
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
          lzt::get_power_limits(p_power_handle, &power_sustained_limit_Initial,
                                nullptr, nullptr);
          zes_power_sustained_limit_t power_sustained_limit_set;
          power_sustained_limit_set.power = 0;
          lzt::set_power_limits(p_power_handle, &power_sustained_limit_set,
                                nullptr, nullptr);
          auto before_throttletime = lzt::get_throttle_time(pfreq_handle);
          zes_freq_throttle_time_t throttletime;
          std::thread first(load_for_gpu);
          std::thread second(get_throttle_time_init, pfreq_handle,
                             std::ref(throttletime));
          first.join();
          second.join();
          auto after_throttletime = lzt::get_throttle_time(pfreq_handle);
          EXPECT_LT(before_throttletime.throttleTime,
                    after_throttletime.throttleTime);
          EXPECT_NE(before_throttletime.timestamp,
                    after_throttletime.timestamp);
          lzt::set_power_limits(p_power_handle, &power_sustained_limit_Initial,
                                nullptr, nullptr);
        }
      }
    }
  }
}

} // namespace
