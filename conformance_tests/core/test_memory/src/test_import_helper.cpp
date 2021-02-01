/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <sys/socket.h>
#include <sys/un.h>

#include <chrono>
#include <thread>

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "net/unix_comm.hpp"

namespace lzt = level_zero_tests;

/*
  This app should
  (A) Allocate device memory
  (B) Create an export fd for that memory
  (C) Send that fd to a unix local socket
*/
int main(int argc, char **argv) {
  const char *socket_path = "external_memory_socket";

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_WARNING << "zeInit failed";
    exit(1);
  }

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  auto external_memory_properties = lzt::get_external_memory_properties(device);
  if (!(external_memory_properties.memoryAllocationExportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    LOG_WARNING << "Device does not support exporting DMA_BUF\n";
    exit(1);
  }

  // allocate device memory and export
  void *exported_memory;
  auto size = 1024;

  ze_external_memory_export_desc_t export_desc = {};
  export_desc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_DESC;
  export_desc.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  ze_device_mem_alloc_desc_t device_alloc_desc = {};
  device_alloc_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_alloc_desc.pNext = &export_desc;

  result = zeMemAllocDevice(context, &device_alloc_desc, size, 1, device,
                            &exported_memory);
  if (ZE_RESULT_SUCCESS != result) {
    LOG_WARNING << "Error allocating device memory to be imported\n";
    exit(1);
  }

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

  ze_external_memory_export_fd_t export_fd = {};
  export_fd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_FD;
  export_fd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  ze_memory_allocation_properties_t alloc_props = {};
  alloc_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  alloc_props.pNext = &export_fd;
  lzt::get_mem_alloc_properties(context, exported_memory, &alloc_props);

  //*********send memory file descriptor to separate process *************
  struct sockaddr_un remote_addr;
  remote_addr.sun_family = AF_UNIX;

  strcpy(remote_addr.sun_path, socket_path);
  int send_socket = socket(AF_UNIX, SOCK_STREAM, 0);

  if (send_socket == -1) {
    perror("Socket Create Error");
    throw std::runtime_error("Error creating import helper local socket");
  }

  std::chrono::milliseconds wait_time = std::chrono::milliseconds(0);
  while (connect(send_socket, (struct sockaddr *)&remote_addr,
                 sizeof(remote_addr)) == -1) {
    LOG_DEBUG << "Connection error, sleeping and retrying..." << std::endl;
    std::this_thread::sleep_for(lzt::CONNECTION_WAIT);
    wait_time += lzt::CONNECTION_WAIT;

    if (wait_time > lzt::CONNECTION_TIMEOUT) {
      close(send_socket);
      throw std::runtime_error(
          "Timed out waiting to send memory file descriptor");
    }
  }

  LOG_DEBUG << "Exporter Connected" << std::endl;

  if (lzt::write_fd_to_socket(send_socket, export_fd.fd) < 0) {
    close(send_socket);
    perror("Import helper write error");
    throw std::runtime_error("Error sending memory handle");
  }

  LOG_DEBUG << "Wrote memory file descriptor to socket" << std::endl;
  close(send_socket);

  // cleanup
  // wait until receive done signal from importer
  std::string done;
  std::cin >> done;

  LOG_DEBUG << "Exporter cleaning up" << std::endl;
  lzt::free_memory(context, exported_memory);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_context(context);
  exit(0);
}
