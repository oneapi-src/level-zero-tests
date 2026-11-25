/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "command_line/command_line.hpp"

#include <algorithm>
#include <optional>
#include <utility>

namespace level_zero_tests {

namespace {

bool looks_like_option(const std::string &argument) {
  return argument.size() > 1 && argument.front() == '-';
}

} // namespace

command_line_options::command_line_options(std::string caption)
    : caption_(std::move(caption)) {}

command_line_options &
command_line_options::flag(const std::string &names,
                           const std::string &description) {
  add(names, description, nullptr, false);
  return *this;
}

void command_line_options::add(const std::string &names,
                               const std::string &description,
                               assign_function assign, bool is_required) {
  option entry;
  const auto comma = names.find(',');
  entry.long_name = names.substr(0, comma);
  if (comma != std::string::npos && comma + 1 < names.size()) {
    entry.short_name = names[comma + 1];
  }
  entry.description = description;
  entry.assign = std::move(assign);
  entry.is_required = is_required;
  options_.push_back(std::move(entry));
}

command_line_options::option *
command_line_options::find_long(const std::string &name) {
  const auto exact =
      std::find_if(options_.begin(), options_.end(),
                   [&](const option &o) { return o.long_name == name; });
  if (exact != options_.end()) {
    return &*exact;
  }
  if (name.empty()) {
    return nullptr;
  }
  std::vector<option *> candidates;
  for (auto &entry : options_) {
    if (entry.long_name.rfind(name, 0) == 0) {
      candidates.push_back(&entry);
    }
  }
  if (candidates.size() > 1) {
    std::string names;
    for (const option *entry : candidates) {
      names += (names.empty() ? "'--" : ", '--") + entry->long_name + "'";
    }
    throw command_line_error("option '--" + name +
                             "' is ambiguous and matches " + names);
  }
  return candidates.empty() ? nullptr : candidates.front();
}

command_line_options::option *command_line_options::find_short(char name) {
  const auto it =
      std::find_if(options_.begin(), options_.end(),
                   [&](const option &o) { return o.short_name == name; });
  return it == options_.end() ? nullptr : &*it;
}

std::vector<std::string> command_line_options::parse(int argc, char **argv,
                                                     bool allow_unrecognized) {
  return parse(std::vector<std::string>(argv + 1, argv + argc),
               allow_unrecognized);
}

std::vector<std::string>
command_line_options::parse(const std::vector<std::string> &arguments,
                            bool allow_unrecognized) {
  std::vector<std::string> unrecognized;
  for (size_t i = 0; i < arguments.size(); ++i) {
    const std::string &argument = arguments[i];
    option *entry = nullptr;
    std::optional<std::string> inline_value;
    if (argument.rfind("--", 0) == 0) {
      std::string name = argument.substr(2);
      const auto equals = name.find('=');
      if (equals != std::string::npos) {
        inline_value = name.substr(equals + 1);
        name.resize(equals);
      }
      entry = find_long(name);
    } else if (looks_like_option(argument)) {
      entry = find_short(argument[1]);
      if (entry && argument.size() > 2) {
        inline_value = argument.substr(2);
      }
    }

    if (!entry) {
      if (!allow_unrecognized) {
        throw command_line_error(looks_like_option(argument)
                                     ? "unrecognised option '" + argument + "'"
                                     : "unexpected argument '" + argument +
                                           "'");
      }
      unrecognized.push_back(argument);
      continue;
    }

    entry->is_given = true;
    if (!entry->assign) {
      if (inline_value) {
        throw command_line_error("option '--" + entry->long_name +
                                 "' does not take any arguments");
      }
      continue;
    }

    if (!inline_value) {
      if (i + 1 >= arguments.size() || looks_like_option(arguments[i + 1])) {
        throw command_line_error("the required argument for option '--" +
                                 entry->long_name + "' is missing");
      }
      inline_value = arguments[++i];
    }
    if (!entry->assign(*inline_value)) {
      throw command_line_error("the argument ('" + *inline_value +
                               "') for option '--" + entry->long_name +
                               "' is invalid");
    }
  }

  for (const auto &entry : options_) {
    if (entry.is_required && !entry.is_given) {
      throw command_line_error("the option '--" + entry.long_name +
                               "' is required but missing");
    }
  }
  return unrecognized;
}

bool command_line_options::given(const std::string &name) const {
  return std::any_of(options_.begin(), options_.end(), [&](const option &o) {
    return o.long_name == name && o.is_given;
  });
}

std::ostream &operator<<(std::ostream &os,
                         const command_line_options &options) {
  std::vector<std::string> usages;
  size_t width = 0;
  for (const auto &entry : options.options_) {
    std::string usage = "--" + entry.long_name;
    if (entry.short_name != '\0') {
      usage = std::string("-") + entry.short_name + " [ " + usage + " ]";
    }
    if (entry.assign) {
      usage += " arg";
    }
    width = std::max(width, usage.size());
    usages.push_back(std::move(usage));
  }

  os << options.caption_ << ":\n";
  for (size_t i = 0; i < usages.size(); ++i) {
    os << "  " << usages[i] << std::string(width - usages[i].size() + 2, ' ')
       << options.options_[i].description << '\n';
  }
  return os;
}

} // namespace level_zero_tests
