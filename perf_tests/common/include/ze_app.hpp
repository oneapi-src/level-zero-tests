/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _ZE_APP_HPP_
#define _ZE_APP_HPP_

#include <level_zero/ze_api.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <vector>

class ZeApp {
public:
  ZeApp(void);
  ZeApp(std::string module_path);
  ~ZeApp();

  bool canAccessPeer(uint32_t device_index_0, uint32_t device_index_1);
  void memoryAlloc(size_t size, void **ptr);
  void memoryAlloc(const uint32_t device_index, size_t size, void **ptr);
  void memoryAllocHost(size_t size, void **ptr);
  // Host allocation placed on a NUMA node. Returns false, with the allocation
  // still usable, when the placement could not be applied; pass
  // numa_support::no_node to keep the kernel's own choice.
  bool memoryAllocHost(size_t size, void **ptr, int numa_node,
                       std::string *bind_error = nullptr);
  // Host allocation routed through one device's context. With per-device
  // contexts enabled the driver places the pages on that device's NUMA node,
  // which is the whole reason the mode exists.
  void memoryAllocHost(const uint32_t device_index, size_t size, void **ptr);
  //---------------------------------------------------------------------
  // Per-device contexts.
  //
  // The driver serves a host allocation from the node of the device it
  // routes the allocation through, and in a context holding several devices
  // that is always the first of them. Giving each device a context of its
  // own therefore places every host buffer on its own device's node, using
  // nothing outside the core API.
  //
  // Off by default: ZeApp::context stays the single shared context, which is
  // what every other perf test uses.
  //---------------------------------------------------------------------
  bool enablePerDeviceContexts(void);
  bool perDeviceContextsEnabled(void) const {
    return !_device_contexts.empty();
  }
  // Context owning a device's objects: the per-device one when enabled, the
  // shared one otherwise. Anything created from a context must be used only
  // with objects from the same context, events included.
  ze_context_handle_t contextFor(const uint32_t device_index) const;
  // Context an allocation was made from, for freeing it again.
  ze_context_handle_t contextOfAllocation(const void *ptr) const;
  // True when a host allocation can be placed on a NUMA node and still be
  // used by the driver, which needs the import extension.
  bool numaPlacedHostAllocationsSupported(void);
  // The driver supports system memory on copy operations only, so an imported
  // pointer is resolved correctly by zeCommandListAppendMemoryCopy but is not
  // a USM allocation and must not become a kernel argument. A caller whose
  // host buffers reach a kernel turns the import path off.
  void setHostAllocationImportEnabled(bool enabled) {
    _host_allocation_import_enabled = enabled;
  }
  void memoryAllocImportFd(const uint32_t device_index, size_t size, int fd,
                           ze_device_mem_alloc_flags_t flags, void **ptr);
  void memoryAllocHostImportFd(size_t size, int fd,
                               ze_host_mem_alloc_flags_t flags, void **ptr);
  void memoryGetAddressRange(void *ptr, void **base, size_t *size);
  void memoryFree(const void *ptr);
  void memoryOpenIpcHandle(const uint32_t device_index,
                           ze_ipc_mem_handle_t pIpcHandle, void **zeIpcBuffer);
  void deviceGetCommandQueueGroupProperties(
      const uint32_t device_index, uint32_t *numQueueGroups,
      ze_command_queue_group_properties_t *queueProperties);

  void functionCreate(ze_kernel_handle_t *function, const char *pFunctionName);
  void functionCreate(const uint32_t device_index, ze_kernel_handle_t *function,
                      const char *pFunctionName);
  void functionDestroy(ze_kernel_handle_t function);

  void imageCreate(const ze_image_desc_t *imageDesc, ze_image_handle_t *image);
  void imageCreate(ze_image_handle_t *image, uint32_t width, uint32_t height,
                   uint32_t depth);
  void imageDestroy(ze_image_handle_t image);

  void commandListCreate(ze_command_list_handle_t *phCommandList);
  void commandListCreate(uint32_t device_index,
                         ze_command_list_handle_t *phCommandList);
  void commandListCreate(uint32_t device_index,
                         uint32_t command_queue_group_ordinal,
                         ze_command_list_handle_t *phCommandList);
  void immediateCommandListCreate(uint32_t device_index,
                                  ze_command_list_handle_t *phCommandList);
  void immediateCommandListCreate(uint32_t device_index,
                                  uint32_t command_queue_group_ordinal,
                                  uint32_t command_queue_index,
                                  ze_command_list_handle_t *phCommandList);
  void commandListDestroy(ze_command_list_handle_t phCommandList);
  void commandListClose(ze_command_list_handle_t phCommandList);
  void commandListReset(ze_command_list_handle_t phCommandList);
  void getIpcHandle(void *ptr, ze_ipc_mem_handle_t *pIpcHandle);
  void closeIpcHandle(void *ipc_ptr);
  void commandListAppendImageCopyFromMemory(
      ze_command_list_handle_t command_list, ze_image_handle_t image,
      uint8_t *srcBuffer, ze_image_region_t *Region);
  void commandListAppendImageCopyFromMemory(
      ze_command_list_handle_t command_list, ze_image_handle_t image,
      uint8_t *srcBuffer, ze_image_region_t *Region, ze_event_handle_t hEvent);
  void commandListAppendMemoryCopy(ze_command_list_handle_t command_list,
                                   void *dstptr, void *srcptr, size_t size);
  void commandListAppendBarrier(ze_command_list_handle_t command_list);

  void commandListAppendImageCopyToMemory(ze_command_list_handle_t command_list,
                                          uint8_t *dstBuffer,
                                          ze_image_handle_t image,
                                          ze_image_region_t *Region);
  void commandListAppendImageCopyToMemory(ze_command_list_handle_t command_list,
                                          uint8_t *dstBuffer,
                                          ze_image_handle_t image,
                                          ze_image_region_t *Region,
                                          ze_event_handle_t hEvent);
  void commandListAppendWaitOnEvents(ze_command_list_handle_t CommandList,
                                     uint32_t numEvents,
                                     ze_event_handle_t *phEvents);
  void commandListAppendSignalEvent(ze_command_list_handle_t CommandList,
                                    ze_event_handle_t hEvent);
  void commandListAppendResetEvent(ze_command_list_handle_t CommandList,
                                   ze_event_handle_t hEvent);

  void hostEventSignal(ze_event_handle_t hEvent);
  void hostSynchronize(ze_event_handle_t hEvent, uint32_t timeout);
  void hostSynchronize(ze_event_handle_t hEvent);

  void commandQueueCreate(const uint32_t command_queue_id,
                          ze_command_queue_handle_t *command_queue);
  void commandQueueCreate(const uint32_t device_index,
                          const uint32_t command_queue_group_ordinal,
                          ze_command_queue_handle_t *command_queue);
  void commandQueueCreate(const uint32_t device_index,
                          const uint32_t command_queue_group_ordinal,
                          const uint32_t command_queue_index,
                          ze_command_queue_handle_t *command_queue);
  void commandQueueDestroy(ze_command_queue_handle_t command_queue);
  void commandQueueExecuteCommandList(ze_command_queue_handle_t command_queue,
                                      uint32_t numCommandLists,
                                      ze_command_list_handle_t *command_lists);
  void commandQueueSynchronize(ze_command_queue_handle_t command_queue);

  ze_event_pool_handle_t create_event_pool(uint32_t count,
                                           ze_event_pool_flags_t flags);
  ze_event_pool_handle_t create_event_pool(ze_event_pool_desc_t desc);
  void destroy_event_pool(ze_event_pool_handle_t event_pool);

  void create_event(ze_event_pool_handle_t event_pool, ze_event_handle_t &event,
                    uint32_t index);
  void destroy_event(ze_event_handle_t event);

  void singleDeviceInit(void);
  uint32_t allDevicesInit(void);
  void singleDeviceCleanup(void);
  void allDevicesCleanup(void);

  ze_context_handle_t context; // This is used directly by some perf_tests.
  std::vector<ze_device_handle_t> _devices;

private:
  ze_driver_handle_t _driver = nullptr;

  // One context per device when the mode is on, empty otherwise.
  std::vector<ze_context_handle_t> _device_contexts;
  // Which context an allocation came from, so that it can be freed against
  // that context. Only populated when per-device contexts are in use.
  std::map<const void *, ze_context_handle_t> _allocation_contexts;

  // Host allocations the tool placed itself and handed to the driver through
  // the import extension, with the length needed to unmap them again. They
  // are not driver allocations, so memoryFree() must not pass them to
  // zeMemFree().
  std::map<const void *, size_t> _imported_host_allocations;

  // Resolved once, on first use: the import extension is how a host buffer
  // whose NUMA placement the caller controls becomes usable by Level Zero.
  bool importExtensionAvailable(void);
  void rememberAllocationContext(const void *ptr, ze_context_handle_t owner);
  bool _import_extension_resolved = false;
  bool _host_allocation_import_enabled = true;
  ze_result_t (*_import_pointer)(ze_driver_handle_t, void *, size_t) = nullptr;
  ze_result_t (*_release_pointer)(ze_driver_handle_t, void *) = nullptr;

  std::vector<ze_module_handle_t> _modules;
  std::string _module_path;
  std::vector<uint8_t> _binary_file;

  std::vector<uint8_t> load_binary_file(const std::string &file_path);

  void imageCreate(ze_device_handle_t device, const ze_image_desc_t *imageDesc,
                   ze_image_handle_t *image);
  void imageCreate(ze_device_handle_t device, ze_image_handle_t *image,
                   uint32_t width, uint32_t height, uint32_t depth);

  void moduleCreate(ze_device_handle_t device, ze_module_handle_t *module);
  void moduleDestroy(ze_module_handle_t module);

  void initCountDevices(const uint32_t count);
  void cleanupDevices(void);

  void driverGetDevices(ze_driver_handle_t driver, uint32_t device_count,
                        ze_device_handle_t *devices);
  uint32_t deviceCount(ze_driver_handle_t driver);
};

#endif /* _ZE_APP_HPP_*/
