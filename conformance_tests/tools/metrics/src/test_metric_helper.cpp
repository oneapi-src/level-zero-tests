/*
 *
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_metric_utils.hpp"

#include <csignal>
#include <thread>

namespace lzt = level_zero_tests;

void interrupt_process() {

  // wait for signal from parent process
  LOG_DEBUG << "[Child] Waiting for signal from parent process";
  while (true) {
    std::string line;
    std::getline(std::cin, line);
    if (line == "stop") {
      break;
    }
  }

  LOG_DEBUG << "[Child] Interrupting process";
  std::raise(SIGINT);
}

int main(int argc, char **argv) {

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    std::cout << "[Application] zeInit failed";

    exit(1);
  }

  auto driver = lzt::get_default_driver();
  ze_device_handle_t device = nullptr;

  device = lzt::get_default_device(driver);

  ze_command_queue_handle_t commandQueue = lzt::create_command_queue(device);
  zet_command_list_handle_t commandList = lzt::create_command_list(device);

  ze_group_count_t tg;
  auto device_properties = lzt::get_device_properties(device);
  const auto max_threads =
      device_properties.numSlices * device_properties.numSubslicesPerSlice *
      device_properties.numEUsPerSubslice * device_properties.numThreadsPerEU;
  LOG_INFO << "Available threads: " << max_threads;
  const auto dimensions = (max_threads > 4096 ? 1024 : 2);
  void *a_buffer, *b_buffer, *c_buffer;
  ze_kernel_handle_t function = get_matrix_multiplication_kernel(
      device, &tg, &a_buffer, &b_buffer, &c_buffer, dimensions);

  zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                  nullptr);

  auto interrupt_test = false;
  std::thread *tp = nullptr;
  if (argc > 1) {
    const std::string arg = argv[1];
    LOG_DEBUG << "[Child] Option: " << arg << "\n";
    if (arg == "-i") {
      interrupt_test = true;
      // start a monitor thread
      tp = new std::thread(interrupt_process);
    }
  }
  lzt::close_command_list(commandList);
  lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

  lzt::synchronize(commandQueue, UINT64_MAX);

  if (interrupt_test) {
    tp->join();
    delete tp;
    tp = nullptr;
  }

  lzt::destroy_function(function);
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);

  lzt::destroy_command_queue(commandQueue);
  lzt::destroy_command_list(commandList);
}
