/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <thread>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeVirtualMemoryTests : public ::testing::Test {
protected:
  zeVirtualMemoryTests() {
    context = lzt::get_default_context();
    device = lzt::zeDevice::get_instance()->get_device();
  }
  ~zeVirtualMemoryTests() {}
  ze_context_handle_t context;
  ze_device_handle_t device;
  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  void *reservedVirtualMemory = nullptr;
  ze_physical_mem_handle_t reservedPhysicalMemory;
};

TEST_F(zeVirtualMemoryTests,
       GivenNullStartAddressAndValidSizeTheVirtualMemoryReserveReturnsSuccess) {

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

TEST_F(
    zeVirtualMemoryTests,
    GivenVirtualMemoryReservationThenSettingTheMemoryAccessAttributeReturnsSuccess) {
  ze_memory_access_attribute_t access;
  ze_memory_access_attribute_t previousAccess;
  size_t memorySize = 0;
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);
  lzt::virtual_memory_reservation_get_access(
      context, reservedVirtualMemory, allocationSize, &access, &memorySize);
  EXPECT_EQ(access, ZE_MEMORY_ACCESS_ATTRIBUTE_NONE);
  EXPECT_EQ(memorySize, allocationSize);

  std::vector<ze_memory_access_attribute_t> memoryAccessFlags = {
      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE, ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY,
      ZE_MEMORY_ACCESS_ATTRIBUTE_NONE};

  for (auto accessFlags : memoryAccessFlags) {
    previousAccess = access;
    lzt::virtual_memory_reservation_set_access(context, reservedVirtualMemory,
                                               allocationSize, accessFlags);
    lzt::virtual_memory_reservation_get_access(
        context, reservedVirtualMemory, allocationSize, &access, &memorySize);
    EXPECT_NE(previousAccess, access);
    EXPECT_EQ(accessFlags, access);
  }

  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

TEST_F(zeVirtualMemoryTests,
       GivenPageAlignedSizeThenVirtualAndPhysicalMemoryReservedSuccessfully) {
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);
  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
}

TEST_F(
    zeVirtualMemoryTests,
    GivenPageAlignedSizeThenPhysicalMemoryisSuccessfullyReservedForAllAccessTypes) {
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  std::vector<ze_memory_access_attribute_t> memoryAccessFlags = {
      ZE_MEMORY_ACCESS_ATTRIBUTE_NONE, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE,
      ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY};

  for (auto accessFlags : memoryAccessFlags) {
    lzt::virtual_memory_map(context, reservedVirtualMemory, allocationSize,
                            reservedPhysicalMemory, 0, accessFlags);
    lzt::virtual_memory_unmap(context, reservedVirtualMemory, allocationSize);
  }

  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

TEST_F(
    zeVirtualMemoryTests,
    GivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  ze_command_list_handle_t cmdlist = lzt::create_command_list(device);
  ze_command_queue_handle_t cmdqueue = lzt::create_command_queue();

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  ASSERT_EQ(zeVirtualMemMap(context, reservedVirtualMemory, allocationSize,
                            reservedPhysicalMemory, 0,
                            ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE),
            ZE_RESULT_SUCCESS);
  int8_t pattern = 9;
  void *memory = lzt::allocate_shared_memory(allocationSize, pageSize);
  lzt::append_memory_fill(cmdlist, reservedVirtualMemory, &pattern,
                          sizeof(pattern), allocationSize, nullptr);
  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
  lzt::append_memory_copy(cmdlist, memory, reservedVirtualMemory,
                          allocationSize, nullptr);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);
  uint8_t *data = reinterpret_cast<uint8_t *>(memory);
  for (int i = 0; i < allocationSize; i++) {
    ASSERT_EQ(data[i], pattern);
  }

  lzt::virtual_memory_unmap(context, reservedVirtualMemory, allocationSize);
  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
  lzt::free_memory(memory);
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
}

TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnOneDeviceThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  ze_command_list_handle_t cmdlist = lzt::create_command_list(device);
  ze_command_queue_handle_t cmdqueue = lzt::create_command_queue();

  allocationSize = 2097152 / 2;
  ze_physical_mem_handle_t reservedPhysicalMemorySecondHalf;
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);
  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemorySecondHalf);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize * 2,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  ASSERT_EQ(zeVirtualMemMap(context, reservedVirtualMemory, allocationSize,
                            reservedPhysicalMemory, 0,
                            ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE),
            ZE_RESULT_SUCCESS);
  uint64_t offsetAddr =
      reinterpret_cast<uint64_t>(reservedVirtualMemory) + allocationSize;
  ASSERT_EQ(zeVirtualMemMap(context, reinterpret_cast<void *>(offsetAddr),
                            allocationSize, reservedPhysicalMemorySecondHalf, 0,
                            ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE),
            ZE_RESULT_SUCCESS);
  int8_t pattern = 9;
  void *memory = lzt::allocate_shared_memory(allocationSize * 2, pageSize);
  lzt::append_memory_fill(cmdlist, reservedVirtualMemory, &pattern,
                          sizeof(pattern), allocationSize, nullptr);
  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
  lzt::append_memory_fill(cmdlist, reinterpret_cast<void *>(offsetAddr),
                          &pattern, sizeof(pattern), allocationSize, nullptr);
  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
  lzt::append_memory_copy(cmdlist, memory, reservedVirtualMemory,
                          allocationSize, nullptr);
  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
  uint64_t offsetSharedAddr =
      reinterpret_cast<uint64_t>(memory) + allocationSize;
  lzt::append_memory_copy(cmdlist, reinterpret_cast<void *>(offsetSharedAddr),
                          reinterpret_cast<void *>(offsetAddr), allocationSize,
                          nullptr);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);
  uint8_t *data = reinterpret_cast<uint8_t *>(memory);
  for (int i = 0; i < allocationSize * 2; i++) {
    ASSERT_EQ(data[i], pattern);
  }

  lzt::virtual_memory_unmap(context, reservedVirtualMemory, allocationSize);
  lzt::virtual_memory_unmap(context, reinterpret_cast<void *>(offsetAddr),
                            allocationSize);
  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
  lzt::physical_memory_destroy(context, reservedPhysicalMemorySecondHalf);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize * 2);
  lzt::free_memory(memory);
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
}

} // namespace