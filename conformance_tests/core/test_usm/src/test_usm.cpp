/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "random/random.hpp"

namespace lzt = level_zero_tests;
#include <level_zero/ze_api.h>

namespace {

using lzt::to_u32;

class zeDriverMemoryMigrationPageFaultTestsMultiDevice
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<uint32_t, uint32_t, bool, bool>> {
public:
  void run_functions(const ze_device_handle_t device, ze_module_handle_t module,
                     void *pattern_memory, uint64_t pattern_memory_count,
                     uint16_t sub_pattern,
                     uint64_t *host_expected_output_buffer,
                     uint64_t *gpu_expected_output_buffer,
                     uint64_t *host_found_output_buffer,
                     uint64_t *gpu_found_output_buffer, uint32_t output_count,
                     ze_context_handle_t context, bool is_immediate) {
    ze_kernel_flags_t flag = 0;
    /* Prepare the fill function */
    ze_kernel_handle_t fill_function =
        lzt::create_function(module, flag, "fill_device_memory");

    // set the thread group size to make sure all the device threads are
    // occupied
    uint32_t groupSizeX, groupSizeY, groupSizeZ;
    size_t groupSize;
    lzt::suggest_group_size(fill_function, to_u32(pattern_memory_count), 1, 1,
                            groupSizeX, groupSizeY, groupSizeZ);

    groupSize = groupSizeX * groupSizeY * groupSizeZ;
    LOG_DEBUG << "thread group size X is ::" << groupSizeX;
    LOG_DEBUG << "thread group size Y is ::" << groupSizeY;
    LOG_DEBUG << "thread group size Z is ::" << groupSizeZ;
    LOG_DEBUG << "thread group size is ::" << groupSize;
    lzt::set_group_size(fill_function, groupSizeX, groupSizeY, groupSizeZ);

    lzt::set_argument_value(fill_function, 0, sizeof(pattern_memory),
                            &pattern_memory);

    lzt::set_argument_value(fill_function, 1, sizeof(sub_pattern),
                            &sub_pattern);

    /* Prepare the test function */
    ze_kernel_handle_t test_function =
        lzt::create_function(module, flag, "test_device_memory");
    lzt::set_group_size(test_function, groupSizeX, groupSizeY, groupSizeZ);

    lzt::set_argument_value(test_function, 0, sizeof(pattern_memory),
                            &pattern_memory);

    lzt::set_argument_value(test_function, 1, sizeof(sub_pattern),
                            &sub_pattern);

    lzt::set_argument_value(test_function, 2,
                            sizeof(gpu_expected_output_buffer),
                            &gpu_expected_output_buffer);

    lzt::set_argument_value(test_function, 3, sizeof(gpu_found_output_buffer),
                            &gpu_found_output_buffer);

    lzt::set_argument_value(test_function, 4, sizeof(output_count),
                            &output_count);

    auto cmd_bundle = lzt::create_command_bundle(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

    // if groupSize is greater then memory count, then at least one thread group
    // should be dispatched
    uint32_t threadGroup = pattern_memory_count / groupSize > 1
                               ? to_u32(pattern_memory_count / groupSize)
                               : 1;
    LOG_DEBUG << "thread group dimension is ::" << threadGroup;
    ze_group_count_t thread_group_dimensions = {threadGroup, 1, 1};

    lzt::append_memory_copy(cmd_bundle.list, gpu_expected_output_buffer,
                            host_expected_output_buffer,
                            output_count * sizeof(uint64_t), nullptr);
    lzt::append_memory_copy(cmd_bundle.list, gpu_found_output_buffer,
                            host_found_output_buffer,
                            output_count * sizeof(uint64_t), nullptr);

    // Access to pattern buffer from device using the compute kernel to fill
    // data.
    lzt::append_launch_function(cmd_bundle.list, fill_function,
                                &thread_group_dimensions, nullptr, 0, nullptr);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

    // Access to pattern buffer from device using the compute kernel to test
    // data.
    lzt::append_launch_function(cmd_bundle.list, test_function,
                                &thread_group_dimensions, nullptr, 0, nullptr);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

    lzt::append_memory_copy(cmd_bundle.list, host_expected_output_buffer,
                            gpu_expected_output_buffer,
                            output_count * sizeof(uint64_t), nullptr);

    lzt::append_memory_copy(cmd_bundle.list, host_found_output_buffer,
                            gpu_found_output_buffer,
                            output_count * sizeof(uint64_t), nullptr);

    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    lzt::destroy_command_bundle(cmd_bundle);
    lzt::destroy_function(fill_function);
    lzt::destroy_function(test_function);
  }

  typedef struct MemoryTestArguments {
    /* The index into the driver array to test */
    uint32_t driver_index;
    /*
     * The total size on the device is divided by this value
     */
    uint32_t memory_size_factor;
    /* The bool value which decides p2p access */
    bool p2p_access_enabled;
    bool is_immediate;
  } MemoryTestArguments_t;

  void memoryMigrationPageFaultTests(MemoryTestArguments_t test_arguments) {

    uint32_t driver_index = test_arguments.driver_index;
    uint32_t memory_size_factor = test_arguments.memory_size_factor;
    bool p2p_access_enabled = test_arguments.p2p_access_enabled;

    LOG_DEBUG << "TEST ARGUMENTS "
              << "driver_index " << driver_index << " memory_size_factor "
              << memory_size_factor << " p2p_access_enabled "
              << p2p_access_enabled;

    uint32_t driver_count = 0;

    level_zero_tests::driverInfo_t *driver_info =
        level_zero_tests::collect_driver_info(driver_count);

    ze_driver_handle_t driver_handle = driver_info->driver_handle;
    ze_context_handle_t context = lzt::create_context(driver_handle);

    if (p2p_access_enabled && (driver_info->number_device_handles < 2)) {
      LOG_DEBUG
          << "Only one device is active, so just return without performing "
             "peer2peer access";
      level_zero_tests::free_driver_info(driver_info, driver_count);
      GTEST_SKIP();
    }
    // For each combination of memory, iterate through all the valid devices

    for (uint32_t i = 0U; i < driver_info->number_device_handles; i++) {
      device_in_driver_index = i;

      ze_device_handle_t device_handle =
          driver_info->device_handles[device_in_driver_index];

      size_t totalSize =
          driver_info->device_memory_properties[device_in_driver_index]
              .totalSize;

      LOG_DEBUG << " Total size on device " << i << " is ::" << totalSize;

      size_t pattern_memory_size = totalSize / memory_size_factor;
      uint64_t pattern_memory_count =
          pattern_memory_size >> 3; // array of uint64_t

      // create pattern buffer on which device  will perform writes using the
      // compute kernel, create found and expected/gold buffers which are used
      // to validate the data written by device

      uint64_t *gpu_pattern_buffer;
      gpu_pattern_buffer = (uint64_t *)level_zero_tests::allocate_shared_memory(
          pattern_memory_size, 8, 0, 0, device_handle, context);

      uint64_t *gpu_expected_output_buffer;
      gpu_expected_output_buffer =
          (uint64_t *)level_zero_tests::allocate_device_memory(
              output_size_, 8, 0, use_this_ordinal_on_device_, device_handle,
              context);

      uint64_t *gpu_found_output_buffer;
      gpu_found_output_buffer =
          (uint64_t *)level_zero_tests::allocate_device_memory(
              output_size_, 8, 0, use_this_ordinal_on_device_, device_handle,
              context);

      uint64_t *host_expected_output_buffer = new uint64_t[output_count_];
      std::fill(host_expected_output_buffer,
                host_expected_output_buffer + output_count_, 0);

      uint64_t *host_found_output_buffer = new uint64_t[output_size_];
      std::fill(host_found_output_buffer,
                host_found_output_buffer + output_count_, 0);

      uint16_t pattern_base = lzt::generate_value<uint16_t>();
      uint16_t pattern_base_1 = lzt::generate_value<uint16_t>();

      LOG_DEBUG << "PREPARE TO RUN START for device " << i;
      LOG_DEBUG << "totalSize " << totalSize;
      LOG_DEBUG << "gpu_pattern_buffer " << gpu_pattern_buffer;
      LOG_DEBUG << "pattern_memory_count " << pattern_memory_count;
      LOG_DEBUG << "pattern_memory_size " << pattern_memory_size;
      LOG_DEBUG << "pattern_base " << pattern_base;
      LOG_DEBUG << "gpu_expected_output_buffer " << gpu_expected_output_buffer;
      LOG_DEBUG << "host_expected_output_buffer "
                << host_expected_output_buffer;
      LOG_DEBUG << "gpu_found_output_buffer " << gpu_found_output_buffer;
      LOG_DEBUG << "host_found_output_buffer " << host_found_output_buffer;
      LOG_DEBUG << "output count " << output_count_;
      LOG_DEBUG << "output size " << output_size_;
      LOG_DEBUG << "PREPARE TO RUN END for device";

      LOG_DEBUG << "call create module for device " << i;
      ze_module_handle_t module_handle = lzt::create_module(
          context, device_handle, "test_fill_device_memory_usm.spv",
          ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);

      LOG_DEBUG << "host access pattern buffer created on device " << i;
      // Access to pattern buffer from host.
      std::fill(gpu_pattern_buffer, gpu_pattern_buffer + pattern_memory_count,
                0x0);

      LOG_DEBUG << "call run_functions for device " << i;
      run_functions(device_handle, module_handle, gpu_pattern_buffer,
                    pattern_memory_count, pattern_base,
                    host_expected_output_buffer, gpu_expected_output_buffer,
                    host_found_output_buffer, gpu_found_output_buffer,
                    output_count_, context, test_arguments.is_immediate);

      LOG_DEBUG << "check output buffer copied from device";
      bool memory_test_failure = false;
      for (uint32_t i = 0; i < output_count_; i++) {
        if (host_expected_output_buffer[i] || host_found_output_buffer[i]) {
          LOG_DEBUG << "Index of difference " << i << " found "
                    << host_found_output_buffer[i] << " expected "
                    << host_expected_output_buffer[i];
          memory_test_failure = true;
          break;
        }
      }

      EXPECT_EQ(false, memory_test_failure);

      // Logic to assign the peer which will access the shared memory
      int index =
          (device_in_driver_index + 1) % driver_info->number_device_handles;
      ze_device_handle_t device_handle_1 = driver_info->device_handles[index];

      // check if peer can access the shared memory

      ze_bool_t can_access = false;
      EXPECT_ZE_RESULT_SUCCESS(

          zeDeviceCanAccessPeer(device_handle, device_handle_1, &can_access));

      // Before accessing make sure the peer's device size
      // can fit in shared memory buffer
      size_t totalSize_1 =
          driver_info->device_memory_properties[index].totalSize;

      can_access &= (totalSize_1 >= pattern_memory_size);

      if (p2p_access_enabled && can_access) {

        LOG_DEBUG << "Accessing shared memory on device :" << i
                  << " from peer :" << index;

        LOG_DEBUG << "call create module for device :" << index;
        ze_module_handle_t module_handle_1 = lzt::create_module(
            context, device_handle_1, "test_fill_device_memory_usm.spv",
            ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);

        uint64_t *gpu_expected_output_buffer_1;
        gpu_expected_output_buffer_1 =
            (uint64_t *)level_zero_tests::allocate_device_memory(
                output_size_, 8, 0, use_this_ordinal_on_device_,
                device_handle_1, context);

        uint64_t *gpu_found_output_buffer_1;
        gpu_found_output_buffer_1 =
            (uint64_t *)level_zero_tests::allocate_device_memory(
                output_size_, 8, 0, use_this_ordinal_on_device_,
                device_handle_1, context);

        uint64_t *host_expected_output_buffer_1 = new uint64_t[output_count_];
        std::fill(host_expected_output_buffer_1,
                  host_expected_output_buffer_1 + output_count_, 0);

        uint64_t *host_found_output_buffer_1 = new uint64_t[output_size_];
        std::fill(host_found_output_buffer_1,
                  host_found_output_buffer_1 + output_count_, 0);

        LOG_DEBUG << "call run_functions for device :" << index;
        run_functions(device_handle_1, module_handle_1, gpu_pattern_buffer,
                      pattern_memory_count, pattern_base_1,
                      host_expected_output_buffer_1,
                      gpu_expected_output_buffer_1, host_found_output_buffer_1,
                      gpu_found_output_buffer_1, output_count_, context,
                      test_arguments.is_immediate);

        LOG_DEBUG << "End of p2p access by device :" << index;

        LOG_DEBUG << "check output buffer copied from device :" << index;
        bool memory_test_failure = false;
        for (uint32_t i = 0; i < output_count_; i++) {
          if (host_expected_output_buffer_1[i] ||
              host_found_output_buffer_1[i]) {
            LOG_DEBUG << "Index of difference " << i << " found "
                      << host_found_output_buffer_1[i] << " expected "
                      << host_expected_output_buffer_1[i];
            memory_test_failure = true;
            break;
          }
        }

        EXPECT_EQ(false, memory_test_failure);

        LOG_DEBUG << "host access pattern_buffer touched by device :" << index;
        // Access to pattern buffer from host
        bool host_test_failure = false;
        for (uint32_t i = 0; i < pattern_memory_count; i++) {
          if (gpu_pattern_buffer[i] !=
              ((i << (sizeof(uint16_t) * 8)) + pattern_base_1)) {
            LOG_DEBUG << "Index of difference " << i << " found "
                      << gpu_pattern_buffer[i] << " expected "
                      << ((i << (sizeof(uint16_t) * 8)) + pattern_base_1);
            host_test_failure = true;
            break;
          }
        }

        EXPECT_EQ(false, host_test_failure);

        LOG_DEBUG << "call free memory for device :" << index;
        level_zero_tests::free_memory(context, gpu_expected_output_buffer_1);
        level_zero_tests::free_memory(context, gpu_found_output_buffer_1);

        LOG_DEBUG << "call destroy module for device :" << index;
        EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(module_handle_1));

        delete[] host_expected_output_buffer_1;
        delete[] host_found_output_buffer_1;
      } else {
        LOG_DEBUG
            << "Skipping P2P flow, cannot access shared memory on device :" << i
            << " from peer :" << index;
      }

      LOG_DEBUG << "host touches pattern_buffer with new pattern";
      // host will touch the pattern buffer,
      uint16_t pattern_base_2 = lzt::generate_value<uint16_t>();
      LOG_DEBUG << "pattern_base " << pattern_base_1;
      for (uint32_t i = 0; i < pattern_memory_count; i++)
        gpu_pattern_buffer[i] = (i << (sizeof(uint16_t) * 8)) + pattern_base_2;

      bool host_test_failure = false;
      for (uint32_t i = 0; i < pattern_memory_count; i++) {
        if (gpu_pattern_buffer[i] !=
            ((i << (sizeof(uint16_t) * 8)) + pattern_base_2)) {
          LOG_DEBUG << "Index of difference " << i << " found "
                    << gpu_pattern_buffer[i] << " expected "
                    << ((i << (sizeof(uint16_t) * 8)) + pattern_base_2);
          host_test_failure = true;
          break;
        }
      }

      EXPECT_EQ(false, host_test_failure);

      LOG_DEBUG << "call free memory for device";
      level_zero_tests::free_memory(context, gpu_pattern_buffer);
      level_zero_tests::free_memory(context, gpu_expected_output_buffer);
      level_zero_tests::free_memory(context, gpu_found_output_buffer);
      LOG_DEBUG << "call destroy module for device";
      EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(module_handle));
      delete[] host_expected_output_buffer;
      delete[] host_found_output_buffer;
    }
    level_zero_tests::free_driver_info(driver_info, driver_count);
    level_zero_tests::destroy_context(context);
  }

  void memoryMigrationAccessTest(MemoryTestArguments_t test_arguments) {
    uint32_t driver_index = test_arguments.driver_index;
    uint32_t memory_size_factor = test_arguments.memory_size_factor;
    bool p2p_access_enabled = test_arguments.p2p_access_enabled;

    LOG_DEBUG << "TEST ARGUMENTS "
              << "driver_index " << driver_index << " memory_size_factor "
              << memory_size_factor << " p2p_access_enabled "
              << p2p_access_enabled;

    uint32_t driver_count = 0;

    level_zero_tests::driverInfo_t *driver_info =
        level_zero_tests::collect_driver_info(driver_count);

    ze_driver_handle_t driver_handle = driver_info->driver_handle;
    ze_context_handle_t context = lzt::create_context(driver_handle);

    // For each combination of memory, iterate through all the valid devices

    for (uint32_t i = 0U; i < driver_info->number_device_handles; i++) {
      device_in_driver_index = i;

      ze_device_handle_t device_handle =
          driver_info->device_handles[device_in_driver_index];

      size_t totalSize =
          driver_info->device_memory_properties[device_in_driver_index]
              .totalSize;

      LOG_DEBUG << " Total size on device " << i << " is ::" << totalSize;

      size_t pattern_memory_size = totalSize / memory_size_factor;
      uint64_t pattern_memory_count =
          pattern_memory_size >> 3; // array of uint64_t

      // create pattern buffer on which device  will perform writes using the
      // compute kernel, create found and expected/gold buffers which are used
      // to validate the data written by device

      uint64_t *gpu_pattern_buffer;
      gpu_pattern_buffer = (uint64_t *)level_zero_tests::allocate_shared_memory(
          pattern_memory_size, 8, 0, 0, device_handle, context);

      uint64_t *gpu_expected_output_buffer;
      gpu_expected_output_buffer =
          (uint64_t *)level_zero_tests::allocate_device_memory(
              output_size_, 8, 0, use_this_ordinal_on_device_, device_handle,
              context);

      uint64_t *gpu_found_output_buffer;
      gpu_found_output_buffer =
          (uint64_t *)level_zero_tests::allocate_device_memory(
              output_size_, 8, 0, use_this_ordinal_on_device_, device_handle,
              context);

      uint64_t *host_expected_output_buffer = new uint64_t[output_count_];
      std::fill(host_expected_output_buffer,
                host_expected_output_buffer + output_count_, 0);

      uint64_t *host_found_output_buffer = new uint64_t[output_size_];
      std::fill(host_found_output_buffer,
                host_found_output_buffer + output_count_, 0);

      uint16_t pattern_base = lzt::generate_value<uint16_t>();
      uint16_t pattern_base_1 = lzt::generate_value<uint16_t>();

      LOG_DEBUG << "PREPARE TO RUN START for device " << i;
      LOG_DEBUG << "totalSize " << totalSize;
      LOG_DEBUG << "gpu_pattern_buffer " << gpu_pattern_buffer;
      LOG_DEBUG << "pattern_memory_count " << pattern_memory_count;
      LOG_DEBUG << "pattern_memory_size " << pattern_memory_size;
      LOG_DEBUG << "pattern_base " << pattern_base;
      LOG_DEBUG << "gpu_expected_output_buffer " << gpu_expected_output_buffer;
      LOG_DEBUG << "host_expected_output_buffer "
                << host_expected_output_buffer;
      LOG_DEBUG << "gpu_found_output_buffer " << gpu_found_output_buffer;
      LOG_DEBUG << "host_found_output_buffer " << host_found_output_buffer;
      LOG_DEBUG << "output count " << output_count_;
      LOG_DEBUG << "output size " << output_size_;
      LOG_DEBUG << "PREPARE TO RUN END for device";

      LOG_DEBUG << "call create module for device " << i;
      ze_module_handle_t module_handle = lzt::create_module(
          context, device_handle, "test_fill_device_memory_usm.spv",
          ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);

      LOG_DEBUG << "host access pattern buffer created on device " << i;
      // Access to pattern buffer from host.
      std::fill(gpu_pattern_buffer, gpu_pattern_buffer + pattern_memory_count,
                0x0);

      LOG_DEBUG << "call run_functions for device " << i;
      run_functions(device_handle, module_handle, gpu_pattern_buffer,
                    pattern_memory_count, pattern_base,
                    host_expected_output_buffer, gpu_expected_output_buffer,
                    host_found_output_buffer, gpu_found_output_buffer,
                    output_count_, context, test_arguments.is_immediate);

      LOG_DEBUG << "check output buffer copied from device";
      bool memory_test_failure = false;
      for (uint32_t i = 0; i < output_count_; i++) {
        if (host_expected_output_buffer[i] || host_found_output_buffer[i]) {
          LOG_DEBUG << "Index of difference " << i << " found "
                    << host_found_output_buffer[i] << " expected "
                    << host_expected_output_buffer[i];
          memory_test_failure = true;
          break;
        }
      }

      EXPECT_EQ(false, memory_test_failure);

      LOG_DEBUG << "host access pattern_buffer touched by device " << i;
      // Access to pattern buffer from host
      bool host_test_failure = false;
      for (uint32_t i = 0; i < pattern_memory_count; i++) {
        if (gpu_pattern_buffer[i] !=
            ((i << (sizeof(uint16_t) * 8)) + pattern_base)) {
          LOG_DEBUG << "Index of difference " << i << " found "
                    << gpu_pattern_buffer[i] << " expected "
                    << ((i << (sizeof(uint16_t) * 8)) + pattern_base);
          host_test_failure = true;
          break;
        }
      }

      EXPECT_EQ(false, host_test_failure);

      LOG_DEBUG << "call free memory for device";
      level_zero_tests::free_memory(context, gpu_pattern_buffer);
      level_zero_tests::free_memory(context, gpu_expected_output_buffer);
      level_zero_tests::free_memory(context, gpu_found_output_buffer);
      LOG_DEBUG << "call destroy module for device";
      EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(module_handle));
      delete[] host_expected_output_buffer;
      delete[] host_found_output_buffer;
    }
    level_zero_tests::free_driver_info(driver_info, driver_count);
    level_zero_tests::destroy_context(context);
  }

  uint32_t device_in_driver_index;
  uint32_t use_this_ordinal_on_device_ = 0;
  uint32_t output_count_ = 64;
  size_t output_size_ = output_count_ * sizeof(uint64_t);
};

class zeDriverMemoryMigrationPageFaultTestsSingleDevice
    : public ::zeDriverMemoryMigrationPageFaultTestsMultiDevice {};

LZT_TEST_P(
    zeDriverMemoryMigrationPageFaultTestsSingleDevice,
    GivenSingleDeviceWhenMemoryAccessedFromHostAndDeviceDataIsValidAndSuccessful) {

  MemoryTestArguments_t test_arguments = {
      std::get<0>(GetParam()), // driver index
      std::get<1>(GetParam()), // memory size factor, rounded up to uint16_t
      std::get<2>(GetParam())  // p2p access flag
  };
  memoryMigrationAccessTest(test_arguments);
}

LZT_TEST_P(
    zeDriverMemoryMigrationPageFaultTestsMultiDevice,
    GivenMultipleDevicesWhenMemoryAccessedFromHostAndDeviceAndPeerToPeerDataIsValidAndSuccessful) {

  MemoryTestArguments_t test_arguments = {
      std::get<0>(GetParam()), // driver index
      std::get<1>(GetParam()), // memory size factor, rounded up to uint16_t
      std::get<2>(GetParam()), // p2p access flag
      std::get<3>(GetParam())  // immediate cmdlist flag
  };
  memoryMigrationPageFaultTests(test_arguments);
}

LZT_TEST_P(
    zeDriverMemoryMigrationPageFaultTestsSingleDevice,
    GivenSingleDeviceWhenMemoryAccessedFromHostAndDeviceAndPeerToPeerDataIsValidAndSuccessful) {

  MemoryTestArguments_t test_arguments = {
      std::get<0>(GetParam()), // driver index
      std::get<1>(GetParam()), // memory size factor, rounded up to uint16_t
      std::get<2>(GetParam()), // p2p access flag
      std::get<3>(GetParam())  // immediate cmdlist flag
  };
  memoryMigrationPageFaultTests(test_arguments);
}

INSTANTIATE_TEST_SUITE_P(TestAllInputPermuntationsForSingleDevice,
                         zeDriverMemoryMigrationPageFaultTestsSingleDevice,
                         ::testing::Combine(::testing::Values(0),
                                            ::testing::Values(1024 * 1024 * 64,
                                                              1024 * 1024,
                                                              1024 * 64),
                                            ::testing::Values(0),
                                            ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(TestAllInputPermuntationsForMultiDevice,
                         zeDriverMemoryMigrationPageFaultTestsMultiDevice,
                         ::testing::Combine(::testing::Values(0),
                                            ::testing::Values(1024 * 1024 * 64,
                                                              1024 * 1024,
                                                              1024 * 64),
                                            ::testing::Values(1),
                                            ::testing::Bool()));

class zeConcurrentAccessToMemoryTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<size_t, bool>> {};
LZT_TEST_P(
    zeConcurrentAccessToMemoryTests,
    GivenSharedMemoryDividedIntoTwoChunksWhenBothHostAndDeviceAccessAChunkConcurrentlyThenSuccessIsReturned) {

  ze_device_handle_t device =
      lzt::get_default_device(lzt::get_default_driver());

  auto device_mem_access_props = lzt::get_memory_access_properties(device);
  if ((device_mem_access_props.sharedSingleDeviceAllocCapabilities &
       ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT) == 0) {
    LOG_WARNING << "Concurrent access for shared allocations unsupported by "
                   "device, skipping test";
    GTEST_SKIP();
  }

  size_t size_shared_memory = std::get<0>(GetParam());
  size_t size_of_chunk = size_shared_memory / 2;
  bool is_immediate = std::get<1>(GetParam());
  ze_context_handle_t context = lzt::create_context();
  auto memory_shared =
      lzt::allocate_shared_memory(size_shared_memory, 1, 0, 0, device, context);

  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  uint8_t *host_mem = static_cast<uint8_t *>(memory_shared);
  uint8_t *device_mem = static_cast<uint8_t *>(memory_shared) + size_of_chunk;
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);
  ;

  lzt::append_memory_fill(cmd_bundle.list, static_cast<uint8_t *>(device_mem),
                          &pattern, pattern_size, size_of_chunk, nullptr);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }
  memset(host_mem, 0x0, size_of_chunk);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  for (uint32_t i = 0; i < size_of_chunk; i++) {
    ASSERT_EQ((device_mem)[i], pattern);
    ASSERT_EQ(host_mem[i], 0x0);
  }
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory(context, memory_shared);
  lzt::destroy_context(context);
}

INSTANTIATE_TEST_SUITE_P(
    VaryMemorySize, zeConcurrentAccessToMemoryTests,
    ::testing::Combine(::testing::Values(8 * 1024, 16 * 1024, 32 * 1024,
                                         64 * 1024, 128 * 1024),
                       ::testing::Bool()));

static void
test_multi_device_shared_memory(std::vector<ze_device_handle_t> devices,
                                bool is_immediate) {

  devices.erase(std::remove_if(
                    devices.begin(), devices.end(),
                    [](ze_device_handle_t device) {
                      auto device_props =
                          lzt::get_memory_access_properties(device);
                      return !(device_props.sharedCrossDeviceAllocCapabilities &
                               ZE_MEMORY_ACCESS_CAP_FLAG_RW);
                    }),
                devices.end());

  for (size_t i = 0U; i < devices.size(); i++) {
    if (!lzt::can_access_peer(devices[0], devices[i])) {
      LOG_WARNING << "P2P Access not supported between devices, skipping test";
      GTEST_SKIP();
    }
  }

  if (devices.size() < 2) {
    LOG_WARNING << "Less than two devices, skipping test";
    GTEST_SKIP();
  }

  const size_t memory_size = 1024;
  auto memory = lzt::allocate_shared_memory(memory_size, 1, 0, 0, devices[0]);

  const int pattern_size = 1;
  uint8_t pattern = 0x01;

  for (size_t i = 0U; i < devices.size(); i++) {
    auto cmd_bundle = lzt::create_command_bundle(devices[i], is_immediate);
    lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size,
                            memory_size, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
    lzt::destroy_command_bundle(cmd_bundle);

    for (size_t j = 0U; j < memory_size; j++) {
      ASSERT_EQ(static_cast<uint8_t *>(memory)[j], pattern);
    }
    pattern++;
  }

  lzt::free_memory(memory);
}

LZT_TEST(
    MultiDeviceSharedMemoryTests,
    GivenMultipleRootDevicesUsingSharedMemoryWhenExecutingMemoryFillThenCorrectDataWritten) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  test_multi_device_shared_memory(devices, false);
}

LZT_TEST(
    MultiDeviceSharedMemoryTests,
    GivenMultipleRootDevicesUsingSharedMemoryWhenExecutingMemoryFillOnImmediateCmdListThenCorrectDataWritten) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  test_multi_device_shared_memory(devices, true);
}

LZT_TEST(
    MultiDeviceSharedMemoryTests,
    GivenMultipleSubDevicesUsingSharedMemoryWhenExecutingMemoryFillThenCorrectDataWritten) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_ze_devices();
  for (auto device : lzt::get_devices(driver)) {
    auto subdevices = lzt::get_ze_sub_devices(device);

    test_multi_device_shared_memory(subdevices, false);
  }
}

LZT_TEST(
    MultiDeviceSharedMemoryTests,
    GivenMultipleSubDevicesUsingSharedMemoryWhenExecutingMemoryFillOnImmediateCmdListThenCorrectDataWritten) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_ze_devices();
  for (auto device : lzt::get_devices(driver)) {
    auto subdevices = lzt::get_ze_sub_devices(device);

    test_multi_device_shared_memory(subdevices, true);
  }
}

} // namespace
