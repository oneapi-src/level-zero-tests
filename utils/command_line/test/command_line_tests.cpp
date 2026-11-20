/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "command_line/command_line.hpp"
#include "utils/utils_gtest_helper.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace lzt = level_zero_tests;

namespace {

LZT_TEST(CommandLineOptions, ParsesLongOptionWithEqualsAndSeparateValue) {
  uint32_t width = 0;
  std::string name;
  lzt::command_line_options options;
  options.value("width", width, "").value("name", name, "");
  options.parse({"--width=64", "--name", "image"});
  EXPECT_EQ(64u, width);
  EXPECT_EQ("image", name);
}

LZT_TEST(CommandLineOptions, ParsesShortAliasWithSeparateAndAdjacentValue) {
  uint32_t width = 0;
  uint32_t height = 0;
  lzt::command_line_options options;
  options.value("width,w", width, "").value("height,h", height, "");
  options.parse({"-w", "8", "-h16"});
  EXPECT_EQ(8u, width);
  EXPECT_EQ(16u, height);
  EXPECT_TRUE(options.given("width"));
}

LZT_TEST(CommandLineOptions, KeepsDefaultWhenOptionIsAbsent) {
  uint32_t width = 2048;
  lzt::command_line_options options;
  options.value("width", width, "").flag("help", "");
  options.parse({});
  EXPECT_EQ(2048u, width);
  EXPECT_FALSE(options.given("width"));
  EXPECT_FALSE(options.given("help"));
}

LZT_TEST(CommandLineOptions, RecordsFlags) {
  lzt::command_line_options options;
  options.flag("use_sub_devices", "").flag("help", "");
  options.parse({"--use_sub_devices"});
  EXPECT_TRUE(options.given("use_sub_devices"));
  EXPECT_FALSE(options.given("help"));
}

LZT_TEST(CommandLineOptions, RejectsValueForFlag) {
  lzt::command_line_options options;
  options.flag("help", "");
  EXPECT_THROW(options.parse({"--help=1"}), lzt::command_line_error);
}

LZT_TEST(CommandLineOptions, RejectsMissingValue) {
  uint32_t width = 0;
  lzt::command_line_options options;
  options.value("width", width, "").flag("help", "");
  EXPECT_THROW(options.parse({"--width"}), lzt::command_line_error);
  EXPECT_THROW(options.parse({"--width", "--help"}), lzt::command_line_error);
}

LZT_TEST(CommandLineOptions, RejectsInvalidValues) {
  uint32_t width = 7;
  lzt::command_line_options options;
  options.value("width", width, "");
  EXPECT_THROW(options.parse({"--width=abc"}), lzt::command_line_error);
  EXPECT_THROW(options.parse({"--width=12abc"}), lzt::command_line_error);
  EXPECT_THROW(options.parse({"--width=-1"}), lzt::command_line_error);
  EXPECT_THROW(options.parse({"--width= -1"}), lzt::command_line_error);
  EXPECT_THROW(options.parse({"--width="}), lzt::command_line_error);
  EXPECT_EQ(7u, width);
}

enum test_mode { first_mode, second_mode, third_mode };

LZT_TEST(CommandLineOptions, ParsesEnumThroughUnderlyingType) {
  test_mode mode = first_mode;
  lzt::command_line_options options;
  options.value("mode", mode, "");
  options.parse({"--mode=2"});
  EXPECT_EQ(third_mode, mode);
  EXPECT_THROW(options.parse({"--mode=abc"}), lzt::command_line_error);
}

LZT_TEST(CommandLineOptions, AcceptsEmptyStringValue) {
  std::string module = "default";
  lzt::command_line_options options;
  options.value("module", module, "");
  options.parse({"--module="});
  EXPECT_TRUE(module.empty());
}

LZT_TEST(CommandLineOptions, RejectsMissingRequiredOption) {
  uint64_t index = 0;
  lzt::command_line_options options;
  options.required("index", index, "");
  EXPECT_THROW(options.parse({}), lzt::command_line_error);
  options.parse({"--index=5"});
  EXPECT_EQ(5u, index);
}

LZT_TEST(CommandLineOptions, AcceptsUniquePrefixOfLongOption) {
  uint32_t width = 0;
  lzt::command_line_options options;
  options.flag("help", "").value("width", width, "");
  options.parse({"--h", "--wid=7"});
  EXPECT_TRUE(options.given("help"));
  EXPECT_EQ(7u, width);
}

LZT_TEST(CommandLineOptions, PrefersExactMatchOverPrefix) {
  uint32_t size = 0;
  uint32_t size_x = 0;
  lzt::command_line_options options;
  options.value("size", size, "").value("size_x", size_x, "");
  options.parse({"--size=4"});
  EXPECT_EQ(4u, size);
  EXPECT_FALSE(options.given("size_x"));
}

LZT_TEST(CommandLineOptions, RejectsAmbiguousPrefix) {
  lzt::command_line_options options;
  options.flag("data-validation", "").flag("data-type", "").flag("debug", "");
  EXPECT_THROW(options.parse({"--d"}), lzt::command_line_error);
  EXPECT_THROW(options.parse({"--da"}), lzt::command_line_error);
  EXPECT_THROW(options.parse({"--da"}, true), lzt::command_line_error);
  options.parse({"--de"});
  EXPECT_TRUE(options.given("debug"));
}

LZT_TEST(CommandLineOptions, RejectsUnrecognizedArgumentsByDefault) {
  lzt::command_line_options options;
  EXPECT_THROW(options.parse({"--unknown"}), lzt::command_line_error);
  EXPECT_THROW(options.parse({"positional"}), lzt::command_line_error);
}

LZT_TEST(CommandLineOptions, ReturnsUnrecognizedArgumentsInOrder) {
  uint32_t width = 0;
  lzt::command_line_options options;
  options.value("width", width, "");
  const auto rest = options.parse(
      {"--gtest_filter=A.*", "--width=3", "positional", "-x"}, true);
  EXPECT_EQ(3u, width);
  EXPECT_EQ(
      (std::vector<std::string>{"--gtest_filter=A.*", "positional", "-x"}),
      rest);
}

LZT_TEST(CommandLineOptions, PrintsUsage) {
  uint32_t width = 0;
  lzt::command_line_options options("Allowed options");
  options.flag("help", "produce help message")
      .value("width,w", width, "set image width");
  std::ostringstream usage;
  usage << options;
  EXPECT_EQ("Allowed options:\n"
            "  --help              produce help message\n"
            "  -w [ --width ] arg  set image width\n",
            usage.str());
}

} // namespace
