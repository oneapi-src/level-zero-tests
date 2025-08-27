/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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

enum shared_memory_type { SHARED_LOCAL, SHARED_CROSS, SHARED_SYSTEM };
enum test_memory_type {
  USM_HOST,
  USM_DEVICE,
  USM_SHARED_LOCAL,
  USM_SHARED_CROSS,
  USM_SHARED_SYSTEM
};

const size_t size = 8;

struct copy_data {
  uint32_t *data;
};

class KernelCopyTests : public ::testing::Test,
                        public ::testing::WithParamInterface<
                            std::tuple<ze_memory_type_t, int, bool>> {};

LZT_TEST_P(KernelCopyTests,
           GivenDirectMemoryWhenCopyingDataInKernelThenCopyIsCorrect) {
  ze_memory_type_t memory_type = std::get<0>(GetParam());
  size_t offset = std::get<1>(GetParam());
  bool is_immediate = std::get<2>(GetParam());

  for (auto driver : lzt::get_all_driver_handles()) {
    for (auto device : lzt::get_devices(driver)) {
      // set up
      auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);

      auto module = lzt::create_module(device, "copy_module.spv");
      auto kernel = lzt::create_function(module, "copy_data");

      int *input_data, *output_data;
      if (memory_type == ZE_MEMORY_TYPE_HOST) {
        input_data =
            static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));
        output_data =
            static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));
      } else {
        input_data = static_cast<int *>(
            lzt::allocate_shared_memory(size * sizeof(int), device));
        output_data = static_cast<int *>(
            lzt::allocate_shared_memory(size * sizeof(int), device));
      }

      lzt::write_data_pattern(input_data, size * sizeof(int), 1);
      memset(output_data, 0, size * sizeof(int));

      lzt::set_argument_value(kernel, 0, sizeof(input_data), &input_data);
      lzt::set_argument_value(kernel, 1, sizeof(output_data), &output_data);
      lzt::set_argument_value(kernel, 2, sizeof(offset), &offset);
      lzt::set_argument_value(kernel, 3, sizeof(size), &size);

      lzt::set_group_size(kernel, 1, 1, 1);

      ze_group_count_t group_count;
      group_count.groupCountX = 1;
      group_count.groupCountY = 1;
      group_count.groupCountZ = 1;

      lzt::append_launch_function(cmd_bundle.list, kernel, &group_count,
                                  nullptr, 0, nullptr);

      lzt::close_command_list(cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

      ASSERT_EQ(0, memcmp(input_data, output_data + offset,
                          (size - offset) * sizeof(int)));

      // cleanup
      lzt::free_memory(input_data);
      lzt::free_memory(output_data);
      lzt::destroy_function(kernel);
      lzt::destroy_module(module);
      lzt::destroy_command_bundle(cmd_bundle);
    }
  }
}

LZT_TEST_P(KernelCopyTests,
           GivenInDirectMemoryWhenCopyingDataInKernelThenCopyIsCorrect) {
  ze_memory_type_t memory_type = std::get<0>(GetParam());
  int offset = std::get<1>(GetParam());
  bool is_immediate = std::get<2>(GetParam());
  for (auto driver : lzt::get_all_driver_handles()) {
    for (auto device : lzt::get_devices(driver)) {
      // set up
      auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);

      auto module = lzt::create_module(device, "copy_module.spv");
      auto kernel = lzt::create_function(module, "copy_data_indirect");

      // Set up input/output data as containers of memory buffers
      copy_data *input_data, *output_data;

      if (memory_type == ZE_MEMORY_TYPE_HOST) {
        input_data = static_cast<copy_data *>(
            lzt::allocate_host_memory(size * sizeof(copy_data)));
        output_data = static_cast<copy_data *>(
            lzt::allocate_host_memory(size * sizeof(copy_data)));

        for (size_t i = 0U; i < size; i++) {
          input_data[i].data = static_cast<uint32_t *>(
              lzt::allocate_host_memory(size * sizeof(uint32_t)));
          output_data[i].data = static_cast<uint32_t *>(
              lzt::allocate_host_memory(size * sizeof(uint32_t)));
        }
      } else { // shared memory
        input_data = static_cast<copy_data *>(
            lzt::allocate_shared_memory(size * sizeof(copy_data), device));
        output_data = static_cast<copy_data *>(
            lzt::allocate_shared_memory(size * sizeof(copy_data), device));

        for (size_t i = 0U; i < size; i++) {
          input_data[i].data = static_cast<uint32_t *>(
              lzt::allocate_shared_memory(size * sizeof(uint32_t), device));
          output_data[i].data = static_cast<uint32_t *>(
              lzt::allocate_shared_memory(size * sizeof(uint32_t), device));
        }
      }

      for (size_t i = 0U; i < size; i++) {
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

      lzt::append_launch_function(cmd_bundle.list, kernel, &group_count,
                                  nullptr, 0, nullptr);

      lzt::close_command_list(cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

      for (size_t i = 0U; i < size; i++) {
        ASSERT_EQ(0, memcmp(input_data[i].data, output_data[i].data + offset,
                            (size - offset) * sizeof(uint32_t)));
      }

      // cleanup
      for (size_t i = 0U; i < size; i++) {
        lzt::free_memory(input_data[i].data);
        lzt::free_memory(output_data[i].data);
      }
      lzt::free_memory(input_data);
      lzt::free_memory(output_data);
      lzt::destroy_function(kernel);
      lzt::destroy_module(module);
      lzt::destroy_command_bundle(cmd_bundle);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    LZT, KernelCopyTests,
    ::testing::Combine(::testing::Values(ZE_MEMORY_TYPE_HOST,
                                         ZE_MEMORY_TYPE_SHARED),
                       ::testing::Values(0, 1, size / 4, size / 2),
                       ::testing::Bool()));

class KernelCopyTestsWithIndirectMemoryTypes
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<test_memory_type, test_memory_type, test_memory_type,
                     test_memory_type, int, bool>> {

protected:
  void SetUp() override {
    drivers = lzt::get_all_driver_handles();
    for (size_t i = 0U; i < drivers.size(); i++) {
      devices.push_back(std::vector<ze_device_handle_t>());
      devices[i] = lzt::get_devices(drivers[i]);
      contexts.push_back(lzt::create_context(drivers[i]));
      memory_access_properties.push_back(
          std::vector<ze_device_memory_access_properties_t>());
      for (size_t j = 0U; j < devices[i].size(); j++) {
        ze_device_memory_access_properties_t mem_access_properties{
            ZE_STRUCTURE_TYPE_DEVICE_MEMORY_ACCESS_PROPERTIES, nullptr};
        EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetMemoryAccessProperties(
            devices[i][j], &mem_access_properties));
        memory_access_properties[i].push_back(mem_access_properties);
      }
    }
  }

  void TearDown() override {
    drivers.clear();
    devices.clear();
    for (size_t i = 0U; i < contexts.size(); i++) {
      lzt::destroy_context(contexts[i]);
    }
    contexts.clear();
    memory_access_properties.clear();
  }

  void *allocate_memory(uint32_t driver_index, uint32_t device_index,
                        uint32_t cross_device_index, ze_memory_type_t mem_type,
                        shared_memory_type shr_type, size_t size) {
    return allocate_memory(driver_index, device_index, cross_device_index,
                           mem_type, shr_type, size, contexts[driver_index]);
  }

  void *allocate_memory(uint32_t driver_index, uint32_t device_index,
                        uint32_t cross_device_index, ze_memory_type_t mem_type,
                        shared_memory_type shr_type, size_t size,
                        ze_context_handle_t context) {

    auto device = devices[driver_index][device_index];
    auto mem_access_properties =
        memory_access_properties[driver_index][device_index];
    if (mem_type == ZE_MEMORY_TYPE_HOST) {
      auto memory_access_cap = mem_access_properties.hostAllocCapabilities;
      if ((memory_access_cap & ZE_MEMORY_ACCESS_CAP_FLAG_RW) == 0) {
        LOG_INFO << "WARNING: Unable to allocate host memory, skipping";
        return nullptr;
      }
      ze_result_t result;
      return lzt::allocate_host_memory_no_check(size, 1, context, &result);
    } else if (mem_type == ZE_MEMORY_TYPE_DEVICE) {
      auto memory_access_cap = mem_access_properties.deviceAllocCapabilities;
      if ((memory_access_cap & ZE_MEMORY_ACCESS_CAP_FLAG_RW) == 0) {
        LOG_INFO << "WARNING: Unable to allocate device memory, skipping";
        return nullptr;
      }
      ze_result_t result;
      return lzt::allocate_device_memory_no_check(size, 1, 0, nullptr, 0,
                                                  device, context, &result);
    } else { // shared
      if (shr_type == SHARED_SYSTEM) {
        if (!lzt::supports_shared_system_alloc(mem_access_properties)) {
          LOG_INFO
              << "WARNING: Unable to allocate shared system memory, skipping";
          return nullptr;
        }
        return malloc(size);
      } else { // shared_cross or shared_local
        if (shr_type == SHARED_LOCAL) {
          auto memory_access_cap =
              mem_access_properties.sharedSingleDeviceAllocCapabilities;
          if ((memory_access_cap & ZE_MEMORY_ACCESS_CAP_FLAG_RW) == 0) {
            LOG_INFO
                << "WARNING: Unable to allocate shared local memory, skipping";
            return nullptr;
          }
          ze_result_t result;
          return lzt::allocate_shared_memory_no_check(
              size, 1, 0, nullptr, 0, nullptr, device, context, &result);
        } else { // shared cross
          auto cross_device = devices[driver_index][cross_device_index];
          auto memory_access_cap =
              mem_access_properties.sharedCrossDeviceAllocCapabilities;
          if ((memory_access_cap & ZE_MEMORY_ACCESS_CAP_FLAG_RW) == 0) {
            LOG_INFO
                << "WARNING: Unable to allocate shared cross memory, skipping";
            return nullptr;
          }
          ze_result_t result;
          return lzt::allocate_shared_memory_no_check(
              size, 1, 0, nullptr, 0, nullptr, cross_device, context, &result);
        }
      }
    }
  }

  void free_memory(void *ptr, ze_context_handle_t context,
                   ze_memory_type_t mem_type,
                   shared_memory_type shr_type = SHARED_LOCAL) {
    if (mem_type == ZE_MEMORY_TYPE_SHARED && shr_type == SHARED_SYSTEM) {
      free(ptr);
    } else {
      lzt::free_memory(context, ptr);
    }
  }

  uint32_t get_cross_device_index(uint32_t driver_index, uint32_t device_index) {
    for (uint32_t i = 0U; i < devices.size(); i++) {
      if (i != device_index) {
        return i;
      }
    }
    return device_index;
  }

  bool get_memory_info(ze_memory_type_t &mem_type, shared_memory_type &shr_type,
                       const test_memory_type test_type) {

    switch (test_type) {
    case USM_HOST:
      mem_type = ZE_MEMORY_TYPE_HOST;
      shr_type = SHARED_LOCAL;
      break;
    case USM_DEVICE:
      mem_type = ZE_MEMORY_TYPE_DEVICE;
      shr_type = SHARED_LOCAL;
      break;
    case USM_SHARED_LOCAL:
      mem_type = ZE_MEMORY_TYPE_SHARED;
      shr_type = SHARED_LOCAL;
      break;
    case USM_SHARED_CROSS:
      mem_type = ZE_MEMORY_TYPE_SHARED;
      shr_type = SHARED_CROSS;
      break;
    case USM_SHARED_SYSTEM:
      mem_type = ZE_MEMORY_TYPE_SHARED;
      shr_type = SHARED_SYSTEM;
      break;
    default:
      LOG_INFO << "WARNING: unknown system type detected";
      return false;
    }
    return true;
  }

  void test_indirect_memory_kernel_copy(
      uint32_t driver_index, uint32_t device_index, ze_memory_type_t src_mem_type,
      ze_memory_type_t src_ptr_mem_type, ze_memory_type_t dst_mem_type,
      ze_memory_type_t dst_ptr_mem_type, int offset,
      shared_memory_type src_shr_type = SHARED_LOCAL,
      shared_memory_type src_ptr_shr_type = SHARED_LOCAL,
      shared_memory_type dst_shr_type = SHARED_LOCAL,
      shared_memory_type dst_ptr_shr_type = SHARED_LOCAL,
      bool is_immediate = false) {

    auto driver = drivers[driver_index];
    auto device = devices[driver_index][device_index];
    auto context = contexts[device_index];

    uint32_t cross_device_index = get_cross_device_index(driver_index, device_index);
    if (cross_device_index == device_index &&
        (src_shr_type == SHARED_CROSS || src_ptr_shr_type == SHARED_CROSS ||
         dst_shr_type == SHARED_CROSS || dst_ptr_shr_type == SHARED_CROSS)) {
      LOG_INFO << "WARNING: cannot find alternative device to setup shared"
                  " cross devices, skipping...";
      GTEST_SKIP();
    }
    // set up
    auto cmd_bundle = lzt::create_command_bundle(context, device, is_immediate);

    auto module = lzt::create_module(device, "copy_module.spv");
    auto kernel = lzt::create_function(module, "copy_data_indirect");

    // Set up src/dst data as containers of memory buffers
    copy_data *src_data, *dst_data, *src_data_host_ptr = nullptr,
                                    *dst_data_host_ptr = nullptr;
    std::vector<uint32_t *> src_data_ptr_array, dst_data_ptr_array;
    src_data = static_cast<copy_data *>(allocate_memory(
        driver_index, device_index, cross_device_index, src_ptr_mem_type,
        src_ptr_shr_type, size * sizeof(copy_data), context));
    dst_data = static_cast<copy_data *>(allocate_memory(
        driver_index, device_index, cross_device_index, dst_ptr_mem_type,
        dst_ptr_shr_type, size * sizeof(copy_data), context));
    if (src_data == nullptr || dst_data == nullptr) {
      if (src_data)
        free_memory(src_data, context, src_ptr_mem_type, src_ptr_shr_type);
      if (dst_data)
        free_memory(dst_data, context, dst_ptr_mem_type, dst_ptr_shr_type);
      GTEST_SKIP() << "Not Enough memory to execute test\n";
      return;
    }

    for (size_t i = 0U; i < size; i++) {
      src_data_ptr_array.push_back(static_cast<uint32_t *>(allocate_memory(
          driver_index, device_index, cross_device_index, src_mem_type,
          src_shr_type, size * sizeof(uint32_t), context)));
      dst_data_ptr_array.push_back(static_cast<uint32_t *>(allocate_memory(
          driver_index, device_index, cross_device_index, dst_mem_type,
          dst_shr_type, size * sizeof(uint32_t), context)));
      if (src_data_ptr_array[i] == nullptr ||
          dst_data_ptr_array[i] == nullptr) {
        // deallocate all previous memory allocations before skipping the test
        for (size_t j = 0U; j < i; j++) {
          free_memory(src_data_ptr_array[j], context, src_mem_type,
                      src_shr_type);
          free_memory(dst_data_ptr_array[j], context, dst_mem_type,
                      dst_shr_type);
        }
        if (src_data_ptr_array[i])
          free_memory(src_data_ptr_array[i], context, src_mem_type,
                      src_shr_type);
        if (dst_data_ptr_array[i])
          free_memory(dst_data_ptr_array[i], context, dst_mem_type,
                      dst_shr_type);
        if (src_data)
          free_memory(src_data, context, src_ptr_mem_type, src_ptr_shr_type);
        if (dst_data)
          free_memory(dst_data, context, dst_ptr_mem_type, dst_ptr_shr_type);
        GTEST_SKIP() << "Not Enough memory to execute test\n";
        return;
      }
    }

    // Set up input/output data as containers of memory buffers, host ptr
    copy_data *input_data, *output_data;
    input_data = new copy_data[size];
    output_data = new copy_data[size];

    for (size_t i = 0U; i < size; i++) {
      input_data[i].data = new uint32_t[size];
      output_data[i].data = new uint32_t[size];
      memset(output_data[i].data, 0, size * sizeof(uint32_t));
      lzt::write_data_pattern(input_data[i].data, size * sizeof(uint32_t), 1);
    }

    // copy input data to src_data
    if (src_mem_type == ZE_MEMORY_TYPE_DEVICE) {
      for (size_t i = 0U; i < size; i++) {
        lzt::append_memory_copy(cmd_bundle.list, src_data_ptr_array[i],
                                input_data[i].data, size * sizeof(uint32_t),
                                nullptr);
      }
      lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    } else {
      for (size_t i = 0U; i < size; i++) {
        memcpy(src_data_ptr_array[i], input_data[i].data,
               size * sizeof(uint32_t));
      }
    }
    // handling pointers in src_data[i].data
    if (src_ptr_mem_type == ZE_MEMORY_TYPE_DEVICE) {
      // generate copy_data array in host and copy the array to device
      src_data_host_ptr = new copy_data[size];
      for (size_t i = 0U; i < size; i++) {
        src_data_host_ptr[i].data = src_data_ptr_array[i];
      }
      lzt::append_memory_copy(cmd_bundle.list, src_data, src_data_host_ptr,
                              size * sizeof(copy_data), nullptr);
      lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    } else {
      for (size_t i = 0U; i < size; i++) {
        src_data[i].data = src_data_ptr_array[i];
      }
    }
    // handling pointers in dst_data[i].data
    if (dst_ptr_mem_type == ZE_MEMORY_TYPE_DEVICE) {
      // generate copy_data array in host and copy the array to device
      dst_data_host_ptr = new copy_data[size];
      for (size_t i = 0U; i < size; i++) {
        dst_data_host_ptr[i].data = dst_data_ptr_array[i];
      }
      lzt::append_memory_copy(cmd_bundle.list, dst_data, dst_data_host_ptr,
                              size * sizeof(copy_data), nullptr);
      lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    } else {
      for (size_t i = 0U; i < size; i++) {
        dst_data[i].data = dst_data_ptr_array[i];
      }
    }

    // set kernel_copy
    lzt::set_argument_value(kernel, 0, sizeof(src_data), &src_data);
    lzt::set_argument_value(kernel, 1, sizeof(dst_data), &dst_data);
    lzt::set_argument_value(kernel, 2, sizeof(int), &offset);
    lzt::set_argument_value(kernel, 3, sizeof(int), &size);

    ze_kernel_indirect_access_flags_t indirect_flags = 0;
    if (src_mem_type == ZE_MEMORY_TYPE_HOST ||
        dst_mem_type == ZE_MEMORY_TYPE_HOST)
      indirect_flags |= ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST;
    if (src_mem_type == ZE_MEMORY_TYPE_DEVICE ||
        dst_mem_type == ZE_MEMORY_TYPE_DEVICE)
      indirect_flags |= ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE;
    if (src_mem_type == ZE_MEMORY_TYPE_SHARED ||
        dst_mem_type == ZE_MEMORY_TYPE_SHARED)
      indirect_flags |= ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED;

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

    lzt::append_launch_function(cmd_bundle.list, kernel, &group_count, nullptr,
                                0, nullptr);

    // copy dst_data to output_data for device memory
    if (dst_mem_type == ZE_MEMORY_TYPE_DEVICE) {
      lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
      for (size_t i = 0U; i < size; i++) {
        lzt::append_memory_copy(cmd_bundle.list, output_data[i].data,
                                dst_data_ptr_array[i], size * sizeof(uint32_t),
                                nullptr);
      }
      lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    }

    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    // copy dst_data to output_data for non-device memory
    if (dst_mem_type != ZE_MEMORY_TYPE_DEVICE) {
      for (size_t i = 0U; i < size; i++) {
        // if dst_data is device mem, dst_data[i].data won't work.
        // so, we are using dst_data_ptr_array[i] instead
        memcpy(output_data[i].data, dst_data_ptr_array[i],
               size * sizeof(uint32_t));
      }
    }

    for (size_t i = 0U; i < size; i++) {
      EXPECT_EQ(0, memcmp(input_data[i].data, output_data[i].data + offset,
                          (size - offset) * sizeof(uint32_t)));
      // break to cleanup
      if (::testing::Test::HasFailure()) {
        break;
      }
    }

    // cleanup
    for (size_t i = 0U; i < size; i++) {
      delete[] input_data[i].data;
      delete[] output_data[i].data;
      free_memory(src_data_ptr_array[i], context, src_mem_type, src_shr_type);
      free_memory(dst_data_ptr_array[i], context, dst_mem_type, dst_shr_type);
    }
    delete[] input_data;
    delete[] output_data;
    if (src_data_host_ptr)
      delete[] src_data_host_ptr;
    if (dst_data_host_ptr)
      delete[] dst_data_host_ptr;
    free_memory(src_data, context, src_ptr_mem_type, src_ptr_shr_type);
    free_memory(dst_data, context, dst_ptr_mem_type, dst_ptr_shr_type);
    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  std::vector<ze_driver_handle_t> drivers;
  std::vector<std::vector<ze_device_handle_t>> devices;
  std::vector<ze_context_handle_t> contexts;
  std::vector<std::vector<ze_device_memory_access_properties_t>>
      memory_access_properties;
};

LZT_TEST_P(
    KernelCopyTestsWithIndirectMemoryTypes,
    GivenInDirectMemoryWithDifferentMemoryTypesWhenCopyingDataInKernelThenCopyIsCorrect) {
  test_memory_type src_test_type = std::get<0>(GetParam());
  test_memory_type src_ptr_test_type = std::get<1>(GetParam());
  test_memory_type dst_test_type = std::get<2>(GetParam());
  test_memory_type dst_ptr_test_type = std::get<3>(GetParam());
  size_t offset = std::get<4>(GetParam());
  bool is_immediate = std::get<5>(GetParam());

  ze_memory_type_t src_mem_type;
  ze_memory_type_t src_ptr_mem_type;
  ze_memory_type_t dst_mem_type;
  ze_memory_type_t dst_ptr_mem_type;
  shared_memory_type src_shr_type;
  shared_memory_type src_ptr_shr_type;
  shared_memory_type dst_shr_type;
  shared_memory_type dst_ptr_shr_type;

  EXPECT_EQ(true, get_memory_info(src_mem_type, src_shr_type, src_test_type));
  EXPECT_EQ(true, get_memory_info(src_ptr_mem_type, src_ptr_shr_type,
                                  src_ptr_test_type));
  EXPECT_EQ(true, get_memory_info(dst_mem_type, dst_shr_type, dst_test_type));
  EXPECT_EQ(true, get_memory_info(dst_ptr_mem_type, dst_ptr_shr_type,
                                  dst_ptr_test_type));

  EXPECT_GE(drivers.size(), 1);
  EXPECT_GE(devices[0].size(), 1);

  test_indirect_memory_kernel_copy(0, 0, src_mem_type, src_ptr_mem_type,
                                   dst_mem_type, dst_ptr_mem_type, offset,
                                   src_shr_type, src_ptr_shr_type, dst_shr_type,
                                   dst_ptr_shr_type, is_immediate);
}

INSTANTIATE_TEST_SUITE_P(
    LZT, KernelCopyTestsWithIndirectMemoryTypes,
    ::testing::Combine(::testing::Values(USM_HOST, USM_DEVICE, USM_SHARED_LOCAL,
                                         USM_SHARED_CROSS, USM_SHARED_SYSTEM),
                       ::testing::Values(USM_HOST, USM_DEVICE, USM_SHARED_LOCAL,
                                         USM_SHARED_CROSS, USM_SHARED_SYSTEM),
                       ::testing::Values(USM_HOST, USM_DEVICE, USM_SHARED_LOCAL,
                                         USM_SHARED_CROSS, USM_SHARED_SYSTEM),
                       ::testing::Values(USM_HOST, USM_DEVICE, USM_SHARED_LOCAL,
                                         USM_SHARED_CROSS, USM_SHARED_SYSTEM),
                       ::testing::Values(0), ::testing::Bool()));

} // namespace
