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

void adjust_max_memory_allocation(
    const ze_driver_handle_t &driver,
    const ze_device_properties_t &device_properties,
    const std::vector<ze_device_memory_properties_t> &device_memory_properties,
    uint64_t &total_memory_size, uint64_t &one_allocation_size,
    const uint64_t number_of_all_alloc, const float total_memory_limit,
    const float one_allocation_size_limit, bool &relax_memory_capability);

enum memory_test_type { MTT_SHARED, MTT_HOST, MTT_DEVICE };
std::string print_allocation_type(memory_test_type);

template <typename T>
T *allocate_memory(const ze_context_handle_t &context,
                   const ze_device_handle_t &device, memory_test_type mem_type,
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
  if (mem_type == MTT_DEVICE) {
    new_allocation = (T *)lzt::allocate_device_memory(
        allocation_size, 0, 0, pNext, 0, device, context);
  } else if (mem_type == MTT_HOST) {
    new_allocation =
        (T *)lzt::allocate_host_memory(allocation_size, 0, 0, pNext, context);

  } else if (mem_type == MTT_SHARED) {
    new_allocation = (T *)lzt::allocate_shared_memory(
        allocation_size, 0, 0, pNext, 0, nullptr, device, context);
  }
  return new_allocation;
};

typedef struct TestArguments {
  float total_memory_size_limit;
  float one_allocation_size_limit;
  uint64_t multiplier;
  memory_test_type memory_type;

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

extern float one_percent;
extern float five_percent;
extern float ten_percent;
extern float twenty_percent;
extern float thirty_percent;
extern float fourty_percent;
extern float fifty_percent;
extern float sixty_percent;
extern float seventy_percent;
extern float eighty_percent;
extern float ninety_percent;
extern float hundred_percent;
#endif /* _STRESS_COMMON_FUNC_HPP_*/
