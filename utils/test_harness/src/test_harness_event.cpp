/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "gtest/gtest.h"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

ze_event_pool_handle_t create_event_pool(ze_context_handle_t context,
                                         uint32_t count,
                                         ze_event_pool_flags_t flags) {
  ze_event_pool_handle_t event_pool;
  ze_event_pool_desc_t descriptor = {};
  descriptor.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;

  descriptor.pNext = nullptr;
  descriptor.flags = flags;
  descriptor.count = count;

  return create_event_pool(context, descriptor);
}

ze_event_pool_handle_t create_event_pool(ze_event_pool_desc_t desc) {

  return create_event_pool(lzt::get_default_context(), desc);
}

ze_event_pool_handle_t create_event_pool(ze_context_handle_t context,
                                         ze_event_pool_desc_t desc) {
  ze_event_pool_handle_t event_pool;

  auto context_initial = context;
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventPoolCreate(context, &desc, 0, nullptr, &event_pool));
  EXPECT_EQ(context, context_initial);
  EXPECT_NE(nullptr, event_pool);
  return event_pool;
}

ze_event_pool_handle_t
create_event_pool(ze_context_handle_t context, ze_event_pool_desc_t desc,
                  std::vector<ze_device_handle_t> devices) {
  ze_event_pool_handle_t event_pool;

  auto context_initial = context;
  auto devices_initial = devices;
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventPoolCreate(context, &desc, to_u32(devices.size()), devices.data(),
                        &event_pool));
  EXPECT_EQ(context, context_initial);
  for (size_t i = 0U; i < devices.size(); i++) {
    EXPECT_EQ(devices[i], devices_initial[i]);
  }
  EXPECT_NE(nullptr, event_pool);
  return event_pool;
}

ze_event_handle_t create_event(ze_event_pool_handle_t event_pool,
                               ze_event_desc_t desc) {
  ze_event_handle_t event;
  auto event_pool_initial = event_pool;
  EXPECT_ZE_RESULT_SUCCESS(zeEventCreate(event_pool, &desc, &event));
  EXPECT_EQ(event_pool, event_pool_initial);
  EXPECT_NE(nullptr, event);
  return event;
}

void destroy_event(ze_event_handle_t event) {
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
}

void destroy_event_pool(ze_event_pool_handle_t event_pool) {
  EXPECT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(event_pool));
}

ze_kernel_timestamp_result_t
get_event_kernel_timestamp(ze_event_handle_t event) {
  auto event_initial = event;
  ze_kernel_timestamp_result_t value = {};
  EXPECT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &value));
  EXPECT_EQ(event, event_initial);
  return value;
}

void get_event_kernel_timestamps_from_mapped_timestamp_event(
    const ze_event_handle_t &event, const ze_device_handle_t &device,
    std::vector<ze_kernel_timestamp_result_t> &kernel_timestamp_buffer,
    std::vector<ze_synchronized_timestamp_result_ext_t>
        &synchronized_timestamp_buffer) {

  uint32_t count = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventQueryKernelTimestampsExt(event, device, &count, nullptr));
  EXPECT_GT(count, 0);

  ze_event_query_kernel_timestamps_results_ext_properties_t properties{};
  properties.pNext = nullptr;
  properties.stype =
      ZE_STRUCTURE_TYPE_EVENT_QUERY_KERNEL_TIMESTAMPS_RESULTS_EXT_PROPERTIES;

  kernel_timestamp_buffer.clear();
  synchronized_timestamp_buffer.clear();

  kernel_timestamp_buffer.resize(count);
  synchronized_timestamp_buffer.resize(count);

  properties.pKernelTimestampsBuffer = kernel_timestamp_buffer.data();
  properties.pSynchronizedTimestampsBuffer =
      synchronized_timestamp_buffer.data();

  EXPECT_ZE_RESULT_SUCCESS(
      zeEventQueryKernelTimestampsExt(event, device, &count, &properties));
}

double get_timestamp_time(const ze_kernel_timestamp_data_t *timestamp,
                          uint64_t timer_resolution,
                          uint64_t kernel_timestamp_valid_bits,
                          const ze_structure_type_t property_type) {
  uint64_t timestamp_freq = timer_resolution;
  uint64_t timestamp_max_val = ~(to_u64(-1) << kernel_timestamp_valid_bits);

  double timer_period = 0;
  if (property_type == ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES) {
    timer_period = (1000000000.0 / to_f64(timestamp_freq));
  } else if (property_type == ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2) {
    timer_period = to_f64(timestamp_freq);
  } else {
    LOG_ERROR << "INVALID DEVICE_PROPERTY_TYPE";
  }
  const auto time_ns =
      (timestamp->kernelEnd >= timestamp->kernelStart)
          ? (timestamp->kernelEnd - timestamp->kernelStart) * timer_period
          : ((timestamp_max_val - timestamp->kernelStart) +
             timestamp->kernelEnd + 1) *
                timer_period;

  return time_ns;
}

double
get_timestamp_global_duration(const ze_kernel_timestamp_result_t *timestamp,
                              const ze_device_handle_t &device,
                              const ze_driver_handle_t driver,
                              const ze_structure_type_t property_type) {

  auto device_properties = lzt::get_device_properties(device, property_type);
  uint64_t timestamp_freq = device_properties.timerResolution;
  uint64_t timestamp_max_val =
      ~(to_u64(-1) << device_properties.kernelTimestampValidBits);

  return lzt::get_timestamp_time(&timestamp->global, timestamp_freq,
                                 device_properties.kernelTimestampValidBits,
                                 property_type);
}

double
get_timestamp_context_duration(const ze_kernel_timestamp_result_t *timestamp,
                               const ze_device_handle_t &device,
                               const ze_driver_handle_t driver,
                               const ze_structure_type_t property_type) {
  auto device_properties = lzt::get_device_properties(device, property_type);
  uint64_t timestamp_freq = device_properties.timerResolution;
  uint64_t timestamp_max_val =
      ~(to_u64(-1) << device_properties.kernelTimestampValidBits);

  return lzt::get_timestamp_time(&timestamp->context, timestamp_freq,
                                 device_properties.kernelTimestampValidBits,
                                 property_type);
}

#ifdef ZE_EVENT_QUERY_TIMESTAMPS_EXP_NAME
uint32_t get_timestamp_count(const ze_event_handle_t &event,
                             const ze_device_handle_t &device) {
  uint32_t count = 0;

  EXPECT_ZE_RESULT_SUCCESS(
      zeEventQueryTimestampsExp(event, device, &count, nullptr));

  return count;
}

std::vector<ze_kernel_timestamp_result_t>
get_event_timestamps_exp(const ze_event_handle_t &event,
                         const ze_device_handle_t &device) {

  return get_event_timestamps_exp(event, device,
                                  get_timestamp_count(event, device));
}

std::vector<ze_kernel_timestamp_result_t>
get_event_timestamps_exp(const ze_event_handle_t &event,
                         const ze_device_handle_t &device, uint32_t count) {

  std::vector<ze_kernel_timestamp_result_t> timestamp_results(count);
  EXPECT_ZE_RESULT_SUCCESS(zeEventQueryTimestampsExp(event, device, &count,
                                                     timestamp_results.data()));
  return timestamp_results;
}

#endif // ifdef ZE_EVENT_QUERY_TIMESTAMPS_EXP_NAME

void zeEventPool::InitEventPool() { InitEventPool(32); }

void zeEventPool::InitEventPool(uint32_t count) {
  InitEventPool(count, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
}
void zeEventPool::InitEventPool(uint32_t count, ze_event_pool_flags_t flags) {
  if (event_pool_ == nullptr) {
    if (context_ == nullptr) {
      context_ = lzt::get_default_context();
    }
    event_pool_ = create_event_pool(context_, count, flags);
    pool_indexes_available_.resize(count, true);
  }
}

void zeEventPool::InitEventPool(ze_context_handle_t context, uint32_t count,
                                ze_event_pool_flags_t flags) {
  context_ = context;
  if (event_pool_ == nullptr) {
    if (context_ == nullptr) {
      context_ = lzt::get_default_context();
    }
    event_pool_ = create_event_pool(context_, count, flags);
    pool_indexes_available_.resize(count, true);
  }
}

void zeEventPool::InitEventPool(ze_context_handle_t context, uint32_t count) {
  context_ = context;
  if (event_pool_ == nullptr) {
    if (context_ == nullptr) {
      context_ = lzt::get_default_context();
    }
    event_pool_ =
        create_event_pool(context_, count, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
    pool_indexes_available_.resize(count, true);
  }
}

void zeEventPool::InitEventPool(ze_event_pool_desc_t desc) {
  if (event_pool_ == nullptr) {
    if (context_ == nullptr) {
      context_ = lzt::get_default_context();
    }
    event_pool_ = create_event_pool(context_, desc);
    pool_indexes_available_.resize(desc.count, true);
  }
}

void zeEventPool::InitEventPool(ze_event_pool_desc_t desc,
                                std::vector<ze_device_handle_t> devices) {
  if (event_pool_ == nullptr) {
    if (context_ == nullptr) {
      context_ = lzt::get_default_context();
    }
    event_pool_ = create_event_pool(context_, desc, devices);
    pool_indexes_available_.resize(desc.count, true);
  }
}

zeEventPool::zeEventPool() {}

zeEventPool::~zeEventPool() {
  // If the event pool was never created, do not attempt to destroy it
  // as that will needlessly cause a test failure.
  if (event_pool_) {
    auto result = zeEventPoolDestroy(event_pool_);
    if (ZE_RESULT_SUCCESS != result) {
      try {
        LOG_ERROR << "Failed to destroy event pool: " << result;
      } catch (...) {
        // Do nothing
      }
    }
  }
}

uint32_t find_index(const std::vector<bool> &indexes_available) {
  for (uint32_t i = 0; i < indexes_available.size(); i++)
    if (indexes_available[i])
      return i;
  return to_u32(-1);
}

void zeEventPool::create_event(ze_event_handle_t &event) {
  create_event(event, 0, 0);
}

void zeEventPool::create_event(ze_event_handle_t &event,
                               ze_event_scope_flags_t signal,
                               ze_event_scope_flags_t wait) {
  // Make sure the event pool is initialized to at least defaults:
  InitEventPool();
  ze_event_desc_t desc = {};
  memset(&desc, 0, sizeof(desc));
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  desc.pNext = nullptr;
  desc.signal = signal;
  desc.wait = wait;
  event = nullptr;
  desc.index = find_index(pool_indexes_available_);
  EXPECT_ZE_RESULT_SUCCESS(zeEventCreate(event_pool_, &desc, &event));
  EXPECT_NE(nullptr, event);
  handle_to_index_map_[event] = desc.index;
  pool_indexes_available_[desc.index] = false;
}

// Use to bypass zeEventPool management of event indexes
void zeEventPool::create_event(ze_event_handle_t &event, ze_event_desc_t desc) {
  EXPECT_ZE_RESULT_SUCCESS(zeEventCreate(event_pool_, &desc, &event));
  handle_to_index_map_[event] = desc.index;
  pool_indexes_available_[desc.index] = false;
}

void zeEventPool::create_events(std::vector<ze_event_handle_t> &events,
                                size_t event_count) {
  create_events(events, event_count, 0, 0);
}

void zeEventPool::create_events(std::vector<ze_event_handle_t> &events,
                                size_t event_count,
                                ze_event_scope_flags_t signal,
                                ze_event_scope_flags_t wait) {
  events.clear();
  for (size_t i = 0; i < event_count; i++) {
    ze_event_handle_t event;
    create_event(event, signal, wait);
    events.push_back(event);
  }
}

void zeEventPool::destroy_event(ze_event_handle_t event) {
  std::map<ze_event_handle_t, uint32_t>::iterator it =
      handle_to_index_map_.find(event);

  EXPECT_NE(it, handle_to_index_map_.end());
  pool_indexes_available_[(*it).second] = true;
  handle_to_index_map_.erase(it);
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
}

void zeEventPool::destroy_events(std::vector<ze_event_handle_t> &events) {
  for (auto event : events)
    destroy_event(event);
  events.clear();
}

void zeEventPool::get_ipc_handle(ze_ipc_event_pool_handle_t *hIpc) {
  auto event_pool_initial = event_pool_;
  ASSERT_ZE_RESULT_SUCCESS(zeEventPoolGetIpcHandle(event_pool_, hIpc));
  EXPECT_EQ(event_pool_, event_pool_initial);
}

void close_ipc_event_handle(ze_event_pool_handle_t eventPool) {
  auto event_pool_initial = eventPool;
  EXPECT_ZE_RESULT_SUCCESS(zeEventPoolCloseIpcHandle(eventPool));
  EXPECT_EQ(event_pool_initial, eventPool);
}

void put_ipc_event_handle(ze_context_handle_t context,
                          ze_ipc_event_pool_handle_t event_handle) {
  auto context_initial = context;
  EXPECT_ZE_RESULT_SUCCESS(zeEventPoolPutIpcHandle(context, event_handle));
  EXPECT_EQ(context, context_initial);
}

void open_ipc_event_handle(ze_context_handle_t context,
                           ze_ipc_event_pool_handle_t hIpc,
                           ze_event_pool_handle_t *eventPool) {
  auto context_initial = context;
  EXPECT_ZE_RESULT_SUCCESS(zeEventPoolOpenIpcHandle(context, hIpc, eventPool));
  EXPECT_EQ(context, context_initial);
}

void signal_event_from_host(ze_event_handle_t hEvent) {
  auto event_initial = hEvent;
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSignal(hEvent));
  EXPECT_EQ(hEvent, event_initial);
}

void event_host_synchronize(ze_event_handle_t hEvent, uint64_t timeout) {
  auto event_initial = hEvent;
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(hEvent, timeout));
  EXPECT_EQ(hEvent, event_initial);
}

void event_host_reset(ze_event_handle_t hEvent) {
  auto event_initial = hEvent;
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(hEvent));
  EXPECT_EQ(hEvent, event_initial);
}
}; // namespace level_zero_tests
