/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils_system.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>

#include <boost/process.hpp>

#if defined(_WIN64) || defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <Sysinfoapi.h>
#else
#include <unistd.h>
#endif

namespace level_zero_tests {

#if defined(_WIN64) || defined(_WIN32)

uint64_t total_available_host_memory() {
  MEMORYSTATUSEX stat;
  stat.dwLength = sizeof(stat);
  GlobalMemoryStatusEx(&stat);

  return stat.ullAvailPhys;
}

namespace detail {
uint64_t get_page_size() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwPageSize;
}
} // namespace detail

uint32_t get_process_id() { return to_u32(GetCurrentProcessId()); }

#else

uint64_t total_available_host_memory() {
  const uint64_t page_count = to_u64(sysconf(_SC_AVPHYS_PAGES));
  const uint64_t page_size = to_u64(sysconf(_SC_PAGE_SIZE));
  return page_count * page_size;
}

uint32_t get_process_id() { return to_u32(getpid()); }

namespace detail {
uint64_t get_page_size() { return to_u64(sysconf(_SC_PAGE_SIZE)); }
} // namespace detail

#endif

std::optional<std::string> getenv(const std::string &name) {
#if defined(_WIN64) || defined(_WIN32)
  char *raw_value = nullptr;
  size_t length = 0;
  if (_dupenv_s(&raw_value, &length, name.c_str()) != 0 ||
      raw_value == nullptr) {
    return std::nullopt;
  }
  const std::unique_ptr<char, decltype(&std::free)> owned(raw_value,
                                                          &std::free);
  std::string value(raw_value);
#else
  const char *raw_value = std::getenv(name.c_str());
  if (raw_value == nullptr) {
    return std::nullopt;
  }
  std::string value(raw_value);
#endif
  if (value.empty()) {
    return std::nullopt;
  }
  return value;
}

void putenv(const std::string &name, const std::string &value) {
#if defined(_WIN64) || defined(_WIN32)
  const int error = _putenv_s(name.c_str(), value.c_str());
#else
  const int error = setenv(name.c_str(), value.c_str(), 1) == 0 ? 0 : errno;
#endif
  if (error != 0) {
    throw std::runtime_error("Failed to set environment variable " + name +
                             ": " + std::strerror(error));
  }
}

void clearenv(const std::string &name) {
#if defined(_WIN64) || defined(_WIN32)
  const int error = _putenv_s(name.c_str(), "");
#else
  const int error = unsetenv(name.c_str()) == 0 ? 0 : errno;
#endif
  if (error != 0) {
    throw std::runtime_error("Failed to clear environment variable " + name +
                             ": " + std::strerror(error));
  }
}

std::vector<std::string>
child_environment(const std::map<std::string, std::string> &overrides) {
  namespace bp = boost::process::v2;

  // environment::key comparison follows the platform's rules, so the overrides
  // also match case-insensitively on Windows.
  std::vector<bp::environment::key> overridden_keys;
  overridden_keys.reserve(overrides.size());
  for (const auto &override_entry : overrides) {
    overridden_keys.emplace_back(override_entry.first);
  }

  std::vector<std::string> environment;
  for (auto entry : bp::environment::current()) {
    const auto overridden =
        std::any_of(overridden_keys.begin(), overridden_keys.end(),
                    [&entry](const bp::environment::key &key) {
                      return entry.key().compare(key.native_view()) == 0;
                    });
    if (!overridden) {
      environment.push_back(entry.string());
    }
  }
  for (const auto &override_entry : overrides) {
    environment.push_back(override_entry.first + "=" + override_entry.second);
  }
  return environment;
}

boost::filesystem::path find_helper_executable(
    const boost::filesystem::path &name,
    const std::vector<boost::filesystem::path> &directories) {
  for (const auto &directory : directories) {
    const auto environment = child_environment({{"PATH", directory.string()}});
    auto executable =
        boost::process::v2::environment::find_executable(name, environment);
    if (!executable.empty()) {
      return executable;
    }
  }
  throw std::runtime_error("Could not find helper executable: " +
                           name.string());
}

} // namespace level_zero_tests
