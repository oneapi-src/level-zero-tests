/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness_device.hpp"
#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zeDevice *zeDevice::instance_ = nullptr;
std::once_flag zeDevice::instance;

zeDevice *zeDevice::get_instance() {
  std::call_once(instance, []() {
    instance_ = new zeDevice;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

    instance_->driver_ = lzt::get_default_driver();
    instance_->device_ = lzt::get_default_device(instance_->driver_);
  });
  return instance_;
}

ze_device_handle_t zeDevice::get_device() { return get_instance()->device_; }
ze_driver_handle_t zeDevice::get_driver() { return get_instance()->driver_; }

zeDevice::zeDevice() {
  device_ = nullptr;
  driver_ = nullptr;
}

uint32_t get_ze_device_count() {
  return get_ze_device_count(lzt::get_default_driver());
}

uint32_t get_ze_device_count(ze_driver_handle_t driver) {
  uint32_t count = 0;
  auto driver_initial = driver;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGet(driver, &count, nullptr));
  EXPECT_EQ(driver, driver_initial);

  return count;
}

std::vector<ze_device_handle_t> get_ze_devices() {
  return get_ze_devices(get_ze_device_count());
}

std::vector<ze_device_handle_t> get_ze_devices(uint32_t count) {
  return get_ze_devices(count, lzt::get_default_driver());
}

std::vector<ze_device_handle_t> get_ze_devices(ze_driver_handle_t driver) {
  return get_ze_devices(get_ze_device_count(driver), driver);
}

std::vector<ze_device_handle_t> get_ze_devices(uint32_t count,
                                               ze_driver_handle_t driver) {
  uint32_t count_out = count;
  std::vector<ze_device_handle_t> devices(count);

  auto driver_initial = driver;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGet(driver, &count_out, devices.data()));
  EXPECT_EQ(driver, driver_initial);
  if (count == get_ze_device_count())
    EXPECT_EQ(count_out, count);

  return devices;
}

uint32_t get_ze_sub_device_count(ze_device_handle_t device) {
  uint32_t count = 0;

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetSubDevices(device, &count, nullptr));
  EXPECT_EQ(device, device_initial);
  return count;
}

std::vector<ze_device_handle_t> get_ze_sub_devices(ze_device_handle_t device) {
  return get_ze_sub_devices(device, get_ze_sub_device_count(device));
}

std::vector<ze_device_handle_t> get_ze_sub_devices(ze_device_handle_t device,
                                                   uint32_t count) {
  std::vector<ze_device_handle_t> sub_devices(count);

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetSubDevices(device, &count, sub_devices.data()));
  EXPECT_EQ(device, device_initial);
  return sub_devices;
}

ze_device_properties_t get_device_properties(ze_device_handle_t device) {
  ze_structure_type_t stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  return get_device_properties(device, stype);
}

ze_device_properties_t get_device_properties(ze_device_handle_t device,
                                             ze_structure_type_t stype) {
  auto device_initial = device;
  ze_device_properties_t properties = {};
  memset(&properties, 0, sizeof(properties));
  if (stype == ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2) {
    properties.stype = stype;
  } else {
    properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  }
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &properties));
  EXPECT_EQ(device, device_initial);
  return properties;
}

ze_device_compute_properties_t
get_compute_properties(ze_device_handle_t device) {
  ze_device_compute_properties_t properties;
  memset(&properties, 0, sizeof(properties));
  properties = {ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES};

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetComputeProperties(device, &properties));
  EXPECT_EQ(device, device_initial);
  return properties;
}

uint32_t get_memory_properties_count(ze_device_handle_t device) {
  uint32_t count = 0;

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetMemoryProperties(device, &count, nullptr));
  EXPECT_EQ(device, device_initial);
  return count;
}

std::vector<ze_device_memory_properties_t>
get_memory_properties(ze_device_handle_t device) {
  return get_memory_properties(device, get_memory_properties_count(device));
}

std::vector<ze_device_memory_properties_t>
get_memory_properties(ze_device_handle_t device, uint32_t count) {
  std::vector<ze_device_memory_properties_t> properties(count);
  memset(properties.data(), 0, sizeof(ze_device_memory_properties_t) * count);
  for (auto &prop : properties) {
    prop.stype = ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES;
  }

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetMemoryProperties(device, &count, properties.data()));
  EXPECT_EQ(device, device_initial);
  return properties;
}

ze_device_external_memory_properties_t
get_external_memory_properties(ze_device_handle_t device) {
  ze_device_external_memory_properties_t properties;
  memset(&properties, 0, sizeof(properties));
  properties = {ZE_STRUCTURE_TYPE_DEVICE_EXTERNAL_MEMORY_PROPERTIES};

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetExternalMemoryProperties(device, &properties));
  EXPECT_EQ(device, device_initial);

  return properties;
}

ze_device_memory_access_properties_t
get_memory_access_properties(ze_device_handle_t device) {
  ze_device_memory_access_properties_t properties;
  memset(&properties, 0, sizeof(properties));
  properties = {ZE_STRUCTURE_TYPE_DEVICE_MEMORY_ACCESS_PROPERTIES};

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetMemoryAccessProperties(device, &properties));
  EXPECT_EQ(device, device_initial);
  return properties;
}

uint32_t get_command_queue_group_properties_count(ze_device_handle_t device) {
  uint32_t count = 0;

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetCommandQueueGroupProperties(device, &count, nullptr));
  EXPECT_EQ(device, device_initial);

  return count;
}

std::vector<ze_command_queue_group_properties_t>
get_command_queue_group_properties(ze_device_handle_t device, uint32_t count) {
  std::vector<ze_command_queue_group_properties_t> properties(count);
  memset(properties.data(), 0,
         sizeof(ze_command_queue_group_properties_t) * count);
  for (auto &properties_item : properties) {
    properties_item.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES;
  }

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetCommandQueueGroupProperties(
                                   device, &count, properties.data()));
  EXPECT_EQ(device, device_initial);
  return properties;
}

std::vector<ze_command_queue_group_properties_t>
get_command_queue_group_properties(ze_device_handle_t device) {
  return get_command_queue_group_properties(
      device, get_command_queue_group_properties_count(device));
}

std::vector<ze_device_cache_properties_t>
get_cache_properties(ze_device_handle_t device) {

  std::vector<ze_device_cache_properties_t> properties;
  uint32_t count = 0;
  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetCacheProperties(device, &count, nullptr));
  properties.resize(count);
  memset(properties.data(), 0, sizeof(ze_device_cache_properties_t) * count);
  for (auto &prop : properties) {
    prop.stype = ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES;
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetCacheProperties(device, &count, properties.data()));
  EXPECT_EQ(device, device_initial);
  return properties;
}

ze_device_image_properties_t get_image_properties(ze_device_handle_t device) {
  ze_device_image_properties_t properties;
  memset(&properties, 0, sizeof(properties));
  properties = {ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES};

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetImageProperties(device, &properties));
  EXPECT_EQ(device, device_initial);
  return properties;
}

ze_device_module_properties_t
get_device_module_properties(ze_device_handle_t device) {
  ze_device_module_properties_t properties;
  memset(&properties, 0, sizeof(properties));
  properties = {ZE_STRUCTURE_TYPE_DEVICE_MODULE_PROPERTIES};

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetModuleProperties(device, &properties));
  EXPECT_EQ(device, device_initial);
  return properties;
}

#ifdef ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME
ze_scheduling_hint_exp_properties_t
get_device_kernel_schedule_hints(ze_device_handle_t device) {
  ze_device_module_properties_t properties;
  memset(&properties, 0, sizeof(properties));
  properties = {ZE_STRUCTURE_TYPE_DEVICE_MODULE_PROPERTIES};

  ze_scheduling_hint_exp_properties_t hints = {};
  properties.pNext = &hints;
  hints.stype = ZE_STRUCTURE_TYPE_SCHEDULING_HINT_EXP_PROPERTIES;
  hints.schedulingHintFlags = ZE_SCHEDULING_HINT_EXP_FLAG_FORCE_UINT32;

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetModuleProperties(device, &properties));
  EXPECT_EQ(device, device_initial);
  EXPECT_NE(hints.schedulingHintFlags,
            ZE_SCHEDULING_HINT_EXP_FLAG_FORCE_UINT32);
  return hints;
}
#endif

ze_device_p2p_properties_t get_p2p_properties(ze_device_handle_t dev1,
                                              ze_device_handle_t dev2) {
  ze_device_p2p_properties_t properties;
  memset(&properties, 0, sizeof(properties));
  properties = {ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES};

  auto dev1_initial = dev1;
  auto dev2_initial = dev2;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetP2PProperties(dev1, dev2, &properties));
  EXPECT_EQ(dev1, dev1_initial);
  EXPECT_EQ(dev2, dev2_initial);
  return properties;
}

std::tuple<uint64_t, uint64_t>
get_global_timestamps(ze_device_handle_t device) {

  uint64_t host_timestamp = 0, device_timestamp = 0;

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetGlobalTimestamps(
                                   device, &host_timestamp, &device_timestamp));

  return std::make_tuple(host_timestamp, device_timestamp);
}

ze_bool_t can_access_peer(ze_device_handle_t dev1, ze_device_handle_t dev2) {
  ze_bool_t can_access;

  auto dev1_initial = dev1;
  auto dev2_initial = dev2;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceCanAccessPeer(dev1, dev2, &can_access));
  EXPECT_EQ(dev1, dev1_initial);
  EXPECT_EQ(dev2, dev2_initial);
  return can_access;
}

void set_kernel_cache_config(ze_kernel_handle_t kernel,
                             ze_cache_config_flags_t config) {
  auto kernel_initial = kernel;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelSetCacheConfig(kernel, config));
  EXPECT_EQ(kernel, kernel_initial);
}

void make_memory_resident(ze_device_handle_t device, void *memory,
                          const size_t size) {
  auto device_initial = device;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeContextMakeMemoryResident(get_default_context(), device, memory, size));
  EXPECT_EQ(device, device_initial);
}

void evict_memory(ze_device_handle_t device, void *memory, const size_t size) {
  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeContextEvictMemory(get_default_context(), device, memory, size));
  EXPECT_EQ(device, device_initial);
}

}; // namespace level_zero_tests
