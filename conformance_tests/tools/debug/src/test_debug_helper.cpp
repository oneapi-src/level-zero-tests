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

#include <boost/program_options.hpp>

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_debug.hpp"

namespace bi = boost::interprocess;
namespace po = boost::program_options;
namespace lzt = level_zero_tests;

auto use_sub_devices = 0;
uint32_t num_kernels = 1;
std::string device_id_in = "";
std::string kernel_name_in = "";
debug_test_type_t test_selected = BASIC;

bool *debugger_signal;
bool *debugee_signal;

void basic(ze_context_handle_t context, ze_driver_handle_t driver,
           ze_device_handle_t device, bi::named_condition *condition,
           bool *condvar, bi::scoped_lock<bi::named_mutex> *lock) {

  LOG_INFO << "Waiting for debugger to attach";
  condition->wait(*lock, [&] { return *condvar; });
  LOG_INFO << "Debugged process proceeding";

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  auto module = lzt::create_module(context, device, "debug_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

  auto kernel = lzt::create_function(module, "debug_add_constant_2");

  auto size = 100000001234U;
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

// Debugger attaches after module created
void attach_after_module_created_test(ze_context_handle_t context,
                                      ze_driver_handle_t driver,
                                      ze_device_handle_t device,
                                      bi::named_condition *condition,
                                      bool *condvar_1, bool *condvar_2,
                                      bi::scoped_lock<bi::named_mutex> *lock,
                                      bi::named_mutex *mutex) {

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  auto module = lzt::create_module(context, device, "debug_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

  auto kernel = lzt::create_function(module, "debug_add_constant_2");

  auto size = 100000001234U;
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

  mutex->lock();
  *condvar_2 = true;
  mutex->unlock();
  condition->notify_all();

  LOG_INFO << "Waiting for debugger to attach";
  condition->wait(*lock, [&] { return *condvar_1; });
  LOG_INFO << "Debugged process proceeding";

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

// debuggee process creates multiple modules
void multiple_modules_created_test(ze_context_handle_t context,
                                   ze_driver_handle_t driver,
                                   ze_device_handle_t device) {

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  auto module = lzt::create_module(context, device, "debug_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

  auto module2 = lzt::create_module(context, device, "1kernel.spv",
                                    ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);
  auto kernel2 = lzt::create_function(module2, "kernel1");

  ze_group_count_t group_count = {};
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;
  lzt::append_launch_function(command_list, kernel2, &group_count, nullptr, 0,
                              nullptr);

  auto size = 100000001234U;
  const int addval = 3;

  auto kernels = std::vector<ze_kernel_handle_t>(num_kernels);
  auto initial_buffers = std::vector<void *>(num_kernels);
  auto final_buffers = std::vector<void *>(num_kernels);
  for (int k = 0; k < num_kernels; k++) {
    auto kernel = lzt::create_function(module, "debug_add_constant_2");

    initial_buffers[k] =
        lzt::allocate_shared_memory(size, 0, 0, 0, device, context);
    final_buffers[k] =
        lzt::allocate_device_memory(size, 0, 0, 0, device, context);

    std::memset(initial_buffers[k], 0, size);
    for (size_t i = 0; i < size; i++) {
      static_cast<uint8_t *>(initial_buffers[k])[i] = (i & 0xFF);
    }

    lzt::set_argument_value(kernel, 0, sizeof(final_buffers[k]),
                            &(final_buffers[k]));
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

    lzt::append_memory_copy(command_list, final_buffers[k], initial_buffers[k],
                            size);
    lzt::append_barrier(command_list);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::append_barrier(command_list);
    lzt::append_memory_copy(command_list, initial_buffers[k], final_buffers[k],
                            size);
  }

  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  // validation
  for (int k = 0; k < num_kernels; k++) {
    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<uint8_t *>(initial_buffers[k])[i],
                static_cast<uint8_t>((i & 0xFF) + addval));

      if (::testing::Test::HasFailure()) {
        exit(1);
      }
    }
  }

  // cleanup
  for (int k = 0; k < num_kernels; k++) {
    lzt::free_memory(context, initial_buffers[k]);
    lzt::free_memory(context, final_buffers[k]);

    lzt::destroy_function(kernels[k]);
  }
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_module(module);
}

// debugger waits and attaches after module created and destroyed
void attach_after_module_destroyed_test(
    ze_context_handle_t context, ze_driver_handle_t driver,
    ze_device_handle_t device, bi::named_condition *condition, bool *condvar,
    bi::scoped_lock<bi::named_mutex> *lock, bi::named_mutex *mutex) {

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  auto module = lzt::create_module(context, device, "debug_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, "-g", nullptr);

  auto kernels = std::vector<ze_kernel_handle_t>(num_kernels);

  auto kernel = lzt::create_function(module, "debug_add_constant_2");

  auto size = 100000001234U;
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

  mutex->lock();
  *condvar = true;
  mutex->unlock();
  condition->notify_all();

  LOG_INFO << "Waiting for debugger to attach";
  condition->wait(*lock, [&] { return *condvar; });
  LOG_INFO << "Debugged process proceeding";

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
}

void long_running_kernel_interrupted_test() {}

void breakpoints_test() {

  // lzt::debug_write_memory(debug_session, device_thread, desc, size, buffer);
}

void kernel_resume_test() {}

void page_fault_test(ze_context_handle_t context, ze_driver_handle_t driver,
                     ze_device_handle_t device, bi::named_condition *condition,
                     bool *condvar, bi::scoped_lock<bi::named_mutex> *lock) {
  LOG_INFO << "Waiting for debugger to attach";
  condition->wait(*lock, [&] { return *condvar; });
  LOG_INFO << "Debugged process proceeding";

  const size_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto properties = lzt::get_device_properties(device);

  if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
    LOG_INFO << "[" << properties.name << "] "
             << "Device has on demand page fault support - skipping";
    return;
  }

  // set up
  auto command_list = lzt::create_command_list(device);
  auto command_queue = lzt::create_command_queue(device);

  module = lzt::create_module(device, "residency_tests.spv");
  kernel = lzt::create_function(module, "residency_function");

  auto device_flags = 0;
  auto host_flags = 0;

  typedef struct _node {
    uint32_t value;
    struct _node *next;
  } node;

  node *data = static_cast<node *>(lzt::allocate_shared_memory(
      sizeof(node), 1, device_flags, host_flags, device, context));
  data->value = 0;
  node *temp = data;
  for (int i = 0; i < size; i++) {
    temp->next = static_cast<node *>(lzt::allocate_shared_memory(
        sizeof(node), 1, device_flags, host_flags, device, context));
    temp = temp->next;
    temp->value = i + 1;
  }

  ze_group_count_t group_count;
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
  lzt::set_argument_value(kernel, 1, sizeof(size_t), &size);
  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);
  lzt::close_command_list(command_list);

  temp = data->next;
  node *temp2;
  for (int i = 0; i < size; i++) {
    temp2 = temp->next;
    lzt::make_memory_resident(device, temp, sizeof(node));
    temp = temp2;
  }

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  temp = data->next;
  for (int i = 0; i < size; i++) {
    lzt::evict_memory(device, temp, sizeof(node));
    temp = temp->next;
  }

  // cleanup
  temp = data;
  // total of size elements linked *after* initial element
  for (int i = 0; i < size + 1; i++) {
    // the kernel increments each node's value by 1
    ASSERT_EQ(temp->value, i + 1);

    temp2 = temp->next;
    lzt::free_memory(temp);
    temp = temp2;
  }

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

void device_lost_test(ze_device_handle_t device, bi::named_condition *condition,
                      bool *condvar, bi::scoped_lock<bi::named_mutex> *lock) {
  LOG_INFO << "Waiting for debugger to attach";
  condition->wait(*lock, [&] { return *condvar; });
  LOG_INFO << "Debugged process proceeding";

  lzt::sysman_device_reset(device);
}

void thread_unavailable_test(bi::named_condition *condition, bool *condvar,
                             bi::scoped_lock<bi::named_mutex> *lock) {

  LOG_INFO << "Waiting for debugger to attach";
  condition->wait(*lock, [&] { return *condvar; });
  LOG_INFO << "Debugged process proceeding";

  exit(0);
}

int main(int argc, char **argv) {

  // Open already created shared memory object.
  bi::shared_memory_object shm(bi::open_only, "debug_bool", bi::read_write);
  bi::mapped_region region(shm, bi::read_write);

  bi::named_mutex mutex(bi::open_only, "debugger_mutex");
  bi::named_condition condition(bi::open_only, "debug_bool_set");

  bi::scoped_lock<bi::named_mutex> lock(mutex);
  debugger_signal =
      &(static_cast<debug_signals_t *>(region.get_address())->debugger_signal);
  debugee_signal =
      &(static_cast<debug_signals_t *>(region.get_address())->debugee_signal);

  {
    po::options_description desc("Allowed Options");
    auto options = desc.add_options();

    uint32_t test_selected_int = 0;
    options(test_type_string, po::value<uint32_t>(&test_selected_int),
            "the test type to run");
    options(kernel_string,
            po::value<std::string>(&kernel_name_in)
                ->default_value("debug_add_constant_2"),
            "the kernel to run");
    options(num_kernels_string,
            po::value<uint32_t>(&num_kernels)->default_value(1),
            "number of instances of kernel to create");
    options(use_sub_devices_string, "run the test on subdevices if available");
    options(device_id_string, po::value<std::string>(&device_id_in),
            "Device ID of device to test");

    std::vector<std::string> parser(argv + 1, argv + argc);
    po::parsed_options parsed_options = po::command_line_parser(parser)
                                            .options(desc)
                                            .allow_unregistered()
                                            .run();

    po::variables_map variables_map;
    po::store(parsed_options, variables_map);
    po::notify(variables_map);

    {
      if (variables_map.count(test_type_string)) {
        LOG_INFO << "test type: "
                 << variables_map[test_type_string].as<std::string>() << " "
                 << test_selected;

        if (test_selected_int >= BASIC && test_selected_int <= PAGE_FAULT) {
          test_selected = (debug_test_type_t)test_selected;
        } else {
          LOG_WARNING << "invalid test type selected, performing basic test";
        }
      }
      if (variables_map.count(device_id_string)) {
        LOG_INFO << "device ID: "
                 << variables_map[device_id_string].as<std::string>() << " "
                 << device_id_in;
      }
      if (variables_map.count(use_sub_devices_string)) {
        LOG_INFO << "Using sub devices ";
        use_sub_devices = 1;
      }
      if (variables_map.count(kernel_string)) {
        LOG_INFO << "Kernel:  "
                 << variables_map[kernel_string].as<std::string>() << " "
                 << kernel_name_in;
      }
      if (variables_map.count(num_kernels_string)) {
        LOG_INFO << "Number of kernels: "
                 << variables_map[num_kernels_string].as<uint32_t>();
      }
    }

    ze_result_t result = zeInit(0);
    if (result != ZE_RESULT_SUCCESS) {
      LOG_ERROR << "zeInit failed";
      exit(1);
    }

    auto driver = lzt::get_default_driver();
    auto context = lzt::create_context(driver);
    ze_device_handle_t device = nullptr;
    for (auto &root_device : lzt::get_devices(driver)) {
      if (use_sub_devices) {
        for (auto &sub_device : lzt::get_ze_sub_devices(root_device)) {
          auto device_properties = lzt::get_device_properties(sub_device);
          if (strncmp(device_id_in.c_str(),
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
        auto device_properties = lzt::get_device_properties(root_device);

        if (device_id_in.c_str(),
            lzt::to_string(device_properties.uuid).c_str(),
            ZE_MAX_DEVICE_NAME) {
          continue;
        }
        device = root_device;
      }

      switch (test_selected) {
      case BASIC:
        basic(context, driver, device, &condition, debugger_signal, &lock);
        break;
      case ATTACH_AFTER_MODULE_CREATED:
        attach_after_module_created_test(context, driver, device, &condition,
                                         debugger_signal, debugee_signal, &lock,
                                         &mutex);
        break;
      case MULTIPLE_MODULES_CREATED:
        multiple_modules_created_test(context, driver, device);
        break;
      case ATTACH_AFTER_MODULE_DESTROYED:
        attach_after_module_destroyed_test(context, driver, device, &condition,
                                           debugger_signal, &lock, &mutex);
        break;
      case LONG_RUNNING_KERNEL_INTERRUPTED:
        long_running_kernel_interrupted_test();
        break;
      case BREAKPOINTS:
        breakpoints_test();
        break;
      case KERNEL_RESUME:
        kernel_resume_test();
        break;
      case THREAD_STOPPED:
        device_lost_test(device, &condition, debugger_signal, &lock);
        break;
      case THREAD_UNAVAILABLE:
        thread_unavailable_test(&condition, debugger_signal, &lock);
        break;
      case PAGE_FAULT:
        page_fault_test(context, driver, device, &condition, debugger_signal,
                        &lock);
        break;
      default:
        LOG_ERROR << "Unrecognized test type: " << test_selected;
        exit(1);
      }
    }
    lzt::destroy_context(context);
  }
}
