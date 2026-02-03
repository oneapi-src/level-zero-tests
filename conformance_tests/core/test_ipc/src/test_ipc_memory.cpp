/*
 *
 * Copyright (C) 2020-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifdef __linux__
#include <unistd.h>
#endif
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_memory.hpp"
#include "net/test_ipc_comm.hpp"

#include <level_zero/ze_api.h>

namespace bipc = boost::interprocess;

namespace lzt = level_zero_tests;

namespace {

using lzt::to_u32;

#ifdef __linux__
static void run_ipc_mem_access_test(ipc_mem_access_test_t test_type,
                                    size_t size, bool reserved,
                                    ze_ipc_memory_flags_t flags,
                                    bool is_immediate) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Parent zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Parent] Driver initialized\n";
  lzt::print_platform_overview();

  bipc::named_semaphore::remove("ipc_memory_test_semaphore");
  bipc::named_semaphore semaphore(bipc::create_only,
                                  "ipc_memory_test_semaphore", 0);

  bipc::shared_memory_object::remove("ipc_memory_test");
  // launch child
  boost::process::child c("./ipc/test_ipc_memory_helper");

  ze_ipc_mem_handle_t ipc_handle = {};
  shared_data_t test_data = {test_type, TEST_SOCK,    to_u32(size),
                             flags,     is_immediate, ipc_handle, 0, 0};
  bipc::shared_memory_object shm(bipc::create_only, "ipc_memory_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);
  std::memcpy(region.get_address(), &test_data, sizeof(shared_data_t));

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto cmd_bundle = lzt::create_command_bundle(context, device, is_immediate);

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::write_data_pattern(buffer, size, 1);
  size_t allocSize = size;
  ze_physical_mem_handle_t reservedPhysicalMemory = {};
  void *memory = nullptr;
  if (reserved) {
    memory = lzt::reserve_allocate_and_map_device_memory(
        context, device, allocSize, &reservedPhysicalMemory);
  } else {
    memory = lzt::allocate_device_memory(size, 1, 0, context);
  }
  lzt::append_memory_copy(cmd_bundle.list, memory, buffer, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandle(context, memory, &ipc_handle));
  ze_ipc_mem_handle_t ipc_handle_zero{};
  ASSERT_NE(0, memcmp((void *)&ipc_handle, (void *)&ipc_handle_zero,
                      sizeof(ipc_handle)));

  // Wait until the child is ready to receive the IPC handle
  semaphore.wait();

  lzt::send_ipc_handle(ipc_handle);

  // Free device memory once receiver is done
  c.wait();
  EXPECT_EQ(c.exit_code(), 0);

  ASSERT_ZE_RESULT_SUCCESS(zeMemPutIpcHandle(context, ipc_handle));
  bipc::shared_memory_object::remove("ipc_memory_test");

  if (reserved) {
    lzt::unmap_and_free_reserved_memory(context, memory, reservedPhysicalMemory,
                                        allocSize);
  } else {
    lzt::free_memory(context, memory);
  }
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
}
#endif // __linux__

static void run_ipc_dev_mem_access_test_opaque(ipc_mem_access_test_t test_type,
                                               size_t size, bool reserved,
                                               ze_ipc_memory_flags_t flags,
                                               bool is_immediate) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Parent zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Parent] Driver initialized\n";
  lzt::print_platform_overview();

  bipc::shared_memory_object::remove("ipc_memory_test");

  ze_ipc_mem_handle_t ipc_handle = {};

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  ze_device_handle_t device;
  uint32_t device_id_parent = 0;
  uint32_t device_id_child = 0;

  // For multi-device tests, find valid P2P device pair
  if (test_type == TEST_MULTIDEVICE_ACCESS) {
    auto devices = lzt::get_ze_devices(driver);
    if (devices.size() < 2) {
      LOG_WARNING << "[Parent] Multi-device test requires at least 2 devices, found "
                  << devices.size() << ". Skipping test.";
      GTEST_SKIP();
    }
    
    // Find a valid device pair that supports peer access
    bool found_valid_pair = false;
    for (uint32_t i = 0; i < devices.size() && !found_valid_pair; ++i) {
      for (uint32_t j = 0; j < devices.size() && !found_valid_pair; ++j) {
        if (i == j) continue;
        
        ze_bool_t can_access_peer = false;
        ze_result_t result = zeDeviceCanAccessPeer(devices[i], devices[j], &can_access_peer);
        if (result == ZE_RESULT_SUCCESS && can_access_peer) {
          device_id_parent = i;
          device_id_child = j;
          found_valid_pair = true;
          LOG_DEBUG << "[Parent] Found valid P2P pair: device " << i << " -> device " << j;
        }
      }
    }
    
    if (!found_valid_pair) {
      LOG_WARNING << "[Parent] No valid P2P device pairs found. Skipping test.";
      GTEST_SKIP();
    }
    
    device = devices[device_id_parent];
  } else {
    device = lzt::zeDevice::get_instance()->get_device();
  }

  auto cmd_bundle = lzt::create_command_bundle(context, device, is_immediate);

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::write_data_pattern(buffer, size, 1);
  size_t allocSize = size;
  ze_physical_mem_handle_t reservedPhysicalMemory = {};
  void *memory = nullptr;
  if (reserved) {
    memory = lzt::reserve_allocate_and_map_device_memory(
        context, device, allocSize, &reservedPhysicalMemory);
  } else {
    memory = lzt::allocate_device_memory(size, 1, 0, context);
  }
  lzt::append_memory_copy(cmd_bundle.list, memory, buffer, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandle(context, memory, &ipc_handle));

  ze_ipc_mem_handle_t ipc_handle_zero{};
  ASSERT_NE(0, memcmp((void *)&ipc_handle, (void *)&ipc_handle_zero,
                      sizeof(ipc_handle)));

  // launch child
#ifdef _WIN32
  std::string helper_path = ".\\ipc\\test_ipc_memory_helper.exe";
#else
  std::string helper_path = "./ipc/test_ipc_memory_helper";
#endif
  boost::process::child c;
  try {
    c = boost::process::child(helper_path);
  } catch (const boost::process::process_error &e) {
    std::cerr << "Failed to launch child process: " << e.what() << std::endl;
    throw;
  }

  bipc::shared_memory_object shm(bipc::create_only, "ipc_memory_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);

  // copy ipc handle data to shm
  shared_data_t test_data = {test_type, TEST_NONSOCK, to_u32(size),
                             flags,     is_immediate, ipc_handle,
                             device_id_parent, device_id_child};
  std::memcpy(region.get_address(), &test_data, sizeof(shared_data_t));

  // Free device memory once receiver is done
  c.wait();
  EXPECT_EQ(c.exit_code(), 0);

  ASSERT_ZE_RESULT_SUCCESS(zeMemPutIpcHandle(context, ipc_handle));
  bipc::shared_memory_object::remove("ipc_memory_test");

  if (reserved) {
    lzt::unmap_and_free_reserved_memory(context, memory, reservedPhysicalMemory,
                                        allocSize);
  } else {
    lzt::free_memory(context, memory);
  }
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
}

static void run_ipc_host_mem_access_test_opaque(size_t size,
                                                ze_ipc_memory_flags_t flags) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Parent zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Parent] Driver initialized\n";
  lzt::print_platform_overview();

  // Ensure shared memory object is removed in case it already exists
  // -- can happen if previous test exited abnormally
  bipc::shared_memory_object::remove("ipc_memory_test");

  ze_ipc_mem_handle_t ipc_handle = {};

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::write_data_pattern(buffer, size, 1);

  ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandle(context, buffer, &ipc_handle));

  ze_ipc_mem_handle_t ipc_handle_zero{};
  ASSERT_NE(0, memcmp((void *)&ipc_handle, (void *)&ipc_handle_zero,
                      sizeof(ipc_handle)));

  // Launch child
#ifdef _WIN32
  std::string helper_path = ".\\ipc\\test_ipc_memory_helper.exe";
#else
  std::string helper_path = "./ipc/test_ipc_memory_helper";
#endif
  boost::process::child c;
  try {
    c = boost::process::child(helper_path);
  } catch (const boost::process::process_error &e) {
    std::cerr << "Failed to launch child process: " << e.what() << std::endl;
    throw;
  }

  bipc::shared_memory_object shm(bipc::create_only, "ipc_memory_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);

  // Copy ipc handle data to shm
  shared_data_t test_data = {
      TEST_HOST_ACCESS, TEST_NONSOCK, to_u32(size), flags, false, ipc_handle, 0, 0};
  std::memcpy(region.get_address(), &test_data, sizeof(shared_data_t));

  // Free device memory once receiver is done
  c.wait();
  EXPECT_EQ(c.exit_code(), 0);

  ASSERT_ZE_RESULT_SUCCESS(zeMemPutIpcHandle(context, ipc_handle));
  bipc::shared_memory_object::remove("ipc_memory_test");

  lzt::free_memory(context, buffer);
  lzt::destroy_context(context);
}

static void run_ipc_mem_access_test_opaque_with_properties(
    ipc_mem_access_test_t test_type, size_t size, bool reserved,
    ze_ipc_memory_flags_t flags, bool is_immediate,
    ze_ipc_mem_handle_type_flags_t handle_type_flags) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Parent zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Parent] Driver initialized\n";
  lzt::print_platform_overview();

  bipc::shared_memory_object::remove("ipc_memory_test");

  ze_ipc_mem_handle_t ipc_handle = {};

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  ze_device_handle_t device;
  uint32_t device_id_parent = 0;
  uint32_t device_id_child = 0;

  // For multi-device tests, find valid P2P device pair
  if (test_type == TEST_MULTIDEVICE_ACCESS) {
    auto devices = lzt::get_ze_devices(driver);
    if (devices.size() < 2) {
      LOG_WARNING << "[Parent] Multi-device test requires at least 2 devices, found "
                  << devices.size() << ". Skipping test.";
      GTEST_SKIP();
    }
    
    // Find a valid device pair that supports peer access
    bool found_valid_pair = false;
    for (uint32_t i = 0; i < devices.size() && !found_valid_pair; ++i) {
      for (uint32_t j = 0; j < devices.size() && !found_valid_pair; ++j) {
        if (i == j) continue;
        
        ze_bool_t can_access_peer = false;
        ze_result_t result = zeDeviceCanAccessPeer(devices[i], devices[j], &can_access_peer);
        if (result == ZE_RESULT_SUCCESS && can_access_peer) {
          device_id_parent = i;
          device_id_child = j;
          found_valid_pair = true;
          LOG_DEBUG << "[Parent] Found valid P2P pair: device " << i << " -> device " << j;
        }
      }
    }
    
    if (!found_valid_pair) {
      LOG_WARNING << "[Parent] No valid P2P device pairs found. Skipping test.";
      GTEST_SKIP();
    }
    
    device = devices[device_id_parent];
  } else {
    device = lzt::zeDevice::get_instance()->get_device();
  }

  auto cmd_bundle = lzt::create_command_bundle(context, device, is_immediate);

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::write_data_pattern(buffer, size, 1);
  size_t allocSize = size;
  ze_physical_mem_handle_t reservedPhysicalMemory = {};
  void *memory = nullptr;
  if (reserved) {
    memory = lzt::reserve_allocate_and_map_device_memory(
        context, device, allocSize, &reservedPhysicalMemory);
  } else {
    memory = lzt::allocate_device_memory(size, 1, 0, context);
  }
  lzt::append_memory_copy(cmd_bundle.list, memory, buffer, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  // Use zeMemGetIpcHandleWithProperties with extension properties
  ze_ipc_mem_handle_type_ext_desc_t handle_type_desc = {};
  handle_type_desc.stype = ZE_STRUCTURE_TYPE_IPC_MEM_HANDLE_TYPE_EXT_DESC;
  handle_type_desc.pNext = nullptr;
  handle_type_desc.typeFlags = handle_type_flags;

  ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandleWithProperties(
      context, memory, &handle_type_desc, &ipc_handle));

  ze_ipc_mem_handle_t ipc_handle_zero{};
  ASSERT_NE(0, memcmp((void *)&ipc_handle, (void *)&ipc_handle_zero,
                      sizeof(ipc_handle)));

  // launch child
#ifdef _WIN32
  std::string helper_path = ".\\ipc\\test_ipc_memory_helper.exe";
#else
  std::string helper_path = "./ipc/test_ipc_memory_helper";
#endif
  boost::process::child c;
  try {
    c = boost::process::child(helper_path);
  } catch (const boost::process::process_error &e) {
    std::cerr << "Failed to launch child process: " << e.what() << std::endl;
    throw;
  }

  bipc::shared_memory_object shm(bipc::create_only, "ipc_memory_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);

  // copy ipc handle data to shm
  shared_data_t test_data = {test_type, TEST_NONSOCK, to_u32(size),
                             flags,     is_immediate, ipc_handle,
                             device_id_parent, device_id_child};
  std::memcpy(region.get_address(), &test_data, sizeof(shared_data_t));

  // Free device memory once receiver is done
  c.wait();
  EXPECT_EQ(c.exit_code(), 0);

  ASSERT_ZE_RESULT_SUCCESS(zeMemPutIpcHandle(context, ipc_handle));
  bipc::shared_memory_object::remove("ipc_memory_test");

  if (reserved) {
    lzt::unmap_and_free_reserved_memory(context, memory, reservedPhysicalMemory,
                                        allocSize);
  } else {
    lzt::free_memory(context, memory);
  }
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
}

#ifdef __linux__
LZT_TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

LZT_TEST(
    IpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_mem_access_test(TEST_SUBDEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

LZT_TEST(
    IpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_mem_access_test(TEST_SUBDEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}
#endif // __linux__

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly) {
  run_ipc_dev_mem_access_test_opaque(TEST_DEVICE_ACCESS, 4096, false,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_dev_mem_access_test_opaque(TEST_DEVICE_ACCESS, 4096, false,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly) {
  run_ipc_dev_mem_access_test_opaque(TEST_DEVICE_ACCESS, 4096, false,
                                     ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_dev_mem_access_test_opaque(TEST_DEVICE_ACCESS, 4096, false,
                                     ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectly) {
  run_ipc_dev_mem_access_test_opaque(TEST_DEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_dev_mem_access_test_opaque(TEST_DEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectly) {
  run_ipc_dev_mem_access_test_opaque(TEST_DEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_dev_mem_access_test_opaque(TEST_DEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleSubDevice,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_dev_mem_access_test_opaque(TEST_SUBDEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleSubDevice,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_dev_mem_access_test_opaque(TEST_SUBDEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleMultiDevice,
    GivenL0MemoryWhenUsingL0IPCThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_dev_mem_access_test_opaque(TEST_MULTIDEVICE_ACCESS, 4096, false,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleMultiDevice,
    GivenL0MemoryWhenUsingL0IPCOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_dev_mem_access_test_opaque(TEST_MULTIDEVICE_ACCESS, 4096, false,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleMultiDevice,
    GivenL0MemoryBiasCachedWhenUsingL0IPCThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_dev_mem_access_test_opaque(TEST_MULTIDEVICE_ACCESS, 4096, false,
                                     ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleMultiDevice,
    GivenL0MemoryBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_dev_mem_access_test_opaque(TEST_MULTIDEVICE_ACCESS, 4096, false,
                                     ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleMultiDevice,
    GivenReservedPhysicalMemoryWhenUsingL0IPCThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_dev_mem_access_test_opaque(TEST_MULTIDEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleMultiDevice,
    GivenReservedPhysicalMemoryWhenUsingL0IPCOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_dev_mem_access_test_opaque(TEST_MULTIDEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleMultiDevice,
    GivenReservedPhysicalMemoryBiasCachedWhenUsingL0IPCThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_dev_mem_access_test_opaque(TEST_MULTIDEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleMultiDevice,
    GivenReservedPhysicalMemoryBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_dev_mem_access_test_opaque(TEST_MULTIDEVICE_ACCESS, 4096, true,
                                     ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenUncachedHostMemoryAllocatedInParentProcessThenChildProcessReadsMemoryCorrectly) {
  run_ipc_host_mem_access_test_opaque(4096, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandle,
    GivenCachedHostMemoryAllocatedInParentProcessThenChildProcessReadsMemoryCorrectly) {
  run_ipc_host_mem_access_test_opaque(4096, ZE_IPC_MEMORY_FLAG_BIAS_CACHED);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCWithDefaultHandleTypeThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCWithDefaultHandleTypeOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCWithDefaultHandleTypeThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCWithDefaultHandleTypeOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCWithDefaultHandleTypeThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCWithDefaultHandleTypeOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCWithDefaultHandleTypeThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCWithDefaultHandleTypeOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCWithFabricAccessibleHandleTypeThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCWithFabricAccessibleHandleTypeOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCWithFabricAccessibleHandleTypeThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCWithFabricAccessibleHandleTypeOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCWithFabricAccessibleHandleTypeThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCWithFabricAccessibleHandleTypeOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCWithFabricAccessibleHandleTypeThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithProperties,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCWithFabricAccessibleHandleTypeOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_DEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesSubDevice,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCWithDefaultHandleTypeThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_SUBDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesSubDevice,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCWithDefaultHandleTypeOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_SUBDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesSubDevice,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCWithFabricAccessibleHandleTypeThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_SUBDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesSubDevice,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCWithFabricAccessibleHandleTypeOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_SUBDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true,
      ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenL0MemoryWithDefaultHandleTypeThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenL0MemoryWithDefaultHandleTypeOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      true, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenL0MemoryBiasCachedWithDefaultHandleTypeThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_CACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenL0MemoryBiasCachedWithDefaultHandleTypeOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_CACHED,
      true, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenReservedPhysicalMemoryWithDefaultHandleTypeThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenReservedPhysicalMemoryWithDefaultHandleTypeOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      true, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenReservedPhysicalMemoryBiasCachedWithDefaultHandleTypeThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_CACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenReservedPhysicalMemoryBiasCachedWithDefaultHandleTypeOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_CACHED,
      true, ZE_IPC_MEM_HANDLE_TYPE_FLAG_DEFAULT);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenL0MemoryWithFabricAccessibleHandleTypeThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenL0MemoryWithFabricAccessibleHandleTypeOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      true, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenL0MemoryBiasCachedWithFabricAccessibleHandleTypeThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_CACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenL0MemoryBiasCachedWithFabricAccessibleHandleTypeOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, false, ZE_IPC_MEMORY_FLAG_BIAS_CACHED,
      true, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenReservedPhysicalMemoryWithFabricAccessibleHandleTypeThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenReservedPhysicalMemoryWithFabricAccessibleHandleTypeOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED,
      true, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenReservedPhysicalMemoryBiasCachedWithFabricAccessibleHandleTypeThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_CACHED,
      false, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

LZT_TEST(
    IpcMemoryAccessTestOpaqueIpcHandleWithPropertiesMultiDevice,
    GivenReservedPhysicalMemoryBiasCachedWithFabricAccessibleHandleTypeOnImmediateCmdListThenChildReadsCorrectlyOnDifferentDevice) {
  run_ipc_mem_access_test_opaque_with_properties(
      TEST_MULTIDEVICE_ACCESS, 4096, true, ZE_IPC_MEMORY_FLAG_BIAS_CACHED,
      true, ZE_IPC_MEM_HANDLE_TYPE_FLAG_FABRIC_ACCESSIBLE);
}

} // namespace

// We put the main here because L0 doesn't currently specify how
// zeInit should be handled with fork(), so each process must call
// zeInit
int main(int argc, char **argv) {
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);

  try {
    auto result = RUN_ALL_TESTS();
    return result;
  } catch (std::runtime_error &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
