/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils.hpp"

namespace compute_api_bench {

std::string load_text_file(size_t &length, const std::string &file_path) {
  std::ifstream stream(file_path.c_str());
  if (stream.fail()) {
    std::cout << "Misising kernel file: " << file_path << std::endl;
    exit(0);
  }

  std::istreambuf_iterator<char> eos;
  std::string string(std::istreambuf_iterator<char>(stream), eos);
  length = string.length();
  stream.close();
  return string;
}

std::vector<uint8_t> load_binary_file(size_t &length,
                                      const std::string &file_path) {
  std::ifstream stream(file_path, std::ios::in | std::ios::binary);
  std::vector<uint8_t> binary_file;
  if (stream.fail()) {
    std::cout << "Misising kernel file: " << file_path << std::endl;
    exit(0);
  }

  stream.seekg(0, stream.end);
  length = static_cast<size_t>(stream.tellg());
  stream.seekg(0, stream.beg);

  length = ceil(length / 4.0) * 4;
  binary_file.resize(length);
  stream.read(reinterpret_cast<char *>(binary_file.data()), length);
  stream.close();
  return binary_file;
}

void save_csv(const std::string &csv_string, const std::string &csv_filename) {
  std::ofstream stream(csv_filename);
  if (stream.fail()) {
    std::cout << "Cannot write to csv!" << std::endl;
    exit(0);
  }
  stream << csv_string;
  stream.close();
}

} // namespace compute_api_bench
