/*
 *
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifdef __linux
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include <chrono>
#include <thread>

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#ifdef __linux
#include "net/unix_comm.hpp"
#endif
#include <iostream>
#include <fstream>

namespace lzt = level_zero_tests;

/*
  This app should
  (A) Allocate device memory
  (B) Create an export fd for that memory
  (C) Send that fd to a unix local socket
*/
int main(int argc, char **argv) {
  ze_result_t result = zeInit(1);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_WARNING << "zeInit failed";
    exit(1);
  }

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  bool is_immediate = (argv[2][0] != '0');

#ifdef __linux__
  const char *socket_path = "external_memory_socket";
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

  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

  uint8_t pattern = 0xAB;
  lzt::append_memory_fill(cmd_bundle.list, exported_memory, &pattern,
                          sizeof(pattern), size, nullptr);

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

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

  char data[ZE_MAX_IPC_HANDLE_SIZE];
  if (lzt::write_fd_to_socket(send_socket, export_fd.fd, data) < 0) {
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
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
  exit(0);
#else
  auto external_memory_properties = lzt::get_external_memory_properties(device);
  std::string handle_type;
  std::cin >> handle_type;
  if (atoi(handle_type.c_str()) ==
      lzt::lztWin32HandleTestType::LZT_OPAQUE_WIN32) {
    if (!(external_memory_properties.memoryAllocationExportTypes &
          ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32)) {
      LOG_WARNING << "Device does not support importing OPAQUE_WIN32\n";
      exit(1);
    }
  } else if (atoi(handle_type.c_str()) ==
             lzt::lztWin32HandleTestType::LZT_KMT_WIN32) {
    if (!(external_memory_properties.memoryAllocationExportTypes &
          ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT)) {
      LOG_WARNING << "Device does not support importing WIN32 KMT Handle\n";
      exit(1);
    }
  } else {
    exit(1);
  }

  HANDLE hPipe;
  uint64_t targetHandle;
  BOOL pipeCommandSuccess = FALSE;
  DWORD bytesRead, pipeMode;
  LPTSTR externalMemoryTestPipeName =
      TEXT("\\\\.\\pipe\\external_memory_socket");

  while (1) {
    hPipe = CreateFile(externalMemoryTestPipeName, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                       0, NULL);
    if (hPipe != INVALID_HANDLE_VALUE)
      break;

    if (GetLastError() != ERROR_PIPE_BUSY) {
      LOG_ERROR << "Could not open pipe with Error " << GetLastError()
                << std::endl;
      return -1;
    }

    if (!WaitNamedPipe(externalMemoryTestPipeName, 20000)) {
      LOG_ERROR << "WaitNamedPipe timedout " << GetLastError() << std::endl;
      return -1;
    }
  }

  // The pipe connected; change to message-read mode.

  pipeMode = PIPE_READMODE_MESSAGE;
  pipeCommandSuccess = SetNamedPipeHandleState(hPipe, &pipeMode, NULL, NULL);
  if (!pipeCommandSuccess) {
    LOG_ERROR << "SetNamedPipeHandleState failed with Error " << GetLastError()
              << std::endl;
  }

  do {
    pipeCommandSuccess =
        ReadFile(hPipe, &targetHandle, sizeof(uint64_t), &bytesRead, NULL);
    if (!pipeCommandSuccess && GetLastError() != ERROR_MORE_DATA)
      break;
  } while (!pipeCommandSuccess); // repeat loop if ERROR_MORE_DATA

  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

  void *imported_memory;
  auto size = 1024;

  // set up request to import the external memory handle
  auto driver_properties = lzt::get_driver_properties(driver);
  ze_external_memory_import_win32_handle_t import_handle = {};
  import_handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32;
  if (atoi(handle_type.c_str()) ==
      lzt::lztWin32HandleTestType::LZT_OPAQUE_WIN32) {
    import_handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32;
  } else if (atoi(handle_type.c_str()) ==
             lzt::lztWin32HandleTestType::LZT_KMT_WIN32) {
    import_handle.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT;
  } else {
    exit(1);
  }

  import_handle.handle = reinterpret_cast<void *>(targetHandle);

  ze_device_mem_alloc_desc_t device_alloc_desc = {};
  device_alloc_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_alloc_desc.pNext = &import_handle;
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocDevice(context, &device_alloc_desc, size, 1, device,
                             &imported_memory));

  auto verification_memory =
      lzt::allocate_shared_memory(size, 1, 0, 0, device, context);
  lzt::append_memory_copy(cmd_bundle.list, verification_memory, imported_memory,
                          size);

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(verification_memory)[i],
              0xAB); // this pattern is written in the parent
  }

  lzt::free_memory(context, imported_memory);
  lzt::free_memory(context, verification_memory);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
  exit(0);

#endif
}
