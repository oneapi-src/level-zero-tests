/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

int main(int argc, char **argv) {

  ze_result_t result = zeInit(2);
  if (result != ZE_RESULT_SUCCESS) {
    exit(1);
  }

  LOG_INFO << "child";
  int proc_number = std::stoi(argv[1]);
  bool is_immediate = std::stoi(argv[2]) == 0 ? false : true;
  bool is_stress_test = std::stoi(argv[3]) == 0 ? false : true;

  auto driver = lzt::get_default_driver();
  auto device_0 = lzt::get_devices(driver)[0];
  auto devices = lzt::get_devices(driver);
  if (is_stress_test) {
    devices = lzt::get_ze_sub_devices(device_0);
  }
  int deviceIndex = proc_number % devices.size();
  auto device = devices[deviceIndex];
  auto device_properties = lzt::get_device_properties(device);
  auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);

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

  if (is_stress_test) {
    for (int i = 0; i < 10; i++) {
      lzt::append_launch_function(cmd_bundle.list, kernel, &group_count,
                                  nullptr, 0, nullptr);
    }
  } else {
    lzt::append_launch_function(cmd_bundle.list, kernel, &group_count, nullptr,
                                0, nullptr);
  }

  lzt::close_command_list(cmd_bundle.list);
  if (is_stress_test) {
    for (int i = 0; i < 1000; i++) {
      lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list,
                                 nullptr);
    }
  } else {
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  }

  if (is_stress_test) {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  lzt::destroy_command_bundle(cmd_bundle);

  if (is_stress_test) {
    exit(0);
  }

  // verify kernel execution
  for (int i = 0; i < memory_size; i++) {
    if (input_a[i] != 2) {
      exit(1);
    }
  }
  exit(0);
}
