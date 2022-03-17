/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <chrono>
#include <thread>

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_debug.hpp"
#include "test_debug_helper.hpp"

#ifdef EXTENDED_TESTS
#include "test_debug_extended.hpp"
#endif

namespace lzt = level_zero_tests;

void basic(ze_context_handle_t context, ze_device_handle_t device,
           process_synchro &synchro, debug_options &options) {

  synchro.wait_for_attach();

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  std::string module_name = (options.use_custom_module == true)
                                ? options.module_name_in
                                : "debug_add.spv";
  auto module = lzt::create_module(context, device, module_name,
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

  auto kernel = lzt::create_function(module, "debug_add_constant_2");

  auto size = 8192;
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
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

// Debugger attaches after module created
void attach_after_module_created_test(ze_context_handle_t context,
                                      ze_device_handle_t device,
                                      process_synchro &synchro,
                                      debug_options &options) {
  LOG_INFO << "[Application] Attach After Module Created Test";

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  std::string module_name = (options.use_custom_module == true)
                                ? options.module_name_in
                                : "debug_add.spv";
  auto module = lzt::create_module(context, device, module_name,
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

  auto kernel = lzt::create_function(module, "debug_add_constant_2");

  auto size = 8192;
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

  LOG_INFO << "[Application] Appending commands";

  lzt::append_memory_copy(command_list, buffer_b, buffer_a, size);
  lzt::append_barrier(command_list);
  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list);
  lzt::append_memory_copy(command_list, buffer_a, buffer_b, size);
  lzt::close_command_list(command_list);

  synchro.notify_debugger();
  synchro.wait_for_attach();

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
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

// debuggee process creates multiple modules
void multiple_modules_created_test(ze_context_handle_t context,
                                   ze_device_handle_t device,
                                   process_synchro &synchro,
                                   debug_options &options) {

  synchro.wait_for_attach();

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  std::string module_name = (options.use_custom_module == true)
                                ? options.module_name_in
                                : "debug_add.spv";
  auto module = lzt::create_module(context, device, module_name,
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);
  auto kernel = lzt::create_function(module, "debug_add_constant_2");

  auto module2 = lzt::create_module(context, device, "1kernel.spv",
                                    ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);
  auto kernel2 = lzt::create_function(module2, "kernel1");

  ze_group_count_t group_count = {};
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;
  lzt::append_launch_function(command_list, kernel2, &group_count, nullptr, 0,
                              nullptr);

  auto size = 8192;
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
  lzt::destroy_function(kernel);
  lzt::destroy_function(kernel2);
  lzt::destroy_module(module);
  lzt::destroy_module(module2);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

// debugger waits and attaches after module created and destroyed
void attach_after_module_destroyed_test(ze_context_handle_t context,
                                        ze_device_handle_t device,
                                        process_synchro &synchro,
                                        debug_options &options) {

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  std::string module_name = (options.use_custom_module == true)
                                ? options.module_name_in
                                : "debug_add.spv";
  auto module = lzt::create_module(context, device, module_name,
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

  auto kernel = lzt::create_function(module, "debug_add_constant_2");

  auto size = 8192;
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
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);

  LOG_INFO << "[Application] All resouces freed";
  synchro.wait_for_attach();

  // Allow debugger to attach
  std::this_thread::sleep_for(std::chrono::seconds(10));
}

void run_long_kernel(ze_context_handle_t context, ze_device_handle_t device,
                     process_synchro &synchro, debug_options &options) {

  auto command_list = lzt::create_command_list(device);
  auto command_queue = lzt::create_command_queue(device);
  std::string module_name = (options.use_custom_module == true)
                                ? options.module_name_in
                                : "debug_loop.spv";

  std::string kernel_name = (options.use_custom_module == true)
                                ? options.kernel_name_in
                                : "long_kernel";
  synchro.wait_for_attach();
  auto module =
      lzt::create_module(device, module_name, ZE_MODULE_FORMAT_IL_SPIRV,
                         "-g" /* include debug symbols*/, nullptr);

  auto kernel = lzt::create_function(module, kernel_name);

  auto size = 512;

  auto dest_buffer_d =
      lzt::allocate_device_memory(size, size, 0, 0, device, context);
  auto dest_buffer_s =
      lzt::allocate_shared_memory(size, size, 0, 0, device, context);
  auto src_buffer_d =
      lzt::allocate_device_memory(size, size, 0, 0, device, context);
  auto src_buffer_s =
      lzt::allocate_shared_memory(size, size, 0, 0, device, context);

  unsigned long loop_max = 100000000;
  auto loop_counter_d = lzt::allocate_device_memory(
      sizeof(unsigned long), sizeof(unsigned long), 0, 0, device, context);
  auto loop_counter_s = lzt::allocate_shared_memory(
      sizeof(unsigned long), sizeof(unsigned long), 0, 0, device, context);

  LOG_DEBUG << "[Application] Allocated source device memory at: " << std::hex
            << src_buffer_d;
  LOG_DEBUG << "[Application] Allocated destination device memory at: "
            << std::hex << dest_buffer_d;

  synchro.update_gpu_buffer_address(reinterpret_cast<uint64_t>(src_buffer_d));
  synchro.notify_debugger();

  std::memset(dest_buffer_s, 1, size);
  std::memset(src_buffer_s, 0, size);
  std::memset(loop_counter_s, 0, sizeof(loop_counter_s));
  for (size_t i = 0; i < size; i++) {
    static_cast<uint8_t *>(src_buffer_s)[i] = (i + 1 & 0xFF);
  }

  lzt::set_argument_value(kernel, 0, sizeof(dest_buffer_d), &dest_buffer_d);
  lzt::set_argument_value(kernel, 1, sizeof(src_buffer_d), &src_buffer_d);
  lzt::set_argument_value(kernel, 2, sizeof(loop_counter_d), &loop_counter_d);
  lzt::set_argument_value(kernel, 3, sizeof(loop_max), &loop_max);

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

  lzt::append_memory_copy(command_list, src_buffer_d, src_buffer_s, size);
  lzt::append_barrier(command_list);
  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list);
  lzt::append_memory_copy(command_list, dest_buffer_s, dest_buffer_d, size);
  lzt::append_memory_copy(command_list, loop_counter_s, loop_counter_d,
                          sizeof(loop_counter_d));
  lzt::close_command_list(command_list);

  LOG_DEBUG << "[Application] launching execution long_kernel";
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  // validation
  // skip buffer[0] since some tests force it to 0 to break the loop
  for (size_t i = 1; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(dest_buffer_s)[i],
              static_cast<uint8_t *>(src_buffer_s)[i]);
  }

  EXPECT_LT(*(static_cast<unsigned long *>(loop_counter_s)), loop_max);
  LOG_DEBUG << "[Application] GPU Kernel looped "
            << *(static_cast<unsigned long *>(loop_counter_s)) << " out of "
            << loop_max;

  if (::testing::Test::HasFailure()) {
    FAIL() << "[Application] Sanity check did not pass";
    exit(1);
  }

  // cleanup
  lzt::free_memory(context, dest_buffer_s);
  lzt::free_memory(context, dest_buffer_d);
  lzt::free_memory(context, src_buffer_s);
  lzt::free_memory(context, src_buffer_d);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

// ***************************************************************************************
void thread_unavailable_test(process_synchro &synchro) {

  synchro.wait_for_attach();

  // do nothing
  std::this_thread::sleep_for(std::chrono::seconds(30));
}

int main(int argc, char **argv) {

  debug_options options;
  options.parse_options(argc, argv);

  process_synchro synchro(options.enable_synchro, false);

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_ERROR << "[Application] zeInit failed";
    exit(1);
  }

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  ze_device_handle_t device = nullptr;
  for (auto &root_device : lzt::get_devices(driver)) {
    if (options.use_sub_devices) {
      LOG_INFO << "[Application] Using subdevices";

      for (auto &sub_device : lzt::get_ze_sub_devices(root_device)) {
        auto device_properties = lzt::get_device_properties(sub_device);
        if (strncmp(options.device_id_in.c_str(),
                    lzt::to_string(device_properties.uuid).c_str(),
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
      LOG_INFO << "[Application] Using root device";

      auto device_properties = lzt::get_device_properties(root_device);

      if (strncmp(options.device_id_in.c_str(),
                  lzt::to_string(device_properties.uuid).c_str(),
                  ZE_MAX_DEVICE_NAME)) {
        continue;
      }
      device = root_device;
    }

    LOG_INFO << "[Application] Proceeding with test";
    switch (options.test_selected) {
    case BASIC:
      basic(context, device, synchro, options);
      break;
    case ATTACH_AFTER_MODULE_CREATED:
      attach_after_module_created_test(context, device, synchro, options);
      break;
    case MULTIPLE_MODULES_CREATED:
      multiple_modules_created_test(context, device, synchro, options);
      break;
    case ATTACH_AFTER_MODULE_DESTROYED:
      attach_after_module_destroyed_test(context, device, synchro, options);
      break;
    case LONG_RUNNING_KERNEL_INTERRUPTED:
      run_long_kernel(context, device, synchro, options);
      break;
    case KERNEL_RESUME:
      run_long_kernel(context, device, synchro, options);
      break;
    case THREAD_UNAVAILABLE:
      thread_unavailable_test(synchro);
      break;
    default:
#ifdef EXTENDED_TESTS
      if (is_extended_debugger_test(options.test_selected)) {

        run_extended_debugger_test(options.test_selected, context, device,
                                   synchro, options);
        break;
      }
#endif
      LOG_ERROR << "[Application] Unrecognized test type: "
                << options.test_selected;
      exit(1);
    }
  }
  lzt::destroy_context(context);

  LOG_INFO << "[Application] Done, exiting";
}
