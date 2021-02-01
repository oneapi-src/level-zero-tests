/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <thread>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <sys/mman.h>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "net/unix_comm.hpp"

namespace bp = boost::process;
namespace fs = boost::filesystem;

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {
static int get_imported_fd(std::string driver_id, bp::opstream &child_input) {
  int fd;
  const char *socket_path = "external_memory_socket";

  // launch a new process that exports memory
  fs::path helper_path(fs::current_path() / "memory");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);
  fs::path helper = bp::search_path("test_import_helper", paths);
  bp::child import_memory_helper(helper, driver_id, bp::std_in < child_input);
  import_memory_helper.detach();

  struct sockaddr_un local_addr, remote_addr;
  local_addr.sun_family = AF_UNIX;
  strcpy(local_addr.sun_path, socket_path);
  unlink(local_addr.sun_path);

  int receive_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (receive_socket < 0) {
    perror("Socket Create Error");
    throw std::runtime_error("Could not create socket");
  }

  if (bind(receive_socket, (struct sockaddr *)&local_addr,
           strlen(local_addr.sun_path) + sizeof(local_addr.sun_family)) < 0) {
    close(receive_socket);
    perror("Socket Bind Error");
    throw std::runtime_error("Could not bind to socket");
  }

  LOG_DEBUG << "Receiver listening...";

  if (listen(receive_socket, 1) < 0) {
    close(receive_socket);
    perror("Socket Listen Error");
    throw std::runtime_error("Could not listen on socket");
  }

  int len = sizeof(struct sockaddr_un);
  int other_socket = accept(receive_socket, (struct sockaddr *)&remote_addr,
                            (socklen_t *)&len);
  if (other_socket == -1) {
    close(receive_socket);
    perror("Server Accept Error");
    throw std::runtime_error("[Server] Could not accept connection");
  }
  LOG_DEBUG << "[Server] Connection accepted";

  if ((fd = lzt::read_fd_from_socket(other_socket)) < 0) {
    close(other_socket);
    close(receive_socket);
    throw std::runtime_error("Failed to receive memory fd from exporter");
  }
  LOG_DEBUG << "[Server] Received memory file descriptor from client";

  close(other_socket);
  close(receive_socket);
  return fd;
}

TEST(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryAsDMABufThenHostCanMMAPBufferContainingValidData) {

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  auto external_memory_properties = lzt::get_external_memory_properties(device);
  if (!(external_memory_properties.memoryAllocationExportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    LOG_WARNING << "Device does not support exporting DMA_BUF";
    return;
  }

  ze_external_memory_export_desc_t export_desc = {};
  export_desc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_DESC;
  export_desc.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;

  ze_device_mem_alloc_desc_t device_alloc_desc = {};
  device_alloc_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_alloc_desc.pNext = &export_desc;

  void *exported_memory;
  auto size = 1024;
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocDevice(context, &device_alloc_desc, size, 1, device,
                             &exported_memory));

  // fill the allocated memory with some pattern so we can verify
  // it was exported successfully
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  uint8_t pattern = 0xAB;
  lzt::append_memory_fill(command_list, exported_memory, &pattern,
                          sizeof(pattern), size, nullptr);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  // set up request to export the external memory handle
  ze_external_memory_export_fd_t export_fd = {};
  export_fd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_FD;
  export_fd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  ze_memory_allocation_properties_t alloc_props = {};
  alloc_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  alloc_props.pNext = &export_fd;
  lzt::get_mem_alloc_properties(context, exported_memory, &alloc_props);

  // mmap the fd to the exported device memory to the host space
  // and verify host can read
  EXPECT_NE(export_fd.fd, 0);
  auto mapped_memory =
      mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, export_fd.fd, 0);

  if (mapped_memory == MAP_FAILED) {
    perror("Error:");
    FAIL() << "Error mmap-ing exported file descriptor";
  }

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(mapped_memory)[i], pattern);
  }

  // cleanup
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::free_memory(context, exported_memory);
  lzt::destroy_context(context);
}

TEST(zeDeviceGetExternalMemoryProperties,
     GivenValidDeviceWhenImportingMemoryThenImportedBufferHasCorrectData) {

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);
  auto device = devices[0];

  auto external_memory_properties = lzt::get_external_memory_properties(device);
  if (!(external_memory_properties.memoryAllocationImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    LOG_WARNING << "Device does not support importing DMA_BUF";
    return;
  }
  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);

  void *imported_memory;
  auto size = 1024;

  // set up request to import the external memory handle
  auto driver_properties = lzt::get_driver_properties(driver);
  bp::opstream child_input;
  auto imported_fd =
      get_imported_fd(lzt::to_string(driver_properties.uuid), child_input);
  ze_external_memory_import_fd_t import_fd = {};
  import_fd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
  import_fd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  import_fd.fd = imported_fd;

  ze_device_mem_alloc_desc_t device_alloc_desc = {};
  device_alloc_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_alloc_desc.pNext = &import_fd;
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocDevice(context, &device_alloc_desc, size, 1, device,
                             &imported_memory));

  auto verification_memory =
      lzt::allocate_shared_memory(size, 1, 0, 0, device, context);
  lzt::append_memory_copy(command_list, verification_memory, imported_memory,
                          size);

  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  LOG_DEBUG << "Importer sending done msg " << std::endl;
  // import helper can now call free on its handle to memory
  child_input << "Done"
              << std::endl; // The content of this message doesn't really matter

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(verification_memory)[i],
              0xAB); // this pattern is written in test_import_helper
  }

  // cleanup
  lzt::free_memory(context, imported_memory);
  lzt::free_memory(context, verification_memory);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_context(context);
}
} // namespace
