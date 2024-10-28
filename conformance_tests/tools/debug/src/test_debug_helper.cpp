/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <chrono>
#include <thread>
#include <map>

#include "test_debug.hpp"
#include "test_debug_utils.hpp"
#include "test_debug_helper.hpp"

#ifdef EXTENDED_TESTS
#include "test_debug_extended.hpp"
#endif

namespace lzt = level_zero_tests;

static constexpr auto loop_counter_alloc_size = sizeof(unsigned long);

void basic(ze_context_handle_t context, ze_device_handle_t device,
           process_synchro &synchro, debug_options &options) {

  synchro.wait_for_debugger_signal();
  LOG_DEBUG << "[Application] Child Proceeding";

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

  auto size = 256;
  ze_kernel_properties_t kernel_properties = {
      ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelGetProperties(kernel, &kernel_properties));
  int threadCount = std::ceil(size / kernel_properties.maxSubgroupSize);

  LOG_INFO << "[Application] Problem size: " << size
           << ". Kernel maxSubGroupSize: " << kernel_properties.maxSubgroupSize
           << ". GPU thread count: ceil (P size/maxSubGroupSize) = "
           << threadCount;

  synchro.update_gpu_thread_count(threadCount);
  synchro.notify_debugger();

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
      LOG_ERROR << "[Application] Sanity check did not pass";
      break;
    }
  }

  // cleanup
  lzt::free_memory(context, buffer_a);
  lzt::free_memory(context, buffer_b);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);

  if (::testing::Test::HasFailure()) {
    exit(1);
  }
}

// Debugger attaches after module created
void attach_after_module_created_test(ze_context_handle_t context,
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

  LOG_INFO << "[Application] Creating module";
  auto module = lzt::create_module(context, device, module_name,
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

  LOG_INFO << "[Application] Creating kernel";
  auto kernel = lzt::create_function(module, "debug_add_constant_2");

  auto size = 8192;
  ze_kernel_properties_t kernel_properties = {
      ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelGetProperties(kernel, &kernel_properties));
  int threadCount = std::ceil(size / kernel_properties.maxSubgroupSize);

  LOG_INFO << "[Application] Problem size: " << size
           << ". Kernel maxSubGroupSize: " << kernel_properties.maxSubgroupSize
           << ". GPU thread count: ceil (P size/maxSubGroupSize) = "
           << threadCount;
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

  // Notify:
  //  After the module and command queue are created: send
  //  ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD
  //  Before destroying the module or the comand queue
  synchro.notify_debugger();
  synchro.wait_for_debugger_signal();

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  // validation
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer_a)[i],
              static_cast<uint8_t>((i & 0xFF) + addval));
    if (::testing::Test::HasFailure()) {
      LOG_ERROR << "[Application] Sanity check did not pass";
      break;
    }
  }

  // cleanup
  lzt::free_memory(context, buffer_a);
  lzt::free_memory(context, buffer_b);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);

  if (::testing::Test::HasFailure()) {
    exit(1);
  }
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
  ze_kernel_properties_t kernel_properties = {
      ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelGetProperties(kernel, &kernel_properties));
  int threadCount = std::ceil(size / kernel_properties.maxSubgroupSize);

  LOG_INFO << "[Application] Problem size: " << size
           << ". Kernel maxSubGroupSize: " << kernel_properties.maxSubgroupSize
           << ". GPU thread count: ceil (P size/maxSubGroupSize) = "
           << threadCount;
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

  // cleanup
  LOG_INFO << "[Application] Freeing resources";
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);

  // Notify:
  //  After the module is created and destroyed
  //  Before destroying the comand queue : send
  //  ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT)
  synchro.notify_debugger();
  synchro.wait_for_debugger_signal();

  // validation
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer_a)[i],
              static_cast<uint8_t>((i & 0xFF) + addval));

    if (::testing::Test::HasFailure()) {
      LOG_ERROR << "[Application] Sanity check did not pass";
      break;
    }
  }

  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);

  lzt::free_memory(context, buffer_a);
  lzt::free_memory(context, buffer_b);

  if (::testing::Test::HasFailure()) {
    exit(1);
  }
}

// debuggee process creates multiple modules
void multiple_modules_created_test(ze_context_handle_t context,
                                   ze_device_handle_t device,
                                   process_synchro &synchro,
                                   debug_options &options) {

  synchro.wait_for_debugger_signal();

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
  ze_kernel_properties_t kernel_properties = {
      ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelGetProperties(kernel, &kernel_properties));
  int threadCount = std::ceil(size / kernel_properties.maxSubgroupSize);

  LOG_INFO << "[Application] Problem size: " << size
           << ". Kernel maxSubGroupSize: " << kernel_properties.maxSubgroupSize
           << ". GPU thread count: ceil (P size/maxSubGroupSize) = "
           << threadCount;
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
      LOG_ERROR << "[Application] Sanity check did not pass";
      break;
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
  if (::testing::Test::HasFailure()) {
    exit(1);
  }
}

void run_long_kernel(ze_context_handle_t context, ze_device_handle_t device,
                     process_synchro &synchro, debug_options &options) {

  auto command_list = lzt::create_command_list(device);
  auto command_queue = lzt::create_command_queue(device);
  std::string module_name = (options.use_custom_module == true)
                                ? options.module_name_in
                                : "debug_loop.spv";

  std::string kernel_name =
      (options.use_custom_module == true)
          ? (!options.kernel_name_in.empty() ? options.kernel_name_in
                                             : "long_kernel")
          : "long_kernel";
  bool slm = false;
  size_t slm_buffer_size =
      512; // NOTE: Not all SKUs have same SLM so can go too big.
  if (!module_name.compare("debug_loop_slm.spv")) {
    LOG_INFO << "[Application] Testing SLM access ";
    slm = true;
  }

  synchro.wait_for_debugger_signal();

  std::string mod_options = "-g";
  if (options.module_options_in != "") {
    mod_options.append("," + options.module_options_in);
  }
  LOG_INFO << "Module Options: " << mod_options;
  auto module =
      lzt::create_module(device, module_name, ZE_MODULE_FORMAT_IL_SPIRV,
                         mod_options.c_str(), nullptr);

  auto kernel = lzt::create_function(module, kernel_name);

  auto size = slm_buffer_size;

  ze_kernel_properties_t kernel_properties = {
      ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelGetProperties(kernel, &kernel_properties));
  int threadCount = std::ceil(size / kernel_properties.maxSubgroupSize);

  LOG_INFO << "[Application] Problem size: " << size
           << ". Kernel maxSubGroupSize: " << kernel_properties.maxSubgroupSize
           << ". GPU thread count: ceil (P size/maxSubGroupSize) = "
           << threadCount;

  auto dest_buffer_d =
      lzt::allocate_device_memory(size, size, 0, 0, device, context);
  auto dest_buffer_s =
      lzt::allocate_shared_memory(size, size, 0, 0, device, context);
  auto src_buffer_d =
      lzt::allocate_device_memory(size, size, 0, 0, device, context);
  auto src_buffer_s =
      lzt::allocate_shared_memory(size, size, 0, 0, device, context);

  void *slm_output_s = nullptr;
  if (slm) {
    slm_output_s = lzt::allocate_shared_memory(slm_buffer_size, slm_buffer_size,
                                               0, 0, device, context);
  }

  unsigned long loop_max = 1000000000;

  auto loop_counter_d = lzt::allocate_device_memory(
      loop_counter_alloc_size, loop_counter_alloc_size, 0, 0, device, context);
  auto loop_counter_s = lzt::allocate_shared_memory(
      loop_counter_alloc_size, loop_counter_alloc_size, 0, 0, device, context);

  LOG_DEBUG << "[Application] Allocated source device memory at: " << std::hex
            << src_buffer_d;
  LOG_DEBUG << "[Application] Allocated destination device memory at: "
            << std::hex << dest_buffer_d;

  std::memset(dest_buffer_s, 1, size);
  std::memset(src_buffer_s, 0, size);
  std::memset(loop_counter_s, 0, loop_counter_alloc_size);
  for (size_t i = 0; i < size; i++) {
    static_cast<uint8_t *>(src_buffer_s)[i] = (i + 1 & 0xFF);
  }

  lzt::set_argument_value(kernel, 0, sizeof(dest_buffer_d), &dest_buffer_d);
  lzt::set_argument_value(kernel, 1, sizeof(src_buffer_d), &src_buffer_d);
  lzt::set_argument_value(kernel, 2, sizeof(loop_counter_d), &loop_counter_d);
  lzt::set_argument_value(kernel, 3, sizeof(loop_max), &loop_max);
  if (slm) {
    lzt::set_argument_value(kernel, 4, sizeof(slm_output_s), &slm_output_s);
  }

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
                          loop_counter_alloc_size);
  lzt::close_command_list(command_list);

  LOG_DEBUG << "[Application] launching execution of " << kernel_name;

  synchro.update_gpu_buffer_address(reinterpret_cast<uint64_t>(src_buffer_d));
  synchro.notify_debugger();

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  // validation
  // skip buffer[0] since some tests force it to 0 to break the loop
  for (size_t i = 1; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(dest_buffer_s)[i],
              static_cast<uint8_t *>(src_buffer_s)[i]);
    if (static_cast<uint8_t *>(dest_buffer_s)[i] !=
        static_cast<uint8_t *>(src_buffer_s)[i]) {
      LOG_ERROR << "[Application] Buffer Sanity check did not pass";
      break;
    }
  }

  if (slm) {
    unsigned char slm_pattern[24] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
        0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};

    // debugger will modify the first bytes
    for (size_t i = 0; i < sizeof(slm_pattern); i++) {
      EXPECT_EQ(static_cast<uint8_t *>(slm_output_s)[i],
                static_cast<uint8_t *>(slm_pattern)[i]);
      if (static_cast<uint8_t *>(slm_output_s)[i] !=
          static_cast<uint8_t *>(slm_pattern)[i]) {
        LOG_ERROR << "[Application] SLM Buffer Sanity check did not pass";
        break;
      }
    }

    // remaining is expecte to same as the source buffer
    for (size_t i = sizeof(slm_pattern); i < slm_buffer_size; i++) {
      EXPECT_EQ(static_cast<uint8_t *>(slm_output_s)[i],
                static_cast<uint8_t *>(src_buffer_s)[i]);
      if (static_cast<uint8_t *>(slm_output_s)[i] !=
          static_cast<uint8_t *>(src_buffer_s)[i]) {
        LOG_ERROR << "[Application] SLM Buffer Sanity check did not pass";
        break;
      }
    }
  }

  EXPECT_LT(*(static_cast<unsigned long *>(loop_counter_s)), loop_max);
  LOG_DEBUG << "[Application] GPU Kernel looped "
            << *(static_cast<unsigned long *>(loop_counter_s)) << " out of "
            << loop_max;

  // cleanup
  lzt::free_memory(context, dest_buffer_s);
  lzt::free_memory(context, dest_buffer_d);
  lzt::free_memory(context, src_buffer_s);
  lzt::free_memory(context, src_buffer_d);
  lzt::free_memory(context, loop_counter_s);
  lzt::free_memory(context, loop_counter_d);

  if (slm) {
    lzt::free_memory(context, slm_output_s);
  }

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);

  if (::testing::Test::HasFailure()) {
    exit(1);
  }
}

void run_multiple_threads(ze_context_handle_t context,
                          ze_device_handle_t device, process_synchro &synchro,
                          debug_options &options) {
  constexpr auto num_threads = 2;
  std::vector<std::unique_ptr<std::thread>> threads;

  for (int i = 0; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(new std::thread(
        basic, context, device, std::ref(synchro), std::ref(options))));
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

void loop_create_destroy_multiple_cq_immcl(ze_context_handle_t context,
                                           ze_device_handle_t device,
                                           process_synchro &synchro,
                                           debug_options &options,
                                           debug_test_type_t test_selected) {

  std::map<int, int> ordinalCQs;
  int totalNumCQs = get_numCQs_per_ordinal(device, ordinalCQs);
  std::vector<ze_command_queue_handle_t> cmdQueues;
  std::vector<ze_command_list_handle_t> cmdLists;

  for (int loop = 0; loop < 3; loop++) {
    LOG_DEBUG << "[Application]--- Loop " << loop << " ---";
    synchro.wait_for_debugger_signal();
    synchro.clear_debugger_signal();

    LOG_DEBUG << "[Application] Child Proceeding";

    for (std::map<int, int>::iterator ordinalCQ = ordinalCQs.begin();
         ordinalCQ != ordinalCQs.end(); ordinalCQ++) {
      int ordinal = ordinalCQ->first;
      int numQueues = ordinalCQ->second;

      for (int index = 0; index < numQueues; index++) {
        if (test_selected == MULTIPLE_CQ) {
          ze_command_queue_handle_t cmdqueue = lzt::create_command_queue(
              context, device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY,
              ZE_COMMAND_QUEUE_MODE_DEFAULT, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
              ordinal, index);
          cmdQueues.push_back(cmdqueue);
        } else {
          ze_command_list_handle_t immCommandList =
              lzt::create_immediate_command_list(
                  context, device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY,
                  ZE_COMMAND_QUEUE_MODE_DEFAULT,
                  ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, index);

          cmdLists.push_back(immCommandList);
        }

        LOG_DEBUG << "[Application] Created "
                  << (test_selected == MULTIPLE_CQ ? " CQ "
                                                   : " immediate CMD List ")
                  << "with ordinal: " << ordinal << " and index: " << index;

        synchro.notify_debugger();
        synchro.wait_for_debugger_signal();
        synchro.clear_debugger_signal();
      }
    }

    // cleanup
    int counter = 1;
    if (test_selected == MULTIPLE_CQ) {
      for (auto cmdqueue : cmdQueues) {
        LOG_DEBUG << "[Application] Destroying CQ: " << counter;
        lzt::destroy_command_queue(cmdqueue);

        synchro.notify_debugger();
        synchro.wait_for_debugger_signal();
        synchro.clear_debugger_signal();
        counter++;
      }
      cmdQueues.clear();
    } else {
      for (auto cmdList : cmdLists) {
        LOG_DEBUG << "[Application] Destroying Immedate CMD List: " << counter;
        lzt::destroy_command_list(cmdList);

        synchro.notify_debugger();
        synchro.wait_for_debugger_signal();
        synchro.clear_debugger_signal();
        counter++;
      }
      cmdLists.clear();
    }
  }

  if (::testing::Test::HasFailure()) {
    exit(1);
  }
}

class Job {
public:
  Job(ze_context_handle_t &context, ze_device_handle_t &device);
  void set_up_work(debug_options &options);
  void notify_debugger_address_and_thread_count(process_synchro *synchro);
  void execute();
  void synchronize();
  void validate();
  void cleanup();

private:
  static constexpr uint32_t size = 512;
  static constexpr unsigned long loop_max = 1000000000;
  ze_device_handle_t device;
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;
  void *dest_buffer_s;
  void *dest_buffer_d;
  void *src_buffer_s;
  void *src_buffer_d;
  void *loop_counter_s;
  void *loop_counter_d;
  uint32_t gpu_thread_count = 0;
  ze_context_handle_t context;
};
constexpr uint32_t Job::size;
constexpr unsigned long Job::loop_max;

Job::Job(ze_context_handle_t &context, ze_device_handle_t &device) {
  this->context = context;
  this->device = device;
}

void Job::set_up_work(debug_options &options) {

  command_queue = lzt::create_command_queue(
      context, device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY,
      ZE_COMMAND_QUEUE_MODE_DEFAULT, ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  command_list = lzt::create_command_list(context, device, 0, 0);

  std::string module_name = (options.use_custom_module == true)
                                ? options.module_name_in
                                : "debug_loop.spv";

  std::string kernel_name =
      (options.use_custom_module == true)
          ? (!options.kernel_name_in.empty() ? options.kernel_name_in
                                             : "long_kernel")
          : "long_kernel";

  LOG_INFO << "[Application] Creating Module";

  module = lzt::create_module(device, module_name, ZE_MODULE_FORMAT_IL_SPIRV,
                              "-g", nullptr);

  LOG_INFO << "[Application] Creating Kernels";

  kernel = lzt::create_function(module, kernel_name);

  ze_kernel_properties_t kernel_properties = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelGetProperties(kernel, &kernel_properties));
  int threadCount = std::ceil(size / kernel_properties.maxSubgroupSize);

  LOG_INFO << "[Application] Problem size: " << size
           << ". Kernel maxSubGroupSize: " << kernel_properties.maxSubgroupSize
           << ". GPU thread count: ceil (P size/maxSubGroupSize) = "
           << threadCount;

  gpu_thread_count = threadCount;

  dest_buffer_d =
      lzt::allocate_device_memory(size, size, 0, 0, device, context);
  dest_buffer_s =
      lzt::allocate_shared_memory(size, size, 0, 0, device, context);
  src_buffer_d = lzt::allocate_device_memory(size, size, 0, 0, device, context);
  src_buffer_s = lzt::allocate_shared_memory(size, size, 0, 0, device, context);

  loop_counter_d = lzt::allocate_device_memory(
      loop_counter_alloc_size, loop_counter_alloc_size, 0, 0, device, context);
  loop_counter_s = lzt::allocate_shared_memory(
      loop_counter_alloc_size, loop_counter_alloc_size, 0, 0, device, context);

  LOG_DEBUG << "[Application] Allocated source device memory at: " << std::hex
            << src_buffer_d;
  LOG_DEBUG << "[Application] Allocated destination device memory at: "
            << std::hex << dest_buffer_d;

  std::memset(dest_buffer_s, 1, size);
  std::memset(src_buffer_s, 0, size);
  std::memset(loop_counter_s, 0, loop_counter_alloc_size);
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
                          loop_counter_alloc_size);
  lzt::close_command_list(command_list);
}

void Job::notify_debugger_address_and_thread_count(process_synchro *synchro) {
  synchro->update_gpu_thread_count(gpu_thread_count);
  synchro->update_gpu_buffer_address(reinterpret_cast<uint64_t>(src_buffer_d));
  synchro->notify_debugger();
}

void Job::execute() {
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
}

void Job::synchronize() { lzt::synchronize(command_queue, UINT64_MAX); }

void Job::validate() {
  // skip buffer[0] since some tests force it to 0 to break the loop
  for (size_t i = 1; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(dest_buffer_s)[i],
              static_cast<uint8_t *>(src_buffer_s)[i]);
    if (::testing::Test::HasFailure()) {
      LOG_ERROR << "[Application] Sanity check did not pass";
      break;
    }
  }

  EXPECT_LT(*(static_cast<unsigned long *>(loop_counter_s)), loop_max);
  LOG_DEBUG << "[Application] GPU Kernel looped "
            << *(static_cast<unsigned long *>(loop_counter_s)) << " out of "
            << loop_max;
}

void Job::cleanup() {
  lzt::free_memory(context, dest_buffer_s);
  lzt::free_memory(context, dest_buffer_d);
  lzt::free_memory(context, src_buffer_s);
  lzt::free_memory(context, src_buffer_d);
  lzt::free_memory(context, loop_counter_s);
  lzt::free_memory(context, loop_counter_d);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

void use_two_devices(ze_context_handle_t context, ze_device_handle_t &device_0,
                     ze_device_handle_t &device_1, process_synchro &synchro,
                     debug_options &options) {

  synchro.wait_for_debugger_signal();

  auto device_properties = lzt::get_device_properties(device_0);
  auto device_properties_1 = lzt::get_device_properties(device_1);

  LOG_DEBUG << "[Application] Device 0: " << device_properties.name << " "
            << device_properties.deviceId << " " << device_properties.uuid;
  LOG_DEBUG << "[Application] Device 1: " << device_properties_1.name << " "
            << device_properties_1.deviceId << " " << device_properties_1.uuid;

  LOG_INFO << "[Application] Creating job objects";
  Job job_0(context, device_0);
  Job job_1(context, device_1);

  LOG_INFO << "[Application] Preparing Device 0 for execution";
  job_0.set_up_work(options);
  LOG_INFO << "[Application] Preparing Device 1 for execution";
  job_1.set_up_work(options);

  LOG_INFO << "[Application] Setting buffer address 0 for debugger to read";
  job_0.notify_debugger_address_and_thread_count(&synchro);
  LOG_INFO
      << "[Application] Waiting for debugger to signal it read first address";
  synchro.clear_debugger_signal();
  synchro.wait_for_debugger_signal();
  LOG_INFO << "[Application] Setting buffer address 1 for debugger to read";
  job_1.notify_debugger_address_and_thread_count(&synchro);

  job_0.execute();
  job_1.execute();

  job_0.synchronize();
  job_1.synchronize();

  job_0.validate();
  job_1.validate();

  job_0.cleanup();
  job_1.cleanup();

  if (::testing::Test::HasFailure()) {
    exit(1);
  }
}

void multidevice_resource_stress_test(ze_context_handle_t &context,
                                      ze_device_handle_t &device_0,
                                      ze_device_handle_t &device_1,
                                      process_synchro &synchro,
                                      debug_options &options) {

  auto result = ZE_RESULT_SUCCESS;
  auto size = 65536;
  auto alignment = 8;

  void *values =
      lzt::allocate_device_memory(size, alignment, 0, device_0, context);
  auto validation_buffer_0 = new uint8_t[size];

  auto command_queue = lzt::create_command_queue(
      context, device_0, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  ze_command_list_desc_t command_list_desc = {};
  command_list_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
  command_list_desc.pNext = nullptr;
  command_list_desc.flags = 0;

  ze_command_list_handle_t command_list;
  result =
      zeCommandListCreate(context, device_0, &command_list_desc, &command_list);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
  auto fence = lzt::create_fence(command_queue);

  std::vector<ze_device_handle_t> devices = {device_0, device_1};
  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
                                          nullptr,
                                          ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 256};
  auto event_pool = lzt::create_event_pool(context, event_pool_desc, devices);

  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr,
                                ZE_EVENT_SCOPE_FLAG_HOST,
                                ZE_EVENT_SCOPE_FLAG_HOST};
  auto event = lzt::create_event(event_pool, event_desc);

  std::vector<uint8_t> src(size);
  std::fill_n(src.begin(), size, 1);
  lzt::append_memory_copy(command_list, values, src.data(), size, event, 0,
                          nullptr);
  lzt::close_command_list(command_list);

  result =
      zeCommandQueueExecuteCommandLists(command_queue, 1, &command_list, fence);
  if (ZE_RESULT_SUCCESS != result) {
    LOG_ERROR << "[Application] " << __LINE__ << " FAIL: " << result;
    exit(1);
  }

  lzt::reset_command_list(command_list);
  fence = lzt::create_fence(command_queue);
  event = lzt::create_event(event_pool, event_desc);
  lzt::append_memory_copy(command_list, values, src.data(), size, event, 0,
                          nullptr);
  lzt::close_command_list(command_list);

  result =
      zeCommandQueueExecuteCommandLists(command_queue, 1, &command_list, fence);
  if (ZE_RESULT_SUCCESS != result) {
    LOG_ERROR << "[Application] " << __LINE__ << " FAIL: " << result;
    exit(1);
  }
  lzt::event_host_synchronize(event, UINT64_MAX);
  lzt::reset_command_list(command_list);

  auto module = lzt::create_module(device_0, "debug_transform.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV,
                                   "-g -cl-opt-disable", nullptr);
  auto kernel = lzt::create_function(module, "transform_kernel");

  auto device_idx = 0;
  lzt::set_argument_value(kernel, 0, sizeof(values), (&values));
  lzt::set_argument_value(kernel, 1, sizeof(device_idx), &device_idx);

  std::vector<uint32_t> WG(3);
  lzt::suggest_group_size(kernel, size, 0, 0, WG[0], WG[1], WG[2]);
  lzt::set_group_size(kernel, WG[0], WG[1], WG[2]);
  event = lzt::create_event(event_pool, event_desc);

  ze_group_count_t ZeThreadGroupDimensions = {size / WG[0], 1, 1};
  lzt::append_launch_function(command_list, kernel, &ZeThreadGroupDimensions,
                              event, 0, nullptr);
  lzt::append_barrier(command_list);
  lzt::append_memory_copy(command_list, validation_buffer_0, values, size,
                          nullptr, 0, nullptr);
  lzt::close_command_list(command_list);

  result =
      zeCommandQueueExecuteCommandLists(command_queue, 1, &command_list, fence);
  if (ZE_RESULT_SUCCESS != result) {
    LOG_ERROR << "[Application] " << __LINE__ << " FAIL: " << result;
    exit(1);
  }

  lzt::synchronize(command_queue, UINT64_MAX);
  lzt::reset_command_list(command_list);

  // Device 1
  void *values_1;
  values_1 = lzt::allocate_device_memory(size, alignment, 0, device_1, context);
  auto validation_buffer_1 = new uint8_t[size];

  ze_command_queue_handle_t command_queue_1 = lzt::create_command_queue(
      context, device_1, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  ze_command_list_handle_t command_list_1;
  result = zeCommandListCreate(context, device_1, &command_list_desc,
                               &command_list_1);
  if (ZE_RESULT_SUCCESS != result) {
    LOG_ERROR << "[Application] " << __LINE__ << " FAIL: " << result;
    delete[] validation_buffer_0;
    delete[] validation_buffer_1;
    lzt::free_memory(context, values);
    lzt::free_memory(context, values_1);
    FAIL();
  }
  auto fence_1 = lzt::create_fence(command_queue_1);
  auto event_1 = lzt::create_event(event_pool, event_desc);
  lzt::append_memory_copy(command_list_1, values_1, src.data(), size, event_1,
                          0, nullptr);
  lzt::close_command_list(command_list_1);

  result = zeCommandQueueExecuteCommandLists(command_queue_1, 1,
                                             &command_list_1, fence_1);
  if (ZE_RESULT_SUCCESS != result) {
    LOG_ERROR << "[Application] " << __LINE__ << " FAIL: " << result;
    exit(1);
  }

  lzt::reset_command_list(command_list_1);
  lzt::append_memory_copy(command_list_1, values_1, src.data(), size, event_1,
                          0, nullptr);
  lzt::close_command_list(command_list_1);

  auto exec_result = zeCommandQueueExecuteCommandLists(
      command_queue_1, 1, &command_list_1, fence_1);
  if (ZE_RESULT_SUCCESS != result) {
    LOG_ERROR << "[Application] " << __LINE__ << " FAIL: " << result;
    exit(1);
  }
  lzt::synchronize(command_queue_1, UINT64_MAX);
  lzt::reset_command_list(command_list_1);

  module = lzt::create_module(device_1, "debug_transform.spv",
                              ZE_MODULE_FORMAT_IL_SPIRV, "-g -cl-opt-disable",
                              nullptr);
  kernel = lzt::create_function(module, "transform_kernel");

  device_idx = 1;
  lzt::set_argument_value(kernel, 0, sizeof(values_1), (&values_1));
  lzt::set_argument_value(kernel, 1, sizeof(device_idx), &device_idx);

  lzt::suggest_group_size(kernel, size, 0, 0, WG[0], WG[1], WG[2]);
  lzt::set_group_size(kernel, WG[0], WG[1], WG[2]);

  lzt::append_launch_function(command_list_1, kernel, &ZeThreadGroupDimensions,
                              event_1, 0, nullptr);
  lzt::append_barrier(command_list_1);
  lzt::append_memory_copy(command_list_1, validation_buffer_1, values_1, size,
                          nullptr, 0, nullptr);
  lzt::close_command_list(command_list_1);

  result = zeCommandQueueExecuteCommandLists(command_queue_1, 1,
                                             &command_list_1, fence_1);
  if (ZE_RESULT_SUCCESS != result) {
    LOG_ERROR << "[Application] " << __LINE__ << " FAIL: " << result;
    exit(1);
  }
  lzt::synchronize(command_queue_1, UINT64_MAX);

  // validation
  for (int i = 0; i < size; i++) {
    // Kernel function: values[xid] = values[xid] * 3 + 11 * (device_idx + 1);
    auto val_0 = 1 * 3 + 11 * 1;
    EXPECT_EQ(validation_buffer_0[i], val_0);

    auto val_1 = 1 * 3 + 11 * 2;
    EXPECT_EQ(validation_buffer_1[i], val_1);

    if (::testing::Test::HasFailure()) {
      exit(1);
    }
  }

  // cleanup
  delete[] validation_buffer_0;
  delete[] validation_buffer_1;
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_fence(fence);
  lzt::destroy_fence(fence_1);
  lzt::destroy_event(event);
  lzt::destroy_event(event_1);
  lzt::destroy_event_pool(event_pool);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_list(command_list_1);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_command_queue(command_queue_1);
  lzt::free_memory(context, values);
  lzt::free_memory(context, values_1);

  if (::testing::Test::HasFailure()) {
    exit(1);
  }
}

// ***************************************************************************************

int main(int argc, char **argv) {

  static char enable_debug[] = "ZET_ENABLE_PROGRAM_DEBUGGING=1";
  putenv(enable_debug);

  debug_options options;
  try {
    options.parse_options(argc, argv);
  } catch (const boost::bad_any_cast &e) {
    LOG_ERROR << e.what();
    exit(1);
  }

  process_synchro synchro(options.enable_synchro, false, options.index_in);

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_ERROR << "[Application] zeInit failed";
    exit(1);
  }

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  ze_device_handle_t device = nullptr;
  ze_device_handle_t device_0 = nullptr;
  ze_device_handle_t device_1 = nullptr;

  if (options.test_selected != USE_TWO_DEVICES &&
      options.test_selected != MULTI_DEVICE_RESOURCE_STRESS) {

    for (auto &root_device : lzt::get_devices(driver)) {
      if (options.use_sub_devices) {
        LOG_INFO << "[Application] Comparing subdevices";

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
        if (device) {
          LOG_INFO << "Selected sub device: " << device
                   << " of root device: " << root_device;
          break;
        }
      } else {
        LOG_INFO << "[Application] Comparing root device";

        auto device_properties = lzt::get_device_properties(root_device);
        if (strncmp(options.device_id_in.c_str(),
                    lzt::to_string(device_properties.uuid).c_str(),
                    ZE_MAX_DEVICE_NAME)) {
          continue;
        }
        device = root_device;
        LOG_INFO << "Selected root device: " << device;
      }
    }
  } else {

    std::vector<ze_device_handle_t> devices;
    if (!options.use_sub_devices) {
      devices = lzt::get_devices(driver);
    } else {
      // ToDo:: Consider subdevices on different devices
      devices = lzt::get_ze_sub_devices(lzt::get_devices(driver)[0]);
    }

    devices.erase(
        std::remove_if(devices.begin(), devices.end(),
                       [](ze_device_handle_t &device) {
                         auto device_properties =
                             lzt::get_device_properties(device);
                         auto properties = lzt::get_debug_properties(device);

                         return !(ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH &
                                  properties.flags);
                       }),
        devices.end());

    if (devices.size() < 2) {
      LOG_ERROR << "[Application] Not enough devices to compare";
      exit(1);
    }
    lzt::sort_devices(devices);
    device_0 = devices[0];
    device_1 = devices[1];
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
  case LONG_RUNNING_KERNEL_INTERRUPTED_SLM:
    options.use_custom_module = true;
    options.module_name_in = "debug_loop_slm.spv";
    options.kernel_name_in = "long_kernel_slm";
    run_long_kernel(context, device, synchro, options);
    break;
  case MULTIPLE_THREADS:
    run_multiple_threads(context, device, synchro, options);
    break;
  case MULTIPLE_CQ:
    loop_create_destroy_multiple_cq_immcl(context, device, synchro, options,
                                          MULTIPLE_CQ);
    break;
  case MULTIPLE_IMM_CL:
    loop_create_destroy_multiple_cq_immcl(context, device, synchro, options,
                                          MULTIPLE_IMM_CL);
    break;
  case USE_TWO_DEVICES:
    use_two_devices(context, device_0, device_1, synchro, options);
    break;
  case MULTI_DEVICE_RESOURCE_STRESS:
    multidevice_resource_stress_test(context, device_0, device_1, synchro,
                                     options);
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

  lzt::destroy_context(context);

  LOG_INFO << "[Application] Done, exiting";
  return 0;
}
