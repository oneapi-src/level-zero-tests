/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "system/shared_library.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace level_zero_tests {

SharedLibrary::SharedLibrary(const std::string &name)
    : handle_{LoadLibraryA(name.c_str())} {
  if (handle_ == nullptr) {
    throw SharedLibraryNotFoundException("Failed to load library: " + name);
  }
}

SharedLibrary::~SharedLibrary() {
  if (handle_) {
    FreeLibrary(static_cast<HMODULE>(handle_));
  }
}

void *SharedLibrary::get_function_ptr(const std::string &name) const {
  void *function = reinterpret_cast<void *>(
      GetProcAddress(static_cast<HMODULE>(handle_), name.c_str()));
  if (function == nullptr) {
    throw SharedLibraryFunctionNotFoundException(
        "Failed to load library function: " + name);
  }
  return function;
}

} // namespace level_zero_tests
