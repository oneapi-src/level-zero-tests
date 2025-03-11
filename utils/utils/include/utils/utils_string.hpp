/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_UTILS_STRING_HPP
#define level_zero_tests_UTILS_STRING_HPP

#include <string>

#include <level_zero/ze_api.h>

namespace level_zero_tests {

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
std::string to_string(const ze_device_fp_flag_t flags);
std::string to_string(const ze_memory_access_cap_flag_t flags);
std::string to_string(const ze_device_property_flag_t flags);
std::string to_string(const ze_driver_uuid_t uuid);
std::string to_string(const ze_device_uuid_t uuid);
std::string to_string(const ze_native_kernel_uuid_t uuid);
ze_image_format_layout_t to_layout(const std::string layout);
ze_image_format_type_t to_format_type(const std::string format_type);
ze_image_flags_t to_image_flag(const std::string flag);
uint32_t num_bytes_per_pixel(ze_image_format_layout_t layout);
ze_image_type_t to_image_type(const std::string type);

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