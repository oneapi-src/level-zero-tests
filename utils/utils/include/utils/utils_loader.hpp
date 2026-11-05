/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "system/shared_library.hpp"

#include <string>
#include <string_view>

namespace level_zero_tests {

[[nodiscard]] inline void *
get_loader_function_address(const std::string_view name) {
#ifdef _WIN32
  static const SharedLibrary loader("ze_loader.dll");
#else
  static const SharedLibrary loader("libze_loader.so.1");
#endif
  return loader.get_function_ptr(std::string(name));
}

template <typename Function>
[[nodiscard]] Function get_loader_function(const std::string_view name) {
  return reinterpret_cast<Function>(get_loader_function_address(name));
}

} // namespace level_zero_tests

#ifdef LZT_USE_STATIC_LOADER
#define LZT_LOADER_FUNCTION(function) (&function)
#else
#define LZT_LOADER_FUNCTION(function)                                          \
  level_zero_tests::get_loader_function<decltype(&function)>(#function)
#endif
