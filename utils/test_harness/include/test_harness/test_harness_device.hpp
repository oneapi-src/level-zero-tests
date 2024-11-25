/*
 *
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_DEVICE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_DEVICE_HPP

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include "gtest/gtest.h"
#include <mutex>

namespace level_zero_tests {

class zeDevice {
public:
  static zeDevice *get_instance();
  ze_device_handle_t get_device();
  ze_driver_handle_t get_driver();

private:
  zeDevice();
  static zeDevice *instance_;
  static std::once_flag instance;
  ze_device_handle_t device_ = nullptr;
  ze_driver_handle_t driver_ = nullptr;
};

void initialize_core();
ze_device_handle_t get_root_device(ze_device_handle_t device);
uint32_t get_zes_device_count();
uint32_t get_zes_device_count(zes_driver_handle_t driver);
std::vector<zes_device_handle_t> get_zes_devices();
std::vector<zes_device_handle_t> get_zes_devices(uint32_t count);
std::vector<zes_device_handle_t> get_zes_devices(zes_driver_handle_t driver);
std::vector<zes_device_handle_t> get_zes_devices(uint32_t count,
                                                 zes_driver_handle_t driver);
uint32_t get_ze_device_count();
uint32_t get_ze_device_count(ze_driver_handle_t driver);
std::vector<ze_device_handle_t> get_ze_devices();
std::vector<ze_device_handle_t> get_ze_devices(uint32_t count);
std::vector<ze_device_handle_t> get_ze_devices(ze_driver_handle_t driver);
std::vector<ze_device_handle_t> get_ze_devices(uint32_t count,
                                               ze_driver_handle_t driver);
uint32_t get_ze_sub_device_count(ze_device_handle_t device);
std::vector<ze_device_handle_t> get_ze_sub_devices(ze_device_handle_t device);
std::vector<ze_device_handle_t> get_ze_sub_devices(ze_device_handle_t device,
                                                   uint32_t count);

ze_device_properties_t get_device_properties(ze_device_handle_t device,
                                             ze_structure_type_t stype);
ze_device_properties_t get_device_properties(ze_device_handle_t device);
uint32_t get_command_queue_group_properties_count(ze_device_handle_t device);
std::vector<ze_command_queue_group_properties_t>
get_command_queue_group_properties(ze_device_handle_t device);
std::vector<ze_command_queue_group_properties_t>
get_command_queue_group_properties(ze_device_handle_t device, uint32_t count);
ze_device_compute_properties_t
get_compute_properties(ze_device_handle_t device);
uint32_t get_memory_properties_count(ze_device_handle_t device);
std::vector<ze_device_memory_properties_t>
get_memory_properties(ze_device_handle_t device);
std::vector<ze_device_memory_properties_t>
get_memory_properties(ze_device_handle_t device, uint32_t count);
std::vector<ze_device_memory_properties_t> get_memory_properties_ext(
    ze_device_handle_t device, uint32_t count,
    std::vector<ze_device_memory_ext_properties_t> &extProperties);
ze_device_external_memory_properties_t
get_external_memory_properties(ze_device_handle_t device);
ze_device_memory_access_properties_t
get_memory_access_properties(ze_device_handle_t device);
bool is_concurrent_memory_access_supported(ze_device_handle_t device);
std::vector<ze_device_cache_properties_t>
get_cache_properties(ze_device_handle_t device);
ze_device_image_properties_t get_image_properties(ze_device_handle_t device);
ze_device_module_properties_t
get_device_module_properties(ze_device_handle_t device);
std::tuple<uint64_t, uint64_t> get_global_timestamps(ze_device_handle_t device);

ze_device_p2p_properties_t get_p2p_properties(ze_device_handle_t dev1,
                                              ze_device_handle_t dev2);
ze_bool_t can_access_peer(ze_device_handle_t dev1, ze_device_handle_t dev2);

void set_kernel_cache_config(ze_kernel_handle_t kernel,
                             ze_cache_config_flags_t config);

void make_memory_resident(ze_device_handle_t device, void *memory,
                          const size_t size);
ze_result_t get_device_status(ze_device_handle_t device);
void evict_memory(ze_device_handle_t device, void *memory, const size_t size);
ze_float_atomic_ext_properties_t
get_device_module_float_atomic_properties(ze_device_handle_t device);
#ifdef ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME
ze_scheduling_hint_exp_properties_t
get_device_kernel_schedule_hints(ze_device_handle_t device);
#endif

}; // namespace level_zero_tests
#endif
