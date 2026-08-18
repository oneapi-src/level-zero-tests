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
#include <filesystem>
#include <set>

LZT_TEST(LoadBinaryFile, ValidFile) {
  const std::vector<uint8_t> bytes =
      level_zero_tests::load_binary_file("binary_file.bin");
  const std::vector<uint8_t> reference = {0x00, 0x11, 0x22, 0x33};
  EXPECT_EQ(reference, bytes);
}

LZT_TEST(LoadBinaryFile, NotExistingFile) {
  const std::vector<uint8_t> bytes =
      level_zero_tests::load_binary_file("invalid/path");
  const std::vector<uint8_t> reference = {};
  EXPECT_EQ(reference, bytes);
}

LZT_TEST(SaveBinaryFile, ValidFile) {
  const std::vector<uint8_t> bytes = {0x00, 0x11, 0x22, 0x33};
  const std::string path = "output.bin";

  EXPECT_TRUE(level_zero_tests::save_binary_file(bytes, path));
  const std::vector<uint8_t> output = level_zero_tests::load_binary_file(path);
  if (std::remove(path.c_str()) != 0) {
    perror("Error deleting file");
  }

  EXPECT_EQ(bytes, output);
}

LZT_TEST(SaveBinaryFile, UnwritablePathReportsFailure) {
  const std::vector<uint8_t> bytes = {0x00, 0x11, 0x22, 0x33};
  EXPECT_FALSE(
      level_zero_tests::save_binary_file(bytes, "invalid/path/output.bin"));
}

LZT_TEST(ScopedTempFile, PathIsUniqueAcrossInstances) {
  std::set<std::string> paths;
  for (int i = 0; i < 32; i++) {
    const level_zero_tests::scoped_temp_file temp_file("module_add", ".native");
    EXPECT_TRUE(paths.insert(temp_file.path()).second)
        << "Duplicate temporary path: " << temp_file.path();
  }
}

LZT_TEST(ScopedTempFile, PathIncludesProcessIdToSeparateConcurrentRuns) {
  const level_zero_tests::scoped_temp_file temp_file("module_add", ".native");
  const std::string pid = std::to_string(level_zero_tests::get_process_id());

  EXPECT_NE(temp_file.path().find(pid), std::string::npos)
      << temp_file.path() << " does not contain pid " << pid;
  EXPECT_EQ(std::filesystem::path(temp_file.path()).extension(), ".native");
}

LZT_TEST(ScopedTempFile, FileIsRemovedOnScopeExit) {
  const std::vector<uint8_t> bytes = {0x00, 0x11, 0x22, 0x33};
  std::string path;
  {
    const level_zero_tests::scoped_temp_file temp_file("module_add", ".native");
    path = temp_file.path();
    ASSERT_TRUE(level_zero_tests::save_binary_file(bytes, path));
    EXPECT_TRUE(std::filesystem::exists(path));
  }
  EXPECT_FALSE(std::filesystem::exists(path));
}

LZT_TEST(ScopedTempFile, DestructorToleratesMissingFile) {
  std::string path;
  {
    const level_zero_tests::scoped_temp_file temp_file("module_add", ".native");
    path = temp_file.path();
  }
  EXPECT_FALSE(std::filesystem::exists(path));
}
