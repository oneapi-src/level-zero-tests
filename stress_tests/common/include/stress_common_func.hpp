/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _STRESS_COMMON_FUNC_HPP_
#define _STRESS_COMMON_FUNC_HPP_

#include <level_zero/ze_api.h>
#include "test_harness/test_harness.hpp"

#include <vector>

std::string print_allocation_type(ze_memory_type_t);
template <typename T>
T *allocate_memory(const ze_context_handle_t &context,
                   const ze_device_handle_t &device, ze_memory_type_t mem_type,
                   size_t allocation_size, bool relax_memory_capability) {
  void *pNext = nullptr;
  T *new_allocation = nullptr;
  ze_relaxed_allocation_limits_exp_desc_t relaxed_allocation_limits_desc = {};
  if (relax_memory_capability) {
    relaxed_allocation_limits_desc.stype =
        ZE_STRUCTURE_TYPE_RELAXED_ALLOCATION_LIMITS_EXP_DESC;
    relaxed_allocation_limits_desc.flags =
        ZE_RELAXED_ALLOCATION_LIMITS_EXP_FLAG_MAX_SIZE;
    pNext = &relaxed_allocation_limits_desc;
  }
  if (mem_type == ZE_MEMORY_TYPE_DEVICE) {
    new_allocation = (T *)lzt::allocate_device_memory(
        allocation_size, 0, 0, pNext, 0, device, context);
  } else if (mem_type == ZE_MEMORY_TYPE_HOST) {
    new_allocation =
        (T *)lzt::allocate_host_memory(allocation_size, 0, 0, pNext, context);

  } else if (mem_type == ZE_MEMORY_TYPE_SHARED) {
    new_allocation = (T *)lzt::allocate_shared_memory(
        allocation_size, 0, 0, pNext, 0, nullptr, device, context);
  }
  return new_allocation;
};

typedef struct TestArguments {
  double total_memory_size_limit;
  double one_allocation_size_limit;
  uint64_t multiplier;
  ze_memory_type_t memory_type;

  void print_test_arguments(const ze_device_properties_t &device_properties) {
    LOG_INFO << "TESTED device: " << device_properties.name;
    LOG_INFO << "TESTING user ARGS: "
             << " Limit total memory to = " << 100 * total_memory_size_limit
             << "% "
             << " | Limit one allocation to = "
             << 100 * one_allocation_size_limit << " %"
             << " | Multiplier set = " << multiplier
             << " | User memory allocation type: "
             << print_allocation_type(memory_type);
  };
} TestArguments_t;

void adjust_max_memory_allocation(
    const ze_driver_handle_t &driver,
    const ze_device_properties_t &device_properties,
    const std::vector<ze_device_memory_properties_t> &device_memory_properties,
    uint64_t &total_allocation_size, uint64_t &one_allocation_size,
    uint64_t &number_of_all_alloc, const TestArguments_t test_arguments,
    bool &relax_memory_capability);

void get_mem_page_size(const ze_driver_handle_t &driver,
                       ze_memory_type_t mem_type, size_t &page_size);
extern double one_percent;
extern double five_percent;
extern double ten_percent;
extern double twenty_percent;
extern double thirty_percent;
extern double fourty_percent;
extern double fifty_percent;
extern double sixty_percent;
extern double seventy_percent;
extern double eighty_percent;
extern double ninety_percent;
extern double hundred_percent;
#endif /* _STRESS_COMMON_FUNC_HPP_*/
