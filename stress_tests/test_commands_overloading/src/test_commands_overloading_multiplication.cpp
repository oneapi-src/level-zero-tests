/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "stress_common_func.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <bitset>

namespace {

using lzt::to_u32;

class zeDriverMultiplyObjectsStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<double, double, uint32_t, ze_memory_type_t, uint64_t,
                     uint64_t, uint64_t, uint64_t, bool, bool>> {
protected:
  ze_kernel_handle_t create_kernel(ze_module_handle_t module,
                                   const std::string &kernel_name,
                                   std::vector<uint32_t *> &memory_allocations,
                                   uint64_t dispatch_id) {
    ze_kernel_handle_t test_function =
        lzt::create_function(module, kernel_name);
    lzt::set_group_size(test_function, workgroup_size_x_, 1, 1);
    lzt::set_argument_value(test_function, 0,
                            sizeof(memory_allocations[dispatch_id]),
                            &memory_allocations[dispatch_id]);
    return test_function;
  };

  void set_command_list(ze_command_list_handle_t &command_list,
                        const ze_kernel_handle_t &test_kernel,
                        uint32_t one_case_data_count, uint64_t dispatch_id,
                        std::vector<uint32_t *> &memory_allocations,
                        std::vector<std::vector<uint32_t>> &data_out) {
    uint32_t group_count_x = one_case_data_count / workgroup_size_x_;
    ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};
    lzt::append_memory_fill(command_list, memory_allocations[dispatch_id],
                            &dispatch_id, sizeof(uint32_t),
                            one_case_data_count * sizeof(uint32_t), nullptr);

    lzt::append_barrier(command_list, nullptr);

    lzt::append_launch_function(command_list, test_kernel,
                                &thread_group_dimensions, nullptr, 0, nullptr);
    lzt::append_barrier(command_list, nullptr);

    lzt::append_memory_copy(command_list, data_out[dispatch_id].data(),
                            memory_allocations[dispatch_id],
                            data_out[dispatch_id].size() * sizeof(uint32_t),
                            nullptr);
    lzt::append_barrier(command_list, nullptr);
  };

  void send_command_to_execution(ze_command_list_handle_t command_list,
                                 const ze_command_queue_handle_t &command_queue,
                                 bool is_immediate) {
    if (is_immediate) {
      lzt::synchronize_command_list_host(command_list, UINT64_MAX);
    } else {
      lzt::close_command_list(command_list);
      lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
      lzt::synchronize(command_queue, UINT64_MAX);
    }

    // can be used in next iteration. Need to be recycled
    lzt::reset_command_list(command_list);
  };

  typedef struct OverloadingMTestArguments {
    TestArguments base_arguments;
    uint64_t multiply_kernels;
    uint64_t multiply_modules;
    uint64_t multiply_command_lists;
    uint64_t multiply_command_queues;
    bool is_immediate;
    bool is_registered_on_immediate;
  } OverloadingMTestArguments_t;

  void set_modules_for_cmdlists(
      const ze_context_handle_t &context, const ze_device_handle_t &device,
      OverloadingMTestArguments_t test_arguments,
      ze_command_list_handle_t &next_cmd_list,
      uint32_t test_single_allocation_count, uint64_t &dispatch_id,
      std::vector<uint32_t *> &output_allocations,
      std::vector<std::vector<uint32_t>> &data_for_all_dispatches,
      std::vector<ze_module_handle_t> &multiple_module_handle,
      std::vector<ze_kernel_handle_t> &multiple_kernel_handle) {
    for (uint64_t module_id = 0; module_id < test_arguments.multiply_modules;
         module_id++) {
      ze_module_handle_t module_handle = lzt::create_module(
          context, device, "test_commands_overloading.spv",
          ZE_MODULE_FORMAT_IL_SPIRV, "-ze-opt-greater-than-4GB-buffer-required",
          nullptr);
      multiple_module_handle.push_back(module_handle);
      for (uint64_t kernel_id = 0; kernel_id < test_arguments.multiply_kernels;
           kernel_id++) {
        std::string kernel_name = "test_device_memory1";

        ze_kernel_handle_t kernel_handle = create_kernel(
            module_handle, kernel_name, output_allocations, dispatch_id);

        multiple_kernel_handle.push_back(kernel_handle);
        set_command_list(next_cmd_list, kernel_handle,
                         test_single_allocation_count, dispatch_id,
                         output_allocations, data_for_all_dispatches);
        dispatch_id++;
      }
    }
  };

  uint32_t use_this_ordinal_on_device_ = 0;
  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernels_in_source_ = 1;
}; // namespace

LZT_TEST_P(zeDriverMultiplyObjectsStressTest,
           MultiplyKernelsModulsCommandsListsAndCommandQueues) {

  OverloadingMTestArguments_t test_arguments = {
      std::get<0>(GetParam()), // total memory size limit
      std::get<1>(GetParam()), // one allocation size limit
      std::get<2>(GetParam()), // dispatch multiplier
      std::get<3>(GetParam()), // memory type
      std::get<4>(GetParam()), // multiply kernels
      std::get<5>(GetParam()), // multiply modules
      std::get<6>(GetParam()), // multiply command lists
      std::get<7>(GetParam()), // multiply command queues
      std::get<8>(GetParam()), // is immediate command list
      std::get<9>(
          GetParam()) // is registerd command lists on immediate command list

  };

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  if ((test_arguments.multiply_command_queues > 1 &&
       test_arguments.is_immediate) ||
      (test_arguments.multiply_command_queues > 1 &&
       test_arguments.is_registered_on_immediate)) {
    GTEST_SKIP()
        << " Test skipped. Immediate command lists do not use command queues";
  }

  if (test_arguments.is_immediate &&
      test_arguments.is_registered_on_immediate) {
    GTEST_SKIP()
        << " Test skipped. Immediate command list is designed only to run only "
           "registered command lists";
  }

  ze_device_properties_t device_properties = lzt::get_device_properties(device);
  test_arguments.base_arguments.print_test_arguments(device_properties);

  std::vector<ze_device_memory_properties_t> device_memory_properties =
      lzt::get_memory_properties(device);

  uint64_t requested_dispatches = test_arguments.multiply_kernels *
                                  test_arguments.multiply_modules *
                                  test_arguments.multiply_command_lists *
                                  test_arguments.multiply_command_queues;
  const uint32_t used_vectors_in_test = 3;
  uint64_t number_of_all_allocations =
      used_vectors_in_test * requested_dispatches;

  double total_memory_size_limit =
      test_arguments.base_arguments.total_memory_size_limit;
  double one_allocation_size_limit =
      test_arguments.base_arguments.one_allocation_size_limit;

  uint64_t test_single_allocation_memory_size = 0;
  uint64_t test_total_memory_size = 0;
  bool relax_memory_capability = false;

  adjust_max_memory_allocation(
      driver, device_properties, device_memory_properties,
      test_total_memory_size, test_single_allocation_memory_size,
      number_of_all_allocations, test_arguments.base_arguments,
      relax_memory_capability);

  if (number_of_all_allocations !=
      used_vectors_in_test * requested_dispatches) {

    LOG_INFO << "Need to limit dispatches from : " << requested_dispatches
             << " to: " << number_of_all_allocations / used_vectors_in_test;

    requested_dispatches = number_of_all_allocations / used_vectors_in_test;
    while (requested_dispatches < test_arguments.multiply_kernels *
                                      test_arguments.multiply_modules *
                                      test_arguments.multiply_command_lists *
                                      test_arguments.multiply_command_queues) {
      if (test_arguments.multiply_kernels == 1 &&
          test_arguments.multiply_modules == 1 &&
          test_arguments.multiply_command_lists == 1 &&
          test_arguments.multiply_command_queues == 1)
        break;

      if (test_arguments.multiply_kernels > 1)
        --test_arguments.multiply_kernels;
      else if (test_arguments.multiply_modules > 1)
        --test_arguments.multiply_modules;
      else if (test_arguments.multiply_command_lists > 1)
        --test_arguments.multiply_command_lists;
      else if (test_arguments.multiply_command_queues > 1)
        --test_arguments.multiply_command_queues;
    }

    requested_dispatches = test_arguments.multiply_kernels *
                           test_arguments.multiply_modules *
                           test_arguments.multiply_command_lists *
                           test_arguments.multiply_command_queues;
  }

  uint32_t tmp_count =
      to_u32(test_single_allocation_memory_size / sizeof(uint32_t));
  uint32_t test_single_allocation_count =
      tmp_count - tmp_count % workgroup_size_x_;

  bool memory_test_failure = false;

  LOG_INFO << "Test one allocation data count: "
           << test_single_allocation_count;
  LOG_INFO << "Test number of allocations: " << number_of_all_allocations;
  LOG_INFO << "Test kernel dispatches count: " << requested_dispatches;

  std::vector<uint32_t *> output_allocations;
  std::vector<std::vector<uint32_t>> data_for_all_dispatches;
  std::vector<std::string> test_kernel_names;
  std::vector<ze_module_handle_t> multiple_module_handle;
  std::vector<ze_kernel_handle_t> multiple_kernel_handle;
  std::vector<ze_command_list_handle_t> multiple_command_list_handle;
  std::vector<ze_command_queue_handle_t> multiple_command_queue_handle;

  LOG_INFO << "call allocation memory... ";
  for (uint64_t dispatch_id = 0; dispatch_id < requested_dispatches;
       dispatch_id++) {
    uint32_t *output_allocation;
    output_allocation = allocate_memory<uint32_t>(
        context, device, test_arguments.base_arguments.memory_type,
        test_single_allocation_memory_size, relax_memory_capability);
    output_allocations.push_back(output_allocation);
    std::vector<uint32_t> data_out(test_single_allocation_count, 0);
    data_for_all_dispatches.push_back(data_out);
  }

  LOG_INFO << "call create command queues... ";
  for (uint64_t cmd_queue_id = 0;
       cmd_queue_id < test_arguments.multiply_command_queues; cmd_queue_id++) {
    ze_command_queue_handle_t command_queue = nullptr;
    if (!test_arguments.is_immediate) {
      command_queue = lzt::create_command_queue(
          context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    }
    multiple_command_queue_handle.push_back(command_queue);
  }

  LOG_INFO << "call create command lists... ";
  for (uint64_t cmd_list_id = 0;
       cmd_list_id < test_arguments.multiply_command_lists; cmd_list_id++) {
    ze_command_list_handle_t command_list = nullptr;
    if (test_arguments.is_immediate) {
      command_list = lzt::create_immediate_command_list(device);
    } else {
      command_list = lzt::create_command_list(context, device, 0);
    }
    multiple_command_list_handle.push_back(command_list);
  }

  LOG_INFO << "call create modules and kernels, set command lists.";
  LOG_INFO << "finally send command list to execution... ";
  ze_command_list_handle_t immediate_for_registered_command_lists = nullptr;
  uint64_t dispatch_id = 0;
  if (test_arguments.is_registered_on_immediate) {
    immediate_for_registered_command_lists =
        lzt::create_immediate_command_list(device);
    for (auto &next_cmd_list : multiple_command_list_handle) {
      set_modules_for_cmdlists(context, device, test_arguments, next_cmd_list,
                               test_single_allocation_count, dispatch_id,
                               output_allocations, data_for_all_dispatches,
                               multiple_module_handle, multiple_kernel_handle);
      lzt::close_command_list(next_cmd_list);
    }
    lzt::append_command_lists_immediate_exp(
        immediate_for_registered_command_lists,
        to_u32(multiple_command_list_handle.size()),
        multiple_command_list_handle.data());
    lzt::synchronize_command_list_host(immediate_for_registered_command_lists,
                                       UINT64_MAX);
  } else {
    for (auto &next_cmd_queue : multiple_command_queue_handle) {
      for (auto &next_cmd_list : multiple_command_list_handle) {
        set_modules_for_cmdlists(context, device, test_arguments, next_cmd_list,
                                 test_single_allocation_count, dispatch_id,
                                 output_allocations, data_for_all_dispatches,
                                 multiple_module_handle,
                                 multiple_kernel_handle);
        send_command_to_execution(next_cmd_list, next_cmd_queue,
                                  test_arguments.is_immediate);
      }
    }
  }

  LOG_INFO << "call verification output";
  dispatch_id = 0;
  uint64_t data_id = 0;
  for (auto each_data_out : data_for_all_dispatches) {

    for (uint32_t i = 0; i < test_single_allocation_count; i++) {
      if (data_id + dispatch_id != each_data_out[i]) {
        LOG_ERROR << "Results for dispatch ==  " << dispatch_id << " failed. "
                  << " The index " << i << " found = " << each_data_out[i]
                  << " expected = " << data_id + dispatch_id;
        memory_test_failure = true;
        break;
      }
      data_id++;
    }
    dispatch_id++;
    data_id = 0;
  }

  LOG_INFO << "call free test memory objects";
  for (auto next_allocation : output_allocations) {
    lzt::free_memory(context, next_allocation);
  }

  for (auto next_kernel : multiple_kernel_handle) {
    lzt::destroy_function(next_kernel);
  }

  for (auto next_module : multiple_module_handle) {
    lzt::destroy_module(next_module);
  }

  for (auto next_cmd_list : multiple_command_list_handle) {
    lzt::destroy_command_list(next_cmd_list);
  }

  for (auto next_cmd_queue : multiple_command_queue_handle) {
    if (next_cmd_queue != nullptr) {
      lzt::destroy_command_queue(next_cmd_queue);
    }
  }
  if (test_arguments.is_registered_on_immediate) {
    lzt::destroy_command_list(immediate_for_registered_command_lists);
  }
  lzt::destroy_context(context);

  EXPECT_EQ(false, memory_test_failure);
}

struct CombinationsTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    ss << "kernels_" << std::get<4>(info.param) << "_modules_"
       << std::get<5>(info.param) << "_cmd_lists_" << std::get<6>(info.param)
       << "_cmd_queues_" << std::get<7>(info.param) << "_is_immediate_"
       << std::get<8>(info.param) << "_registered_onimmediate_"
       << std::get<9>(info.param);
    return ss.str();
  }
};

std::vector<uint64_t> input_values = {1, 2, 4, 10, 100, 1000, 2000};
std::vector<uint64_t> multiple_kernels = input_values;
std::vector<uint64_t> multiple_command_lists = input_values;
std::vector<uint64_t> multiple_command_queues = input_values;
std::vector<uint64_t> multiple_modules = input_values;
std::vector<bool> is_immediate_command_list = {true, false};
std::vector<bool> is_registered_on_immediate_command_list = {true, false};

INSTANTIATE_TEST_CASE_P(
    zeDriverMultiplyObjectsStressTestMinMemory,
    zeDriverMultiplyObjectsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(one_percent),
        ::testing::Values(1), testing::Values(ZE_MEMORY_TYPE_DEVICE),
        ::testing::ValuesIn(multiple_kernels),
        ::testing::ValuesIn(multiple_modules),
        ::testing::ValuesIn(multiple_command_lists),
        ::testing::ValuesIn(multiple_command_queues),
        ::testing::ValuesIn(is_immediate_command_list),
        ::testing::ValuesIn(is_registered_on_immediate_command_list)),
    CombinationsTestNameSuffix());
INSTANTIATE_TEST_CASE_P(
    zeDriverMultiplyObjectsStressTestMaxMemory,
    zeDriverMultiplyObjectsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(hundred_percent),
        ::testing::Values(1), testing::Values(ZE_MEMORY_TYPE_DEVICE),
        ::testing::ValuesIn(multiple_kernels),
        ::testing::ValuesIn(multiple_modules),
        ::testing::ValuesIn(multiple_command_lists),
        ::testing::ValuesIn(multiple_command_queues),
        ::testing::ValuesIn(is_immediate_command_list),
        ::testing::ValuesIn(is_registered_on_immediate_command_list)),
    CombinationsTestNameSuffix());

} // namespace
