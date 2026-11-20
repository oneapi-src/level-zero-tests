/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class benchmark_record {
public:
  template <typename T> void put(const std::string &key, const T &value) {
    std::ostringstream stream;
    if constexpr (std::is_floating_point_v<T>) {
      stream.precision(std::numeric_limits<T>::max_digits10);
    }
    stream << value;
    fields_.emplace_back(key, stream.str());
  }

  const std::vector<std::pair<std::string, std::string>> &fields() const {
    return fields_;
  }

private:
  std::vector<std::pair<std::string, std::string>> fields_;
};

class benchmark_report {
public:
  explicit benchmark_report(std::string path) : path_(std::move(path)) {}

  void add(const std::string &section, std::vector<benchmark_record> records);
  void write() const;

private:
  std::string path_;
  std::vector<std::pair<std::string, std::vector<benchmark_record>>> sections_;
};
