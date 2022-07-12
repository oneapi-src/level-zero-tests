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
    uint64_t &number_of_all_alloc, const TestArguments_t test_arguments,
    bool &relax_memory_capability) {

  // NEO return always one property that why index 0.
  uint64_t device_total_memory_size = device_memory_properties[0].totalSize;

  // initial values base on test arguments
  one_allocation_size = test_arguments.one_allocation_size_limit *
                        device_total_memory_size / number_of_all_alloc;

  total_allocation_size = number_of_all_alloc * one_allocation_size *
                          test_arguments.total_memory_size_limit;

  LOG_INFO << "Device total memory size == "
           << (float)device_total_memory_size / (1024 * 1024) << " MB";

  LOG_INFO << "Test total memory size requested: "
           << (float)total_allocation_size / (1024 * 1024) << "MB";
  LOG_INFO << "Test single allocation memory size requested: "
           << (float)one_allocation_size / (1024 * 1024) << "MB";
  uint64_t min_page_size = 4096;
  get_mem_page_size(driver, test_arguments.memory_type, min_page_size);

  LOG_INFO << "Device minimum memory allocation page size = " << min_page_size
           << " bytes";

  if (one_allocation_size < min_page_size) {
    LOG_INFO << "One allocation size too low. Change from "
             << one_allocation_size << " to " << min_page_size;
    one_allocation_size = min_page_size; // increased
    total_allocation_size = number_of_all_alloc * one_allocation_size *
                            test_arguments.total_memory_size_limit;
  }

  // take into account max memory allocation supported by device
  if (one_allocation_size > device_properties.maxMemAllocSize) {
    if (!lzt::check_if_extension_supported(
            driver, "ZE_experimental_relaxed_allocation_limits")) {
      LOG_INFO << "Requested allocation size higher then max alloc size. Need "
                  "to limit... ";
      one_allocation_size = test_arguments.one_allocation_size_limit *
                            device_properties.maxMemAllocSize;
      total_allocation_size = number_of_all_alloc * one_allocation_size *
                              test_arguments.total_memory_size_limit;
    } else {
      LOG_INFO << "Relax memory capability enabled. Higher memory allocation "
                  "allowed. ";
    }
  }

  // verify if total memory used by test is bigger that device memory
  if (total_allocation_size > device_total_memory_size) {
    LOG_INFO << "Requested  total memory size higher then device memory size. "
                "Need to limit... ";
    total_allocation_size =
        test_arguments.total_memory_size_limit * device_total_memory_size;
    one_allocation_size = test_arguments.one_allocation_size_limit *
                          total_allocation_size / number_of_all_alloc;
    if (one_allocation_size < min_page_size) {
      one_allocation_size = min_page_size;
      while (number_of_all_alloc * one_allocation_size *
                 test_arguments.total_memory_size_limit >
             total_allocation_size) {
        number_of_all_alloc--;
      }
    }
  }

  LOG_INFO << "Test total memory size submitted: "
           << (float)total_allocation_size / (1024 * 1024) << "MB";
  LOG_INFO << "Test single allocation memory size submitted: "
           << (float)one_allocation_size / (1024 * 1024) << "MB";
};

std::string print_allocation_type(ze_memory_type_t mem_type) {
  switch (mem_type) {
  case ZE_MEMORY_TYPE_HOST:
    return "HOST";
  case ZE_MEMORY_TYPE_SHARED:
    return "SHARED";
  case ZE_MEMORY_TYPE_DEVICE:
    return "DEVICE";
  default:
    return "UNKNOWN";
  }
};

void get_mem_page_size(const ze_driver_handle_t &driver,
                       ze_memory_type_t mem_type, size_t &page_size) {

  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);
  ze_memory_allocation_properties_t mem_props = {};
  mem_props.pNext = nullptr;
  mem_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  void *simple_allocation = nullptr;
  lzt::allocate_mem(&simple_allocation, ze_memory_type_t(mem_type), 1);
  lzt::get_mem_alloc_properties(context, simple_allocation, &mem_props);
  page_size = mem_props.pageSize;
  lzt::free_memory(context, simple_allocation);
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
