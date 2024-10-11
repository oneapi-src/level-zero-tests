/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "gtest/gtest.h"
#include <cstdio>

TEST(LoadBinaryFile, ValidFile) {
  const std::vector<uint8_t> bytes =
      level_zero_tests::load_binary_file("binary_file.bin");
  const std::vector<uint8_t> reference = {0x00, 0x11, 0x22, 0x33};
  EXPECT_EQ(reference, bytes);
}

TEST(LoadBinaryFile, NotExistingFile) {
  const std::vector<uint8_t> bytes =
      level_zero_tests::load_binary_file("invalid/path");
  const std::vector<uint8_t> reference = {};
  EXPECT_EQ(reference, bytes);
}

TEST(SaveBinaryFile, ValidFile) {
  const std::vector<uint8_t> bytes = {0x00, 0x11, 0x22, 0x33};
  const std::string path = "output.bin";

  level_zero_tests::save_binary_file(bytes, path);
  const std::vector<uint8_t> output = level_zero_tests::load_binary_file(path);
  if (std::remove(path.c_str()) != 0) {
    perror("Error deleting file");
  }

  EXPECT_EQ(bytes, output);
}
