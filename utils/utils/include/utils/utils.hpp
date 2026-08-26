/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_UTILS_HPP
#define level_zero_tests_UTILS_HPP

#include <cstdlib>
#include <cassert>
#include <limits>
#include <type_traits>
#include <vector>
#include <memory>
#include <map>
#include <optional>
#include <string>

#include <level_zero/zet_api.h>
#include <level_zero/zes_api.h>

#include "utils/utils_type_convert.hpp"
#include "utils/utils_string.hpp"
#include "utils/utils_gtest_helper.hpp"
#include "utils/utils_command_bundle.hpp"

namespace level_zero_tests {

uint64_t total_available_host_memory();
uint32_t get_process_id();

namespace detail {
uint64_t get_page_size();
}

template <typename T = uint64_t> [[nodiscard]] inline T get_page_size() {
  static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
                "get_page_size<T>() requires an integral T");
  const uint64_t page_size = detail::get_page_size();
  assert(page_size <= to_u64(std::numeric_limits<T>::max()));
  return static_cast<T>(page_size);
}

constexpr uint64_t nanosPerSecond = 1000000000;
constexpr uint64_t ns_in_five_ms = 5000000;

using nanosec = std::chrono::nanoseconds;

template <typename D> uint64_t to_nanoseconds(D &&dur) {
  return to_u64(
      std::chrono::duration_cast<nanosec>(std::forward<D>(dur)).count());
}

zes_driver_handle_t get_default_zes_driver();

ze_context_handle_t get_default_context();
std::vector<zes_driver_handle_t> get_all_zes_driver_handles();
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

std::optional<uint32_t>
get_queue_ordinal(const std::vector<ze_command_queue_group_properties_t>
                      &cmd_queue_group_props,
                  ze_command_queue_group_property_flags_t include_flags,
                  ze_command_queue_group_property_flags_t exclude_flags);

void print_driver_version();
void print_driver_overview(const ze_driver_handle_t driver);
void print_driver_overview(const std::vector<ze_driver_handle_t> driver);
void print_platform_overview(const std::string context);
void print_platform_overview();

std::vector<uint8_t> load_binary_file(const std::string &file_path);
[[nodiscard]] bool save_binary_file(const std::vector<uint8_t> &data,
                                    const std::string &file_path);

class scoped_temp_file {
public:
  scoped_temp_file(const std::string &stem, const std::string &extension);
  ~scoped_temp_file();

  scoped_temp_file(const scoped_temp_file &) = delete;
  scoped_temp_file &operator=(const scoped_temp_file &) = delete;

  const std::string &path() const { return path_; }

private:
  std::string path_;
};
uint32_t nextPowerOfTwo(uint32_t value);
inline uint32_t nextPowerOfTwo(uint64_t value) {
  return nextPowerOfTwo(to_u32(value));
}

template <typename T> size_t size_in_bytes(const std::vector<T> &v) {
  return sizeof(T) * v.size();
}

void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, uint32_t group_size,
                                 void *arg, command_list_mode_t mode);

struct FunctionArg {
  size_t arg_size;
  void *arg_value;
};

// Group size can only be set in x dimension
// Accepts arbitrary amounts of function arguments
void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, uint32_t group_size,
                                 const std::vector<FunctionArg> &args,
                                 command_list_mode_t mode);

extern std::unique_ptr<std::map<std::string, std::vector<uint8_t>>>
    binary_file_map;

} // namespace level_zero_tests

#endif
