/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_UTILS_SYSTEM_HPP
#define level_zero_tests_UTILS_SYSTEM_HPP

#include <cassert>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
#include <boost/filesystem/path.hpp>

#include "utils/utils_type_convert.hpp"

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

[[nodiscard]] std::optional<std::string> getenv(const std::string &name);

void putenv(const std::string &name, const std::string &value);

void clearenv(const std::string &name);

// Returns the current environment as "KEY=VALUE" entries, with overrides
// replacing any inherited entry of the same name. Appending instead of
// replacing would leave duplicate keys, and getenv() reports the first one.
std::vector<std::string>
child_environment(const std::map<std::string, std::string> &overrides);

boost::filesystem::path
find_helper_executable(const boost::filesystem::path &name,
                       const std::vector<boost::filesystem::path> &directories);

} // namespace level_zero_tests

#endif
