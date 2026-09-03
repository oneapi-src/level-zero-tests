/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "system/shared_library.hpp"

#include <dlfcn.h>

namespace level_zero_tests {

SharedLibrary::SharedLibrary(const std::string &name)
    : handle_{dlopen(name.c_str(), RTLD_LAZY)} {
  if (handle_ == nullptr) {
    throw SharedLibraryNotFoundException("Failed to load library: " + name);
  }
}

SharedLibrary::~SharedLibrary() {
  if (handle_) {
    dlclose(handle_);
  }
}

void *SharedLibrary::get_function_ptr(const std::string &name) const {
  void *function = dlsym(handle_, name.c_str());
  if (function == nullptr) {
    throw SharedLibraryFunctionNotFoundException(
        "Failed to load library function: " + name);
  }
  return function;
}

} // namespace level_zero_tests
