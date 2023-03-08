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
    GivenValidStartAddressAndNewValidSizeThenResizingVirtualReservationSucceeds) {

  size_t largeAllocSize = allocationSize * 4;
  lzt::query_page_size(context, device, largeAllocSize, &pageSize);
  largeAllocSize = lzt::create_page_aligned_size(largeAllocSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, largeAllocSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, largeAllocSize);
  size_t smallerAllocSize = allocationSize * 2;
  lzt::query_page_size(context, device, smallerAllocSize, &pageSize);
  smallerAllocSize = lzt::create_page_aligned_size(smallerAllocSize, pageSize);
  lzt::virtual_memory_reservation(context, reservedVirtualMemory,
                                  smallerAllocSize, &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, smallerAllocSize);
}

TEST_F(zeVirtualMemoryTests,
       GivenVirtualReservationWithCustomStartAddressThenResizedPtrAllocated) {

  void *originalPtr = nullptr;
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &originalPtr);
  EXPECT_NE(nullptr, originalPtr);

  void *newUpdatedPtr = reinterpret_cast<void *>(
      reinterpret_cast<size_t>(originalPtr) + allocationSize);
  void *recievedPtr = nullptr;
  lzt::virtual_memory_reservation(context, newUpdatedPtr, allocationSize,
                                  &recievedPtr);
  EXPECT_NE(nullptr, recievedPtr);
  if (recievedPtr != newUpdatedPtr) {
    lzt::virtual_memory_free(context, recievedPtr, allocationSize);
    size_t updatedSize = allocationSize + allocationSize;
    lzt::query_page_size(context, device, updatedSize, &pageSize);
    updatedSize = lzt::create_page_aligned_size(updatedSize, pageSize);
    void *largerPtr = nullptr;
    lzt::virtual_memory_reservation(context, nullptr, updatedSize, &largerPtr);
    EXPECT_NE(nullptr, largerPtr);
    lzt::virtual_memory_free(context, originalPtr, allocationSize);
  }
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

enum MemoryReservationTestType {
  MEMORY_RESERVATION_SINGLE_DEVICE,
  MEMORY_RESERVATION_SINGLE_ROOT_DEVICE_MULTI_SUB_DEVICES,
  MEMORY_RESERVATION_MULTI_ROOT_DEVICES
};

void dataCheckMemoryReservations(enum MemoryReservationTestType type) {
  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t rootDevice = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_device_handle_t> devices;
  size_t pageSize = 0;
  void *reservedVirtualMemory = nullptr;
  std::vector<ze_physical_mem_handle_t> reservedPhysicalMemory;
  size_t allocationSize = (1024 * 1024);
  uint32_t numDevices, numSubDevices;

  switch (type) {
  case MemoryReservationTestType::MEMORY_RESERVATION_MULTI_ROOT_DEVICES:
    numDevices = lzt::get_ze_device_count();
    if (numDevices < 2) {
      GTEST_SKIP() << "Multi Root Devices not found, skipping test";
    }
    allocationSize = allocationSize * numDevices;
    devices = lzt::get_ze_devices(numDevices);
    reservedPhysicalMemory.resize(numDevices);
    break;
  case MemoryReservationTestType::
      MEMORY_RESERVATION_SINGLE_ROOT_DEVICE_MULTI_SUB_DEVICES:
    numSubDevices = lzt::get_ze_sub_device_count(rootDevice);
    if (numSubDevices < 2) {
      GTEST_SKIP() << "Multi Sub Devices not found, skipping test";
    }
    allocationSize = allocationSize * numSubDevices;
    devices = lzt::get_ze_sub_devices(rootDevice);
    reservedPhysicalMemory.resize(numSubDevices);
    break;
  case MemoryReservationTestType::MEMORY_RESERVATION_SINGLE_DEVICE:
    reservedPhysicalMemory.resize(2);
    devices.resize(2);
    devices[0] = rootDevice;
    devices[1] = rootDevice;
    break;
  default:
    FAIL() << "Invalid Memory Reservation Test Type";
  }

  ze_command_list_handle_t cmdlist = lzt::create_command_list(rootDevice);
  ze_command_queue_handle_t cmdqueue = lzt::create_command_queue();

  lzt::query_page_size(context, rootDevice, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  for (int i = 0; i < devices.size(); i++) {
    lzt::physical_memory_allocation(context, devices[i], allocationSize,
                                    &reservedPhysicalMemory[i]);
  }
  lzt::virtual_memory_reservation(context, nullptr,
                                  allocationSize * devices.size(),
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  size_t offset = 0;
  for (int i = 0; i < devices.size(); i++) {
    uint64_t offsetAddr =
        reinterpret_cast<uint64_t>(reservedVirtualMemory) + offset;
    ASSERT_EQ(zeVirtualMemMap(context, reinterpret_cast<void *>(offsetAddr),
                              allocationSize, reservedPhysicalMemory[i], 0,
                              ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE),
              ZE_RESULT_SUCCESS);
    offset += allocationSize;
  }

  int8_t pattern = 9;
  void *memory =
      lzt::allocate_host_memory(allocationSize * devices.size(), pageSize);

  offset = 0;
  for (int i = 0; i < devices.size(); i++) {
    uint64_t offsetAddr =
        reinterpret_cast<uint64_t>(reservedVirtualMemory) + offset;
    lzt::append_memory_fill(cmdlist, reinterpret_cast<void *>(offsetAddr),
                            &pattern, sizeof(pattern), allocationSize, nullptr);
    offset += allocationSize;
  }

  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);

  offset = 0;
  for (int i = 0; i < devices.size(); i++) {
    uint64_t offsetAddr =
        reinterpret_cast<uint64_t>(reservedVirtualMemory) + offset;
    uint64_t offsetHostAddr = reinterpret_cast<uint64_t>(memory) + offset;
    lzt::append_memory_copy(cmdlist, reinterpret_cast<void *>(offsetHostAddr),
                            reinterpret_cast<void *>(offsetAddr),
                            allocationSize, nullptr);
    offset += allocationSize;
  }

  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);
  uint8_t *data = reinterpret_cast<uint8_t *>(memory);
  for (int i = 0; i < allocationSize * devices.size(); i++) {
    ASSERT_EQ(data[i], pattern);
  }

  lzt::virtual_memory_unmap(context, reservedVirtualMemory, allocationSize);

  offset = 0;
  for (int i = 0; i < devices.size(); i++) {
    uint64_t offsetAddr =
        reinterpret_cast<uint64_t>(reservedVirtualMemory) + offset;
    lzt::virtual_memory_unmap(context, reinterpret_cast<void *>(offsetAddr),
                              allocationSize);
    lzt::physical_memory_destroy(context, reservedPhysicalMemory[i]);
    offset += allocationSize;
  }

  lzt::virtual_memory_free(context, reservedVirtualMemory,
                           allocationSize * devices.size());
  lzt::free_memory(memory);
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
}

TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnOneDeviceThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::MEMORY_RESERVATION_SINGLE_DEVICE);
}

TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnSingleRootDeviceButAcrossSubDevicesThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::
          MEMORY_RESERVATION_SINGLE_ROOT_DEVICE_MULTI_SUB_DEVICES);
}

TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnMultipleRootDevicesThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::MEMORY_RESERVATION_MULTI_ROOT_DEVICES);
}

} // namespace