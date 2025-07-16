/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class HandleTests : public ::testing::Test {

protected:
  ze_context_handle_t invalid_context = (ze_context_handle_t)0xaaaa;
  ze_driver_handle_t invalid_driver = (ze_driver_handle_t)0xaaaa;
  ze_device_handle_t invalid_device = (ze_device_handle_t)0xaaaa;
  ze_command_queue_handle_t invalid_command_queue =
      (ze_command_queue_handle_t)0xaaaa;
  ze_command_list_handle_t invalid_command_list =
      (ze_command_list_handle_t)0xaaaa;
  ze_fence_handle_t invalid_fence = (ze_fence_handle_t)0xaaaa;
  ze_event_pool_handle_t invalid_event_pool = (ze_event_pool_handle_t)0xaaaa;
  ze_event_handle_t invalid_event = (ze_event_handle_t)0xaaaa;
  ze_image_handle_t invalid_image = (ze_image_handle_t)0xaaaa;
  ze_module_handle_t invalid_module = (ze_module_handle_t)0xaaaa;
  ze_module_build_log_handle_t invalid_module_build_log =
      (ze_module_build_log_handle_t)0xaaaa;
  ze_kernel_handle_t invalid_kernel = (ze_kernel_handle_t)0xaaaa;
  ze_sampler_handle_t invalid_sampler = (ze_sampler_handle_t)0xaaaa;
  ze_physical_mem_handle_t invalid_physical_mem =
      (ze_physical_mem_handle_t)0xaaaa;
  ze_fabric_vertex_handle_t invalid_fabric_vertex =
      (ze_fabric_vertex_handle_t)0xaaaa;
  ze_fabric_edge_handle_t invalid_fabric_edge = (ze_fabric_edge_handle_t)0xaaaa;
  ze_ipc_mem_handle_t invalid_ipc_mem;
  ze_ipc_event_pool_handle_t invalid_ipc_event_pool;
  ze_external_memory_import_win32_handle_t
      invalid_external_memory_import_win32_handle;
  ze_external_memory_export_win32_handle_t
      invalid_external_memory_export_win32_handle;

  zet_driver_handle_t invalid_zet_driver = (zet_driver_handle_t)0xaaaa;
  zet_device_handle_t invalid_zet_device = (zet_device_handle_t)0xaaaa;
  zet_context_handle_t invalid_zet_context = (zet_context_handle_t)0xaaaa;
  zet_command_list_handle_t invalid_zet_command_list =
      (zet_command_list_handle_t)0xaaaa;
  zet_module_handle_t invalid_zet_module = (zet_module_handle_t)0xaaaa;
  zet_kernel_handle_t invalid_zet_kernel = (zet_kernel_handle_t)0xaaaa;
  zet_metric_group_handle_t invalid_zet_metric_group =
      (zet_metric_group_handle_t)0xaaaa;
  zet_metric_handle_t invalid_zet_metric = (zet_metric_handle_t)0xaaaa;
  zet_metric_streamer_handle_t invalid_zet_metric_streamer =
      (zet_metric_streamer_handle_t)0xaaaa;
  zet_metric_query_pool_handle_t invalid_zet_metric_query_pool =
      (zet_metric_query_pool_handle_t)0xaaaa;
  zet_metric_query_handle_t invalid_zet_metric_query =
      (zet_metric_query_handle_t)0xaaaa;
  zet_tracer_exp_handle_t invalid_zet_tracer_exp =
      (zet_tracer_exp_handle_t)0xaaaa;
  zet_debug_session_handle_t invalid_zet_debug_session =
      (zet_debug_session_handle_t)0xaaaa;

  zes_driver_handle_t invalid_zes_driver = (zes_driver_handle_t)0xaaaa;
  zes_device_handle_t invalid_zes_device = (zes_device_handle_t)0xaaaa;
  zes_sched_handle_t invalid_zes_sched = (zes_sched_handle_t)0xaaaa;
  zes_perf_handle_t invalid_zes_perf = (zes_perf_handle_t)0xaaaa;
  zes_pwr_handle_t invalid_zes_pwr = (zes_pwr_handle_t)0xaaaa;
  zes_freq_handle_t invalid_zes_freq = (zes_freq_handle_t)0xaaaa;
  zes_engine_handle_t invalid_zes_engine = (zes_engine_handle_t)0xaaaa;
  zes_standby_handle_t invalid_zes_standby = (zes_standby_handle_t)0xaaaa;
  zes_firmware_handle_t invalid_zes_firmware = (zes_firmware_handle_t)0xaaaa;
  zes_mem_handle_t invalid_zes_mem = (zes_mem_handle_t)0xaaaa;
  zes_fabric_port_handle_t invalid_zes_fabric_port =
      (zes_fabric_port_handle_t)0xaaaa;
  zes_temp_handle_t invalid_zes_temp = (zes_temp_handle_t)0xaaaa;
  zes_psu_handle_t invalid_zes_psu = (zes_psu_handle_t)0xaaaa;
  zes_fan_handle_t invalid_zes_fan = (zes_fan_handle_t)0xaaaa;
  zes_led_handle_t invalid_zes_led = (zes_led_handle_t)0xaaaa;
  zes_ras_handle_t invalid_zes_ras = (zes_ras_handle_t)0xaaaa;
  zes_diag_handle_t invalid_zes_diag = (zes_diag_handle_t)0xaaaa;
  zes_overclock_handle_t invalid_zes_overclock = (zes_overclock_handle_t)0xaaaa;
};

LZT_TEST_F(
    HandleTests,
    GivenUninitializedDriverHandleWhenCreatingContextThenHandleTrackingReturnsInvalidNullHandle) {

  ze_driver_handle_t driver;
  ze_context_handle_t context;
  ze_context_desc_t desc = {};
  desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeContextCreate(driver, &desc, &context));
}

LZT_TEST_F(
    HandleTests,
    GivenCompleteApplicationWhenHandleTrackingEnabledThenHandleTrackingCatchesCoreErrors) {

  auto drivers = lzt::get_all_driver_handles();
  LOG_DEBUG << "Drivers Found: " << drivers.size() << "\n";
  ASSERT_GT(drivers.size(), 0);
  auto driver = drivers[0];
  auto context = lzt::create_context(driver);

  uint32_t count = 0;
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeDeviceGet(invalid_driver, &count, nullptr));
  ASSERT_ZE_RESULT_SUCCESS(zeDeviceGet(driver, &count, nullptr));
  std::vector<ze_device_handle_t> devices(count);
  ASSERT_ZE_RESULT_SUCCESS(zeDeviceGet(driver, &count, devices.data()));

  auto device = devices[0];

  ze_command_queue_handle_t command_queue;
  ze_command_queue_desc_t command_queue_desc = {};
  command_queue_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueCreate(invalid_context, device, &command_queue_desc,
                                 &command_queue));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueCreate(context, invalid_device, &command_queue_desc,
                                 &command_queue));
  ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(
      context, device, &command_queue_desc, &command_queue));

  ze_command_list_desc_t command_list_desc = {};
  command_list_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
  command_list_desc.commandQueueGroupOrdinal = 0;
  command_list_desc.flags = 0;

  ze_command_list_handle_t command_list;
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListCreate(invalid_context, device, &command_list_desc,
                                &command_list));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListCreate(context, invalid_device, &command_list_desc,
                                &command_list));
  ASSERT_ZE_RESULT_SUCCESS(

      zeCommandListCreate(context, device, &command_list_desc, &command_list));

  auto module_name = "handle_tracking_add.spv";
  auto binary_file = lzt::load_binary_file(module_name);
  ze_module_desc_t module_desc = {};
  module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
  module_desc.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_desc.inputSize = binary_file.size();
  module_desc.pInputModule = binary_file.data();
  module_desc.pBuildFlags = "-g";

  ze_module_handle_t module;
  ASSERT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeModuleCreate(invalid_context, device, &module_desc, &module, nullptr));
  ASSERT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeModuleCreate(context, invalid_device, &module_desc, &module, nullptr));
  ASSERT_ZE_RESULT_SUCCESS(
      zeModuleCreate(context, device, &module_desc, &module, nullptr));

  auto kernel_name = "add_constant_2";
  ze_kernel_desc_t kernel_desc = {};
  kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
  kernel_desc.pKernelName = kernel_name;

  ze_kernel_handle_t kernel;
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeKernelCreate(invalid_module, &kernel_desc, &kernel));
  ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernel_desc, &kernel));

  auto size = 8192;
  ze_kernel_properties_t kernel_properties = {
      ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES, nullptr};
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeKernelGetProperties(invalid_kernel, &kernel_properties));
  ASSERT_ZE_RESULT_SUCCESS(zeKernelGetProperties(kernel, &kernel_properties));

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  void *buffer_a = nullptr, *buffer_b = nullptr;

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeMemAllocShared(invalid_context, &device_desc, &host_desc, size, 1,
                             device, &buffer_a));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeMemAllocShared(context, &device_desc, &host_desc, size, 1,
                             invalid_device, &buffer_a));
  ASSERT_ZE_RESULT_SUCCESS(zeMemAllocShared(context, &device_desc, &host_desc,
                                            size, 1, device, &buffer_a));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeMemAllocDevice(invalid_context, &device_desc, size, 1, device,
                             &buffer_b));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeMemAllocDevice(context, &device_desc, size, 1, invalid_device,
                             &buffer_b));
  ASSERT_ZE_RESULT_SUCCESS(
      zeMemAllocDevice(context, &device_desc, size, 1, device, &buffer_b));

  std::memset(buffer_a, 0, size);
  for (size_t i = 0; i < size; i++) {
    static_cast<uint8_t *>(buffer_a)[i] = (i & 0xFF);
  }

  const int addval = 3;
  ASSERT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeKernelSetArgumentValue(invalid_kernel, 0, sizeof(buffer_b), &buffer_b));
  ASSERT_ZE_RESULT_SUCCESS(
      zeKernelSetArgumentValue(kernel, 0, sizeof(buffer_b), &buffer_b));

  ASSERT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeKernelSetArgumentValue(invalid_kernel, 1, sizeof(addval), &addval));
  ASSERT_ZE_RESULT_SUCCESS(
      zeKernelSetArgumentValue(kernel, 1, sizeof(addval), &addval));

  uint32_t group_size_x = 1;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeKernelSuggestGroupSize(invalid_kernel, size, 1, 1, &group_size_x,
                                     &group_size_y, &group_size_z));
  ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(
      kernel, size, 1, 1, &group_size_x, &group_size_y, &group_size_z));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeKernelSetGroupSize(invalid_kernel, group_size_x, group_size_y,
                                 group_size_z));
  ASSERT_ZE_RESULT_SUCCESS(
      zeKernelSetGroupSize(kernel, group_size_x, group_size_y, group_size_z));

  ze_group_count_t group_count = {};
  group_count.groupCountX = size / group_size_x;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListAppendMemoryCopy(invalid_command_list, buffer_b,
                                          buffer_a, size, nullptr, 0, nullptr));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListAppendMemoryCopy(command_list, buffer_b, buffer_a,
                                          size, invalid_event, 0, nullptr));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListAppendMemoryCopy(command_list, buffer_b, buffer_a,
                                          size, nullptr, 1, &invalid_event));
  // TODO: add test for zeCommandListAppendMemoryCopy with list of multiple wait
  // events containing an invalid event
  ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(
      command_list, buffer_b, buffer_a, size, nullptr, 0, nullptr));

  ASSERT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeCommandListAppendBarrier(invalid_command_list, nullptr, 0, nullptr));
  ASSERT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeCommandListAppendBarrier(command_list, invalid_event, 0, nullptr));
  ASSERT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeCommandListAppendBarrier(command_list, nullptr, 1, &invalid_event));
  ASSERT_ZE_RESULT_SUCCESS(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListAppendLaunchKernel(invalid_command_list, kernel,
                                            &group_count, nullptr, 0, nullptr));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListAppendLaunchKernel(command_list, invalid_kernel,
                                            &group_count, nullptr, 0, nullptr));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListAppendLaunchKernel(command_list, kernel, &group_count,
                                            invalid_event, 0, nullptr));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListAppendLaunchKernel(command_list, kernel, &group_count,
                                            nullptr, 1, &invalid_event));
  ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
      command_list, kernel, &group_count, nullptr, 0, nullptr));
  ASSERT_ZE_RESULT_SUCCESS(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));

  ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(
      command_list, buffer_a, buffer_b, size, nullptr, 0, nullptr));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueExecuteCommandLists(invalid_command_queue, 1,
                                              &command_list, nullptr));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueExecuteCommandLists(command_queue, 1,
                                              &invalid_command_list, nullptr));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
            zeCommandQueueExecuteCommandLists(command_queue, 1, &command_list,
                                              nullptr));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListClose(invalid_command_list));
  ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(command_list));

  ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(
      command_queue, 1, &command_list, nullptr));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueSynchronize(invalid_command_queue, UINT64_MAX));
  ASSERT_ZE_RESULT_SUCCESS(
      zeCommandQueueSynchronize(command_queue, UINT64_MAX));

  // validation
  LOG_DEBUG << "Validating results";
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer_a)[i],
              static_cast<uint8_t>((i & 0xFF) + addval));
    if (::testing::Test::HasFailure()) {
      LOG_ERROR << "[Application] Sanity check did not pass";
      break;
    }
  }

  // cleanup
  LOG_INFO << "Verifying cleanup and loader returns errors when using "
              "destroyed objects";
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeMemFree(invalid_context, buffer_a));
  ASSERT_ZE_RESULT_SUCCESS(zeMemFree(context, buffer_a));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeMemFree(invalid_context, buffer_b));
  ASSERT_ZE_RESULT_SUCCESS(zeMemFree(context, buffer_b));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeKernelDestroy(invalid_kernel));
  ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));

  // attempt to use kernel after it has been destroyed
  ASSERT_EQ(
      ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
      zeKernelSetGroupSize(kernel, group_size_x, group_size_y, group_size_z));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeModuleDestroy(invalid_module));
  ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

  // attempt to use module after it has been destroyed
  uint32_t kernel_names_count = 0;
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeModuleGetKernelNames(module, &kernel_names_count, nullptr));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListDestroy(invalid_command_list));
  ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(command_list));

  // attempt to use command list after it has been destroyed
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandListReset(command_list));

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueDestroy(invalid_command_queue));
  ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(command_queue));

  // attempt to use command queue after it has been destroyed
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueSynchronize(command_queue, 10000));
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeCommandQueueExecuteCommandLists(command_queue, 1, &command_list,
                                              nullptr));
}
} // namespace
