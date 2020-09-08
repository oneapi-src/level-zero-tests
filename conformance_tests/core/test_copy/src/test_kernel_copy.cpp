/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <array>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

const size_t size = 8;

struct copy_data {
  uint32_t *data;
};

class KernelCopyTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<ze_memory_type_t, int>> {
};

TEST_P(KernelCopyTests,
       GivenDirectMemoryWhenCopyingDataInKernelThenCopyIsCorrect) {
  ze_memory_type_t memory_type = std::get<0>(GetParam());
  int offset = std::get<1>(GetParam());

  for (auto driver : lzt::get_all_driver_handles()) {
    for (auto device : lzt::get_devices(driver)) {
      // set up
      auto command_queue = lzt::create_command_queue();
      auto command_list = lzt::create_command_list();

      auto module = lzt::create_module(device, "copy_module.spv");
      auto kernel = lzt::create_function(module, "copy_data");

      int *input_data, *output_data;
      if (memory_type == ZE_MEMORY_TYPE_HOST) {
        input_data =
            static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));
        output_data =
            static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));
      } else {
        input_data =
            static_cast<int *>(lzt::allocate_shared_memory(size * sizeof(int)));
        output_data =
            static_cast<int *>(lzt::allocate_shared_memory(size * sizeof(int)));
      }

      lzt::write_data_pattern(input_data, size * sizeof(int), 1);
      memset(output_data, 0, size * sizeof(int));

      lzt::set_argument_value(kernel, 0, sizeof(input_data), &input_data);
      lzt::set_argument_value(kernel, 1, sizeof(output_data), &output_data);
      lzt::set_argument_value(kernel, 2, sizeof(int), &offset);
      lzt::set_argument_value(kernel, 3, sizeof(int), &size);

      lzt::set_group_size(kernel, 1, 1, 1);

      ze_group_count_t group_count;
      group_count.groupCountX = 1;
      group_count.groupCountY = 1;
      group_count.groupCountZ = 1;

      lzt::append_launch_function(command_list, kernel, &group_count, nullptr,
                                  0, nullptr);

      lzt::close_command_list(command_list);
      lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
      lzt::synchronize(command_queue, UINT64_MAX);

      ASSERT_EQ(0, memcmp(input_data, output_data + offset,
                          (size - offset) * sizeof(int)));

      // cleanup
      lzt::free_memory(input_data);
      lzt::free_memory(output_data);
      lzt::destroy_function(kernel);
      lzt::destroy_module(module);
      lzt::destroy_command_list(command_list);
      lzt::destroy_command_queue(command_queue);
    }
  }
}

TEST_P(KernelCopyTests,
       GivenInDirectMemoryWhenCopyingDataInKernelThenCopyIsCorrect) {
  ze_memory_type_t memory_type = std::get<0>(GetParam());
  int offset = std::get<1>(GetParam());
  for (auto driver : lzt::get_all_driver_handles()) {
    for (auto device : lzt::get_devices(driver)) {
      // set up
      auto command_queue = lzt::create_command_queue();
      auto command_list = lzt::create_command_list();

      auto module = lzt::create_module(device, "copy_module.spv");
      auto kernel = lzt::create_function(module, "copy_data_indirect");

      // Set up input/output data as containers of memory buffers
      copy_data *input_data, *output_data;

      if (memory_type == ZE_MEMORY_TYPE_HOST) {
        input_data = static_cast<copy_data *>(
            lzt::allocate_host_memory(size * sizeof(copy_data)));
        output_data = static_cast<copy_data *>(
            lzt::allocate_host_memory(size * sizeof(copy_data)));

        for (int i = 0; i < size; i++) {
          input_data[i].data = static_cast<uint32_t *>(
              lzt::allocate_host_memory(size * sizeof(uint32_t)));
          output_data[i].data = static_cast<uint32_t *>(
              lzt::allocate_host_memory(size * sizeof(uint32_t)));
        }
      } else { // shared memory
        input_data = static_cast<copy_data *>(
            lzt::allocate_shared_memory(size * sizeof(copy_data)));
        output_data = static_cast<copy_data *>(
            lzt::allocate_shared_memory(size * sizeof(copy_data)));

        for (int i = 0; i < size; i++) {
          input_data[i].data = static_cast<uint32_t *>(
              lzt::allocate_shared_memory(size * sizeof(uint32_t)));
          output_data[i].data = static_cast<uint32_t *>(
              lzt::allocate_shared_memory(size * sizeof(uint32_t)));
        }
      }

      for (int i = 0; i < size; i++) {
        memset(static_cast<copy_data *>(output_data)[i].data, 0,
               size * sizeof(uint32_t));
        lzt::write_data_pattern(static_cast<copy_data *>(input_data)[i].data,
                                size * sizeof(uint32_t), 1);
      }

      lzt::set_argument_value(kernel, 0, sizeof(input_data), &input_data);
      lzt::set_argument_value(kernel, 1, sizeof(output_data), &output_data);
      lzt::set_argument_value(kernel, 2, sizeof(int), &offset);
      lzt::set_argument_value(kernel, 3, sizeof(int), &size);

      ze_kernel_indirect_access_flags_t indirect_flags =
          (memory_type == ZE_MEMORY_TYPE_HOST)
              ? ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST
              : ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED;
      lzt::kernel_set_indirect_access(kernel, indirect_flags);

      uint32_t group_size_x;
      uint32_t group_size_y;
      uint32_t group_size_z;

      lzt::suggest_group_size(kernel, size, 1, 1, group_size_x, group_size_y,
                              group_size_z);

      lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

      ze_group_count_t group_count;
      group_count.groupCountX = size / group_size_x;
      group_count.groupCountY = 1;
      group_count.groupCountZ = 1;

      lzt::append_launch_function(command_list, kernel, &group_count, nullptr,
                                  0, nullptr);

      lzt::close_command_list(command_list);
      lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
      lzt::synchronize(command_queue, UINT64_MAX);

      for (int i = 0; i < size; i++) {
        ASSERT_EQ(0, memcmp(input_data[i].data, output_data[i].data + offset,
                            (size - offset) * sizeof(uint32_t)));
      }

      // cleanup
      for (int i = 0; i < size; i++) {
        lzt::free_memory(input_data[i].data);
        lzt::free_memory(output_data[i].data);
      }
      lzt::free_memory(input_data);
      lzt::free_memory(output_data);
      lzt::destroy_function(kernel);
      lzt::destroy_module(module);
      lzt::destroy_command_list(command_list);
      lzt::destroy_command_queue(command_queue);
    }
  }
} // namespace

INSTANTIATE_TEST_CASE_P(
    LZT, KernelCopyTests,
    ::testing::Combine(::testing::Values(ZE_MEMORY_TYPE_HOST,
                                         ZE_MEMORY_TYPE_SHARED),
                       ::testing::Values(0, 1, size / 4, size / 2)));

} // namespace
