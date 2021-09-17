/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

int main(int argc, char **argv) {

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    exit(1);
  }

  LOG_INFO << "child";

  int proc_number = std::stoi(argv[1]);

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  int deviceIndex = proc_number % devices.size();
  auto device = devices[deviceIndex];
  auto command_list = lzt::create_command_list(device);
  auto command_queue = lzt::create_command_queue(device);

  auto module =
      lzt::create_module(devices[deviceIndex], "multi_process_add.spv");
  auto kernel = lzt::create_function(module, "add_two_arrays");

  auto constexpr memory_size = 8192;

  auto input_a =
      static_cast<uint8_t *>(lzt::allocate_shared_memory(memory_size, device));

  auto input_b =
      static_cast<uint8_t *>(lzt::allocate_shared_memory(memory_size, device));

  std::fill(input_a, input_a + memory_size, 0x01);
  std::fill(input_b, input_b + memory_size, 0x01);

  uint32_t global_size_x = memory_size;
  uint32_t global_size_y = 1;
  uint32_t global_size_z = 1;

  uint32_t group_size_x = 1;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;

  lzt::suggest_group_size(kernel, global_size_x, global_size_y, global_size_y,
                          group_size_x, group_size_y, group_size_z);
  lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

  ze_group_count_t group_count;

  group_count.groupCountX = memory_size / group_size_x;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  lzt::set_argument_value(kernel, 0, sizeof(input_a), &input_a);
  lzt::set_argument_value(kernel, 1, sizeof(input_b), &input_b);

  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);

  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  // verify kernel execution
  for (int i = 0; i < memory_size; i++) {
    if (input_a[i] != 2) {

      exit(1);
    }
  }
  exit(0);
}
