/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <boost/asio/io_context.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace bp = boost::process::v2;
namespace fs = boost::filesystem;

#include <level_zero/ze_api.h>

namespace {

constexpr size_t num_processes = 8;

void RunGivenMultipleProcessesUsingMultipleDevicesKernelsTest(
    lzt::command_list_mode_t mode, int is_stress_test) {

  std::array<int, num_processes> process_results;
  boost::asio::io_context io_ctx;
  std::vector<bp::process> processes;
  fs::path helper_path(fs::current_path() / "process");
  fs::path helper =
      lzt::find_helper_executable("test_process_helper", {helper_path});

  for (size_t i = 0U; i < num_processes; i++) {
    const auto child_env =
        lzt::child_environment({{"ZE_ENABLE_PCI_ID_DEVICE_ORDER", "1"}});
    bp::process execute_kernel_process(
        io_ctx, helper,
        {std::to_string(i), to_string(mode), std::to_string(is_stress_test)},
        bp::process_environment{child_env});
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
