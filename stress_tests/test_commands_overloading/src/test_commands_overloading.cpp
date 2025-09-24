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

class zeDriverSpreadKernelsStressTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<double, double, uint32_t, ze_memory_type_t, bool, bool,
                     bool, bool, bool>> {
protected:
  void dispatch_kernels(const ze_device_handle_t device,
                        const std::vector<ze_module_handle_t> &module,
                        std::vector<uint32_t *> memory_allocations,
                        std::vector<std::vector<uint32_t>> &data_out,
                        const std::vector<std::string> &test_kernel_names,
                        uint32_t dispatch_count, uint32_t one_case_data_count,
                        ze_context_handle_t context,
                        bool separate_command_lists,
                        bool separate_command_queues, bool separate_modules,
                        bool is_immediate, bool is_registered_on_immediate) {

    std::vector<ze_kernel_handle_t> test_functions;

    ze_command_list_handle_t current_command_list = nullptr;
    std::vector<ze_command_list_handle_t> command_lists;

    ze_command_queue_handle_t current_command_queue = nullptr;
    std::vector<ze_command_queue_handle_t> command_queues;
    bool case_with_immediate = is_immediate || is_registered_on_immediate;
    ze_command_list_handle_t immediate_for_registered_command_lists = nullptr;

    if (separate_command_lists || separate_command_queues) {

      for (uint32_t dispatch_id = 0; dispatch_id < dispatch_count;
           dispatch_id++) {
        if (separate_command_queues && !case_with_immediate) {
          current_command_queue = lzt::create_command_queue(
              context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
              ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
          command_queues.push_back(current_command_queue);
        }
        if (separate_command_lists) {
          if (is_immediate) {
            current_command_list = lzt::create_immediate_command_list(device);
          } else {
            current_command_list = lzt::create_command_list(context, device, 0);
          }
          command_lists.push_back(current_command_list);
        }
      }
    }
    if (!separate_command_queues && !is_immediate) {
      current_command_queue = lzt::create_command_queue(
          context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
      command_queues.push_back(current_command_queue);
    }
    if (!separate_command_lists) {
      if (is_immediate) {
        current_command_list = lzt::create_immediate_command_list(device);
      } else {
        current_command_list = lzt::create_command_list(context, device, 0);
      }
    }

    if (is_registered_on_immediate) {
      immediate_for_registered_command_lists =
          lzt::create_immediate_command_list(device);
    }

    for (uint32_t dispatch_id = 0; dispatch_id < dispatch_count;
         dispatch_id++) {

      if (separate_command_lists) {
        current_command_list = command_lists[dispatch_id];
      }

      ze_kernel_handle_t test_function = nullptr;
      if (separate_modules) {
        test_function = lzt::create_function(module[dispatch_id],
                                             test_kernel_names[dispatch_id]);
      } else {
        test_function =
            lzt::create_function(module[0], test_kernel_names[dispatch_id]);
      }
      lzt::set_group_size(test_function, workgroup_size_x_, 1, 1);
      lzt::set_argument_value(test_function, 0,
                              sizeof(memory_allocations[dispatch_id]),
                              &memory_allocations[dispatch_id]);

      uint32_t group_count_x = one_case_data_count / workgroup_size_x_;
      ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};
      lzt::append_memory_fill(
          current_command_list, memory_allocations[dispatch_id], &dispatch_id,
          sizeof(uint32_t), one_case_data_count * sizeof(uint32_t), nullptr);

      lzt::append_barrier(current_command_list, nullptr);

      lzt::append_launch_function(current_command_list, test_function,
                                  &thread_group_dimensions, nullptr, 0,
                                  nullptr);

      lzt::append_barrier(current_command_list, nullptr);

      lzt::append_memory_copy(
          current_command_list, data_out[dispatch_id].data(),
          memory_allocations[dispatch_id],
          data_out[dispatch_id].size() * sizeof(uint32_t), nullptr);

      lzt::append_barrier(current_command_list, nullptr);

      test_functions.push_back(test_function);
    }
    auto it_command_queues = command_queues.begin();

    if (separate_command_lists) {
      if (is_registered_on_immediate) {
        for (auto command_list : command_lists) {
          lzt::close_command_list(command_list);
        }
        lzt::append_command_lists_immediate_exp(
            immediate_for_registered_command_lists,
            to_u32(command_lists.size()), command_lists.data());
      } else {
        for (auto command_list : command_lists) {
          if (separate_command_queues) {
            current_command_queue = *it_command_queues;
            it_command_queues++;
          }
          if (is_immediate) {
            lzt::synchronize_command_list_host(command_list, UINT64_MAX);
          } else {
            lzt::close_command_list(command_list);
            lzt::execute_command_lists(current_command_queue, 1, &command_list,
                                       nullptr);
            lzt::synchronize(current_command_queue, UINT64_MAX);
          }
        }
      }
    } else {
      lzt::close_command_list(current_command_list);
      if (separate_command_queues) {
        for (auto command_queue : command_queues) {
          if (is_immediate) {
            lzt::synchronize_command_list_host(current_command_list,
                                               UINT64_MAX);
          } else {
            lzt::execute_command_lists(command_queue, 1, &current_command_list,
                                       nullptr);
            lzt::synchronize(current_command_queue, UINT64_MAX);
          }
        }
      } else {
        if (is_immediate) {
          lzt::synchronize_command_list_host(current_command_list, UINT64_MAX);
        } else if (is_registered_on_immediate) {
          std::vector<ze_command_list_handle_t> current_command_lists;
          current_command_lists.emplace_back(current_command_list);
          lzt::append_command_lists_immediate_exp(
              immediate_for_registered_command_lists,
              to_u32(current_command_lists.size()),
              current_command_lists.data());
        } else {
          lzt::execute_command_lists(current_command_queue, 1,
                                     &current_command_list, nullptr);
          lzt::synchronize(current_command_queue, UINT64_MAX);
        }
      }
    }

    if (is_registered_on_immediate) {
      lzt::synchronize_command_list_host(immediate_for_registered_command_lists,
                                         UINT64_MAX);
      lzt::destroy_command_list(immediate_for_registered_command_lists);
    }

    if (!separate_command_queues) {
      if (current_command_queue != nullptr) {
        lzt::destroy_command_queue(current_command_queue);
      }
    }
    if (!separate_command_lists) {
      lzt::destroy_command_list(current_command_list);
    }
    if (separate_command_lists) {
      for (auto command_list : command_lists) {
        lzt::destroy_command_list(command_list);
      }
    }
    if (separate_command_queues) {
      for (auto command_queue : command_queues) {
        if (command_queue != nullptr) {
          lzt::destroy_command_queue(command_queue);
        }
      }
    }

    for (uint64_t dispatch_id = 0; dispatch_id < test_functions.size();
         dispatch_id++) {
      lzt::destroy_function(test_functions[dispatch_id]);
    }
  }

  typedef struct OverloadingTestArguments {
    TestArguments base_arguments;
    bool separate_modules;
    bool separate_command_lists;
    bool separate_command_queues;
    bool is_immediate;
    bool is_registered_on_immediate;
  } OverloadingTestArguments_t;

  uint32_t workgroup_size_x_ = 8;
  uint32_t number_of_kernels_in_module_ = 1;
}; // namespace

LZT_TEST_P(
    zeDriverSpreadKernelsStressTest,
    MultiplyKernelDispatchesSpreadAmongModulesCommandsListsAndCommandQueues) {

  OverloadingTestArguments test_arguments = {
      std::get<0>(GetParam()), // total memory size limit
      std::get<1>(GetParam()), // one allocation size limit
      std::get<2>(GetParam()), // dispatch multiplier
      std::get<3>(GetParam()), // memory type
      std::get<4>(GetParam()), // separated modules mode
      std::get<5>(GetParam()), // separated command lists mode
      std::get<6>(GetParam()), // separated command queues mode
      std::get<7>(GetParam()), // is immediate command list
      std::get<8>(
          GetParam()) // is registerd command lists on immediate command list
  };

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);
  if ((test_arguments.separate_command_queues && test_arguments.is_immediate) ||
      (test_arguments.separate_command_queues &&
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
  LOG_INFO << " | MULTIPLE COMMAND QUEUES = "
           << (test_arguments.separate_command_queues ? "yes" : "no")
           << " | MULTIPLE COMMAND LISTS = "
           << (test_arguments.separate_command_lists ? "yes" : "no")
           << " | MULTIPLE MODULES = "
           << (test_arguments.separate_modules ? "yes" : "no");

  std::vector<ze_device_memory_properties_t> device_memory_properties =
      lzt::get_memory_properties(device);

  const uint32_t used_vectors_in_test = 3;
  const uint32_t multiplier = to_u32(test_arguments.base_arguments.multiplier);
  uint64_t number_of_all_allocations = used_vectors_in_test * multiplier;
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

  uint64_t tmp_count = test_single_allocation_memory_size / sizeof(uint32_t);
  uint64_t test_single_allocation_count =
      tmp_count - tmp_count % workgroup_size_x_;

  uint64_t max_allocation_size = device_properties.maxMemAllocSize;
  uint32_t number_of_requested_dispatches = multiplier;
  bool memory_test_failure = false;

  LOG_INFO << "Test one allocation data count: "
           << test_single_allocation_count;
  LOG_INFO << "Test number of allocations: " << number_of_all_allocations;
  LOG_INFO << "Test kernel dispatches count: "
           << number_of_requested_dispatches;

  std::vector<uint32_t *> output_allocations;
  std::vector<std::vector<uint32_t>> data_for_all_dispatches;
  std::vector<std::string> test_kernel_names;
  std::vector<ze_module_handle_t> multiple_module_handle;
  std::string kernel_file_name = "test_commands_overloading.spv";

  LOG_INFO << "call allocation memory... ";
  for (uint32_t dispatch_id = 0; dispatch_id < number_of_requested_dispatches;
       dispatch_id++) {
    uint32_t *output_allocation;
    output_allocation = allocate_memory<uint32_t>(
        context, device, test_arguments.base_arguments.memory_type,
        test_single_allocation_memory_size, relax_memory_capability);
    output_allocations.push_back(output_allocation);

    std::vector<uint32_t> data_out(test_single_allocation_count, 0);
    data_for_all_dispatches.push_back(data_out);

    std::string kernel_name = "test_device_memory1";
    test_kernel_names.push_back(kernel_name);
    if (test_arguments.separate_modules || dispatch_id == 0) {
      LOG_DEBUG << "call create module";
      ze_module_handle_t module_handle = lzt::create_module(
          context, device, kernel_file_name, ZE_MODULE_FORMAT_IL_SPIRV,
          "-ze-opt-greater-than-4GB-buffer-required", nullptr);
      multiple_module_handle.push_back(module_handle);
    }
  }

  LOG_INFO << "call create module";
  ze_module_handle_t module_handle = lzt::create_module(
      context, device, kernel_file_name, ZE_MODULE_FORMAT_IL_SPIRV,
      "-ze-opt-greater-than-4GB-buffer-required", nullptr);

  LOG_INFO << "call dispatch_kernels";
  dispatch_kernels(device, multiple_module_handle, output_allocations,
                   data_for_all_dispatches, test_kernel_names,
                   number_of_requested_dispatches,
                   to_u32(test_single_allocation_count),
                   context, test_arguments.separate_command_lists,
                   test_arguments.separate_command_queues,
                   test_arguments.separate_modules, test_arguments.is_immediate,
                   test_arguments.is_registered_on_immediate);

  LOG_INFO << "call free output allocation memory";
  for (auto each_allocation : output_allocations) {
    lzt::free_memory(context, each_allocation);
  }

  LOG_INFO << "call destroy multiple modules";
  for (auto next_module : multiple_module_handle) {
    lzt::destroy_module(next_module);
  }

  lzt::destroy_context(context);

  LOG_INFO << "call destroy module";
  lzt::destroy_module(module_handle);

  LOG_INFO << "call verification output";
  uint64_t dispatch_id = 0;
  uint64_t data_id = 0;
  for (auto each_data_out : data_for_all_dispatches) {
    for (uint64_t i = 0; i < each_data_out.size(); i++) {
      if (data_id + dispatch_id != each_data_out[i]) {
        LOG_ERROR << "Index of difference " << i
                  << " dispatch id = " << dispatch_id
                  << " found = " << each_data_out[i]
                  << " expected = " << data_id + dispatch_id;
        memory_test_failure = true;
        break;
      }
      data_id++;
    }
    dispatch_id++;
    data_id = 0;
  }

  EXPECT_EQ(false, memory_test_failure);
}

struct CombinationsTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    ss << "dispatches_" << std::get<2>(info.param) << "_spread_modules_"
       << std::get<4>(info.param) << "_spread_cmd_lists_"
       << std::get<5>(info.param) << "_spread_cmd_queues_"
       << std::get<6>(info.param) << "_immediate_" << std::get<7>(info.param)
       << "_registered_onimmediate_" << std::get<8>(info.param);
    return ss.str();
  }
};

std::vector<uint32_t> dispatches = {1, 4, 100, 1000, 2000};
std::vector<bool> use_separate_modules = {true, false};
std::vector<bool> use_separate_command_lists = {true, false};
std::vector<bool> use_separate_command_queues = {true, false};
std::vector<bool> is_immediate_command_list = {true, false};
std::vector<bool> is_registered_on_immediate_command_list = {true, false};

INSTANTIATE_TEST_CASE_P(
    zeDriverSpreadKernelsStressTestMaxMemory, zeDriverSpreadKernelsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(hundred_percent),
        ::testing::ValuesIn(dispatches), testing::Values(ZE_MEMORY_TYPE_DEVICE),
        ::testing::ValuesIn(use_separate_command_lists),
        ::testing::ValuesIn(use_separate_command_queues),
        ::testing::ValuesIn(use_separate_modules),
        ::testing::ValuesIn(is_immediate_command_list),
        ::testing::ValuesIn(is_registered_on_immediate_command_list)),
    CombinationsTestNameSuffix());
INSTANTIATE_TEST_CASE_P(
    zeDriverSpreadKernelsStressTestMinMemory, zeDriverSpreadKernelsStressTest,
    ::testing::Combine(
        ::testing::Values(hundred_percent), ::testing::Values(five_percent),
        ::testing::ValuesIn(dispatches), testing::Values(ZE_MEMORY_TYPE_DEVICE),
        ::testing::ValuesIn(use_separate_command_lists),
        ::testing::ValuesIn(use_separate_command_queues),
        ::testing::ValuesIn(use_separate_modules),
        ::testing::ValuesIn(is_immediate_command_list),
        ::testing::ValuesIn(is_registered_on_immediate_command_list)),
    CombinationsTestNameSuffix());

} // namespace
