/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_app.hpp"

#include "common.hpp"

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
  stream.read(reinterpret_cast<char *>(binary_file.data()), length);
  if (verbose)
    std::cout << "Binary file loaded" << std::endl;

  return binary_file;
}

ZeApp::ZeApp(void) {
  device = nullptr;
  module = nullptr;
  driver = nullptr;
  this->module_path = "";

  SUCCESS_OR_TERMINATE(zeInit(ZE_INIT_FLAG_NONE));
}

ZeApp::ZeApp(std::string module_path) {
  device = nullptr;
  module = nullptr;
  driver = nullptr;
  this->module_path = module_path;

  SUCCESS_OR_TERMINATE(zeInit(ZE_INIT_FLAG_NONE));

  binary_file = load_binary_file(this->module_path);
  std::cout << std::endl;
}

void ZeApp::moduleCreate(ze_device_handle_t device,
                         ze_module_handle_t *module) {
  ze_module_desc_t module_description = {};
  module_description.version = ZE_MODULE_DESC_VERSION_CURRENT;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = binary_file.size();
  module_description.pInputModule = binary_file.data();
  module_description.pBuildFlags = nullptr;

  SUCCESS_OR_TERMINATE(
      zeModuleCreate(device, &module_description, module, nullptr));
}

void ZeApp::moduleDestroy(ze_module_handle_t module) {
  SUCCESS_OR_TERMINATE(zeModuleDestroy(module));
}

ZeApp::~ZeApp(void) {}

void ZeApp::memoryAlloc(size_t size, void **ptr) {
  assert(this->device != nullptr);
  assert(this->driver != nullptr);
  memoryAlloc(this->driver, this->device, size, ptr);
}

void ZeApp::memoryAlloc(ze_driver_handle_t driver, ze_device_handle_t device,
                        size_t size, void **ptr) {
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
  device_desc.ordinal = 0;
  device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
  SUCCESS_OR_TERMINATE(
      zeDriverAllocDeviceMem(driver, &device_desc, size, 1, device, ptr));
}

void ZeApp::memoryAllocHost(size_t size, void **ptr) {
  assert(this->driver != nullptr);
  memoryAllocHost(this->driver, size, ptr);
}

void ZeApp::memoryAllocHost(ze_driver_handle_t driver, size_t size,
                            void **ptr) {
  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;
  host_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_DEFAULT;
  SUCCESS_OR_TERMINATE(zeDriverAllocHostMem(driver, &host_desc, size, 1, ptr));
}

void ZeApp::memoryFree(const void *ptr) {
  assert(this->driver != nullptr);
  SUCCESS_OR_TERMINATE(zeDriverFreeMem(this->driver, const_cast<void *>(ptr)));
}

void ZeApp::memoryFree(ze_driver_handle_t driver, const void *ptr) {
  SUCCESS_OR_TERMINATE(zeDriverFreeMem(driver, const_cast<void *>(ptr)));
}

void ZeApp::functionCreate(ze_kernel_handle_t *function,
                           const char *pFunctionName) {
  assert(this->module != nullptr);
  functionCreate(module, function, pFunctionName);
}

void ZeApp::functionCreate(ze_module_handle_t module,
                           ze_kernel_handle_t *function,
                           const char *pFunctionName) {
  ze_kernel_desc_t function_description = {};
  function_description.version = ZE_KERNEL_DESC_VERSION_CURRENT;
  function_description.flags = ZE_KERNEL_FLAG_NONE;
  function_description.pKernelName = pFunctionName;

  SUCCESS_OR_TERMINATE(zeKernelCreate(module, &function_description, function));
}

void ZeApp::functionDestroy(ze_kernel_handle_t function) {
  SUCCESS_OR_TERMINATE(zeKernelDestroy(function));
}

void ZeApp::imageCreate(const ze_image_desc_t *imageDesc,
                        ze_image_handle_t *image) {
  assert(this->device != nullptr);
  imageCreate(this->device, imageDesc, image);
}

void ZeApp::imageCreate(ze_device_handle_t device,
                        const ze_image_desc_t *imageDesc,
                        ze_image_handle_t *image) {
  SUCCESS_OR_TERMINATE(zeImageCreate(device, imageDesc, image));
}

void ZeApp::imageCreate(ze_device_handle_t device, ze_image_handle_t *image,
                        uint32_t width, uint32_t height, uint32_t depth) {

  ze_image_format_desc_t formatDesc = {
      ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_TYPE_FLOAT,
      ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_0,
      ZE_IMAGE_FORMAT_SWIZZLE_0, ZE_IMAGE_FORMAT_SWIZZLE_1};

  ze_image_desc_t imageDesc = {ZE_IMAGE_DESC_VERSION_CURRENT,
                               ZE_IMAGE_FLAG_PROGRAM_READ,
                               ZE_IMAGE_TYPE_2D,
                               formatDesc,
                               width,
                               height,
                               depth,
                               0,
                               0};

  imageCreate(device, &imageDesc, image);
}

void ZeApp::imageCreate(ze_image_handle_t *image, uint32_t width,
                        uint32_t height, uint32_t depth) {
  assert(this->device != nullptr);
  imageCreate(this->device, image, width, height, depth);
}

void ZeApp::imageDestroy(ze_image_handle_t image) {
  SUCCESS_OR_TERMINATE(zeImageDestroy(image));
}

void ZeApp::commandListCreate(ze_command_list_handle_t *phCommandList) {
  assert(this->device != nullptr);
  commandListCreate(this->device, phCommandList);
}

void ZeApp::commandListCreate(ze_device_handle_t device,
                              ze_command_list_handle_t *phCommandList) {
  ze_command_list_desc_t command_list_description{};
  command_list_description.version = ZE_COMMAND_LIST_DESC_VERSION_CURRENT;

  SUCCESS_OR_TERMINATE(
      zeCommandListCreate(device, &command_list_description, phCommandList));
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

void ZeApp::commandListAppendImageCopyFromMemory(
    ze_command_list_handle_t command_list, ze_image_handle_t image,
    uint8_t *srcBuffer, ze_image_region_t *Region) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendImageCopyFromMemory(
      command_list, image, srcBuffer, Region, nullptr));
}

void ZeApp::commandListAppendImageCopyFromMemory(
    ze_command_list_handle_t command_list, ze_image_handle_t image,
    uint8_t *srcBuffer, ze_image_region_t *Region, ze_event_handle_t hEvent) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendImageCopyFromMemory(
      command_list, image, srcBuffer, Region, hEvent));
}

void ZeApp::commandListAppendBarrier(ze_command_list_handle_t command_list) {
  SUCCESS_OR_TERMINATE(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
}

void ZeApp::commandListAppendImageCopyToMemory(
    ze_command_list_handle_t command_list, uint8_t *dstBuffer,
    ze_image_handle_t image, ze_image_region_t *Region) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendImageCopyToMemory(
      command_list, dstBuffer, image, Region, nullptr));
}

void ZeApp::commandListAppendImageCopyToMemory(
    ze_command_list_handle_t command_list, uint8_t *dstBuffer,
    ze_image_handle_t image, ze_image_region_t *Region,
    ze_event_handle_t hEvent) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendImageCopyToMemory(
      command_list, dstBuffer, image, Region, hEvent));
}

void ZeApp::commandListAppendMemoryCopy(ze_command_list_handle_t command_list,
                                        void *dstptr, void *srcptr,
                                        size_t size) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(command_list, dstptr,
                                                     srcptr, size, nullptr));
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

  SUCCESS_OR_TERMINATE(zeEventHostSynchronize(hEvent, ~0));
}

void ZeApp::commandQueueCreate(const uint32_t command_queue_id,
                               ze_command_queue_handle_t *command_queue) {
  assert(this->device != nullptr);
  commandQueueCreate(device, command_queue_id, command_queue);
}

void ZeApp::commandQueueCreate(ze_device_handle_t device,
                               const uint32_t command_queue_id,
                               ze_command_queue_handle_t *command_queue) {
  ze_command_queue_desc_t command_queue_description{};
  command_queue_description.version = ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT;
  command_queue_description.ordinal = command_queue_id;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

  SUCCESS_OR_TERMINATE(
      zeCommandQueueCreate(device, &command_queue_description, command_queue));
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
                                                ze_event_pool_flag_t flags) {
  ze_event_pool_handle_t event_pool;
  ze_event_pool_desc_t descriptor = {};

  descriptor.version = ZE_EVENT_POOL_DESC_VERSION_CURRENT;
  descriptor.flags = flags;
  descriptor.count = count;

  return create_event_pool(descriptor);
}

ze_event_pool_handle_t ZeApp::create_event_pool(ze_event_pool_desc_t desc) {
  ze_event_pool_handle_t event_pool;

  SUCCESS_OR_TERMINATE(
      zeEventPoolCreate(this->driver, &desc, 1, &this->device, &event_pool));

  return event_pool;
}

void ZeApp::destroy_event_pool(ze_event_pool_handle_t event_pool) {
  SUCCESS_OR_TERMINATE(zeEventPoolDestroy(event_pool));
}

void ZeApp::create_event(ze_event_pool_handle_t event_pool,
                         ze_event_handle_t &event, uint32_t index) {
  ze_event_desc_t desc = {};
  memset(&desc, 0, sizeof(desc));
  desc.version = ZE_EVENT_DESC_VERSION_CURRENT;
  desc.signal = ZE_EVENT_SCOPE_FLAG_NONE;
  desc.wait = ZE_EVENT_SCOPE_FLAG_NONE;
  event = nullptr;
  desc.index = index;
  SUCCESS_OR_TERMINATE(zeEventCreate(event_pool, &desc, &event));
}

void ZeApp::destroy_event(ze_event_handle_t event) {

  SUCCESS_OR_TERMINATE(zeEventDestroy(event));
}

void ZeApp::commandQueueSynchronize(ze_command_queue_handle_t command_queue) {
  SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(command_queue, UINT32_MAX));
}

void ZeApp::singleDeviceInit(void) {
  uint32_t driver_count = driverCount();
  assert(driver_count > 0);

  driver_count = 1;
  driverGet(&driver_count, &driver);

  uint32_t device_count = deviceCount(driver);
  assert(device_count > 0);
  device_count = 1;
  driverGetDevices(driver, device_count, &device);

  /*  if module paths string size is not 0 then only call modulecreate */
  if (this->module_path.size() != 0)
    moduleCreate(device, &module);
}

void ZeApp::singleDeviceCleanup(void) {

  /*  if module pathsstring size is not 0 then only call moduledestory */
  if (this->module_path.size() != 0)
    moduleDestroy(module);
}

uint32_t ZeApp::driverCount(void) {
  uint32_t driver_count = 0;

  SUCCESS_OR_TERMINATE(zeDriverGet(&driver_count, nullptr));

  if (driver_count == 0) {
    std::cerr << "ERROR: zero devices were found" << std::endl;
    std::terminate();
  }

  return driver_count;
}

/* Retrieve "driver_count" */
void ZeApp::driverGet(uint32_t *driver_count, ze_driver_handle_t *driver) {
  SUCCESS_OR_TERMINATE(zeDriverGet(driver_count, driver));
}

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
