/*
 *
 * Copyright (C) 2019 Intel Corporation
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

  void memoryAlloc(size_t size, void **ptr);
  void memoryAlloc(ze_driver_handle_t driver, ze_device_handle_t device,
                   size_t size, void **ptr);
  void memoryAllocHost(size_t size, void **ptr);
  void memoryAllocHost(ze_driver_handle_t driver, size_t size, void **ptr);
  void memoryFree(const void *ptr);
  void memoryFree(ze_driver_handle_t driver, const void *ptr);
  void functionCreate(ze_kernel_handle_t *function, const char *pFunctionName);
  void functionCreate(ze_module_handle_t module, ze_kernel_handle_t *function,
                      const char *pFunctionName);
  void functionDestroy(ze_kernel_handle_t function);
  void imageCreate(const ze_image_desc_t *imageDesc, ze_image_handle_t *image);
  void imageCreate(ze_device_handle_t device, const ze_image_desc_t *imageDesc,
                   ze_image_handle_t *image);
  void imageCreate(ze_device_handle_t device, ze_image_handle_t *image,
                   uint32_t width, uint32_t height, uint32_t depth);
  void imageCreate(ze_image_handle_t *image, uint32_t width, uint32_t height,
                   uint32_t depth);
  void imageDestroy(ze_image_handle_t image);
  void commandListCreate(ze_command_list_handle_t *phCommandList);
  void commandListCreate(ze_device_handle_t device,
                         ze_command_list_handle_t *phCommandList);
  void commandListDestroy(ze_command_list_handle_t phCommandList);
  void commandListClose(ze_command_list_handle_t phCommandList);
  void commandListReset(ze_command_list_handle_t phCommandList);
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
  void commandQueueCreate(ze_device_handle_t device,
                          const uint32_t command_queue_id,
                          ze_command_queue_handle_t *command_queue);
  void commandQueueDestroy(ze_command_queue_handle_t command_queue);
  void commandQueueExecuteCommandList(ze_command_queue_handle_t command_queue,
                                      uint32_t numCommandLists,
                                      ze_command_list_handle_t *command_lists);
  void commandQueueSynchronize(ze_command_queue_handle_t command_queue);
  void moduleCreate(ze_device_handle_t device, ze_module_handle_t *module);
  void moduleDestroy(ze_module_handle_t module);
  ze_event_pool_handle_t create_event_pool(uint32_t count,
                                           ze_event_pool_flag_t flags);

  ze_event_pool_handle_t create_event_pool(ze_event_pool_desc_t desc);

  void destroy_event_pool(ze_event_pool_handle_t event_pool);

  void create_event(ze_event_pool_handle_t event_pool, ze_event_handle_t &event,
                    uint32_t index);

  void destroy_event(ze_event_handle_t event);

  void singleDeviceInit(void);
  void singleDeviceCleanup(void);

  uint32_t driverCount(void);
  void driverGet(uint32_t *driver_count, ze_driver_handle_t *driver);
  void driverGetDevices(ze_driver_handle_t driver, uint32_t device_count,
                        ze_device_handle_t *devices);
  uint32_t deviceCount(ze_driver_handle_t driver);

  ze_driver_handle_t driver;
  ze_device_handle_t device;
  ze_module_handle_t module;

private:
  std::string module_path;
  std::vector<uint8_t> binary_file;
  std::vector<uint8_t> load_binary_file(const std::string &file_path);
};
#endif /* _ZE_APP_HPP_*/