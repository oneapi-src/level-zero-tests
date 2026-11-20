/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "benchmark_json.hpp"

#include <fstream>
#include <iostream>

namespace {

std::string escape(const std::string &text) {
  static const char hex_digits[] = "0123456789ABCDEF";
  std::string escaped;
  for (const char c : text) {
    switch (c) {
    case '"':
      escaped += "\\\"";
      break;
    case '\\':
      escaped += "\\\\";
      break;
    case '/':
      escaped += "\\/";
      break;
    case '\b':
      escaped += "\\b";
      break;
    case '\f':
      escaped += "\\f";
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    case '\t':
      escaped += "\\t";
      break;
    default: {
      const auto code = static_cast<unsigned char>(c);
      if (code < 0x20) {
        escaped += "\\u00";
        escaped += hex_digits[code / 16];
        escaped += hex_digits[code % 16];
      } else {
        escaped += c;
      }
    }
    }
  }
  return escaped;
}

std::string indent(size_t level) { return std::string(4 * level, ' '); }

} // namespace

void benchmark_report::add(const std::string &section,
                           std::vector<benchmark_record> records) {
  sections_.emplace_back(section, std::move(records));
}

void benchmark_report::write() const {
  if (path_.empty()) {
    return;
  }
  std::ofstream os(path_);
  os << "{\n" << indent(1) << "\"Performance Benchmark\": {\n";
  for (size_t s = 0; s < sections_.size(); ++s) {
    const auto &records = sections_[s].second;
    os << indent(2) << '"' << escape(sections_[s].first) << "\": [\n";
    for (size_t r = 0; r < records.size(); ++r) {
      const auto &fields = records[r].fields();
      os << indent(3) << "{\n";
      for (size_t f = 0; f < fields.size(); ++f) {
        os << indent(4) << '"' << escape(fields[f].first) << "\": \""
           << escape(fields[f].second) << '"'
           << (f + 1 < fields.size() ? "," : "") << '\n';
      }
      os << indent(3) << '}' << (r + 1 < records.size() ? "," : "") << '\n';
    }
    os << indent(2) << ']' << (s + 1 < sections_.size() ? "," : "") << '\n';
  }
  os << indent(1) << "}\n}" << std::endl;
  if (!os) {
    std::cerr << "Error writing json output: " << path_ << std::endl;
  }
}
