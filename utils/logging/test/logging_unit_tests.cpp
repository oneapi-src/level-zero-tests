/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "gtest/gtest.h"

#include "command_line/command_line.hpp"

#include <regex>
#include <sstream>
#include <thread>

namespace lzt = level_zero_tests;

class LoggingTest : public ::testing::Test {
protected:
  void SetUp() override {
    lzt::LoggingSettings settings;
    settings.level = lzt::logging_level::trace;
    settings.format = lzt::logging_format::simple;
    lzt::init_logging(settings);
    logs = std::make_shared<std::stringstream>();
    lzt::add_stream(logs);
  }

  void TearDown() override { lzt::stop_logging(); }

  std::shared_ptr<std::stringstream> logs;
};

LZT_TEST_F(LoggingTest, PrintTrace) {
  LOG_TRACE << "Message";
  EXPECT_EQ("[trace] Message\n", logs->str());
}

LZT_TEST_F(LoggingTest, PrintDebug) {
  LOG_DEBUG << "Message";
  EXPECT_EQ("[debug] Message\n", logs->str());
}

LZT_TEST_F(LoggingTest, PrintInfo) {
  LOG_INFO << "Message";
  EXPECT_EQ("[info] Message\n", logs->str());
}

LZT_TEST_F(LoggingTest, PrintWarning) {
  LOG_WARNING << "Message";
  EXPECT_EQ("[warning] Message\n", logs->str());
}

LZT_TEST_F(LoggingTest, PrintError) {
  LOG_ERROR << "Message";
  EXPECT_EQ("[error] Message\n", logs->str());
}

LZT_TEST_F(LoggingTest, PrintFatal) {
  LOG_FATAL << "Message";
  EXPECT_EQ("[fatal] Message\n", logs->str());
}

LZT_TEST_F(LoggingTest, MessageEndingWithNewlineIsNotDoubled) {
  LOG_INFO << "Message" << std::endl;
  EXPECT_EQ("[info] Message\n", logs->str());
}

static int throw_error() { throw std::runtime_error("error"); }

LZT_TEST_F(LoggingTest, MessageIsDroppedWhenExpressionThrows) {
  EXPECT_THROW(LOG_INFO << "Message" << throw_error(), std::runtime_error);
  EXPECT_EQ("", logs->str());
}

LZT_TEST(LoggingCommandLineParser, ChooseSimpleFormatFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-format=simple"};
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_format::simple, settings.format);
}

LZT_TEST(LoggingCommandLineParser, ChoosePreciseFormatFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-format=precise"};
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_format::precise, settings.format);
}

LZT_TEST(LoggingCommandLineParser, PreciseFormatIsDefault) {
  std::vector<std::string> cmd;
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_format::precise, settings.format);
}

LZT_TEST(LoggingCommandLineParser, ChooseUnknownFormatFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-format=unknown"};
  EXPECT_THROW(lzt::parse_command_line(cmd), lzt::command_line_error);
}

LZT_TEST(LoggingCommandLineParser, ConsumeOnlyKnownOptionsFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-format=precise",
                                  "positional_option", "--option"};
  lzt::parse_command_line(cmd);
  EXPECT_EQ(2, cmd.size());
}

LZT_TEST(LoggingCommandLineParser, ChooseTraceLevelFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-level=trace"};
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_level::trace, settings.level);
}

LZT_TEST(LoggingCommandLineParser, ChooseDebugLevelFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-level=debug"};
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_level::debug, settings.level);
}

LZT_TEST(LoggingCommandLineParser, ChooseInfoLevelFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-level=info"};
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_level::info, settings.level);
}

LZT_TEST(LoggingCommandLineParser, ChooseWarningLevelFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-level=warning"};
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_level::warning, settings.level);
}

LZT_TEST(LoggingCommandLineParser, ChooseErrorLevelFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-level=error"};
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_level::error, settings.level);
}

LZT_TEST(LoggingCommandLineParser, ChooseFatalLevelFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-level=fatal"};
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_level::fatal, settings.level);
}

LZT_TEST(LoggingCommandLineParser, InfoLevelIsDefault) {
  std::vector<std::string> cmd;
  const lzt::LoggingSettings settings = lzt::parse_command_line(cmd);
  EXPECT_EQ(lzt::logging_level::info, settings.level);
}

LZT_TEST(LoggingCommandLineParser, ChooseUnknownLevelFromCommandLine) {
  std::vector<std::string> cmd = {"--logging-level=unknown"};
  EXPECT_THROW(lzt::parse_command_line(cmd), lzt::command_line_error);
}

class LoggingInitTest : public ::testing::Test {
protected:
  void SetUp() override { logs = std::make_shared<std::stringstream>(); }

  void TearDown() override { lzt::stop_logging(); }

  std::shared_ptr<std::stringstream> logs;
};

LZT_TEST_F(LoggingInitTest, SimpleFormatFromSettings) {
  lzt::LoggingSettings settings;
  settings.format = lzt::logging_format::simple;
  lzt::init_logging(settings);
  lzt::add_stream(logs);
  LOG_INFO << "Message";
  EXPECT_EQ("[info] Message\n", logs->str());
}

LZT_TEST_F(LoggingInitTest, PreciseFormatFromSettings) {
  lzt::LoggingSettings settings;
  settings.format = lzt::logging_format::precise;
  lzt::init_logging(settings);
  lzt::add_stream(logs);

  LOG_INFO << "Message";

  const std::string timestamp = "\\[.+\\]";
  const std::string severity = "\\[info\\]";
  const std::string message = "Message\\n";
  const std::regex r(timestamp + " " + severity + " " + message);
  EXPECT_TRUE(std::regex_match(logs->str(), r));
}

LZT_TEST_F(LoggingInitTest, WarningLevelFromSettings) {
  lzt::LoggingSettings settings;
  settings.level = lzt::logging_level::warning;
  settings.format = lzt::logging_format::simple;
  lzt::init_logging(settings);
  lzt::add_stream(logs);

  LOG_INFO << "Message";
  EXPECT_EQ("", logs->str());
  LOG_WARNING << "Message";
  EXPECT_EQ("[warning] Message\n", logs->str());
}

LZT_TEST(LoggingDefaultOutput, PrintsToStdoutWhenNotInitialized) {
  testing::internal::CaptureStdout();
  LOG_FATAL << "Message";
  const std::string output = testing::internal::GetCapturedStdout();

  const std::regex r("\\[\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{6}\\] "
                     "\\[0x[0-9a-f]{16}\\] \\[fatal\\]   Message\\n");
  EXPECT_TRUE(std::regex_match(output, r)) << output;
}

LZT_TEST(LoggingDefaultOutput, ThreadIdIsPrintedInHex) {
  testing::internal::CaptureStdout();
  LOG_FATAL << "Message";
  const std::string output = testing::internal::GetCapturedStdout();

  std::smatch match;
  const std::regex r("\\[[^\\]]+\\] \\[0x([0-9a-f]{16})\\] .*\\n");
  ASSERT_TRUE(std::regex_match(output, match, r)) << output;

  std::ostringstream expected;
  expected << std::this_thread::get_id();
  EXPECT_EQ(std::stoull(expected.str()), std::stoull(match[1], nullptr, 16))
      << output;
}

LZT_TEST(LoggingDefaultOutput, MessageEndingWithNewlineIsNotDoubled) {
  testing::internal::CaptureStdout();
  LOG_FATAL << "Message" << std::endl;
  const std::string output = testing::internal::GetCapturedStdout();

  const std::regex r("\\[[^\\]]+\\] \\[[^\\]]+\\] \\[fatal\\]   Message\\n");
  EXPECT_TRUE(std::regex_match(output, r)) << output;
}

LZT_TEST(VectorToString, Empty) {
  const std::vector<int> x;
  EXPECT_EQ("[]", lzt::to_string(x));
}

LZT_TEST(VectorToString, SingleElement) {
  const std::vector<int> x = {1};
  EXPECT_EQ("[1]", lzt::to_string(x));
}

LZT_TEST(VectorToString, MultipleElements) {
  const std::vector<int> x = {1, 2, 3};
  EXPECT_EQ("[1, 2, 3]", lzt::to_string(x));
}

LZT_TEST(VectorToString, StringType) {
  const std::vector<std::string> x = {"ab", "cd", "ef"};
  EXPECT_EQ("[ab, cd, ef]", lzt::to_string(x));
}

LZT_TEST(VectorToString, LoggingFormatType) {
  const std::vector<lzt::logging_format> x = {lzt::logging_format::simple,
                                              lzt::logging_format::precise};
  EXPECT_EQ("[simple, precise]", lzt::to_string(x));
}
