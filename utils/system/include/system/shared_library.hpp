/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <stdexcept>
#include <string>

namespace level_zero_tests {

class SharedLibrary {
public:
  using Handle = void *;

  SharedLibrary(const std::string &name);

  virtual ~SharedLibrary();

  virtual void *get_function_ptr(const std::string &name) const;

  SharedLibrary(const SharedLibrary &) = delete;
  SharedLibrary(SharedLibrary &&) = delete;
  SharedLibrary &operator=(const SharedLibrary &) = delete;
  SharedLibrary &operator=(SharedLibrary) = delete;

private:
  Handle handle_;
};

class SharedLibraryNotFoundException : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class SharedLibraryFunctionNotFoundException : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

} // namespace level_zero_tests
