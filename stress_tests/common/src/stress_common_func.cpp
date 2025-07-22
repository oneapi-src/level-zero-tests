/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "stress_common_func.hpp"

namespace lzt = level_zero_tests;

#if defined(unix) || defined(__unix__) || defined(__unix)

#include <unistd.h>

uint64_t total_available_host_memory() {
  const long page_count = sysconf(_SC_AVPHYS_PAGES);
  const long page_size = sysconf(_SC_PAGE_SIZE);
  const uint64_t total_bytes = page_count * page_size;

  return total_bytes;
}

uint64_t get_page_size() {
  const long page_size = sysconf(_SC_PAGE_SIZE);
  return page_size;
}
#endif

#if defined(_WIN64) || defined(_WIN64) || defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <Sysinfoapi.h>

uint64_t total_available_host_memory() {
  MEMORYSTATUSEX stat;
  stat.dwLength = sizeof(stat);
  GlobalMemoryStatusEx(&stat);

  return stat.ullAvailVirtual;
}
uint64_t get_page_size() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  const long page_size = si.dwPageSize;
  return page_size;
}

#endif

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
  uint64_t host_available_memory_size = total_available_host_memory();
  uint64_t total_available_memory = device_total_memory_size;

  LOG_INFO << "Device total memory size == "
           << (float)device_total_memory_size / (1024 * 1024) << " MB";

  if (device_properties.flags & ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) {
    LOG_INFO << "Device is Integrated, device will share memory with host. "
                "Available host memory size will be set to half.";
    host_available_memory_size = host_available_memory_size / 2;
  }
  LOG_INFO << "Host Available Memory == "
           << (float)host_available_memory_size / (1024 * 1024) << " MB";

  if (device_total_memory_size > host_available_memory_size) {
    LOG_INFO << "Require equal amount of host memory for "
                "verification, limiting to available host memory size.";
    total_available_memory = host_available_memory_size;
  }
  LOG_INFO << "Total Available Memory == "
           << (float)total_available_memory / (1024 * 1024) << " MB";

  // initial values base on test arguments
  one_allocation_size = test_arguments.one_allocation_size_limit *
                        total_available_memory / number_of_all_alloc;

  total_allocation_size = number_of_all_alloc * one_allocation_size *
                          test_arguments.total_memory_size_limit;

  LOG_INFO << "Test total memory size requested: "
           << (float)total_allocation_size / (1024 * 1024) << "MB";
  LOG_INFO << "Test single allocation memory size requested: "
           << (float)one_allocation_size / (1024 * 1024) << "MB";
  LOG_INFO << "Max single allocation memory size: "
           << (float)device_properties.maxMemAllocSize / (1024 * 1024) << "MB";
  uint64_t min_page_size = 4096;
  get_mem_page_size(driver, test_arguments.memory_type, min_page_size);

  LOG_INFO << "Device minimum memory allocation page size = " << min_page_size
           << " bytes";

  if (one_allocation_size < min_page_size) {
    LOG_INFO << "One allocation size too low. Change from "
             << one_allocation_size << " to " << min_page_size;
    one_allocation_size = min_page_size; // increased
    while (number_of_all_alloc * one_allocation_size *
               test_arguments.total_memory_size_limit >
           total_allocation_size) {
      number_of_all_alloc--;
    }
    total_allocation_size = number_of_all_alloc * one_allocation_size *
                            test_arguments.total_memory_size_limit;
  }

  // take into account max memory allocation supported by device
  if (one_allocation_size > device_properties.maxMemAllocSize) {
    if (!lzt::check_if_extension_supported(
            driver, ZE_RELAXED_ALLOCATION_LIMITS_EXP_NAME)) {
      LOG_INFO << "Requested allocation size higher then max alloc size. Need "
                  "to limit... ";
      one_allocation_size = device_properties.maxMemAllocSize;
      total_allocation_size = number_of_all_alloc * one_allocation_size *
                              test_arguments.total_memory_size_limit;
    } else {
      LOG_INFO << "Relax memory capability enabled. Higher memory allocation "
                  "allowed. ";
      relax_memory_capability = true;
    }
  }

  // verify if total memory used by test is bigger that device memory
  if (total_allocation_size > total_available_memory) {
    LOG_INFO << "Requested  total memory size higher then device memory size. "
                "Need to limit... ";
    total_allocation_size =
        test_arguments.total_memory_size_limit * total_available_memory;
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

  uint64_t maxSingleSizeAllocation =
      (16ull * 1024ull * 1024ull * 1024ull) - min_page_size;
  if (one_allocation_size > maxSingleSizeAllocation) {
    LOG_WARNING << "Single size allocation " << one_allocation_size
                << " exceeds 16GB, adjusting it to 16GB";
    one_allocation_size = maxSingleSizeAllocation;
    total_allocation_size = number_of_all_alloc * one_allocation_size *
                            test_arguments.total_memory_size_limit;
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
  lzt::allocate_mem(&simple_allocation, ze_memory_type_t(mem_type), 1, context);
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
