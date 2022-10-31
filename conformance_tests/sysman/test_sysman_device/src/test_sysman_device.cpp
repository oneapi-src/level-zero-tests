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

uint32_t get_prop_length(char prop[ZES_STRING_PROPERTY_SIZE]) {
  uint32_t length = 0;
  for (int i = 0; i < ZES_STRING_PROPERTY_SIZE; i++) {
    if (prop[i] == '\0') {
      break;
    } else {
      length += 1;
    }
  }

  return length;
}

class SysmanDeviceTest : public lzt::SysmanCtsClass {};

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenRetrievingSysmanDevicePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto properties = lzt::get_sysman_device_properties(device);

    EXPECT_GE(ZE_DEVICE_TYPE_GPU, properties.core.type);
    EXPECT_LE(properties.core.type, ZE_DEVICE_TYPE_MCA);
    if (properties.core.flags <= ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
        ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
        ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      if (properties.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
        EXPECT_LT(properties.core.subdeviceId, UINT32_MAX);
      } else {
        EXPECT_EQ(0, properties.core.subdeviceId);
      }
    } else {
      FAIL();
    }
    EXPECT_LE(get_prop_length(properties.serialNumber),
              ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.boardNumber),
              ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.brandName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.modelName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.vendorName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.driverVersion),
              ZES_STRING_PROPERTY_SIZE);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenRetrievingSysmanDevicePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    auto properties_initial = lzt::get_sysman_device_properties(device);
    auto properties_later = lzt::get_sysman_device_properties(device);
    EXPECT_EQ(properties_initial.core.type, properties_later.core.type);
    if (properties_initial.core.flags <= ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
            ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
            ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING &&
        properties_initial.core.flags <= ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
            ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
            ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      EXPECT_EQ(properties_initial.core.flags, properties_later.core.flags);
      if (properties_initial.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE &&
          properties_later.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
        EXPECT_EQ(properties_initial.core.subdeviceId,
                  properties_later.core.subdeviceId);
      }
    } else {
      FAIL();
    }
    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.serialNumber,
                            properties_later.serialNumber,
                            get_prop_length(properties_initial.serialNumber)));
    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.boardNumber,
                            properties_later.boardNumber,
                            get_prop_length(properties_initial.boardNumber)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.brandName,
                            properties_later.brandName,
                            get_prop_length(properties_initial.brandName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.modelName,
                            properties_later.modelName,
                            get_prop_length(properties_initial.modelName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.vendorName,
                            properties_later.vendorName,
                            get_prop_length(properties_initial.vendorName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.driverVersion,
                            properties_later.driverVersion,
                            get_prop_length(properties_initial.driverVersion)));
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenProcessesCountZeroWhenRetrievingProcessesStateThenSuccessIsReturned) {
  for (auto device : devices) {
    lzt::get_processes_count(device);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenProcessesCountZeroWhenRetrievingProcessesStateThenValidProcessesStateAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto processes = lzt::get_processes_state(device, count);
    if (processes.size() > 0) {
      for (auto process : processes) {
        EXPECT_GT(process.processId, 0u);
        EXPECT_GE(process.memSize, 0u);
        EXPECT_LT(process.sharedSize, UINT64_MAX);
        EXPECT_GE(process.engines, 0);
        EXPECT_LE(process.engines, (1 << ZES_ENGINE_TYPE_FLAG_DMA));
      }
    }
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_processes_state(device, actual_count);
    uint32_t test_count = actual_count + 1;
    lzt::get_processes_state(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenRetrievingSysmanDeviceStateThenValidStateIsReturned) {
  for (auto device : devices) {
    auto state = lzt::get_device_state(device);
    EXPECT_GE(state.reset, 0);
    EXPECT_LE(state.reset,
              ZES_RESET_REASON_FLAG_WEDGED | ZES_RESET_REASON_FLAG_REPAIR);
    EXPECT_GE(state.repaired, ZES_REPAIR_STATUS_UNSUPPORTED);
    EXPECT_LE(state.repaired, ZES_REPAIR_STATUS_PERFORMED);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenResettingSysmanDeviceThenSysmanDeviceResetIsSucceded) {
  for (auto device : devices) {
    lzt::sysman_device_reset(device);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenResettingSysmanDeviceNnumberOfTimesThenSysmanDeviceResetAlwaysSucceded) {
  int number_iterations = 2;
  for (int i = 0; i < number_iterations; i++) {
    for (auto device : devices) {
      lzt::sysman_device_reset(device);
    }
  }
}

static std::vector<float>
submit_workload_for_gpu(std::vector<float> a, std::vector<float> b,
                        uint32_t dim, ze_device_handle_t targetDevice) {
  int m, k, n;
  m = k = n = dim;
  std::vector<float> c(m * n, 0);
  const ze_device_handle_t device = targetDevice;
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

  return c;
}

static std::vector<float>
perform_matrix_multiplication_on_cpu(std::vector<float> a, std::vector<float> b,
                                     uint32_t n) {
  std::vector<float> c_cpu(n * n, 0);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      float sum = 0;
      for (int kl = 0; kl < n; kl++) {
        sum += a[i * n + kl] * b[kl * n + j];
      }
      c_cpu[i * n + j] = sum;
    }
  }
  return c_cpu;
}

static void compare_results(std::vector<float> c, std::vector<float> c_cpu) {
  for (int i = 0; i < c.size(); i++) {
    if (c[i] != c_cpu[i]) {
      EXPECT_EQ(c[i], c_cpu[i]);
    }
  }
  return;
}

TEST_F(
    SysmanDeviceTest,
    GivenWorkingDeviceHandleWhenResettingSysmanDeviceThenWorkloadExecutionAlwaysSucceedsAfterReset) {
  uint32_t n = 512;
  std::vector<float> a(n * n, 1);
  std::vector<float> b(n * n, 1);
  std::vector<float> c;
  std::vector<float> c_cpu;
  c_cpu = perform_matrix_multiplication_on_cpu(a, b, n);

  for (auto device : devices) {
    // Perform workload execution before reset
    c = submit_workload_for_gpu(a, b, n, device);
    compare_results(c, c_cpu);
    c.clear();
    LOG_INFO << "Initiating device reset...\n";
    // perform device reset
    lzt::sysman_device_reset(device);
    LOG_INFO << "End of device reset...\n";

    // Perform workload execution after reset
    c = submit_workload_for_gpu(a, b, n, device);
    compare_results(c, c_cpu);
    c.clear();
  }
}
TEST_F(
    SysmanDeviceTest,
    GivenWorkingDeviceHandleWhenResettingSysmanDeviceThenWorkloadExecutionAlwaysSucceedsAfterResetInMultipleIterations) {
  uint32_t n = 512;
  std::vector<float> a(n * n, 1);
  std::vector<float> b(n * n, 1);
  std::vector<float> c;
  std::vector<float> c_cpu;
  c_cpu = perform_matrix_multiplication_on_cpu(a, b, n);
  const char *valueString = std::getenv("LZT_SYSMAN_DEVICE_TEST_ITERATIONS");
  uint32_t number_iterations = 2;
  if (valueString != nullptr) {
    number_iterations = atoi(valueString);
  }

  for (auto device : devices) {
    for (int i = 0; i < number_iterations; i++) {
      // Perform workload execution before reset
      c = submit_workload_for_gpu(a, b, n, device);
      compare_results(c, c_cpu);
      c.clear();
      LOG_INFO << "Initiating device reset...\n";
      // perform device reset
      lzt::sysman_device_reset(device);
      LOG_INFO << "End of device reset...\n";

      // Perform workload execution after reset
      c = submit_workload_for_gpu(a, b, n, device);
      compare_results(c, c_cpu);
      c.clear();
    }
  }
}

} // namespace
