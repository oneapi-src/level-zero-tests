/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_UTILS_HPP
#define level_zero_tests_UTILS_HPP

#include <vector>
#include <memory>
#include <map>

#include <level_zero/zet_api.h>
#include <level_zero/zes_api.h>

#include "utils/utils_string.hpp"

namespace level_zero_tests {

zes_driver_handle_t get_default_zes_driver();

ze_context_handle_t get_default_context();
ze_device_handle_t get_default_device(ze_driver_handle_t driver);
ze_device_handle_t find_device(ze_driver_handle_t &driver,
                               const char *device_id, bool sub_device);
void sort_devices(std::vector<ze_device_handle_t> &devices);
ze_driver_handle_t get_default_driver();
ze_context_handle_t create_context();
ze_context_handle_t create_context(ze_driver_handle_t driver);

ze_context_handle_t create_context_ex(ze_driver_handle_t driver,
                                      std::vector<ze_device_handle_t> devices);
ze_context_handle_t create_context_ex(ze_driver_handle_t driver);

void destroy_context(ze_context_handle_t context);
std::vector<ze_device_handle_t> get_devices(ze_driver_handle_t driver);
std::vector<ze_driver_handle_t> get_all_driver_handles();
std::vector<ze_device_handle_t> get_all_sub_devices();

uint32_t get_device_count(ze_driver_handle_t driver);
uint32_t get_driver_handle_count();
uint32_t get_sub_device_count(ze_device_handle_t device);

void print_driver_version();
void print_driver_overview(const ze_driver_handle_t driver);
void print_driver_overview(const std::vector<ze_driver_handle_t> driver);
void print_platform_overview(const std::string context);
void print_platform_overview();

std::vector<uint8_t> load_binary_file(const std::string &file_path);
void save_binary_file(const std::vector<uint8_t> &data,
                      const std::string &file_path);
uint32_t nextPowerOfTwo(uint32_t value);

template <typename T> int size_in_bytes(const std::vector<T> &v) {
  return static_cast<int>(sizeof(T) * v.size());
}

extern std::unique_ptr<std::map<std::string, std::vector<uint8_t>>>
    binary_file_map;

} // namespace level_zero_tests

#endif
