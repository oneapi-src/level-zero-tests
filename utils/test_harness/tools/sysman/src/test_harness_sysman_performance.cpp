/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "test_harness/test_harness.hpp"
#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests 
{

uint32_t get_performance_handle_count(ze_device_handle_t device)
 {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumPerformanceFactorDomains(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}
std::vector<zes_perf_handle_t> get_performance_handles(ze_device_handle_t device,uint32_t &count)
 {
  if (count == 0)
 {
    count = get_performance_handle_count(device);
  }
  std::vector<zes_perf_handle_t> performanceHandles(count, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumPerformanceFactorDomains(device, &count, performanceHandles.data()));
  return performanceHandles;
}

zes_perf_properties_t get_performance_properties(zes_perf_handle_t performanceHandle) 
{
  zes_perf_properties_t properties = {ZES_STRUCTURE_TYPE_PERF_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,zesPerformanceFactorGetProperties(performanceHandle, &properties));
  return properties;
}

 void set_performance_config(zes_perf_handle_t performanceHandle,double factor)
{
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesPerformanceFactorSetConfig(performanceHandle,factor));
}

double get_performance_config(zes_perf_handle_t performanceHandle)
 {
double factor= 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesPerformanceFactorGetConfig(performanceHandle,&factor);
  return factor;
}
} // namespace level_zero_tests
