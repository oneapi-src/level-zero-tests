/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <chrono>
#include <thread>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace bi = boost::interprocess;
namespace lzt = level_zero_tests;

int main(int argc, char **argv) {

  // Open already created shared memory object.
  bi::shared_memory_object shm(bi::open_only, "debug_bool", bi::read_only);
  bi::mapped_region region(shm, bi::read_only);

  bi::named_mutex mutex(bi::open_only, "debugger_mutex");
  bi::named_condition condition(bi::open_only, "debug_bool_set");

  bi::scoped_lock<bi::named_mutex> lock(mutex);
  bool *debugger_attached = static_cast<bool *>(region.get_address());
  LOG_INFO << "Waiting for debugger to attach";
  condition.wait(lock, [&] { return *debugger_attached; });
  LOG_INFO << "Debugged process proceeding";

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_WARNING << "zeInit failed";
    exit(1);
  }

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto use_sub_devices = std::stoi(argv[2]);
  ze_device_handle_t device = nullptr;
  for (auto &root_device : lzt::get_devices(driver)) {
    if (use_sub_devices) {
      for (auto &sub_device : lzt::get_ze_sub_devices(root_device)) {
        auto device_properties = lzt::get_device_properties(sub_device);
        if (strncmp(argv[1], lzt::to_string(device_properties.uuid).c_str(),
                    ZE_MAX_DEVICE_NAME)) {
          continue;
        } else {
          device = sub_device;
          break;
        }
      }
      if (!device)
        continue;
    } else {
      auto device_properties = lzt::get_device_properties(root_device);

      if (strncmp(argv[1], lzt::to_string(device_properties.uuid).c_str(),
                  ZE_MAX_DEVICE_NAME)) {
        continue;
      }
      device = root_device;
    }

    auto command_queue = lzt::create_command_queue(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    auto command_list = lzt::create_command_list(context, device, 0, 0);
    auto module = lzt::create_module(context, device, "debug_add.spv",
                                     ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

    auto kernel = lzt::create_function(module, "debug_add_constant_2");

    auto size = 10000000;
    auto buffer_a = lzt::allocate_shared_memory(size, 0, 0, 0, device, context);
    auto buffer_b = lzt::allocate_device_memory(size, 0, 0, 0, device, context);

    std::memset(buffer_a, 0, size);
    for (size_t i = 0; i < size; i++) {
      static_cast<uint8_t *>(buffer_a)[i] = (i & 0xFF);
    }

    const int addval = 3;
    lzt::set_argument_value(kernel, 0, sizeof(buffer_b), &buffer_b);
    lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);

    uint32_t group_size_x = 1;
    uint32_t group_size_y = 1;
    uint32_t group_size_z = 1;
    lzt::suggest_group_size(kernel, size, 1, 1, group_size_x, group_size_y,
                            group_size_z);
    lzt::set_group_size(kernel, group_size_x, 1, 1);
    ze_group_count_t group_count = {};
    group_count.groupCountX = size / group_size_x;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::append_memory_copy(command_list, buffer_b, buffer_a, size);
    lzt::append_barrier(command_list);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::append_barrier(command_list);
    lzt::append_memory_copy(command_list, buffer_a, buffer_b, size);
    lzt::close_command_list(command_list);
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    // validation
    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<uint8_t *>(buffer_a)[i],
                static_cast<uint8_t>((i & 0xFF) + addval));

      if (::testing::Test::HasFailure()) {
        exit(1);
      }
    }

    // cleanup
    lzt::free_memory(context, buffer_a);
    lzt::free_memory(context, buffer_b);
    lzt::destroy_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
  }
  lzt::destroy_context(context);
}
