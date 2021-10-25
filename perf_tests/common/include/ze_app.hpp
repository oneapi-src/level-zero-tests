/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _ZE_APP_HPP_
#define _ZE_APP_HPP_

#include <level_zero/ze_api.h>

#include <fstream>
#include <iostream>
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
