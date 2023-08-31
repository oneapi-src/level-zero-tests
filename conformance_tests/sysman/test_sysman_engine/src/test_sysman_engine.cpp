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

class EngineModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    EngineModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanEngineHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_engine_handle_count(device);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    EngineModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanEngineHandlesThenNotNullEngineHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(engine_handles.size(), count);
    for (auto engine_handle : engine_handles) {
      EXPECT_NE(nullptr, engine_handle);
    }
  }
}

TEST_F(
    EngineModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanEngineHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_engine_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_engine_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    EngineModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarEngineHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles_initial = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles_initial) {
      EXPECT_NE(nullptr, engine_handle);
    }

    count = 0;
    auto engine_handles_later = lzt::get_engine_handles(device, count);
    for (auto engine_handle : engine_handles_later) {
      EXPECT_NE(nullptr, engine_handle);
    }
    EXPECT_EQ(engine_handles_initial, engine_handles_later);
  }
}

TEST_F(
    EngineModuleTest,
    GivenValidEngineHandleWhenRetrievingEnginePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles) {
      ASSERT_NE(nullptr, engine_handle);
      auto properties = lzt::get_engine_properties(engine_handle);
      EXPECT_GE(properties.type, ZES_ENGINE_GROUP_ALL);
      EXPECT_LE(properties.type, ZES_ENGINE_GROUP_3D_ALL);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
    }
  }
}

TEST_F(
    EngineModuleTest,
    GivenValidEngineHandleWhenRetrievingEnginePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles) {
      EXPECT_NE(nullptr, engine_handle);
      auto properties_initial = lzt::get_engine_properties(engine_handle);
      auto properties_later = lzt::get_engine_properties(engine_handle);
      EXPECT_EQ(properties_initial.type, properties_later.type);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
    }
  }
}

TEST_F(
    EngineModuleTest,
    GivenValidEngineHandleWhenRetrievingEngineActivityStatsThenValidStatsIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles) {
      ASSERT_NE(nullptr, engine_handle);
      auto state = lzt::get_engine_activity(engine_handle);
      EXPECT_LT(state.activeTime, UINT32_MAX);
      EXPECT_LT(state.timestamp, UINT32_MAX);
    }
  }
}
TEST_F(
    EngineModuleTest,
    GivenValidEngineHandleWhenRetrievingEngineActivityStatsThenTimestampWillbeIncrementedInNextCalltoEngineActivity) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles) {
      ASSERT_NE(nullptr, engine_handle);
      auto oldstate = lzt::get_engine_activity(engine_handle);
      EXPECT_LT(oldstate.activeTime, UINT32_MAX);
      EXPECT_LT(oldstate.timestamp, UINT32_MAX);
      auto newstate = lzt::get_engine_activity(engine_handle);
      EXPECT_LT(newstate.activeTime, UINT32_MAX);
      EXPECT_LT(newstate.timestamp, UINT32_MAX);
      EXPECT_GT(newstate.timestamp, oldstate.timestamp);
    }
  }
}

static void workload_for_device(ze_device_handle_t device) {
  int m, k, n;
  m = k = n = 5000;
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);
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
    EngineModuleTest,
    GivenValidEngineHandleWhenGpuWorkloadIsSubmittedThenEngineActivityMeasuredIsHigher) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto engine_handle : engine_handles) {
      ASSERT_NE(nullptr, engine_handle);
      auto properties = lzt::get_engine_properties(engine_handle);
      if (properties.type == ZES_ENGINE_GROUP_COMPUTE_ALL) {
        // Get pre-workload utilization
        auto s1 = lzt::get_engine_activity(engine_handle);
        auto s2 = lzt::get_engine_activity(engine_handle);
        double pre_utilization = (static_cast<double>(s2.activeTime) -
                                  static_cast<double>(s1.activeTime)) /
                                 (static_cast<double>(s2.timestamp) -
                                  static_cast<double>(s1.timestamp));
        // submit workload and measure  utilization
        s1 = lzt::get_engine_activity(engine_handle);
        std::thread thread(workload_for_device, device);
        thread.join();
        s2 = lzt::get_engine_activity(engine_handle);
        double post_utilization = (static_cast<double>(s2.activeTime) -
                                   static_cast<double>(s1.activeTime)) /
                                  (static_cast<double>(s2.timestamp) -
                                   static_cast<double>(s1.timestamp));
        // check if engine utilization increases with GPU workload
        EXPECT_LT(pre_utilization, post_utilization);
      }
    }
  }
}

} // namespace
