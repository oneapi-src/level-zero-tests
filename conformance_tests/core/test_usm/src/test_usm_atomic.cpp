/*
 *
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <array>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

const size_t size = 256;
const uint8_t input_pattern = 1;
const uint8_t output_pattern = 2;
const std::vector<ze_memory_type_t> atomic_memory_types = {
    ZE_MEMORY_TYPE_HOST, ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_SHARED};
enum shared_memory_type { SHARED_LOCAL, SHARED_CROSS, SHARED_SYSTEM };
const std::vector<shared_memory_type> shared_memory_types = {SHARED_LOCAL,
                                                             SHARED_SYSTEM};

class KernelAtomicLoadStoreTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<
          ze_memory_type_t, ze_memory_type_t, shared_memory_type, bool>> {

public:
  void launchAtomicLoadStore(ze_device_handle_t device, int *input_data,
                             int *output_data, bool is_immediate) {
    // set up
    auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);

    int *expected_data =
        static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));
    int *verify_data =
        static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));
    lzt::write_data_pattern(expected_data, size * sizeof(int), input_pattern);

    EXPECT_NE(expected_data, nullptr);
    EXPECT_NE(verify_data, nullptr);

    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(input_data),
                            static_cast<const void *>(&input_pattern),
                            sizeof(uint8_t), size * sizeof(int), nullptr);
    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(output_data),
                            static_cast<const void *>(&output_pattern),
                            sizeof(uint8_t), size * sizeof(int), nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

    auto module = lzt::create_module(device, "test_usm_atomic.spv");
    auto kernel = lzt::create_function(module, "atomic_copy_kernel");

    lzt::set_argument_value(kernel, 0, sizeof(input_data), &input_data);
    lzt::set_argument_value(kernel, 1, sizeof(output_data), &output_data);

    lzt::set_group_size(kernel, size, 1, 1);

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::append_launch_function(cmd_bundle.list, kernel, &group_count, nullptr,
                                0, nullptr);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

    lzt::append_memory_copy(cmd_bundle.list, expected_data, input_data,
                            size * sizeof(int));
    lzt::append_memory_copy(cmd_bundle.list, static_cast<void *>(verify_data),
                            static_cast<void *>(output_data),
                            size * sizeof(int), nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(0, memcmp(verify_data, expected_data, size * sizeof(int)));

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_bundle(cmd_bundle);
    lzt::free_memory(expected_data);
    lzt::free_memory(verify_data);
  }
};

LZT_TEST_P(
    KernelAtomicLoadStoreTests,
    GivenDirectMemoryWhenCopyingDataUsingAtomicsInKernelThenCopyIsCorrect) {
  ze_memory_type_t input_memory_type = std::get<0>(GetParam());
  ze_memory_type_t output_memory_type = std::get<1>(GetParam());
  shared_memory_type shared_mem = std::get<2>(GetParam());
  bool is_immediate = std::get<3>(GetParam());
  auto context = lzt::get_default_context();

  for (auto driver : lzt::get_all_driver_handles()) {
    unsigned device_id = 0;
    for (auto device : lzt::get_devices(driver)) {
      // get memory access properties
      auto memory_access_properties = lzt::get_memory_access_properties(device);
      // check atomic access is enabled
      ze_memory_access_cap_flags_t input_access_cap, output_access_cap;
      if (input_memory_type == ZE_MEMORY_TYPE_HOST) {
        input_access_cap = memory_access_properties.hostAllocCapabilities;
      } else if (input_memory_type == ZE_MEMORY_TYPE_DEVICE) {
        input_access_cap = memory_access_properties.deviceAllocCapabilities;
      } else {
        if (shared_mem == SHARED_LOCAL) {
          input_access_cap =
              memory_access_properties.sharedSingleDeviceAllocCapabilities;
        } else {
          input_access_cap =
              memory_access_properties.sharedSystemAllocCapabilities;
        }
      }

      if (output_memory_type == ZE_MEMORY_TYPE_HOST) {
        output_access_cap = memory_access_properties.hostAllocCapabilities;
      } else if (output_memory_type == ZE_MEMORY_TYPE_DEVICE) {
        output_access_cap = memory_access_properties.deviceAllocCapabilities;
      } else {
        if (shared_mem == SHARED_LOCAL) {
          output_access_cap =
              memory_access_properties.sharedSingleDeviceAllocCapabilities;
        } else {
          output_access_cap =
              memory_access_properties.sharedSystemAllocCapabilities;
        }
      }

      bool skip = false;
      auto devices = lzt::get_devices(driver);
      if ((input_access_cap & ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC) == 0) {
        LOG_INFO << "WARNING: Skipping as input memory atomic access for "
                    "device "
                 << device_id << " is not enabled";
        skip = true;
      }
      if ((output_access_cap & ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC) == 0) {
        LOG_INFO << "WARNING: Skipping as output memory atomic access for "
                    "device "
                 << device_id << " is not enabled";
        skip = true;
      }
      device_id++;
      if (skip) {
        continue;
      }

      int *input_data, *output_data;
      if (input_memory_type == ZE_MEMORY_TYPE_HOST) {
        input_data = static_cast<int *>(
            lzt::allocate_host_memory(size * sizeof(int), 1, context));
      } else if (input_memory_type == ZE_MEMORY_TYPE_DEVICE) {
        input_data = static_cast<int *>(lzt::allocate_device_memory(
            size * sizeof(int), 1, 0u, device, context));
      } else {
        if (shared_mem == SHARED_LOCAL) {
          input_data = static_cast<int *>(lzt::allocate_shared_memory(
              size * sizeof(int), 1, 0, 0, device, context));
        } else {
          input_data = new int[size];
        }
      }

      if (output_memory_type == ZE_MEMORY_TYPE_HOST) {
        output_data = static_cast<int *>(
            lzt::allocate_host_memory(size * sizeof(int), 1, context));
      } else if (output_memory_type == ZE_MEMORY_TYPE_DEVICE) {
        output_data = static_cast<int *>(lzt::allocate_device_memory(
            size * sizeof(int), 1, 0u, device, context));
      } else {
        if (shared_mem == SHARED_LOCAL) {
          output_data = static_cast<int *>(lzt::allocate_shared_memory(
              size * sizeof(int), 1, 0, 0, device, context));
        } else {
          output_data = new int[size];
        }
      }

      EXPECT_NE(input_data, nullptr);
      EXPECT_NE(output_data, nullptr);

      launchAtomicLoadStore(device, input_data, output_data, is_immediate);

      // cleanup
      if (shared_mem == SHARED_SYSTEM) {
        if (input_memory_type == ZE_MEMORY_TYPE_SHARED) {
          delete[] input_data;
          input_data = nullptr;
        }
        if (output_memory_type == ZE_MEMORY_TYPE_SHARED) {
          delete[] output_data;
          output_data = nullptr;
        }
      }
      if (input_data)
        lzt::free_memory(context, input_data);
      if (output_data)
        lzt::free_memory(context, output_data);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    LZT, KernelAtomicLoadStoreTests,
    ::testing::Combine(::testing::ValuesIn(atomic_memory_types),
                       ::testing::ValuesIn(atomic_memory_types),
                       ::testing::ValuesIn(shared_memory_types),
                       ::testing::Bool()));

class AtomicAccessTests : public ::testing::Test {};

LZT_TEST_F(AtomicAccessTests, SetAndGetAccessTypeForSharedAllocation) {
  auto context = lzt::get_default_context();
  auto device = lzt::get_default_device(lzt::get_default_driver());

  auto alloc_data = static_cast<int *>(lzt::allocate_shared_memory(
      size * sizeof(int), 1, 0, 0, device, context));

  auto memory_access_properties = lzt::get_memory_access_properties(device);
  ze_memory_atomic_attr_exp_flags_t access_type;
  ze_result_t result;

  EXPECT_ZE_RESULT_SUCCESS(zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_ATOMICS));

  EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
      context, device, alloc_data, size, &access_type));
  EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_ATOMICS, access_type);

  EXPECT_ZE_RESULT_SUCCESS(zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_HOST_ATOMICS));

  EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
      context, device, alloc_data, size, &access_type));
  EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_HOST_ATOMICS, access_type);

  result = zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_HOST_ATOMICS);

  if (!(memory_access_properties.hostAllocCapabilities &
        ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC)) {
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, result);
  } else {
    EXPECT_ZE_RESULT_SUCCESS(result);
    EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
        context, device, alloc_data, size, &access_type));
    EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_HOST_ATOMICS, access_type);
  }

  EXPECT_ZE_RESULT_SUCCESS(zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_DEVICE_ATOMICS));

  EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
      context, device, alloc_data, size, &access_type));
  EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_DEVICE_ATOMICS, access_type);

  result = zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_DEVICE_ATOMICS);

  if (!(memory_access_properties.deviceAllocCapabilities &
        ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC)) {
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, result);
  } else {
    EXPECT_ZE_RESULT_SUCCESS(result);
    EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
        context, device, alloc_data, size, &access_type));
    EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_DEVICE_ATOMICS, access_type);
  }

  EXPECT_ZE_RESULT_SUCCESS(zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_SYSTEM_ATOMICS));

  EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
      context, device, alloc_data, size, &access_type));
  EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_NO_SYSTEM_ATOMICS, access_type);

  result = zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_SYSTEM_ATOMICS);

  if (!(memory_access_properties.sharedSingleDeviceAllocCapabilities &
        ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC)) {
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, result);
  } else {
    EXPECT_ZE_RESULT_SUCCESS(result);
    EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
        context, device, alloc_data, size, &access_type));
    EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_SYSTEM_ATOMICS, access_type);
  }
  lzt::free_memory(context, alloc_data);
}

static void RunSetAndGetPositiveAccessTypeTest(bool is_shared_system) {
  auto context = lzt::get_default_context();
  auto device = lzt::get_default_device(lzt::get_default_driver());

  auto alloc_data =
      static_cast<int *>(lzt::allocate_shared_memory_with_allocator_selector(
          size * sizeof(int), is_shared_system));

  auto memory_access_properties = lzt::get_memory_access_properties(device);
  ze_memory_atomic_attr_exp_flags_t access_type;
  ze_result_t result;

  result = zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_HOST_ATOMICS);

  if (!(memory_access_properties.hostAllocCapabilities &
        ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC)) {
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, result);
  } else {
    EXPECT_ZE_RESULT_SUCCESS(result);
    EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
        context, device, alloc_data, size, &access_type));
    EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_HOST_ATOMICS, access_type);
  }

  result = zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_DEVICE_ATOMICS);

  if (!(memory_access_properties.deviceAllocCapabilities &
        ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC)) {
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, result);
  } else {
    EXPECT_ZE_RESULT_SUCCESS(result);
    EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
        context, device, alloc_data, size, &access_type));
    EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_DEVICE_ATOMICS, access_type);
  }

  result = zeMemSetAtomicAccessAttributeExp(
      context, device, alloc_data, size,
      ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_SYSTEM_ATOMICS);

  auto capabilities =
      is_shared_system
          ? memory_access_properties.sharedSystemAllocCapabilities
          : memory_access_properties.sharedSingleDeviceAllocCapabilities;

  if (!(capabilities & ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC)) {
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, result);
  } else {
    EXPECT_ZE_RESULT_SUCCESS(result);
    EXPECT_ZE_RESULT_SUCCESS(zeMemGetAtomicAccessAttributeExp(
        context, device, alloc_data, size, &access_type));
    EXPECT_EQ(ZE_MEMORY_ATOMIC_ATTR_EXP_FLAG_SYSTEM_ATOMICS, access_type);
  }
  lzt::free_memory_with_allocator_selector(alloc_data, is_shared_system);
}

LZT_TEST_F(AtomicAccessTests,
           SetAndGetPositiveAccessTypeWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunSetAndGetPositiveAccessTypeTest(true);
}

} // namespace
