/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_UTILS_STRING_HPP
#define level_zero_tests_UTILS_STRING_HPP

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string>
#include <vector>

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include <level_zero/zet_api.h>

namespace level_zero_tests {

std::string uuid_to_string(const uint8_t uuid[]);
std::vector<std::string> split_string(const std::string &string,
                                      const std::string &delimiter);
std::string join_strings(const std::vector<std::string> &tokens,
                         const std::string &delimiter);

std::string to_string(const ze_api_version_t version);
std::string to_string(const ze_result_t result);
std::string to_string(const ze_bool_t ze_bool);
std::string to_string(const ze_command_queue_flag_t flags);
std::string to_string(const ze_command_queue_mode_t mode);
std::string to_string(const ze_command_queue_priority_t priority);
std::string to_string(const ze_image_format_layout_t layout);
std::string to_string(const ze_image_format_type_t type);
std::string to_string(const ze_image_format_swizzle_t swizzle);
std::string to_string(const ze_image_flag_t flag);
std::string to_string(const ze_image_type_t type);
std::string to_string(const ze_memory_type_t type);
std::string to_string(const ze_device_fp_flag_t flags);
std::string to_string(const ze_memory_access_cap_flag_t flags);
std::string to_string(const ze_device_property_flag_t flags);
std::string to_string(const ze_driver_uuid_t uuid);
std::string to_string(const ze_device_uuid_t uuid);
std::string to_string(const ze_native_kernel_uuid_t uuid);
std::string to_string(const ze_device_type_t type);
std::string to_string(const zet_metric_group_sampling_type_flag_t flag);
std::string to_string(const zes_engine_group_t type);
std::string to_string(const zes_mem_type_t type);
std::string to_string(const zes_mem_loc_t type);
std::string to_string(const zes_freq_domain_t type);
std::string to_string(const zes_temp_sensors_t type);
std::string to_string(const zes_ras_error_type_t type);
std::string to_string(const zes_standby_type_t type);
std::string to_string(const zes_sched_mode_t mode);
std::string to_string(const zes_pci_bar_type_t type);
std::string to_string(const ze_device_memory_property_flag_t flag);
std::string to_string(const ze_ipc_property_flag_t flag);
std::string to_string(const ze_external_memory_type_flag_t flag);
std::string to_string(const ze_device_cache_property_flag_t flag);
std::string to_string(const ze_device_module_flag_t flag);
std::string to_string(const ze_scheduling_hint_exp_flag_t flag);
std::string to_string(const ze_device_fp_atomic_ext_flag_t flag);
std::string to_string(const ze_device_raytracing_ext_flag_t flag);
std::string to_string(const ze_command_queue_group_property_flag_t flag);
std::string to_string(const ze_mutable_command_list_exp_flags_t flag);
std::string to_string(const ze_mutable_command_exp_flag_t flag);
std::string to_string(const zet_device_debug_property_flag_t flag);
std::string to_string(const zes_engine_type_flag_t flag);
std::string to_string(const ze_memory_access_attribute_t attribute);
std::string to_string(const ze_device_readonly_memory_capability_t capability);
ze_image_format_layout_t to_layout(const std::string layout);
ze_image_format_type_t to_format_type(const std::string format_type);
ze_image_flags_t to_image_flag(const std::string flag);
uint32_t num_bytes_per_pixel(ze_image_format_layout_t layout);
ze_image_type_t to_image_type(const std::string type);

// Decodes a bitmask into a " | "-joined list of flag names by calling the
// matching single-flag to_string overload for each set bit.
template <typename T> std::string flags_to_string(uint32_t flags) {
  std::vector<std::string> output;
  for (int i = 0; i < std::bit_width(flags); ++i) {
    const uint32_t mask = uint32_t{1} << i;
    const uint32_t flag = flags & mask;
    if (flag != 0) {
      output.emplace_back(to_string(static_cast<T>(flag)));
    }
  }
  std::sort(output.begin(), output.end());
  return join_strings(output, " | ");
}

} // namespace level_zero_tests

std::ostream &operator<<(std::ostream &os, const ze_api_version_t &x);
std::ostream &operator<<(std::ostream &os, const ze_result_t &x);
std::ostream &operator<<(std::ostream &os, const ze_bool_t &x);
std::ostream &operator<<(std::ostream &os, const ze_command_queue_flag_t &x);
std::ostream &operator<<(std::ostream &os, const ze_command_queue_mode_t &x);
std::ostream &operator<<(std::ostream &os,
                         const ze_command_queue_priority_t &x);
std::ostream &operator<<(std::ostream &os, const ze_image_format_layout_t &x);
std::ostream &operator<<(std::ostream &os, const ze_image_format_type_t &x);
std::ostream &operator<<(std::ostream &os, const ze_image_format_swizzle_t &x);
std::ostream &operator<<(std::ostream &os, const ze_image_flag_t &x);
std::ostream &operator<<(std::ostream &os, const ze_image_type_t &x);

// std::ostream &operator<<(std::ostream &os, const ze_device_fp_flags_t &x);
std::ostream &operator<<(std::ostream &os, const ze_driver_uuid_t &x);
std::ostream &operator<<(std::ostream &os, const ze_device_uuid_t &x);
std::ostream &operator<<(std::ostream &os, const ze_native_kernel_uuid_t &x);

bool operator==(const ze_device_uuid_t &id_a, const ze_device_uuid_t &id_b);
bool operator!=(const ze_device_uuid_t &id_a, const ze_device_uuid_t &id_b);

bool operator==(const ze_device_thread_t &a, const ze_device_thread_t &b);
bool operator!=(const ze_device_thread_t &a, const ze_device_thread_t &b);

#endif