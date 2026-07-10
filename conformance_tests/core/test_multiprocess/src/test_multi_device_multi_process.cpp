/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <boost/process.hpp>
#include <boost/filesystem.hpp>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace bp = boost::process;
namespace fs = boost::filesystem;

#include <level_zero/ze_api.h>

namespace {

constexpr size_t num_processes = 8;

void RunGivenMultipleProcessesUsingMultipleDevicesKernelsTest(
    lzt::command_list_mode_t mode, int is_stress_test) {

  std::array<int, num_processes> process_results;
  std::vector<bp::child> processes;
  fs::path helper_path(fs::current_path() / "process");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);

  for (size_t i = 0U; i < num_processes; i++) {
    auto env = boost::this_process::environment();
    bp::environment child_env = env;
    child_env["ZE_ENABLE_PCI_ID_DEVICE_ORDER"] = "1";
    fs::path helper = bp::search_path("test_process_helper", paths);
    bp::child execute_kernel_process(
        helper,
        bp::args({std::to_string(i), to_string(mode),
                  std::to_string(is_stress_test)}),
        child_env);
    processes.push_back(std::move(execute_kernel_process));
  }

  // verification
  for (size_t i = 0U; i < num_processes; i++) {
    processes[i].wait();
    int result = processes[i].exit_code();
    EXPECT_EQ(result, 0);
  }
}

LZT_TEST(MultiProcessTests,
         GivenMultipleProcessesUsingMultipleDevicesKernelsExecuteCorrectly) {
  RunGivenMultipleProcessesUsingMultipleDevicesKernelsTest(
      lzt::command_list_mode_t::regular, 0);
}

LZT_TEST(
    MultiProcessTests,
    GivenMultipleProcessesUsingMultipleDevicesKernelsExecuteOnImmediateCmdListCorrectly) {
  RunGivenMultipleProcessesUsingMultipleDevicesKernelsTest(
      lzt::command_list_mode_t::immediate, 0);
}

LZT_TEST(
    MultiProcessTests,
    GivenMultipleProcessesUsingMultipleSubDevicesThenKernelIsStressedAndExecuteSuccessfully) {
  RunGivenMultipleProcessesUsingMultipleDevicesKernelsTest(
      lzt::command_list_mode_t::regular, 1);
}

} // namespace
