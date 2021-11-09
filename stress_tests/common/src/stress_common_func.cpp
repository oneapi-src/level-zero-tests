/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "stress_common_func.hpp"

namespace lzt = level_zero_tests;

// limit total or one allocation size, do not change number of allocations
void adjust_max_memory_allocation(
    const ze_driver_handle_t &driver,
    const ze_device_properties_t &device_properties,
    const std::vector<ze_device_memory_properties_t> &device_memory_properties,
    uint64_t &total_allocation_size, uint64_t &one_allocation_size,
    const uint64_t number_of_all_alloc, const float total_memory_limit,
    const float one_allocation_size_limit, bool &relax_memory_capability) {

  relax_memory_capability = lzt::check_if_extension_supported(
      driver, "ZE_experimental_relaxed_allocation_limits");
  std::string ext_info =
      relax_memory_capability ? "is supported" : "is not supported";
  LOG_INFO << "The extension ZE_experimental_relaxed_allocation_limits "
           << ext_info;

  // NEO return always one property that why index 0.
  uint64_t device_total_memory_size = device_memory_properties[0].totalSize;
  one_allocation_size = one_allocation_size_limit * device_total_memory_size /
                        number_of_all_alloc;
  total_allocation_size =
      number_of_all_alloc * one_allocation_size * total_memory_limit;
  LOG_INFO << "Device total memory size == "
           << (float)device_total_memory_size / (1024 * 1024) << " MB";

  LOG_INFO << "Test total memory size requested: "
           << (float)total_allocation_size / (1024 * 1024) << "MB";
  LOG_INFO << "Test single allocation memory size requested: "
           << (float)one_allocation_size / (1024 * 1024) << "MB";

  if (!relax_memory_capability) {
    if (one_allocation_size > device_properties.maxMemAllocSize) {
      LOG_INFO << "Requested allocation size higher then max alloc size. Need "
                  "to limit... ";
      one_allocation_size =
          one_allocation_size_limit * device_properties.maxMemAllocSize;
    }
    total_allocation_size =
        number_of_all_alloc * one_allocation_size * total_memory_limit;
  }

  if (total_allocation_size > device_total_memory_size) {
    LOG_INFO << "Requested  total memory size higher then device memory size. "
                "Need to limit... ";
    total_allocation_size = total_memory_limit * device_total_memory_size;
    one_allocation_size =
        one_allocation_size_limit * total_allocation_size / number_of_all_alloc;
  }

  LOG_INFO << "Test total memory size submitted: "
           << (float)total_allocation_size / (1024 * 1024) << "MB";
  LOG_INFO << "Test single allocation memory size submitted: "
           << (float)one_allocation_size / (1024 * 1024) << "MB";
};

std::string print_allocation_type(memory_test_type mem_type) {
  switch (mem_type) {
  case MTT_HOST:
    return "HOST";
  case MTT_SHARED:
    return "SHARED";
  case MTT_DEVICE:
    return "DEVICE";
  default:
    return "UNKNOWN";
  }
};

float one_percent = 0.01f;
float five_percent = 0.05f;
float ten_percent = 0.1f;
float twenty_percent = 0.2f;
float thirty_percent = 0.3f;
float fourty_percent = 0.4f;
float fifty_percent = 0.5f;
float sixty_percent = 0.6f;
float seventy_percent = 0.7f;
float eighty_percent = 0.8f;
float ninety_percent = 0.9f;
float hundred_percent = 1.0f;
