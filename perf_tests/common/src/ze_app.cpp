/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/zer_api.h>

#include "ze_app.hpp"

#include "../../common/include/common.hpp"
#include "numa_support.hpp"

#include <assert.h>

bool verbose = false;

std::vector<uint8_t> ZeApp::load_binary_file(const std::string &file_path) {
  if (verbose)
    std::cout << "File path: " << file_path << std::endl;
  std::ifstream stream(file_path, std::ios::in | std::ios::binary);

  std::vector<uint8_t> binary_file;
  if (!stream.good()) {
    std::cerr << "Failed to load binary file: " << file_path;
    return binary_file;
  }

  size_t length = 0;
  stream.seekg(0, stream.end);
  length = static_cast<size_t>(stream.tellg());
  stream.seekg(0, stream.beg);
  if (verbose)
    std::cout << "Binary file length: " << length << std::endl;

  binary_file.resize(length);
  stream.read(reinterpret_cast<char *>(binary_file.data()),
              static_cast<std::streamsize>(length));
  if (verbose)
    std::cout << "Binary file loaded" << std::endl;

  stream.close();

  return binary_file;
}

ZeApp::ZeApp(void) {
  context = nullptr;
  _module_path = "";

  SUCCESS_OR_TERMINATE(zeInit(0));
}

ZeApp::ZeApp(std::string module_path) {
  context = nullptr;
  _module_path = module_path;

  SUCCESS_OR_TERMINATE(zeInit(0));

  _binary_file = load_binary_file(_module_path);
}

void ZeApp::moduleCreate(ze_device_handle_t device,
                         ze_module_handle_t *module) {
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;

  module_description.pNext = nullptr;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = _binary_file.size();
  module_description.pInputModule = _binary_file.data();
  module_description.pBuildFlags = nullptr;

  SUCCESS_OR_TERMINATE(
      zeModuleCreate(context, device, &module_description, module, nullptr));
}

void ZeApp::moduleDestroy(ze_module_handle_t module) {
  SUCCESS_OR_TERMINATE(zeModuleDestroy(module));
}

ZeApp::~ZeApp(void) {}

bool ZeApp::canAccessPeer(uint32_t device_index_0, uint32_t device_index_1) {
  ze_bool_t canAccess = false;

  assert(device_index_0 < _devices.size());
  assert(device_index_1 < _devices.size());

  SUCCESS_OR_TERMINATE(zeDeviceCanAccessPeer(
      _devices[device_index_0], _devices[device_index_1], &canAccess));
  return canAccess;
}

void ZeApp::memoryAlloc(size_t size, void **ptr) {
  assert(_devices.size() != 0);
  assert(context != nullptr);
  memoryAlloc(0, size, ptr);
}

void ZeApp::memoryAlloc(const uint32_t device_index, size_t size, void **ptr) {

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.pNext = nullptr;
  device_desc.ordinal = 0;
  device_desc.flags = 0;

  ze_context_handle_t owner = contextFor(device_index);
  SUCCESS_OR_TERMINATE(zeMemAllocDevice(owner, &device_desc, size, 0,
                                        _devices[device_index], ptr));
  rememberAllocationContext(*ptr, owner);
}

//---------------------------------------------------------------------
// Per-device contexts. zeContextCreateEx restricted to one device is what
// makes the driver place that context's host allocations on the device's own
// NUMA node; a context spanning several devices serves all of them from the
// first device's node.
//---------------------------------------------------------------------
bool ZeApp::enablePerDeviceContexts(void) {
  if (!_device_contexts.empty()) {
    return true;
  }

  if (_driver == nullptr || _devices.empty()) {
    return false;
  }

  std::vector<ze_context_handle_t> contexts;
  contexts.reserve(_devices.size());

  for (size_t i = 0; i < _devices.size(); i++) {
    ze_context_desc_t context_desc = {};
    context_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;

    ze_context_handle_t device_context = nullptr;
    if (zeContextCreateEx(_driver, &context_desc, 1, &_devices[i],
                          &device_context) != ZE_RESULT_SUCCESS) {
      for (auto created : contexts) {
        zeContextDestroy(created);
      }
      return false;
    }

    contexts.push_back(device_context);
  }

  _device_contexts = std::move(contexts);
  return true;
}

ze_context_handle_t ZeApp::contextFor(const uint32_t device_index) const {
  if (_device_contexts.empty() || device_index >= _device_contexts.size()) {
    return context;
  }

  return _device_contexts[device_index];
}

/* An allocation has to be freed against the context it came from, which is
 * only ambiguous once there is more than one. */
void ZeApp::rememberAllocationContext(const void *ptr,
                                      ze_context_handle_t owner) {
  if (!_device_contexts.empty() && ptr != nullptr) {
    _allocation_contexts[ptr] = owner;
  }
}

ze_context_handle_t ZeApp::contextOfAllocation(const void *ptr) const {
  const auto known = _allocation_contexts.find(ptr);
  return (known == _allocation_contexts.end()) ? context : known->second;
}

void ZeApp::memoryAllocHost(const uint32_t device_index, size_t size,
                            void **ptr) {
  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  ze_context_handle_t owner = contextFor(device_index);
  SUCCESS_OR_TERMINATE(zeMemAllocHost(owner, &host_desc, size, 1, ptr));
  rememberAllocationContext(*ptr, owner);
}

void ZeApp::memoryAllocHost(size_t size, void **ptr) {

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  SUCCESS_OR_TERMINATE(zeMemAllocHost(context, &host_desc, size, 1, ptr));
}

/* Host allocation pinned to a NUMA node.
 *
 * zeMemAllocHost() takes no node, and on a driver that answers it from its
 * own device mapping the pages are outside the page allocator entirely, so
 * mbind() on the returned range is accepted and then ignored and numactl
 * does not reach it either. The placement is therefore made first, on
 * anonymous memory the kernel does control, and the result is handed to the
 * driver through the import extension.
 *
 * Without that extension the allocation falls back to zeMemAllocHost() with
 * a binding applied to the returned range, which is what a driver backing
 * host memory with ordinary pages responds to. A failed placement is
 * reported rather than fatal: the buffer is still valid, it is merely
 * wherever the kernel put it. */
bool ZeApp::memoryAllocHost(size_t size, void **ptr, int numa_node,
                            std::string *bind_error) {
  if (bind_error != nullptr) {
    bind_error->clear();
  }

  if (numa_node == numa_support::no_node) {
    memoryAllocHost(size, ptr);
    return true;
  }

  std::string error;

  if (numaPlacedHostAllocationsSupported()) {
    void *placed = numa_support::allocate_on_node(size, numa_node, error);

    if (placed != nullptr) {
      const ze_result_t imported = _import_pointer(_driver, placed, size);

      if (imported == ZE_RESULT_SUCCESS) {
        _imported_host_allocations[placed] = size;
        *ptr = placed;
        return true;
      }

      numa_support::free_on_node(placed, size);
      error = "the driver refused the imported pointer (0x" +
              std::to_string(static_cast<uint32_t>(imported)) + ")";
    }
  } else {
    error = "the placed host allocation path is unavailable";
  }

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  // mbind() works on whole pages, so page alignment is what keeps the
  // fallback binding from reaching into a neighbouring allocation.
  SUCCESS_OR_TERMINATE(zeMemAllocHost(context, &host_desc, size,
                                      numa_support::page_size(), ptr));

  std::string fallback_error;
  const bool bound =
      numa_support::bind_memory(*ptr, size, numa_node, fallback_error);

  if (!bound && bind_error != nullptr) {
    *bind_error =
        error.empty() ? fallback_error : (error + "; " + fallback_error);
  } else if (bind_error != nullptr) {
    *bind_error = error;
  }

  return bound;
}

bool ZeApp::numaPlacedHostAllocationsSupported(void) {
  return _host_allocation_import_enabled && importExtensionAvailable();
}

/* The import extension is not part of the core API, so it is looked up by
 * name and the result cached, including the failure. */
bool ZeApp::importExtensionAvailable(void) {
  if (_import_extension_resolved) {
    return _import_pointer != nullptr && _release_pointer != nullptr;
  }

  _import_extension_resolved = true;

  if (_driver == nullptr) {
    return false;
  }

  void *import_address = nullptr;
  void *release_address = nullptr;

  if (zeDriverGetExtensionFunctionAddress(
          _driver, "zexDriverImportExternalPointer", &import_address) !=
          ZE_RESULT_SUCCESS ||
      zeDriverGetExtensionFunctionAddress(
          _driver, "zexDriverReleaseImportedPointer", &release_address) !=
          ZE_RESULT_SUCCESS) {
    return false;
  }

  _import_pointer =
      reinterpret_cast<ze_result_t (*)(ze_driver_handle_t, void *, size_t)>(
          import_address);
  _release_pointer =
      reinterpret_cast<ze_result_t (*)(ze_driver_handle_t, void *)>(
          release_address);

  return _import_pointer != nullptr && _release_pointer != nullptr;
}

/* Imports an externally created dma_buf into device memory. A size of 0 makes
 * the driver take the extent from the imported fd; use memoryGetAddressRange()
 * to retrieve it. */
void ZeApp::memoryAllocImportFd(const uint32_t device_index, size_t size,
                                int fd, ze_device_mem_alloc_flags_t flags,
                                void **ptr) {
  assert(device_index < _devices.size());

  ze_external_memory_import_fd_t import_desc = {};
  import_desc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
  import_desc.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  import_desc.fd = fd;

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.pNext = &import_desc;
  device_desc.ordinal = 0;
  device_desc.flags = flags;

  SUCCESS_OR_TERMINATE(zeMemAllocDevice(context, &device_desc, size, 0,
                                        _devices[device_index], ptr));
}

/* Imports an externally created dma_buf into host memory. See
 * memoryAllocImportFd() for the size semantics. */
void ZeApp::memoryAllocHostImportFd(size_t size, int fd,
                                    ze_host_mem_alloc_flags_t flags,
                                    void **ptr) {

  ze_external_memory_import_fd_t import_desc = {};
  import_desc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
  import_desc.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  import_desc.fd = fd;

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.pNext = &import_desc;
  host_desc.flags = flags;

  SUCCESS_OR_TERMINATE(zeMemAllocHost(context, &host_desc, size, 1, ptr));
}

void ZeApp::memoryGetAddressRange(void *ptr, void **base, size_t *size) {
  SUCCESS_OR_TERMINATE(zeMemGetAddressRange(context, ptr, base, size));
}

void ZeApp::memoryFree(const void *ptr) {
  // An imported pointer belongs to this process, not to the driver, so it is
  // released and unmapped instead of handed to zeMemFree().
  const auto imported = _imported_host_allocations.find(ptr);
  if (imported != _imported_host_allocations.end()) {
    if (_release_pointer != nullptr) {
      _release_pointer(_driver, const_cast<void *>(ptr));
    }
    numa_support::free_on_node(const_cast<void *>(ptr), imported->second);
    _imported_host_allocations.erase(imported);
    return;
  }

  SUCCESS_OR_TERMINATE(
      zeMemFree(contextOfAllocation(ptr), const_cast<void *>(ptr)));
  _allocation_contexts.erase(ptr);
}

void ZeApp::memoryOpenIpcHandle(const uint32_t device_index,
                                ze_ipc_mem_handle_t pIpcHandle,
                                void **zeIpcBuffer) {
  SUCCESS_OR_TERMINATE(zeMemOpenIpcHandle(context, _devices[device_index],
                                          pIpcHandle, 0u, zeIpcBuffer));
}

void ZeApp::deviceGetCommandQueueGroupProperties(
    const uint32_t device_index, uint32_t *numQueueGroups,
    ze_command_queue_group_properties_t *queueProperties) {
  SUCCESS_OR_TERMINATE(zeDeviceGetCommandQueueGroupProperties(
      _devices[device_index], numQueueGroups, queueProperties));
}

void ZeApp::functionCreate(ze_kernel_handle_t *function,
                           const char *pFunctionName) {
  functionCreate(0, function, pFunctionName);
}

void ZeApp::functionCreate(const uint32_t device_index,
                           ze_kernel_handle_t *function,
                           const char *pFunctionName) {
  ze_kernel_desc_t function_description = {};
  function_description.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;

  function_description.pNext = nullptr;
  function_description.flags = 0;
  function_description.pKernelName = pFunctionName;

  SUCCESS_OR_TERMINATE(
      zeKernelCreate(_modules[device_index], &function_description, function));
}

void ZeApp::functionDestroy(ze_kernel_handle_t function) {
  SUCCESS_OR_TERMINATE(zeKernelDestroy(function));
}

void ZeApp::imageCreate(const ze_image_desc_t *imageDesc,
                        ze_image_handle_t *image) {
  assert(_devices.size() != 0);
  imageCreate(_devices[0], imageDesc, image);
}

void ZeApp::imageCreate(ze_device_handle_t device,
                        const ze_image_desc_t *imageDesc,
                        ze_image_handle_t *image) {
  SUCCESS_OR_TERMINATE(zeImageCreate(context, device, imageDesc, image));
}

void ZeApp::imageCreate(ze_device_handle_t device, ze_image_handle_t *image,
                        uint32_t width, uint32_t height, uint32_t depth) {

  ze_image_format_t formatDesc = {
      ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_TYPE_FLOAT,
      ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_0,
      ZE_IMAGE_FORMAT_SWIZZLE_0, ZE_IMAGE_FORMAT_SWIZZLE_1};

  ze_image_desc_t imageDesc = {};
  imageDesc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  imageDesc.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
  imageDesc.type = ZE_IMAGE_TYPE_2D;
  imageDesc.format = formatDesc;
  imageDesc.width = width;
  imageDesc.height = height;
  imageDesc.depth = depth;
  imageDesc.arraylevels = 0;
  imageDesc.miplevels = 0;

  imageCreate(device, &imageDesc, image);
}

void ZeApp::imageCreate(ze_image_handle_t *image, uint32_t width,
                        uint32_t height, uint32_t depth) {
  assert(_devices.size() != 0);
  imageCreate(_devices[0], image, width, height, depth);
}

void ZeApp::imageDestroy(ze_image_handle_t image) {
  SUCCESS_OR_TERMINATE(zeImageDestroy(image));
}

void ZeApp::commandListCreate(ze_command_list_handle_t *phCommandList) {
  assert(_devices.size() != 0);
  commandListCreate(0, phCommandList);
}

void ZeApp::commandListCreate(uint32_t device_index,
                              ze_command_list_handle_t *phCommandList) {
  ze_command_list_desc_t command_list_description{};
  command_list_description.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
  command_list_description.pNext = nullptr;

  SUCCESS_OR_TERMINATE(
      zeCommandListCreate(contextFor(device_index), _devices[device_index],
                          &command_list_description, phCommandList));
}

void ZeApp::commandListCreate(uint32_t device_index,
                              uint32_t command_queue_group_ordinal,
                              ze_command_list_handle_t *phCommandList) {
  ze_command_list_desc_t command_list_description{};
  command_list_description.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
  command_list_description.commandQueueGroupOrdinal =
      command_queue_group_ordinal;
  command_list_description.pNext = nullptr;

  SUCCESS_OR_TERMINATE(
      zeCommandListCreate(contextFor(device_index), _devices[device_index],
                          &command_list_description, phCommandList));
}

void ZeApp::immediateCommandListCreate(
    uint32_t device_index, ze_command_list_handle_t *phCommandList) {
  SUCCESS_OR_TERMINATE(zeCommandListCreateImmediate(
      contextFor(device_index), _devices[device_index],
      &zeDefaultGPUImmediateCommandQueueDesc, phCommandList));
}

void ZeApp::immediateCommandListCreate(
    uint32_t device_index, uint32_t command_queue_group_ordinal,
    uint32_t command_queue_index, ze_command_list_handle_t *phCommandList) {
  ze_command_queue_desc_t command_queue_description{};
  command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  command_queue_description.pNext = nullptr;
  command_queue_description.ordinal = command_queue_group_ordinal;
  command_queue_description.index = command_queue_index;
  command_queue_description.flags =
      ZE_COMMAND_QUEUE_FLAG_IN_ORDER | ZE_COMMAND_QUEUE_FLAG_COPY_OFFLOAD_HINT;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
  command_queue_description.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
  SUCCESS_OR_TERMINATE(zeCommandListCreateImmediate(
      contextFor(device_index), _devices[device_index],
      &command_queue_description, phCommandList));
}

void ZeApp::commandListDestroy(ze_command_list_handle_t command_list) {
  SUCCESS_OR_TERMINATE(zeCommandListDestroy(command_list));
}

void ZeApp::commandListClose(ze_command_list_handle_t command_list) {
  SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
}

void ZeApp::commandListReset(ze_command_list_handle_t command_list) {
  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));
}

void ZeApp::getIpcHandle(void *ptr, ze_ipc_mem_handle_t *pIpcHandle) {
  SUCCESS_OR_TERMINATE(zeMemGetIpcHandle(context, ptr, pIpcHandle));
}

void ZeApp::closeIpcHandle(void *ipc_ptr) {
  SUCCESS_OR_TERMINATE(zeMemCloseIpcHandle(context, ipc_ptr));
}

void ZeApp::commandListAppendImageCopyFromMemory(
    ze_command_list_handle_t command_list, ze_image_handle_t image,
    uint8_t *srcBuffer, ze_image_region_t *Region) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendImageCopyFromMemory(
      command_list, image, srcBuffer, Region, nullptr, 0, nullptr));
}

void ZeApp::commandListAppendImageCopyFromMemory(
    ze_command_list_handle_t command_list, ze_image_handle_t image,
    uint8_t *srcBuffer, ze_image_region_t *Region, ze_event_handle_t hEvent) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendImageCopyFromMemory(
      command_list, image, srcBuffer, Region, hEvent, 0, nullptr));
}

void ZeApp::commandListAppendBarrier(ze_command_list_handle_t command_list) {
  SUCCESS_OR_TERMINATE(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
}

void ZeApp::commandListAppendImageCopyToMemory(
    ze_command_list_handle_t command_list, uint8_t *dstBuffer,
    ze_image_handle_t image, ze_image_region_t *Region) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendImageCopyToMemory(
      command_list, dstBuffer, image, Region, nullptr, 0, nullptr));
}

void ZeApp::commandListAppendImageCopyToMemory(
    ze_command_list_handle_t command_list, uint8_t *dstBuffer,
    ze_image_handle_t image, ze_image_region_t *Region,
    ze_event_handle_t hEvent) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendImageCopyToMemory(
      command_list, dstBuffer, image, Region, hEvent, 0, nullptr));
}

void ZeApp::commandListAppendMemoryCopy(ze_command_list_handle_t command_list,
                                        void *dstptr, void *srcptr,
                                        size_t size) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
      command_list, dstptr, srcptr, size, nullptr, 0, nullptr));
}

void ZeApp::commandListAppendWaitOnEvents(ze_command_list_handle_t CommandList,
                                          uint32_t numEvents,
                                          ze_event_handle_t *phEvents) {
  SUCCESS_OR_TERMINATE(
      zeCommandListAppendWaitOnEvents(CommandList, numEvents, phEvents));
}

void ZeApp::commandListAppendSignalEvent(ze_command_list_handle_t CommandList,
                                         ze_event_handle_t hEvent) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendSignalEvent(CommandList, hEvent));
}

void ZeApp::commandListAppendResetEvent(ze_command_list_handle_t CommandList,
                                        ze_event_handle_t hEvent) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendEventReset(CommandList, hEvent));
}

void ZeApp::hostEventSignal(ze_event_handle_t hEvent) {

  SUCCESS_OR_TERMINATE(zeEventHostSignal(hEvent));
}

void ZeApp::hostSynchronize(ze_event_handle_t hEvent, uint32_t timeout) {

  SUCCESS_OR_TERMINATE(zeEventHostSynchronize(hEvent, timeout));
}

void ZeApp::hostSynchronize(ze_event_handle_t hEvent) {

  SUCCESS_OR_TERMINATE(zeEventHostSynchronize(hEvent, ~0ULL));
}

void ZeApp::commandQueueCreate(const uint32_t command_queue_id,
                               ze_command_queue_handle_t *command_queue) {
  assert(_devices.size() != 0);
  commandQueueCreate(0, command_queue_id, command_queue);
}

void ZeApp::commandQueueCreate(const uint32_t device_index,
                               const uint32_t command_queue_group_ordinal,
                               ze_command_queue_handle_t *command_queue) {
  ze_command_queue_desc_t command_queue_description{};
  command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  command_queue_description.pNext = nullptr;
  command_queue_description.ordinal = command_queue_group_ordinal;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

  SUCCESS_OR_TERMINATE(
      zeCommandQueueCreate(contextFor(device_index), _devices[device_index],
                           &command_queue_description, command_queue));
}

void ZeApp::commandQueueCreate(const uint32_t device_index,
                               const uint32_t command_queue_group_ordinal,
                               const uint32_t command_queue_index,
                               ze_command_queue_handle_t *command_queue) {
  ze_command_queue_desc_t command_queue_description{};
  command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  command_queue_description.pNext = nullptr;
  command_queue_description.ordinal = command_queue_group_ordinal;
  command_queue_description.index = command_queue_index;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

  SUCCESS_OR_TERMINATE(
      zeCommandQueueCreate(contextFor(device_index), _devices[device_index],
                           &command_queue_description, command_queue));
}

void ZeApp::commandQueueDestroy(ze_command_queue_handle_t command_queue) {
  SUCCESS_OR_TERMINATE(zeCommandQueueDestroy(command_queue));
}

void ZeApp::commandQueueExecuteCommandList(
    ze_command_queue_handle_t command_queue, uint32_t numCommandLists,
    ze_command_list_handle_t *command_lists) {
  SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
      command_queue, numCommandLists, command_lists, nullptr));
}
ze_event_pool_handle_t ZeApp::create_event_pool(uint32_t count,
                                                ze_event_pool_flags_t flags) {
  ze_event_pool_handle_t event_pool;
  ze_event_pool_desc_t descriptor = {};
  descriptor.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;

  descriptor.pNext = nullptr;
  descriptor.flags = flags;
  descriptor.count = count;

  return create_event_pool(descriptor);
}

ze_event_pool_handle_t ZeApp::create_event_pool(ze_event_pool_desc_t desc) {
  ze_event_pool_handle_t event_pool;

  SUCCESS_OR_TERMINATE(
      zeEventPoolCreate(context, &desc, 1, &_devices[0], &event_pool));

  return event_pool;
}

void ZeApp::destroy_event_pool(ze_event_pool_handle_t event_pool) {
  SUCCESS_OR_TERMINATE(zeEventPoolDestroy(event_pool));
}

void ZeApp::create_event(ze_event_pool_handle_t event_pool,
                         ze_event_handle_t &event, uint32_t index) {
  ze_event_desc_t desc = {};
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  memset(&desc, 0, sizeof(desc));

  desc.pNext = nullptr;
  desc.signal = 0;
  desc.wait = 0;
  event = nullptr;
  desc.index = index;
  SUCCESS_OR_TERMINATE(zeEventCreate(event_pool, &desc, &event));
}

void ZeApp::destroy_event(ze_event_handle_t event) {
  SUCCESS_OR_TERMINATE(zeEventDestroy(event));
}

void ZeApp::commandQueueSynchronize(ze_command_queue_handle_t command_queue) {
  SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(command_queue, UINT64_MAX));
}

void ZeApp::initCountDevices(const uint32_t count) {
  uint32_t driver_count = 0;

  SUCCESS_OR_TERMINATE(zeDriverGet(&driver_count, nullptr));
  if (driver_count == 0) {
    std::cerr << "ERROR: zero drivers were found" << std::endl;
    std::terminate();
  }

  driver_count = 1;
  ze_driver_handle_t driver;
  SUCCESS_OR_TERMINATE(zeDriverGet(&driver_count, &driver));
  _driver = driver;

  ze_context_desc_t context_desc = {};
  context_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
  SUCCESS_OR_TERMINATE(zeContextCreate(driver, &context_desc, &context));

  uint32_t device_count = deviceCount(driver);
  assert(device_count > 0);
  if (count != 0)
    device_count = std::min(count, device_count);

  _devices.resize(device_count);

  driverGetDevices(driver, device_count, _devices.data());

  if (_module_path.size() != 0) {
    _modules.resize(device_count);
    for (uint32_t i = 0U; i < device_count; i++) {
      moduleCreate(_devices[i], &_modules[i]);
    }
  }
}

void ZeApp::singleDeviceInit(void) { initCountDevices(1); }

uint32_t ZeApp::allDevicesInit(void) {
  initCountDevices(0);
  return static_cast<uint32_t>(_devices.size());
}

void ZeApp::cleanupDevices(void) {
  if (_module_path.size() != 0) {
    for (size_t i = 0U; i < _devices.size(); i++) {
      moduleDestroy(_modules[i]);
    }
  }

  // Anything the tool imported and did not free has to be handed back before
  // the driver goes away, or the release would run against a dead driver.
  for (const auto &allocation : _imported_host_allocations) {
    void *ptr = const_cast<void *>(allocation.first);
    if (_release_pointer != nullptr) {
      _release_pointer(_driver, ptr);
    }
    numa_support::free_on_node(ptr, allocation.second);
  }
  _imported_host_allocations.clear();

  zeContextDestroy(this->context);

  for (auto device_context : _device_contexts) {
    zeContextDestroy(device_context);
  }
  _device_contexts.clear();
  _allocation_contexts.clear();
}

void ZeApp::singleDeviceCleanup(void) { cleanupDevices(); }

void ZeApp::allDevicesCleanup(void) { cleanupDevices(); }

/* Retrieve array of devices in driver */
void ZeApp::driverGetDevices(ze_driver_handle_t driver, uint32_t device_count,
                             ze_device_handle_t *devices) {
  SUCCESS_OR_TERMINATE(zeDeviceGet(driver, &device_count, devices));
}

uint32_t ZeApp::deviceCount(ze_driver_handle_t driver) {
  uint32_t device_count = 0;

  SUCCESS_OR_TERMINATE(zeDeviceGet(driver, &device_count, nullptr));
  if (device_count == 0) {
    std::cerr << "ERROR: zero devices were found" << std::endl;
    std::terminate();
  }

  return device_count;
}
