/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <functional>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace level_zero_tests {

class command_line_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class command_line_options {
public:
  explicit command_line_options(std::string caption = "Allowed options");

  command_line_options &flag(const std::string &names,
                             const std::string &description);

  template <typename T>
  command_line_options &value(const std::string &names, T &target,
                              const std::string &description) {
    add(names, description, assigner(target), false);
    return *this;
  }

  template <typename T>
  command_line_options &required(const std::string &names, T &target,
                                 const std::string &description) {
    add(names, description, assigner(target), true);
    return *this;
  }

  std::vector<std::string> parse(int argc, char **argv,
                                 bool allow_unrecognized = false);
  std::vector<std::string> parse(const std::vector<std::string> &arguments,
                                 bool allow_unrecognized = false);

  bool given(const std::string &name) const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const command_line_options &options);

private:
  using assign_function = std::function<bool(const std::string &)>;

  struct option {
    std::string long_name;
    char short_name = '\0';
    std::string description;
    assign_function assign;
    bool is_required = false;
    bool is_given = false;
  };

  template <typename T, typename = void>
  struct is_extractable : std::false_type {};
  template <typename T>
  struct is_extractable<T,
                        std::void_t<decltype(std::declval<std::istream &>() >>
                                             std::declval<T &>())>>
      : std::true_type {};

  template <typename T>
  static bool parse_value(const std::string &text, T &target) {
    if constexpr (std::is_same_v<T, std::string>) {
      target = text;
      return true;
    } else if constexpr (std::is_enum_v<T> && !is_extractable<T>::value) {
      std::underlying_type_t<T> raw{};
      if (!parse_value(text, raw)) {
        return false;
      }
      target = static_cast<T>(raw);
      return true;
    } else {
      const auto first = text.find_first_not_of(" \t\n\r\f\v");
      if (std::is_unsigned_v<T> && first != std::string::npos &&
          text[first] == '-') {
        return false;
      }
      std::istringstream stream(text);
      T parsed{};
      if (!(stream >> parsed) || !(stream >> std::ws).eof()) {
        return false;
      }
      target = parsed;
      return true;
    }
  }

  template <typename T> static assign_function assigner(T &target) {
    return [&target](const std::string &text) {
      return parse_value(text, target);
    };
  }

  void add(const std::string &names, const std::string &description,
           assign_function assign, bool is_required);
  template <typename Predicate>
  const option *find_if(Predicate predicate) const;
  option *find_long(const std::string &name);
  option *find_short(char name);

  std::string caption_;
  std::vector<option> options_;
};

} // namespace level_zero_tests
