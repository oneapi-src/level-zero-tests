/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_EVENT_HPP
#define level_zero_tests_ZE_TEST_HARNESS_EVENT_HPP

#include "test_harness/test_harness.hpp"
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace level_zero_tests {

class zeEventPool {
public:
  zeEventPool();
  ~zeEventPool();

  // By default, an event pool is created with 32 events and default flags
  // during the first call to create_events().  To change the default behavior
  // call InitEventPool() with any other values BEFORE calling
  // create_events().
  void InitEventPool();
  void InitEventPool(uint32_t count);
  void InitEventPool(ze_context_handle_t context, uint32_t count);
  void InitEventPool(uint32_t count, ze_event_pool_flags_t flags);
  void InitEventPool(ze_context_handle_t context, uint32_t count,
                     ze_event_pool_flags_t flags);
  void InitEventPool(ze_event_pool_desc_t desc);
  void InitEventPool(ze_event_pool_desc_t desc,
                     std::vector<ze_device_handle_t> devices);

  void create_event(ze_event_handle_t &event);
  void create_event(ze_event_handle_t &event, ze_event_scope_flags_t signal,
                    ze_event_scope_flags_t wait);
  void create_event(ze_event_handle_t &event, ze_event_desc_t desc);

  void create_events(std::vector<ze_event_handle_t> &events,
                     size_t event_count);
  void create_events(std::vector<ze_event_handle_t> &events, size_t event_count,
                     ze_event_scope_flags_t signal,
                     ze_event_scope_flags_t wait);

  void destroy_event(ze_event_handle_t event);
  void destroy_events(std::vector<ze_event_handle_t> &events);

  void get_ipc_handle(ze_ipc_event_pool_handle_t *hIpc);

  ze_event_pool_handle_t event_pool_ = nullptr;
  ze_context_handle_t context_ = nullptr;
  std::vector<bool> pool_indexes_available_;
  std::map<ze_event_handle_t, uint32_t> handle_to_index_map_;
};

void signal_event_from_host(ze_event_handle_t hEvent);

ze_event_pool_handle_t create_event_pool(ze_context_handle_t context,
                                         uint32_t count,
                                         ze_event_pool_flags_t flags);
ze_event_pool_handle_t create_event_pool(ze_event_pool_desc_t desc);
ze_event_pool_handle_t create_event_pool(ze_context_handle_t context,
                                         ze_event_pool_desc_t desc);
ze_event_pool_handle_t
create_event_pool(ze_context_handle_t context, ze_event_pool_desc_t desc,
                  std::vector<ze_device_handle_t> devices);
void event_host_synchronize(ze_event_handle_t hEvent, uint64_t timeout);
void event_host_reset(ze_event_handle_t hEvent);
void put_ipc_event_handle(ze_context_handle_t context,
                          ze_ipc_event_pool_handle_t event_handle);
void open_ipc_event_handle(ze_context_handle_t context,
                           ze_ipc_event_pool_handle_t hIpc,
                           ze_event_pool_handle_t *eventPool);
void close_ipc_event_handle(ze_event_pool_handle_t eventPool);

ze_event_handle_t create_event(ze_event_pool_handle_t event_pool,
                               ze_event_desc_t desc);
void destroy_event(ze_event_handle_t event);
void destroy_event_pool(ze_event_pool_handle_t event_pool);
ze_kernel_timestamp_result_t
get_event_kernel_timestamp(ze_event_handle_t event);
double get_timestamp_time(const ze_kernel_timestamp_data_t *timestamp,
                          uint64_t timer_resolution,
                          uint64_t kernel_timestamp_valid_bits,
                          const ze_structure_type_t property_type =
                              ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES);
double
get_timestamp_global_duration(const ze_kernel_timestamp_result_t *timestamp,
                              const ze_device_handle_t &device,
                              const ze_driver_handle_t driver,
                              const ze_structure_type_t property_type =
                                  ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES);
double
get_timestamp_context_duration(const ze_kernel_timestamp_result_t *timestamp,
                               const ze_device_handle_t &device,
                               const ze_driver_handle_t driver,
                               const ze_structure_type_t property_type =
                                   ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES);

#ifdef ZE_EVENT_QUERY_TIMESTAMPS_EXP_NAME
uint32_t get_timestamp_count(const ze_event_handle_t &event,
                             const ze_device_handle_t &device);
std::vector<ze_kernel_timestamp_result_t>
get_event_timestamps_exp(const ze_event_handle_t &event,
                         const ze_device_handle_t &device);
std::vector<ze_kernel_timestamp_result_t>
get_event_timestamps_exp(const ze_event_handle_t &event,
                         const ze_device_handle_t &device, uint32_t count);
#endif // ZE_EVENT_QUERY_TIMESTAMPS_EXP_NAME
void get_event_kernel_timestamps_from_mapped_timestamp_event(
    const ze_event_handle_t &event, const ze_device_handle_t &device,
    std::vector<ze_kernel_timestamp_result_t> &kernel_timestamp_buffer,
    std::vector<ze_synchronized_timestamp_result_ext_t>
        &synchronized_timestamp_buffer);
class zeEventPoolTests : public ::testing::Test {
public:
  zeEventPool ep;
};

}; // namespace level_zero_tests

#endif
