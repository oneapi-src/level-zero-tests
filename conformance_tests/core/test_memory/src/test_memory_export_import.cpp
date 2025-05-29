/*
 *
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <thread>
#ifdef __linux__
#include <sys/mman.h>
#endif

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "test_harness/test_harness.hpp"
#include "utils/utils.hpp"
#ifdef __linux__
#include "net/unix_comm.hpp"
#endif
#include <sstream>
#include <string>

namespace bp = boost::process;
namespace fs = boost::filesystem;

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

typedef enum class _test_memory_type_t {
  TEST_MEMORY_TYPE_HOST = 0,
  TEST_MEMORY_TYPE_DEVICE = 1,
  TEST_MEMORY_TYPE_IMAGE_1D = 2,
  TEST_MEMORY_TYPE_IMAGE_2D = 3,
  TEST_MEMORY_TYPE_IMAGE_3D = 4,
  TEST_MEMORY_TYPE_IMAGE_BUFFER = 5
} test_memory_type_t;

typedef struct {
  int fd;
  bool is_immediate;
  test_memory_type_t test_memory_type;
} thread_args;

bool verify_external_memory_type_flag_support(
    ze_device_handle_t device,
    ze_external_memory_type_flag_t external_memory_type_flag) {
  auto external_memory_properties = lzt::get_external_memory_properties(device);
  if (!(external_memory_properties.memoryAllocationExportTypes &
        external_memory_type_flag)) {
    switch (external_memory_type_flag) {
    case ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF:
      LOG_WARNING << "Device does not support exporting DMA_BUF";
      break;
    case ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT:
      LOG_WARNING << "Device does not support exporting WIN32 KMT Handle";
      break;
    case ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE:
      LOG_WARNING << "Device does not support exporting D3D Texture";
      break;
    case ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE_KMT:
      LOG_WARNING << "Device does not support exporting D3D Texture KMT";
      break;
    case ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP:
      LOG_WARNING << "Device does not support exporting D3D12 Heap";
      break;
    case ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE:
      LOG_WARNING << "Device does not support exporting D3D12 Resource";
      break;
    default:
      LOG_WARNING << "Device does not support memory flag type: "
                  << external_memory_type_flag;
      break;
    }
    return false;
  }
  return true;
}

void *allocate_exported_memory(
    ze_context_handle_t context, ze_device_handle_t device, size_t size,
    test_memory_type_t test_memory_type,
    ze_external_memory_type_flag_t external_memory_type_flag,
    ze_image_handle_t *image_handle) {
  void *exported_memory = nullptr;

  ze_external_memory_export_desc_t export_desc = {};
  export_desc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_DESC;
  export_desc.flags = external_memory_type_flag;

  if (test_memory_type == test_memory_type_t::TEST_MEMORY_TYPE_HOST) {
    ze_host_mem_alloc_desc_t host_alloc_desc = {};
    host_alloc_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
    host_alloc_desc.pNext = &export_desc;
    exported_memory =
        lzt::allocate_host_memory(size, 1, 0, &export_desc, context);
  } else {
    // Create image for image type test
    if (test_memory_type > test_memory_type_t::TEST_MEMORY_TYPE_DEVICE) {
      ze_image_format_t format = {};
      format.layout = ZE_IMAGE_FORMAT_LAYOUT_8;
      format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      format.x = ZE_IMAGE_FORMAT_SWIZZLE_0;
      format.y = ZE_IMAGE_FORMAT_SWIZZLE_0;
      format.z = ZE_IMAGE_FORMAT_SWIZZLE_0;
      format.w = ZE_IMAGE_FORMAT_SWIZZLE_0;

      ze_image_desc_t image_desc = {};
      image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
      image_desc.format = format;

      switch (test_memory_type) {
      case test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_1D:
        image_desc.type = ZE_IMAGE_TYPE_1D;
        image_desc.width = 1024;
        break;
      case test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_2D:
        image_desc.type = ZE_IMAGE_TYPE_2D;
        image_desc.width = 128;
        image_desc.height = 8;
        break;
      case test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_3D:
        image_desc.type = ZE_IMAGE_TYPE_3D;
        image_desc.width = 32;
        image_desc.height = 8;
        image_desc.depth = 4;
        break;
      default:
        image_desc.type = ZE_IMAGE_TYPE_BUFFER;
        image_desc.width = 1024;
        break;
      }

      *image_handle = lzt::create_ze_image(context, device, image_desc);
    }

    exported_memory = lzt::allocate_device_memory(size, 1, 0, &export_desc, 0,
                                                  device, context);
  }
  return exported_memory;
}

ze_result_t export_memory(
    ze_context_handle_t context, ze_device_handle_t device, bool is_immediate,
    size_t size, uint8_t pattern, test_memory_type_t test_memory_type,
    ze_external_memory_type_flag_t external_memory_type_flag,
    void **exported_memory, int *fd, ze_image_handle_t *image_handle) {

  auto result = ZE_RESULT_SUCCESS;

  /* Export Memory As DMA_BUF*/
  *exported_memory =
      allocate_exported_memory(context, device, size, test_memory_type,
                               external_memory_type_flag, image_handle);

  // Fill the allocated memory with some pattern so we can verify
  // it was exported successfully
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

  lzt::append_memory_fill(cmd_bundle.list, *exported_memory, &pattern,
                          sizeof(pattern), size, nullptr);
  if (test_memory_type > test_memory_type_t::TEST_MEMORY_TYPE_DEVICE) {
    lzt::append_barrier(cmd_bundle.list);
    lzt::append_image_copy_from_mem(cmd_bundle.list, *image_handle,
                                    *exported_memory, nullptr);
  }
  // An immediate command list is not required to be closed or reset
  if (!is_immediate) {
    lzt::close_command_list(cmd_bundle.list);
  }
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  // set up request to export the external memory handle
  ze_external_memory_export_fd_t export_fd = {};
  export_fd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_FD;
  export_fd.flags = external_memory_type_flag;

  ze_memory_allocation_properties_t alloc_props = {};
  alloc_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  alloc_props.pNext = &export_fd;
  lzt::get_mem_alloc_properties(context, *exported_memory, &alloc_props);

  // mmap the fd to the exported device memory to the host space
  // and verify host can read

  *fd = export_fd.fd;

  // cleanup
  lzt::destroy_command_bundle(cmd_bundle);

  return result;
}

ze_result_t import_memory(ze_context_handle_t context,
                          ze_device_handle_t device, size_t size, int fd,
                          test_memory_type_t test_memory_type,
                          void **imported_memory,
                          ze_image_handle_t *image_handle) {

  auto result = ZE_RESULT_SUCCESS;

  ze_external_memory_import_fd_t import_fd = {};
  import_fd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
  import_fd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  import_fd.fd = fd;

  if (test_memory_type != test_memory_type_t::TEST_MEMORY_TYPE_HOST) {
    if (test_memory_type > test_memory_type_t::TEST_MEMORY_TYPE_DEVICE) {
      ze_image_format_t format = {};
      format.layout = ZE_IMAGE_FORMAT_LAYOUT_8;
      format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
      format.x = ZE_IMAGE_FORMAT_SWIZZLE_X;
      format.y = ZE_IMAGE_FORMAT_SWIZZLE_X;
      format.z = ZE_IMAGE_FORMAT_SWIZZLE_X;
      format.w = ZE_IMAGE_FORMAT_SWIZZLE_X;

      ze_image_desc_t image_desc = {};
      image_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
      image_desc.pNext = &import_fd;
      image_desc.format = format;

      switch (test_memory_type) {
      case test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_1D:
        image_desc.type = ZE_IMAGE_TYPE_1D;
        image_desc.width = 1024;
        break;
      case test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_2D:
        image_desc.type = ZE_IMAGE_TYPE_2D;
        image_desc.width = 128;
        image_desc.height = 8;
        break;
      case test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_3D:
        image_desc.type = ZE_IMAGE_TYPE_3D;
        image_desc.width = 32;
        image_desc.height = 8;
        image_desc.depth = 4;
        break;
      default:
        image_desc.type = ZE_IMAGE_TYPE_BUFFER;
        image_desc.width = 1024;
        break;
      }

      *image_handle = lzt::create_ze_image(context, device, image_desc);
    } else {
      *imported_memory = lzt::allocate_device_memory(size, 1, 0, &import_fd, 0,
                                                     device, context);
    }
  } else {
    *imported_memory =
        lzt::allocate_host_memory(size, 1, 0, &import_fd, context);
  }

  return result;
}

#ifdef __linux__

static int get_imported_fd(std::string driver_id, bp::opstream &child_input,
                           bool is_immediate,
                           test_memory_type_t test_memory_type) {
  int fd;
  const char *socket_path = "external_memory_socket";

  // launch a new process that exports memory
  fs::path helper_path(fs::current_path() / "memory");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);
  fs::path helper = bp::search_path("test_import_helper", paths);
  bp::child import_memory_helper(
      helper,
      bp::args({driver_id, is_immediate ? "1" : "0",
                test_memory_type != test_memory_type_t::TEST_MEMORY_TYPE_HOST
                    ? "1"
                    : "0"}),
      bp::std_in < child_input);
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

  char data[ZE_MAX_IPC_HANDLE_SIZE];
  if ((fd = lzt::read_fd_from_socket(other_socket, data)) < 0) {
    close(other_socket);
    close(receive_socket);
    throw std::runtime_error("Failed to receive memory fd from exporter");
  }
  LOG_DEBUG << "[Server] Received memory file descriptor from client";

  close(other_socket);
  close(receive_socket);
  return fd;
}
#else
static int send_handle(std::string driver_id, bp::opstream &child_input,
                       uint64_t handle, lzt::lztWin32HandleTestType handleType,
                       bool is_immediate) {
  // launch a new process that exports memory
  fs::path helper_path(fs::current_path() / "memory");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);
  bp::ipstream output;
  fs::path helper = bp::search_path("test_import_helper", paths);
  bp::child import_memory_helper(
      helper, bp::args({driver_id, is_immediate ? "1" : "0"}),
      bp::std_in<child_input, bp::std_out> output);
  HANDLE targetHandle;
  auto result =
      DuplicateHandle(GetCurrentProcess(), reinterpret_cast<HANDLE>(handle),
                      import_memory_helper.native_handle(), &targetHandle,
                      DUPLICATE_SAME_ACCESS, TRUE, DUPLICATE_SAME_ACCESS);
  if (result > 0) {
    child_input << handleType << std::endl;

    BOOL pipeConnected = FALSE;
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    BOOL writeSuccess = FALSE;
    DWORD bytesWritten;
    LPCTSTR externalMemoryTestPipeName =
        TEXT("\\\\.\\pipe\\external_memory_socket");
    hPipe = CreateNamedPipe(
        externalMemoryTestPipeName, PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, sizeof(uint64_t), sizeof(uint64_t), 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
      LOG_ERROR << "CreateNamedPipe failed with Error " << GetLastError()
                << std::endl;
      return -1;
    }

    pipeConnected = ConnectNamedPipe(hPipe, NULL)
                        ? TRUE
                        : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (!pipeConnected) {
      import_memory_helper.terminate();
      CloseHandle(hPipe);
      return -1;
    }
    writeSuccess =
        WriteFile(hPipe, &targetHandle, sizeof(uint64_t), &bytesWritten, NULL);

    if (!writeSuccess) {
      LOG_ERROR << "WriteFile to pipe failed with Error " << GetLastError()
                << std::endl;
      return -1;
    }
    std::ostringstream streamHandle;
    streamHandle << targetHandle;
    std::string handleString = streamHandle.str();
    child_input << handleString << std::endl;
    std::string line;

    while (std::getline(output, line) && !line.empty())
      LOG_INFO << line << std::endl;
    import_memory_helper.wait();
    return import_memory_helper.native_exit_code();
    CloseHandle(hPipe);
  } else {
    import_memory_helper.terminate();
    return -1;
  }
}
#endif

class zeDeviceGetExternalMemoryProperties : public ::testing::Test {
protected:
  void RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(
      bool is_immediate, test_memory_type_t test_memory_type);
  void RunGivenValidDeviceWhenImportingMemoryTest(
      bool is_immediate, test_memory_type_t test_memory_type);
  void RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      bool is_immediate, test_memory_type_t test_memory_type,
      ze_external_memory_type_flag_t external_memory_type_flag,
      lzt::lztWin32HandleTestType handle_test_type);
  void
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
      bool is_immediate, test_memory_type_t test_memory_type);
  void
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
      bool is_immediate, test_memory_type_t test_memory_type);
  void RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(
      bool is_immediate, test_memory_type_t test_memory_type);
  void RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(
      bool is_immediate, test_memory_type_t test_memory_type);
  void RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(
      bool is_immediate, test_memory_type_t test_memory_type);
  void RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(
      bool is_immediate, test_memory_type_t test_memory_type);
};

#ifdef __linux__
void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  if (!verify_external_memory_type_flag_support(
          device, ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    GTEST_SKIP();
  }

  size_t size = 1024;
  uint8_t pattern = 0xAB;
  void *exported_memory = nullptr;
  int fd = 0;
  ze_image_handle_t export_image_handle;

  ASSERT_ZE_RESULT_SUCCESS(
      export_memory(context, device, is_immediate, size, pattern,
                    test_memory_type, ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF,
                    &exported_memory, &fd, &export_image_handle));
  EXPECT_NE(fd, 0);

  /* Import exported Memory As DMA_BUF*/
  auto external_memory_import_properties =
      lzt::get_external_memory_properties(device);
  if (!(external_memory_import_properties.memoryAllocationImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    LOG_WARNING << "Device does not support importing DMA_BUF";
    GTEST_SKIP();
  }
  auto import_cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

  void *imported_memory = nullptr;
  ze_image_handle_t import_image_handle;
  ASSERT_ZE_RESULT_SUCCESS(import_memory(context, device, size, fd,
                                         test_memory_type, &imported_memory,
                                         &import_image_handle));

  auto verification_memory =
      lzt::allocate_shared_memory(size, 1, 0, 0, device, context);
  if (test_memory_type > test_memory_type_t::TEST_MEMORY_TYPE_DEVICE) {
    lzt::append_image_copy_to_mem(import_cmd_bundle.list, verification_memory,
                                  import_image_handle, nullptr);
  } else {
    lzt::append_memory_copy(import_cmd_bundle.list, verification_memory,
                            imported_memory, size);
  }
  lzt::close_command_list(import_cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(import_cmd_bundle, UINT64_MAX);

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(verification_memory)[i],
              0xAB); // this pattern is written in test_import_helper
  }

  // cleanup
  if (exported_memory) {
    lzt::free_memory(context, exported_memory);
  }
  if (imported_memory) {
    lzt::free_memory(context, imported_memory);
  }
  lzt::free_memory(context, verification_memory);
  lzt::destroy_command_bundle(import_cmd_bundle);
  lzt::destroy_context(context);
}

void memory_import_thread(thread_args *args) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);
  auto device = devices[0];

  auto external_memory_properties = lzt::get_external_memory_properties(device);
  if (!(external_memory_properties.memoryAllocationImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    LOG_WARNING << "Device does not support importing DMA_BUF";
    GTEST_SKIP();
  }
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, args->is_immediate);

  auto size = 1024;
  void *imported_memory = nullptr;
  ze_image_handle_t image_handle;
  ASSERT_ZE_RESULT_SUCCESS(import_memory(context, device, size, args->fd,
                                         args->test_memory_type,
                                         &imported_memory, &image_handle));

  auto verification_memory =
      lzt::allocate_shared_memory(size, 1, 0, 0, device, context);
  if (args->test_memory_type > test_memory_type_t::TEST_MEMORY_TYPE_DEVICE) {
    lzt::append_image_copy_to_mem(cmd_bundle.list, verification_memory,
                                  image_handle, nullptr);
  } else {
    lzt::append_memory_copy(cmd_bundle.list, verification_memory,
                            imported_memory, size);
  }
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(verification_memory)[i],
              0xAB); // this pattern is written in test_import_helper
  }

  // cleanup
  if (imported_memory) {
    lzt::free_memory(context, imported_memory);
  }
  lzt::free_memory(context, verification_memory);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  if (!verify_external_memory_type_flag_support(
          device, ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    GTEST_SKIP();
  }

  size_t size = 1024;
  uint8_t pattern = 0xAB;
  void *exported_memory = nullptr;
  int fd = 0;
  ze_image_handle_t image_handle;
  ASSERT_ZE_RESULT_SUCCESS(export_memory(context, device, is_immediate, size,
                                         pattern, test_memory_type,
                                         ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF,
                                         &exported_memory, &fd, &image_handle));
  EXPECT_NE(fd, 0);

  // spawn a new thread and pass fd as argument
  thread_args args = {};
  args.fd = fd;
  args.is_immediate = is_immediate;
  args.test_memory_type = test_memory_type;
  std::thread thread(memory_import_thread, &args);

  thread.join();

  // cleanup
  if (exported_memory) {
    lzt::free_memory(context, exported_memory);
  }
  lzt::destroy_context(context);
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  if (!verify_external_memory_type_flag_support(
          device, ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    GTEST_SKIP();
  }

  size_t size = 1024;
  uint8_t pattern = 0xAB;
  void *exported_memory = nullptr;
  int fd = 0;
  ze_image_handle_t image_handle;

  ASSERT_ZE_RESULT_SUCCESS(export_memory(context, device, is_immediate, size,
                                         pattern, test_memory_type,
                                         ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF,
                                         &exported_memory, &fd, &image_handle));
  EXPECT_NE(fd, 0);

  auto mapped_memory = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (mapped_memory == MAP_FAILED) {
    perror("Error:");
    if (errno == ENODEV) {
      FAIL() << "Filesystem does not support memory mapping: "
                "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE";
    } else {
      FAIL() << "Error mmap-ing exported file descriptor";
    }
  }

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(mapped_memory)[i], pattern);
  }

  // cleanup
  if (exported_memory) {
    lzt::free_memory(context, exported_memory);
  }
  lzt::destroy_context(context);
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenImportingMemoryTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);
  auto device = devices[0];

  auto external_memory_properties = lzt::get_external_memory_properties(device);
  if (!(external_memory_properties.memoryAllocationImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    LOG_WARNING << "Device does not support importing DMA_BUF";
    GTEST_SKIP();
  }
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

  // set up request to import the external memory handle
  auto driver_properties = lzt::get_driver_properties(driver);
  bp::opstream child_input;
  auto imported_fd =
      get_imported_fd(lzt::to_string(driver_properties.uuid), child_input,
                      is_immediate, test_memory_type);

  auto size = 1024;
  void *imported_memory = nullptr;
  ze_image_handle_t image_handle;
  ASSERT_ZE_RESULT_SUCCESS(import_memory(context, device, size, imported_fd,
                                         test_memory_type, &imported_memory,
                                         &image_handle));

  auto verification_memory =
      lzt::allocate_shared_memory(size, 1, 0, 0, device, context);
  if (test_memory_type > test_memory_type_t::TEST_MEMORY_TYPE_DEVICE) {
    lzt::append_image_copy_to_mem(cmd_bundle.list, verification_memory,
                                  image_handle, nullptr);
  } else {
    lzt::append_memory_copy(cmd_bundle.list, verification_memory,
                            imported_memory, size);
  }
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  LOG_DEBUG << "Importer sending done msg " << std::endl;
  // import helper can now call free on its handle to memory
  child_input << "Done"
              << std::endl; // The content of this message doesn't really matter

  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(verification_memory)[i],
              0xAB); // this pattern is written in test_import_helper
  }

  // cleanup
  if (imported_memory) {
    lzt::free_memory(context, imported_memory);
  }
  lzt::free_memory(context, verification_memory);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
        bool is_immediate, test_memory_type_t test_memory_type,
        ze_external_memory_type_flag_t external_memory_type_flag,
        lzt::lztWin32HandleTestType handle_test_type) {
  GTEST_SKIP() << "Test Not Supported on Linux";
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  GTEST_SKIP() << "Test Not Supported on Linux";
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  GTEST_SKIP() << "Test Not Supported on Linux";
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  GTEST_SKIP() << "Test Not Supported on Linux";
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  GTEST_SKIP() << "Test Not Supported on Linux";
}

#else

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(
        bool is_immediate, test_memory_type_t test_memory_type) {

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);
  auto device = devices[0];

  if (!verify_external_memory_type_flag_support(
          device, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE)) {
    GTEST_SKIP();
  }

  void *exported_memory = nullptr;
  auto size = 1024;
  uint8_t pattern = 0xAB;
  int fd = 0;
  ze_image_handle_t image_handle;

  ASSERT_ZE_RESULT_SUCCESS(
      export_memory(context, device, is_immediate, size, pattern,
                    test_memory_type_t::TEST_MEMORY_TYPE_DEVICE,
                    ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE,
                    &exported_memory, &fd, &image_handle));
  EXPECT_NE(fd, 0);

  // cleanup
  if (exported_memory) {
    lzt::free_memory(context, exported_memory);
  }
  lzt::destroy_context(context);
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(
        bool is_immediate, test_memory_type_t test_memory_type) {

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);
  auto device = devices[0];

  if (!verify_external_memory_type_flag_support(
          device, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE_KMT)) {
    GTEST_SKIP();
  }

  void *exported_memory = nullptr;
  auto size = 1024;
  uint8_t pattern = 0xAB;
  int fd = 0;
  ze_image_handle_t image_handle;

  ASSERT_ZE_RESULT_SUCCESS(
      export_memory(context, device, is_immediate, size, pattern,
                    test_memory_type_t::TEST_MEMORY_TYPE_DEVICE,
                    ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE_KMT,
                    &exported_memory, &fd, &image_handle));
  EXPECT_NE(fd, 0);

  // cleanup
  if (exported_memory) {
    lzt::free_memory(context, exported_memory);
  }
  lzt::destroy_context(context);
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(
        bool is_immediate, test_memory_type_t test_memory_type) {

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);
  auto device = devices[0];

  if (!verify_external_memory_type_flag_support(
          device, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP)) {
    GTEST_SKIP();
  }

  void *exported_memory = nullptr;
  auto size = 1024;
  uint8_t pattern = 0xAB;
  int fd = 0;
  ze_image_handle_t image_handle;

  ASSERT_ZE_RESULT_SUCCESS(
      export_memory(context, device, is_immediate, size, pattern,
                    test_memory_type_t::TEST_MEMORY_TYPE_DEVICE,
                    ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP, &exported_memory,
                    &fd, &image_handle));
  EXPECT_NE(fd, 0);

  // cleanup
  if (exported_memory) {
    lzt::free_memory(context, exported_memory);
  }
  lzt::destroy_context(context);
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(
        bool is_immediate, test_memory_type_t test_memory_type) {

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);
  auto device = devices[0];

  if (!verify_external_memory_type_flag_support(
          device, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE)) {
    GTEST_SKIP();
  }

  void *exported_memory = nullptr;
  auto size = 1024;
  uint8_t pattern = 0xAB;
  int fd = 0;
  ze_image_handle_t image_handle;

  ASSERT_ZE_RESULT_SUCCESS(
      export_memory(context, device, is_immediate, size, pattern,
                    test_memory_type_t::TEST_MEMORY_TYPE_DEVICE,
                    ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE,
                    &exported_memory, &fd, &image_handle));
  EXPECT_NE(fd, 0);

  // cleanup
  if (exported_memory) {
    lzt::free_memory(context, exported_memory);
  }
  lzt::destroy_context(context);
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  GTEST_SKIP() << "Test Not Supported on Windows";
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  GTEST_SKIP() << "Test Not Supported on Windows";
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  GTEST_SKIP() << "Test Not Supported on Windows";
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenImportingMemoryTest(
        bool is_immediate, test_memory_type_t test_memory_type) {
  GTEST_SKIP() << "Test Not Supported on Windows";
}

void zeDeviceGetExternalMemoryProperties::
    RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
        bool is_immediate, test_memory_type_t test_memory_type,
        ze_external_memory_type_flag_t external_memory_type_flag,
        lzt::lztWin32HandleTestType handle_test_type) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);
  auto device = devices[0];

  if (!verify_external_memory_type_flag_support(device,
                                                external_memory_type_flag)) {
    GTEST_SKIP();
  }

  void *exported_memory = nullptr;
  auto size = 1024;
  uint8_t pattern = 0xAB;
  int fd = 0;
  ze_image_handle_t image_handle;

  ASSERT_ZE_RESULT_SUCCESS(export_memory(
      context, device, is_immediate, size, pattern,
      test_memory_type_t::TEST_MEMORY_TYPE_DEVICE, external_memory_type_flag,
      &exported_memory, &fd, &image_handle));
  EXPECT_NE(fd, 0);

  ze_external_memory_export_win32_handle_t export_handle = {};
  export_handle.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_WIN32;
  export_handle.flags = external_memory_type_flag;
  ze_memory_allocation_properties_t alloc_props = {};
  alloc_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  alloc_props.pNext = &export_handle;
  lzt::get_mem_alloc_properties(context, exported_memory, &alloc_props);
  auto driver_properties = lzt::get_driver_properties(driver);
  bp::opstream child_input;
  int child_result =
      send_handle(lzt::to_string(driver_properties.uuid), child_input,
                  reinterpret_cast<uint64_t>(export_handle.handle),
                  handle_test_type, is_immediate);

  // cleanup
  if (exported_memory) {
    lzt::free_memory(context, exported_memory);
  }
  lzt::destroy_context(context);
  if (child_result != 0) {
    FAIL() << "Child Failed import\n";
  }
}

#endif // __linux__

/* DEVICE MEMORY TESTS */

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryAsDMABufThenSameThreadCanImportBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryAsDMABufOnImmediateCmdListThenSameThreadCanImportBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryAsDMABufThenOtherThreadCanImportBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryAsDMABufOnImmediateCmdListThenOtherThreadCanImportBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryAsDMABufThenHostCanMMAPBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryAsDMABufOnImmediateCmdListThenHostCanMMAPBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenImportingMemoryThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenImportingMemoryOnImmediateCmdListThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenImportingMemoryWithNTHandleThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE,
      ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32,
      lzt::lztWin32HandleTestType::LZT_OPAQUE_WIN32);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenImportingMemoryWithNTHandleOnImmediateCmdListThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE,
      ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32,
      lzt::lztWin32HandleTestType::LZT_OPAQUE_WIN32);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenImportingMemoryWithKMTHandleThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE,
      ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT,
      lzt::lztWin32HandleTestType::LZT_KMT_WIN32);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenImportingMemoryWithKMTHandleOnImmediateCmdListThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE,
      ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT,
      lzt::lztWin32HandleTestType::LZT_KMT_WIN32);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryWithD3DTextureThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryWithD3DTextureOnImmediateCmdListThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryWithD3DTextureKmtThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryWithD3DTextureKmtOnImmediateCmdListThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryWithD3D12HeapThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryWithD3D12HeapOnImmediateCmdListThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryWithD3D12ResourceThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidDeviceWhenExportingMemoryWithD3D12ResourceOnImmediateCmdListThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_DEVICE);
}

/* HOST MEMORY TESTS */

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryAsDMABufThenSameThreadCanImportBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryAsDMABufOnImmediateCmdListThenSameThreadCanImportBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryAsDMABufThenOtherThreadCanImportBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryAsDMABufOnImmediateCmdListThenOtherThreadCanImportBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryAsDMABufThenHostCanMMAPBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryAsDMABufOnImmediateCmdListThenHostCanMMAPBufferContainingValidData) {
  RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(zeDeviceGetExternalMemoryProperties,
           GivenValidHostWhenImportingMemoryThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenImportingMemoryOnImmediateCmdListThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenImportingMemoryWithNTHandleThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST,
      ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32,
      lzt::lztWin32HandleTestType::LZT_OPAQUE_WIN32);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenImportingMemoryWithNTHandleOnImmediateCmdListThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST,
      ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32,
      lzt::lztWin32HandleTestType::LZT_OPAQUE_WIN32);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenImportingMemoryWithKMTHandleThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST,
      ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT,
      lzt::lztWin32HandleTestType::LZT_KMT_WIN32);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenImportingMemoryWithKMTHandleOnImmediateCmdListThenImportedBufferHasCorrectData) {
  RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST,
      ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT,
      lzt::lztWin32HandleTestType::LZT_KMT_WIN32);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryWithD3DTextureThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryWithD3DTextureOnImmediateCmdListThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryWithD3DTextureKmtThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryWithD3DTextureKmtOnImmediateCmdListThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryWithD3D12HeapThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryWithD3D12HeapOnImmediateCmdListThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryWithD3D12ResourceThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(
      false, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidHostWhenExportingMemoryWithD3D12ResourceOnImmediateCmdListThenResourceSuccessfullyExported) {
  RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(
      true, test_memory_type_t::TEST_MEMORY_TYPE_HOST);
}

/* IMAGE MEMORY TESTS */

test_memory_type_t image_memory_types[] = {
    test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_1D,
    test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_2D,
    test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_3D,
    test_memory_type_t::TEST_MEMORY_TYPE_IMAGE_BUFFER};

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryAsDMABufThenSameThreadCanImportBufferContainingValidData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
        false, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryAsDMABufOnImmediateCmdListThenSameThreadCanImportBufferContainingValidData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInSameThreadTest(
        true, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryAsDMABufThenOtherThreadCanImportBufferContainingValidData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
        false, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryAsDMABufOnImmediateCmdListThenOtherThreadCanImportBufferContainingValidData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingAndImportingMemoryAsDMABufInMultiThreadTest(
        true, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryAsDMABufThenHostCanMMAPBufferContainingValidData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(false, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryAsDMABufOnImmediateCmdListThenHostCanMMAPBufferContainingValidData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryAsDMABufTest(true, image_type);
  }
}

LZT_TEST_F(zeDeviceGetExternalMemoryProperties,
           GivenValidImageWhenImportingMemoryThenImportedBufferHasCorrectData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenImportingMemoryTest(false, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenImportingMemoryOnImmediateCmdListThenImportedBufferHasCorrectData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenImportingMemoryTest(true, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenImportingMemoryWithNTHandleThenImportedBufferHasCorrectData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
        false, image_type, ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32,
        lzt::lztWin32HandleTestType::LZT_OPAQUE_WIN32);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenImportingMemoryWithNTHandleOnImmediateCmdListThenImportedBufferHasCorrectData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
        true, image_type, ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32,
        lzt::lztWin32HandleTestType::LZT_OPAQUE_WIN32);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenImportingMemoryWithKMTHandleThenImportedBufferHasCorrectData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
        false, image_type, ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT,
        lzt::lztWin32HandleTestType::LZT_KMT_WIN32);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenImportingMemoryWithKMTHandleOnImmediateCmdListThenImportedBufferHasCorrectData) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenImportingMemoryWithNTHandleTest(
        true, image_type, ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT,
        lzt::lztWin32HandleTestType::LZT_KMT_WIN32);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryWithD3DTextureThenResourceSuccessfullyExported) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(false, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryWithD3DTextureOnImmediateCmdListThenResourceSuccessfullyExported) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryWithD3DTextureTest(true, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryWithD3DTextureKmtThenResourceSuccessfullyExported) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(false,
                                                                image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryWithD3DTextureKmtOnImmediateCmdListThenResourceSuccessfullyExported) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryWithD3DTextureKmtTest(true,
                                                                image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryWithD3D12HeapThenResourceSuccessfullyExported) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(false, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryWithD3D12HeapOnImmediateCmdListThenResourceSuccessfullyExported) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryWithD3D12HeapTest(true, image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryWithD3D12ResourceThenResourceSuccessfullyExported) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(false,
                                                                image_type);
  }
}

LZT_TEST_F(
    zeDeviceGetExternalMemoryProperties,
    GivenValidImageWhenExportingMemoryWithD3D12ResourceOnImmediateCmdListThenResourceSuccessfullyExported) {
  for (auto image_type : image_memory_types) {
    RunGivenValidDeviceWhenExportingMemoryWithD3D12ResourceTest(true,
                                                                image_type);
  }
}

} // namespace
