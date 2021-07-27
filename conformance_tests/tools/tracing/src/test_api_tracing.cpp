/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "test_harness/test_harness.hpp"
#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace lzt = level_zero_tests;

namespace {

TEST(
    TracingCreateTests,
    GivenValidDeviceAndTracerDescriptionWhenCreatingTracerThenTracerIsNotNull) {
  uint32_t user_data;

  zet_tracer_exp_desc_t tracer_desc = {};

  tracer_desc.pNext = nullptr;
  tracer_desc.pUserData = &user_data;

  zet_tracer_exp_handle_t tracer_handle =
      lzt::create_tracer_handle(tracer_desc);
  EXPECT_NE(tracer_handle, nullptr);

  lzt::destroy_tracer_handle(tracer_handle);
}

class TracingCreateMultipleTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<uint32_t> {};

TEST_P(TracingCreateMultipleTests,
       GivenExistingTracersWhenCreatingNewTracerThenSuccesIsReturned) {

  std::vector<zet_tracer_exp_handle_t> tracers(GetParam());
  std::vector<zet_tracer_exp_desc_t> descs(GetParam());
  uint32_t *user_data = new uint32_t[GetParam()];

  for (uint32_t i = 0; i < GetParam(); ++i) {

    descs[i].pNext = nullptr;
    descs[i].pUserData = &user_data[i];

    tracers[i] = lzt::create_tracer_handle(descs[i]);
    EXPECT_NE(tracers[i], nullptr);
  }

  for (auto tracer : tracers) {
    lzt::destroy_tracer_handle(tracer);
  }
  delete[] user_data;
}

INSTANTIATE_TEST_CASE_P(CreateMultipleTracerTest, TracingCreateMultipleTests,
                        ::testing::Values(1, 10, 100, 1000));

TEST(TracingDestroyTests,
     GivenSingleDisabledTracerWhenDestroyingTracerThenSuccessIsReturned) {
  uint32_t user_data;

  zet_tracer_exp_desc_t tracer_desc = {};

  tracer_desc.pNext = nullptr;
  tracer_desc.pUserData = &user_data;
  zet_tracer_exp_handle_t tracer_handle =
      lzt::create_tracer_handle(tracer_desc);

  lzt::destroy_tracer_handle(tracer_handle);
}

class TracingPrologueEpilogueTests : public ::testing::Test {
protected:
  void SetUp() override {
    driver = lzt::get_default_driver();
    device = lzt::zeDevice::get_instance()->get_device();
    context = lzt::get_default_context();

    compute_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES;
    compute_properties.pNext = nullptr;

    memory_access_properties.stype =
        ZE_STRUCTURE_TYPE_DEVICE_MEMORY_ACCESS_PROPERTIES;
    memory_properties.pNext = nullptr;

    cache_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES;
    cache_properties.pNext = nullptr;

    device_image_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES;
    device_image_properties.pNext = nullptr;

    p2p_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES;
    p2p_properties.pNext = nullptr;

    zet_tracer_exp_desc_t tracer_desc = {};

    tracer_desc.pUserData = &user_data;
    tracer_handle = lzt::create_tracer_handle(tracer_desc);
  }

  void init_command_queue() {
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueCreate(context, device, &command_queue_desc,
                                   &command_queue));
  }

  void init_command_list() {
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListCreate(context, device, &command_list_desc,
                                  &command_list));
  }

  void init_fence() {
    init_command_queue();

    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeFenceCreate(command_queue, &fence_desc, &fence));
  }

  void init_event_pool() {
    ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolCreate(context, &event_pool_desc, 1,
                                                   &device, &event_pool));
  }

  void init_event() {
    init_event_pool();

    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeEventCreate(event_pool, &event_desc, &event));
  }

  void init_module() {

    ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleCreate(context, device, &module_desc,
                                                &module, &build_log));
  }

  void init_kernel() {
    init_module();

    ASSERT_EQ(ZE_RESULT_SUCCESS, zeKernelCreate(module, &kernel_desc, &kernel));
  }

  void init_image() {
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeImageCreate(context, device, &image_desc, &image));
  }

  void init_memory() {
    // use a memory buffer big enough for all tests
    // the largest buffer will be used in image test, so size
    // based on image dimensions
    memory =
        lzt::allocate_device_memory(image_width * image_height * image_depth);
  }

  void init_ipc() {
    init_memory();

    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeMemGetIpcHandle(context, memory, &ipc_handle));
  }

  void TearDown() override {
    EXPECT_TRUE(user_data.prologue_called);
    EXPECT_TRUE(user_data.epilogue_called);

    if (fence)
      zeFenceDestroy(fence);
    if (event)
      zeEventDestroy(event);
    if (event_pool)
      zeEventPoolDestroy(event_pool);
    if (command_list)
      zeCommandListDestroy(command_list);
    if (command_queue)
      zeCommandQueueDestroy(command_queue);
    if (kernel)
      zeKernelDestroy(kernel);
    if (build_log)
      zeModuleBuildLogDestroy(build_log);
    if (module)
      zeModuleDestroy(module);
    if (image)
      zeImageDestroy(image);
    if (memory)
      lzt::free_memory(memory);
    lzt::disable_tracer(tracer_handle);
    lzt::destroy_tracer_handle(tracer_handle);
  }

  ze_context_handle_t context;
  zet_tracer_exp_handle_t tracer_handle;
  lzt::test_api_tracing_user_data user_data = {};
  ze_callbacks_t prologues = {};
  ze_callbacks_t epilogues = {};

  ze_driver_handle_t driver;

  ze_device_handle_t device;
  ze_device_properties_t properties = {};
  ze_device_compute_properties_t compute_properties = {};

  ze_device_memory_properties_t memory_properties = {};

  ze_device_memory_access_properties_t memory_access_properties = {};

  ze_device_cache_properties_t cache_properties = {};
  ze_device_image_properties_t device_image_properties = {};
  ze_device_p2p_properties_t p2p_properties = {};

  ze_api_version_t api_version;
  ze_driver_ipc_properties_t ipc_properties = {};

  ze_command_queue_desc_t command_queue_desc = {
      ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC, nullptr};

  ze_command_queue_handle_t command_queue = nullptr;

  ze_command_list_desc_t command_list_desc = {
      ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC, nullptr};
  ze_command_list_handle_t command_list = nullptr;

  ze_copy_region_t copy_region;
  ze_image_region_t image_region;

  ze_event_pool_handle_t event_pool = nullptr;
  ze_event_pool_desc_t event_pool_desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
      ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC, 1};
  ze_event_handle_t event = nullptr;
  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr};

  ze_ipc_event_pool_handle_t ipc_event;
  ze_ipc_mem_handle_t ipc_handle;

  ze_fence_handle_t fence = nullptr;
  ze_fence_desc_t fence_desc = {ZE_STRUCTURE_TYPE_FENCE_DESC, nullptr};

  ze_image_format_t format_descriptor = {
      ZE_IMAGE_FORMAT_LAYOUT_8,  // layout
      ZE_IMAGE_FORMAT_TYPE_UINT, // type
      ZE_IMAGE_FORMAT_SWIZZLE_X, // x
      ZE_IMAGE_FORMAT_SWIZZLE_X, // y
      ZE_IMAGE_FORMAT_SWIZZLE_X, // z
      ZE_IMAGE_FORMAT_SWIZZLE_X  // w
  };

  // default image description just for use in testing
  const uint32_t image_width = 1920;
  const uint32_t image_height = 1;
  const uint32_t image_depth = 1;
  ze_image_desc_t image_desc = {ZE_STRUCTURE_TYPE_IMAGE_DESC,
                                nullptr,
                                ZE_IMAGE_FLAG_KERNEL_WRITE,
                                ZE_IMAGE_TYPE_1D,
                                format_descriptor,
                                image_width,
                                image_height,
                                image_depth,
                                0,
                                0};
  ze_image_handle_t image = nullptr;
  ze_image_properties_t image_properties;

  void *device_memory, *host_memory, *shared_memory, *memory = nullptr;

  ze_memory_allocation_properties_t mem_alloc_properties = {};

  std::vector<uint8_t> binary_file =
      level_zero_tests::load_binary_file("module_add.spv");
  ze_module_desc_t module_desc = {ZE_STRUCTURE_TYPE_MODULE_DESC,
                                  nullptr,
                                  ZE_MODULE_FORMAT_IL_SPIRV,
                                  static_cast<uint32_t>(binary_file.size()),
                                  binary_file.data(),
                                  "",
                                  nullptr};

  ze_module_handle_t module = nullptr;
  ze_module_build_log_handle_t build_log = nullptr;

  ze_kernel_desc_t kernel_desc = {ZE_STRUCTURE_TYPE_KERNEL_DESC, nullptr, 0,
                                  "module_add_constant"};

  ze_kernel_handle_t kernel = nullptr;

  ze_sampler_desc_t sampler_desc = {ZE_STRUCTURE_TYPE_SAMPLER_DESC, nullptr};
  ze_sampler_handle_t sampler = nullptr;

  uint32_t num = 0, version;
  ze_driver_properties_t driver_properties = {};
  ze_bool_t can_access;
};

static void ready_tracer(zet_tracer_exp_handle_t tracer,
                         ze_callbacks_t prologues, ze_callbacks_t epilogues) {
  lzt::set_tracer_prologues(tracer, prologues);
  lzt::set_tracer_epilogues(tracer, epilogues);
  lzt::enable_tracer(tracer);
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeInitCallbacksWhenCallingzeInitThenUserDataIsSetAndResultUnchanged) {
  prologues.Global.pfnInitCb = lzt::prologue_callback;
  epilogues.Global.pfnInitCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeInit(0);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeInit(0));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetCallbacksWhenCallingzeDeviceGetThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetCb = lzt::prologue_callback;
  epilogues.Device.pfnGetCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeDeviceGet(driver, &num, nullptr);
  ready_tracer(tracer_handle, prologues, epilogues);
  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGet(driver, &num, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithZeDeviceGetSubDevicesCallbacksWhenCallingzeDeviceGetSubDevicesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetSubDevicesCb = lzt::prologue_callback;
  epilogues.Device.pfnGetSubDevicesCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeDeviceGetSubDevices(device, &num, nullptr);
  ready_tracer(tracer_handle, prologues, epilogues);
  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGetSubDevices(device, &num, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetPropertiesCallbacksWhenCallingzeDeviceGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetPropertiesCb = lzt::prologue_callback;
  epilogues.Device.pfnGetPropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeDeviceGetProperties(device, &properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDeviceGetProperties(device, &properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetComputePropertiesCallbacksWhenCallingzeDeviceGetComputePropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetComputePropertiesCb = lzt::prologue_callback;
  epilogues.Device.pfnGetComputePropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetComputeProperties(device, &compute_properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeDeviceGetComputeProperties(device, &compute_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetMemoryPropertiesCallbacksWhenCallingzeDeviceGetMemoryPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetMemoryPropertiesCb = lzt::prologue_callback;
  epilogues.Device.pfnGetMemoryPropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetMemoryProperties(device, &num, nullptr);
  ready_tracer(tracer_handle, prologues, epilogues);
  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGetMemoryProperties(device, &num, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetMemoryAccessPropertiesCallbacksWhenCallingzeDeviceGetMemoryAccessPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetMemoryAccessPropertiesCb = lzt::prologue_callback;
  epilogues.Device.pfnGetMemoryAccessPropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetMemoryAccessProperties(device, &memory_access_properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDeviceGetMemoryAccessProperties(
                                device, &memory_access_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetCachePropertiesCallbacksWhenCallingzeDeviceGetCachePropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetCachePropertiesCb = lzt::prologue_callback;
  epilogues.Device.pfnGetCachePropertiesCb = lzt::epilogue_callback;
  uint32_t count = 0;
  ze_result_t initial_result =
      zeDeviceGetCacheProperties(device, &count, &cache_properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeDeviceGetCacheProperties(device, &count, &cache_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetImagePropertiesCallbacksWhenCallingzeDeviceGetImagePropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetImagePropertiesCb = lzt::prologue_callback;
  epilogues.Device.pfnGetImagePropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetImageProperties(device, &device_image_properties);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeDeviceGetImageProperties(device, &device_image_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetP2PPropertiesCallbacksWhenCallingzeDeviceGetP2PPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetP2PPropertiesCb = lzt::prologue_callback;
  epilogues.Device.pfnGetP2PPropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetP2PProperties(device, device, &p2p_properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeDeviceGetP2PProperties(device, device, &p2p_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceCanAccessPeerCallbacksWhenCallingzeDeviceCanAccessPeerThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnCanAccessPeerCb = lzt::prologue_callback;
  epilogues.Device.pfnCanAccessPeerCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeDeviceCanAccessPeer(device, device, &can_access);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDeviceCanAccessPeer(device, device, &can_access));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceSystemBarrierCallbacksWhenCallingzeDeviceSystemBarrierThenUserDataIsSetAndResultUnchanged) {
  prologues.Context.pfnSystemBarrierCb = lzt::prologue_callback;
  epilogues.Context.pfnSystemBarrierCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeContextSystemBarrier(context, device);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeContextSystemBarrier(context, device));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceMakeMemoryResidentCallbacksWhenCallingzeDeviceMakeMemoryResidentThenUserDataIsSetAndResultUnchanged) {
  prologues.Context.pfnMakeMemoryResidentCb = lzt::prologue_callback;
  epilogues.Context.pfnMakeMemoryResidentCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeContextMakeMemoryResident(context, device, memory, 0);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeContextMakeMemoryResident(context, device, memory, 0));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceEvictMemoryCallbacksWhenCallingzeDeviceEvictMemoryThenUserDataIsSetAndResultUnchanged) {
  prologues.Context.pfnEvictMemoryCb = lzt::prologue_callback;
  epilogues.Context.pfnEvictMemoryCb = lzt::epilogue_callback;

  init_memory();

  ze_result_t initial_result = zeContextEvictMemory(context, device, memory, 0);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeContextEvictMemory(context, device, memory, 0));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceMakeImageResidentCallbacksWhenCallingzeDeviceMakeImageResidentThenUserDataIsSetAndResultUnchangedAndResultUnchanged) {
  prologues.Context.pfnMakeImageResidentCb = lzt::prologue_callback;
  epilogues.Context.pfnMakeImageResidentCb = lzt::epilogue_callback;

  init_image();

  ze_result_t initial_result =
      zeContextMakeImageResident(context, device, image);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeContextMakeImageResident(context, device, image));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceEvictImageCallbacksWhenCallingzeDeviceEvictImageThenUserDataIsSetAndResultUnchanged) {
  prologues.Context.pfnEvictImageCb = lzt::prologue_callback;
  epilogues.Context.pfnEvictImageCb = lzt::epilogue_callback;

  init_image();

  ze_result_t initial_result = zeContextEvictImage(context, device, image);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeContextEvictImage(context, device, image));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetCallbacksWhenCallingzeDriverGetThenUserDataIsSetAndResultUnchanged) {
  prologues.Driver.pfnGetCb = lzt::prologue_callback;
  epilogues.Driver.pfnGetCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeDriverGet(&num, nullptr);
  ready_tracer(tracer_handle, prologues, epilogues);
  num = 0;
  ASSERT_EQ(initial_result, zeDriverGet(&num, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetPropertiesCallbacksWhenCallingzeDriverGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Driver.pfnGetPropertiesCb = lzt::prologue_callback;
  epilogues.Driver.pfnGetPropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeDriverGetProperties(driver, &driver_properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDriverGetProperties(driver, &driver_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetApiVersionCallbacksWhenCallingzeDriverGetApiVersionThenUserDataIsSetAndResultUnchanged) {
  prologues.Driver.pfnGetApiVersionCb = lzt::prologue_callback;
  epilogues.Driver.pfnGetApiVersionCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeDriverGetApiVersion(driver, &api_version);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDriverGetApiVersion(driver, &api_version));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetIPCPropertiesCallbacksWhenCallingzeDriverGetIPCPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Driver.pfnGetIpcPropertiesCb = lzt::prologue_callback;
  epilogues.Driver.pfnGetIpcPropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeDriverGetIpcProperties(driver, &ipc_properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDriverGetIpcProperties(driver, &ipc_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverAllocSharedMemCallbacksWhenCallingzeDriverAllocSharedMemThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnAllocSharedCb = lzt::prologue_callback;
  epilogues.Mem.pfnAllocSharedCb = lzt::epilogue_callback;

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.flags = 0;
  device_desc.ordinal = 0;
  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  ze_result_t initial_result = zeMemAllocShared(
      context, &device_desc, &host_desc, 1, 0, device, &shared_memory);
  lzt::free_memory(shared_memory);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeMemAllocShared(context, &device_desc, &host_desc,
                                             1, 0, device, &shared_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, shared_memory));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverAllocDeviceMemCallbacksWhenCallingzeDriverAllocDeviceMemThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnAllocDeviceCb = lzt::prologue_callback;
  epilogues.Mem.pfnAllocDeviceCb = lzt::epilogue_callback;

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.flags = 0;
  device_desc.ordinal = 1;

  ze_result_t initial_result =
      zeMemAllocDevice(context, &device_desc, 1, 1, device, &device_memory);
  lzt::free_memory(device_memory);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeMemAllocDevice(context, &device_desc, 1, 1,
                                             device, &device_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, device_memory));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverAllocHostMemCallbacksWhenCallingzeDriverAllocHostMemThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnAllocHostCb = lzt::prologue_callback;
  epilogues.Mem.pfnAllocHostCb = lzt::epilogue_callback;

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  ze_result_t initial_result =
      zeMemAllocHost(context, &host_desc, 1, 1, &host_memory);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeMemAllocHost(context, &host_desc, 1, 1, &host_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, host_memory));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverFreeMemCallbacksWhenCallingzeDriverFreeMemThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnFreeCb = lzt::prologue_callback;
  epilogues.Mem.pfnFreeCb = lzt::epilogue_callback;

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  zeMemAllocHost(context, &host_desc, 1, 0, &host_memory);
  ze_result_t initial_result = zeMemFree(context, host_memory);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocHost(context, &host_desc, 1, 0, &host_memory));
  ASSERT_EQ(initial_result, zeMemFree(context, host_memory));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetMemAllocPropertiesCallbacksWhenCallingzeDriverGetMemAllocPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnGetAllocPropertiesCb = lzt::prologue_callback;
  epilogues.Mem.pfnGetAllocPropertiesCb = lzt::epilogue_callback;

  init_memory();

  mem_alloc_properties.pNext = nullptr;
  mem_alloc_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  ze_result_t initial_result =
      zeMemGetAllocProperties(context, memory, &mem_alloc_properties, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(
      initial_result,
      zeMemGetAllocProperties(context, memory, &mem_alloc_properties, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetMemAddressRangeCallbacksWhenCallingzeDriverGetMemAddressRangeThenUserDataIsSetAndResultUnchanged) {

  prologues.Mem.pfnGetAddressRangeCb = lzt::prologue_callback;
  epilogues.Mem.pfnGetAddressRangeCb = lzt::epilogue_callback;

  init_memory();

  ze_result_t initial_result =
      zeMemGetAddressRange(context, memory, nullptr, nullptr);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeMemGetAddressRange(context, memory, nullptr, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetMemIpcHandleCallbacksWhenCallingzeDriverGetMemIpcHandleThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnGetIpcHandleCb = lzt::prologue_callback;
  epilogues.Mem.pfnGetIpcHandleCb = lzt::epilogue_callback;

  init_memory();

  ze_result_t initial_result = zeMemGetIpcHandle(context, memory, &ipc_handle);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeMemGetIpcHandle(context, memory, &ipc_handle));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverOpenMemIpcHandleCallbacksWhenCallingzeDriverOpenMemIpcHandleThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnOpenIpcHandleCb = lzt::prologue_callback;
  epilogues.Mem.pfnOpenIpcHandleCb = lzt::epilogue_callback;

  init_ipc();

  void *mem = nullptr;

  ze_result_t initial_result =
      zeMemOpenIpcHandle(context, device, ipc_handle, 0, &mem);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeMemOpenIpcHandle(context, device, ipc_handle, 0, &mem));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandQueueCreateCallbacksWhenCallingzeCommandQueueCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandQueue.pfnCreateCb = lzt::prologue_callback;
  epilogues.CommandQueue.pfnCreateCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeCommandQueueCreate(
      context, device, &command_queue_desc, &command_queue);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueDestroy(command_queue));
  ASSERT_EQ(initial_result,
            zeCommandQueueCreate(context, device, &command_queue_desc,
                                 &command_queue));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandQueueDestroyCallbacksWhenCallingzeCommandQueueDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandQueue.pfnDestroyCb = lzt::prologue_callback;
  epilogues.CommandQueue.pfnDestroyCb = lzt::epilogue_callback;

  init_command_queue();

  ze_result_t initial_result = zeCommandQueueDestroy(command_queue);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeCommandQueueCreate(context, device, &command_queue_desc,
                                 &command_queue));
  ASSERT_EQ(initial_result, zeCommandQueueDestroy(command_queue));
  command_queue = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandQueueExecuteCommandListsCallbacksWhenCallingzeCommandQueueExecuteCommandListsThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandQueue.pfnExecuteCommandListsCb = lzt::prologue_callback;
  epilogues.CommandQueue.pfnExecuteCommandListsCb = lzt::epilogue_callback;

  init_command_list();
  init_command_queue();

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(command_list));

  ze_result_t initial_result = zeCommandQueueExecuteCommandLists(
      command_queue, 1, &command_list, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandQueueExecuteCommandLists(
                                command_queue, 1, &command_list, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandQueueSynchronizeCallbacksWhenCallingzeCommandQueueSynchronizeThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandQueue.pfnSynchronizeCb = lzt::prologue_callback;
  epilogues.CommandQueue.pfnSynchronizeCb = lzt::epilogue_callback;

  init_command_queue();

  ze_result_t initial_result = zeCommandQueueSynchronize(command_queue, 0);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandQueueSynchronize(command_queue, 0));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListCreateCallbacksWhenCallingzeCommandListCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnCreateCb = lzt::prologue_callback;
  epilogues.CommandList.pfnCreateCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeCommandListCreate(context, device, &command_list_desc, &command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListDestroy(command_list));
  ASSERT_EQ(
      initial_result,
      zeCommandListCreate(context, device, &command_list_desc, &command_list));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListResetCallbacksWhenCallingzeCommandListResetThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnResetCb = lzt::prologue_callback;
  epilogues.CommandList.pfnResetCb = lzt::epilogue_callback;

  init_command_list();

  ze_result_t initial_result = zeCommandListReset(command_list);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListReset(command_list));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListCloseCallbacksWhenCallingzeCommandListCloseThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnCloseCb = lzt::prologue_callback;
  epilogues.CommandList.pfnCloseCb = lzt::epilogue_callback;

  init_command_list();

  ze_result_t initial_result = zeCommandListClose(command_list);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListClose(command_list));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListDestroyCallbacksWhenCallingzeCommandListDestroyThenUserDataIsSetAndResultUnchanged) {

  prologues.CommandList.pfnDestroyCb = lzt::prologue_callback;
  epilogues.CommandList.pfnDestroyCb = lzt::epilogue_callback;

  init_command_list();

  ze_result_t initial_result = zeCommandListDestroy(command_list);
  ready_tracer(tracer_handle, prologues, epilogues);
  init_command_list();

  ASSERT_EQ(initial_result, zeCommandListDestroy(command_list));
  command_list = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListCreateImmediateCallbacksWhenCallingzeCommandListCreateImmediateThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnCreateImmediateCb = lzt::prologue_callback;
  epilogues.CommandList.pfnCreateImmediateCb = lzt::epilogue_callback;

  ze_result_t initial_result = zeCommandListCreateImmediate(
      context, device, &command_queue_desc, &command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListDestroy(command_list));
  ASSERT_EQ(initial_result,
            zeCommandListCreateImmediate(context, device, &command_queue_desc,
                                         &command_list));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendBarrierCallbacksWhenCallingzeCommandListAppendBarrierThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendBarrierCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendBarrierCb = lzt::epilogue_callback;

  init_command_list();

  ze_result_t initial_result =
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryRangesBarrierCallbacksWhenCallingzeCommandListAppendMemoryRangesBarrierThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryRangesBarrierCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendMemoryRangesBarrierCb = lzt::epilogue_callback;

  init_command_list();

  const std::vector<size_t> range_sizes{4096, 8192};
  std::vector<const void *> ranges{
      lzt::allocate_device_memory(range_sizes[0] * 2),
      lzt::allocate_device_memory(range_sizes[1] * 2)};

  ze_result_t initial_result = zeCommandListAppendMemoryRangesBarrier(
      command_list, ranges.size(), range_sizes.data(), ranges.data(), nullptr,
      0, nullptr);

  zeCommandListReset(command_list);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryRangesBarrier(
                                command_list, ranges.size(), range_sizes.data(),
                                ranges.data(), nullptr, 0, nullptr));

  for (auto &range : ranges)
    lzt::free_memory(range);
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryCopyCallbacksWhenCallingzeCommandListAppendMemoryCopyThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryCopyCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendMemoryCopyCb = lzt::epilogue_callback;

  init_command_list();

  void *src = lzt::allocate_device_memory(10);
  void *dst = lzt::allocate_device_memory(10);

  ze_result_t initial_result = zeCommandListAppendMemoryCopy(
      command_list, dst, src, 10, nullptr, 0, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendMemoryCopy(command_list, dst, src, 10, nullptr,
                                          0, nullptr));
  lzt::free_memory(src);
  lzt::free_memory(dst);
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryFillCallbacksWhenCallingzeCommandListAppendMemoryFillThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryFillCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendMemoryFillCb = lzt::epilogue_callback;

  init_command_list();
  init_memory();

  uint32_t val = 1;
  ze_result_t initial_result = zeCommandListAppendMemoryFill(
      command_list, memory, &val, sizeof(uint32_t), 1, nullptr, 0, nullptr);

  zeCommandListReset(command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryFill(
                                command_list, memory, &val, sizeof(uint32_t), 1,
                                nullptr, 0, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryCopyRegionCallbacksWhenCallingzeCommandListAppendMemoryCopyRegionThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryCopyRegionCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendMemoryCopyRegionCb = lzt::epilogue_callback;

  init_command_list();

  ze_copy_region_t src_region = {0, 0, 0, 1, 1, 0};
  ze_copy_region_t dst_region = {0, 0, 0, 1, 1, 0};

  void *src = lzt::allocate_device_memory(10);
  void *dst = lzt::allocate_device_memory(10);

  ze_result_t initial_result = zeCommandListAppendMemoryCopyRegion(
      command_list, dst, &dst_region, 0, 0, src, &src_region, 0, 0, nullptr, 0,
      nullptr);
  zeCommandListReset(command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryCopyRegion(
                                command_list, dst, &dst_region, 0, 0, src,
                                &src_region, 0, 0, nullptr, 0, nullptr));
  lzt::free_memory(src);
  lzt::free_memory(dst);
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyCallbacksWhenCallingzeCommandListAppendImageCopyThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendImageCopyCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendImageCopyCb = lzt::epilogue_callback;

  init_command_list();

  ze_image_handle_t src_image;
  ze_image_handle_t dst_image;
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(context, device, &image_desc, &src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(context, device, &image_desc, &dst_image));

  ze_result_t initial_result = zeCommandListAppendImageCopy(
      command_list, dst_image, src_image, nullptr, 0, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendImageCopy(command_list, dst_image, src_image,
                                         nullptr, 0, nullptr));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(dst_image));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyRegionCallbacksWhenCallingzeCommandListAppendImageCopyRegionThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendImageCopyRegionCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendImageCopyRegionCb = lzt::epilogue_callback;

  init_command_list();

  ze_image_handle_t src_image;
  ze_image_handle_t dst_image;
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(context, device, &image_desc, &src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(context, device, &image_desc, &dst_image));

  ze_result_t initial_result =
      zeCommandListAppendImageCopyRegion(command_list, dst_image, src_image,
                                         nullptr, nullptr, nullptr, 0, nullptr);
  zeCommandListReset(command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendImageCopyRegion(
                                command_list, dst_image, src_image, nullptr,
                                nullptr, nullptr, 0, nullptr));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(dst_image));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyFromMemoryCallbacksWhenCallingzeCommandListAppendImageCopyFromMemoryThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendImageCopyFromMemoryCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendImageCopyFromMemoryCb = lzt::epilogue_callback;

  init_command_list();
  init_memory();
  init_image();

  ze_result_t initial_result = zeCommandListAppendImageCopyFromMemory(
      command_list, image, memory, nullptr, nullptr, 0, nullptr);
  zeCommandListReset(command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendImageCopyFromMemory(
                command_list, image, memory, nullptr, nullptr, 0, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyToMemoryCallbacksWhenCallingzeCommandListAppendImageCopyToMemoryThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendImageCopyToMemoryCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendImageCopyToMemoryCb = lzt::epilogue_callback;

  init_command_list();
  init_memory();
  init_image();

  ze_result_t initial_result = zeCommandListAppendImageCopyToMemory(
      command_list, memory, image, nullptr, nullptr, 0, nullptr);
  zeCommandListReset(command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendImageCopyToMemory(command_list, memory, image,
                                                 nullptr, nullptr, 0, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryPrefetchCallbacksWhenCallingzeCommandListAppendMemoryPrefetchThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryPrefetchCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendMemoryPrefetchCb = lzt::epilogue_callback;

  init_command_list();

  const size_t size = 1024;
  shared_memory = lzt::allocate_shared_memory(size);

  ze_result_t initial_result =
      zeCommandListAppendMemoryPrefetch(command_list, shared_memory, size / 2);
  zeCommandListReset(command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryPrefetch(
                                command_list, shared_memory, size / 2));
  lzt::free_memory(shared_memory);
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemAdviseCallbacksWhenCallingzeCommandListAppendMemAdviseThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemAdviseCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendMemAdviseCb = lzt::epilogue_callback;

  init_command_list();
  init_memory();

  const size_t size = 1024;
  shared_memory = lzt::allocate_shared_memory(size);

  ze_result_t initial_result =
      zeCommandListAppendMemAdvise(command_list, device, shared_memory, size,
                                   ZE_MEMORY_ADVICE_SET_READ_MOSTLY);
  zeCommandListReset(command_list);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemAdvise(
                                command_list, device, shared_memory, size,
                                ZE_MEMORY_ADVICE_SET_READ_MOSTLY));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendSignalEventCallbacksWhenCallingzeCommandListAppendSignalEventThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendSignalEventCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendSignalEventCb = lzt::epilogue_callback;

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendSignalEvent(command_list, event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendSignalEvent(command_list, event));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendWaitOnEventsCallbacksWhenCallingzeCommandListAppendWaitOnEventsThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendWaitOnEventsCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendWaitOnEventsCb = lzt::epilogue_callback;

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendWaitOnEvents(command_list, 1, &event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendWaitOnEvents(command_list, 1, &event));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendEventResetCallbacksWhenCallingzeCommandListAppendEventResetThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendEventResetCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendEventResetCb = lzt::epilogue_callback;

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendEventReset(command_list, event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendEventReset(command_list, event));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendLaunchKernelCallbacksWhenCallingzeCommandListAppendLaunchKernelThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendLaunchKernelCb = lzt::prologue_callback;
  epilogues.CommandList.pfnAppendLaunchKernelCb = lzt::epilogue_callback;

  init_command_list();
  init_kernel();

  ze_group_count_t args = {1, 0, 0};

  ze_result_t initial_result = zeCommandListAppendLaunchKernel(
      command_list, kernel, &args, nullptr, 0, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchKernel(command_list, kernel, &args,
                                            nullptr, 0, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendLaunchMultipleKernelsIndirectCallbacksWhenCallingzeCommandListAppendLaunchMultipleKernelsIndirectThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendLaunchMultipleKernelsIndirectCb =
      lzt::prologue_callback;
  epilogues.CommandList.pfnAppendLaunchMultipleKernelsIndirectCb =
      lzt::epilogue_callback;

  init_command_list();
  init_kernel();

  uint32_t *count =
      static_cast<uint32_t *>(lzt::allocate_device_memory(sizeof(uint32_t)));
  ze_group_count_t *args = static_cast<ze_group_count_t *>(
      lzt::allocate_device_memory(sizeof(ze_group_count_t)));
  ze_result_t initial_result = zeCommandListAppendLaunchMultipleKernelsIndirect(
      command_list, 1, &kernel, count, args, nullptr, 0, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchMultipleKernelsIndirect(
                command_list, 1, &kernel, count, args, nullptr, 0, nullptr));

  lzt::free_memory(count);
  lzt::free_memory(args);
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceCreateCallbacksWhenCallingzeFenceCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnCreateCb = lzt::prologue_callback;
  epilogues.Fence.pfnCreateCb = lzt::epilogue_callback;

  init_command_queue();

  ze_result_t initial_result =
      zeFenceCreate(command_queue, &fence_desc, &fence);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeFenceDestroy(fence));
  ASSERT_EQ(initial_result, zeFenceCreate(command_queue, &fence_desc, &fence));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceDestroyCallbacksWhenCallingzeFenceDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnDestroyCb = lzt::prologue_callback;
  epilogues.Fence.pfnDestroyCb = lzt::epilogue_callback;

  init_fence();

  ze_result_t initial_result = zeFenceDestroy(fence);
  ready_tracer(tracer_handle, prologues, epilogues);
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeFenceCreate(command_queue, &fence_desc, &fence));

  ASSERT_EQ(initial_result, zeFenceDestroy(fence));
  fence = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceHostSynchronizeCallbacksWhenCallingzeFenceHostSynchronizeThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnHostSynchronizeCb = lzt::prologue_callback;
  epilogues.Fence.pfnHostSynchronizeCb = lzt::epilogue_callback;

  init_fence();

  ze_result_t initial_result = zeFenceHostSynchronize(fence, 0);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeFenceHostSynchronize(fence, 0));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceQueryStatusCallbacksWhenCallingzeFenceQueryStatusThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnQueryStatusCb = lzt::prologue_callback;
  epilogues.Fence.pfnQueryStatusCb = lzt::epilogue_callback;

  init_fence();

  ze_result_t initial_result = zeFenceQueryStatus(fence);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeFenceQueryStatus(fence));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceResetCallbacksWhenCallingzeFenceResetThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnResetCb = lzt::prologue_callback;
  epilogues.Fence.pfnResetCb = lzt::epilogue_callback;

  init_fence();

  ze_result_t initial_result = zeFenceReset(fence);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeFenceReset(fence));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolCreateCallbacksWhenCallingzeEventPoolCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.EventPool.pfnCreateCb = lzt::prologue_callback;
  epilogues.EventPool.pfnCreateCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeEventPoolCreate(context, &event_pool_desc, 1, &device, &event_pool);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolDestroy(event_pool));
  ASSERT_EQ(initial_result, zeEventPoolCreate(context, &event_pool_desc, 1,
                                              &device, &event_pool));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolDestroyCallbacksWhenCallingzeEventPoolDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.EventPool.pfnDestroyCb = lzt::prologue_callback;
  epilogues.EventPool.pfnDestroyCb = lzt::epilogue_callback;

  init_event_pool();

  ze_result_t initial_result = zeEventPoolDestroy(event_pool);
  ready_tracer(tracer_handle, prologues, epilogues);
  init_event_pool();
  ASSERT_EQ(initial_result, zeEventPoolDestroy(event_pool));
  event = nullptr;
  event_pool = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventCreateCallbacksWhenCallingzeEventCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnCreateCb = lzt::prologue_callback;
  epilogues.Event.pfnCreateCb = lzt::epilogue_callback;

  init_event_pool();

  ze_result_t initial_result = zeEventCreate(event_pool, &event_desc, &event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventDestroy(event));
  ASSERT_EQ(initial_result, zeEventCreate(event_pool, &event_desc, &event));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventDestroyCallbacksWhenCallingzeEventDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnDestroyCb = lzt::prologue_callback;
  epilogues.Event.pfnDestroyCb = lzt::epilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventDestroy(event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventCreate(event_pool, &event_desc, &event));
  ASSERT_EQ(initial_result, zeEventDestroy(event));
  event = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolGetIpcHandleCallbacksWhenCallingzeEventPoolGetIpcHandleThenUserDataIsSetAndResultUnchanged) {
  prologues.EventPool.pfnGetIpcHandleCb = lzt::prologue_callback;
  epilogues.EventPool.pfnGetIpcHandleCb = lzt::epilogue_callback;

  init_event_pool();

  ze_result_t initial_result = zeEventPoolGetIpcHandle(event_pool, &ipc_event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventPoolGetIpcHandle(event_pool, &ipc_event));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolOpenIpcHandleCallbacksWhenCallingzeEventPoolOpenIpcHandleThenUserDataIsSetAndResultUnchanged) {
  prologues.EventPool.pfnOpenIpcHandleCb = lzt::prologue_callback;
  epilogues.EventPool.pfnOpenIpcHandleCb = lzt::epilogue_callback;

  init_event_pool();

  ze_ipc_event_pool_handle_t handle;
  ze_event_pool_handle_t event_pool2;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolGetIpcHandle(event_pool, &ipc_event));
  ze_result_t initial_result =
      zeEventPoolOpenIpcHandle(context, ipc_event, &event_pool2);

  zeEventPoolCloseIpcHandle(event_pool2);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolGetIpcHandle(event_pool, &ipc_event));
  ASSERT_EQ(initial_result,
            zeEventPoolOpenIpcHandle(context, ipc_event, &event_pool2));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolCloseIpcHandleCallbacksWhenCallingzeEventPoolCloseIpcHandleThenUserDataIsSetAndResultUnchanged) {
  prologues.EventPool.pfnCloseIpcHandleCb = lzt::prologue_callback;
  epilogues.EventPool.pfnCloseIpcHandleCb = lzt::epilogue_callback;

  init_event_pool();

  ze_ipc_event_pool_handle_t handle;
  ze_event_pool_handle_t event_pool2;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolGetIpcHandle(event_pool, &ipc_event));
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeEventPoolOpenIpcHandle(context, ipc_event, &event_pool2));

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolCloseIpcHandle(event_pool2));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventHostSignalCallbacksWhenCallingzeEventHostSignalThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnHostSignalCb = lzt::prologue_callback;
  epilogues.Event.pfnHostSignalCb = lzt::epilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventHostSignal(event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventHostSignal(event));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventHostSynchronizeCallbacksWhenCallingzeEventHostSynchronizeThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnHostSynchronizeCb = lzt::prologue_callback;
  epilogues.Event.pfnHostSynchronizeCb = lzt::epilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventHostSynchronize(event, 0);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventHostSynchronize(event, 0));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventQueryStatusCallbacksWhenCallingzeEventQueryStatusThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnQueryStatusCb = lzt::prologue_callback;
  epilogues.Event.pfnQueryStatusCb = lzt::epilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventQueryStatus(event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventQueryStatus(event));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventHostResetCallbacksWhenCallingzeEventHostResetThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnHostResetCb = lzt::prologue_callback;
  epilogues.Event.pfnHostResetCb = lzt::epilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventHostReset(event);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventHostReset(event));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageCreateCallbacksWhenCallingzeImageCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Image.pfnCreateCb = lzt::prologue_callback;
  epilogues.Image.pfnCreateCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeImageCreate(context, device, &image_desc, &image);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(image));
  ASSERT_EQ(initial_result,
            zeImageCreate(context, device, &image_desc, &image));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageGetPropertiesCallbacksWhenCallingzeImageGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Image.pfnGetPropertiesCb = lzt::prologue_callback;
  epilogues.Image.pfnGetPropertiesCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeImageGetProperties(device, &image_desc, &image_properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeImageGetProperties(device, &image_desc, &image_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageDestroyCallbacksWhenCallingzeImageDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Image.pfnDestroyCb = lzt::prologue_callback;
  epilogues.Image.pfnDestroyCb = lzt::epilogue_callback;

  init_image();
  ze_result_t initial_result = zeImageDestroy(image);
  ready_tracer(tracer_handle, prologues, epilogues);
  init_image();
  ASSERT_EQ(initial_result, zeImageDestroy(image));
  image = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleCreateCallbacksWhenCallingzeModuleCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnCreateCb = lzt::prologue_callback;
  epilogues.Module.pfnCreateCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeModuleCreate(context, device, &module_desc, &module, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module));
  ASSERT_EQ(initial_result,
            zeModuleCreate(context, device, &module_desc, &module, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleDestroyCallbacksWhenCallingzeModuleDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnDestroyCb = lzt::prologue_callback;
  epilogues.Module.pfnDestroyCb = lzt::epilogue_callback;

  init_module();

  ze_result_t initial_result = zeModuleDestroy(module);
  ready_tracer(tracer_handle, prologues, epilogues);
  init_module();
  ASSERT_EQ(initial_result, zeModuleDestroy(module));
  module = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleGetNativeBinaryCallbacksWhenCallingzeModuleGetNativeBinaryThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnGetNativeBinaryCb = lzt::prologue_callback;
  epilogues.Module.pfnGetNativeBinaryCb = lzt::epilogue_callback;

  init_module();

  size_t binary_size;

  ze_result_t initial_result =
      zeModuleGetNativeBinary(module, &binary_size, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeModuleGetNativeBinary(module, &binary_size, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleGetGlobalPointerCallbacksWhenCallingzeModuleGetGlobalPointerThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnGetGlobalPointerCb = lzt::prologue_callback;
  epilogues.Module.pfnGetGlobalPointerCb = lzt::epilogue_callback;

  init_module();

  void *pointer;

  ze_result_t initial_result =
      zeModuleGetGlobalPointer(module, "xid", nullptr, &pointer);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeModuleGetGlobalPointer(module, "xid", nullptr, &pointer));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleGetFunctionPointerCallbacksWhenCallingzeModuleGetFunctionPointerThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnGetFunctionPointerCb = lzt::prologue_callback;
  epilogues.Module.pfnGetFunctionPointerCb = lzt::epilogue_callback;

  init_module();
  void *function_pointer;

  ze_result_t initial_result = zeModuleGetFunctionPointer(
      module, "module_add_constant", &function_pointer);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeModuleGetFunctionPointer(module, "module_add_constant",
                                       &function_pointer));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleBuildLogDestroyCallbacksWhenCallingzeModuleBuildLogDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.ModuleBuildLog.pfnDestroyCb = lzt::prologue_callback;
  epilogues.ModuleBuildLog.pfnDestroyCb = lzt::epilogue_callback;

  init_module();

  ze_result_t initial_result = zeModuleBuildLogDestroy(build_log);
  ze_module_handle_t module2;
  ready_tracer(tracer_handle, prologues, epilogues);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleCreate(context, device, &module_desc,
                                              &module2, &build_log));
  ASSERT_EQ(initial_result, zeModuleBuildLogDestroy(build_log));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module2));
  build_log = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleBuildLogGetStringCallbacksWhenCallingzeModuleBuildLogGetStringThenUserDataIsSetAndResultUnchanged) {
  prologues.ModuleBuildLog.pfnGetStringCb = lzt::prologue_callback;
  epilogues.ModuleBuildLog.pfnGetStringCb = lzt::epilogue_callback;

  init_module();

  size_t log_size;
  ze_result_t initial_result =
      zeModuleBuildLogGetString(build_log, &log_size, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeModuleBuildLogGetString(build_log, &log_size, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelCreateCallbacksWhenCallingzeKernelCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnCreateCb = lzt::prologue_callback;
  epilogues.Kernel.pfnCreateCb = lzt::epilogue_callback;

  init_module();
  kernel_desc = {ZE_STRUCTURE_TYPE_KERNEL_DESC, nullptr, 0,
                 "module_add_constant"};
  ze_result_t initial_result = zeKernelCreate(module, &kernel_desc, &kernel);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeKernelDestroy(kernel));
  ASSERT_EQ(initial_result, zeKernelCreate(module, &kernel_desc, &kernel));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelDestroyCallbacksWhenCallingzeKernelDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnDestroyCb = lzt::prologue_callback;
  epilogues.Kernel.pfnDestroyCb = lzt::epilogue_callback;

  init_kernel();

  ze_result_t initial_result = zeKernelDestroy(kernel);
  ready_tracer(tracer_handle, prologues, epilogues);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeKernelCreate(module, &kernel_desc, &kernel));

  ASSERT_EQ(initial_result, zeKernelDestroy(kernel));
  kernel = nullptr;
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendLaunchKernelIndirectCallbacksWhenCallingzeCommandListAppendLaunchKernelIndirectThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendLaunchKernelIndirectCb =
      lzt::prologue_callback;
  epilogues.CommandList.pfnAppendLaunchKernelIndirectCb =
      lzt::epilogue_callback;

  init_command_list();
  init_kernel();

  ze_group_count_t args = {1, 0, 0};

  ze_result_t initial_result = zeCommandListAppendLaunchKernelIndirect(
      command_list, kernel, &args, nullptr, 0, nullptr);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchKernelIndirect(command_list, kernel, &args,
                                                    nullptr, 0, nullptr));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelSetGroupSizeCallbacksWhenCallingzeKernelSetGroupSizeThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnSetGroupSizeCb = lzt::prologue_callback;
  epilogues.Kernel.pfnSetGroupSizeCb = lzt::epilogue_callback;

  init_kernel();

  ze_result_t initial_result = zeKernelSetGroupSize(kernel, 1, 1, 1);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeKernelSetGroupSize(kernel, 1, 1, 1));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelSuggestGroupSizeCallbacksWhenCallingzeKernelSuggestGroupSizeThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnSuggestGroupSizeCb = lzt::prologue_callback;
  epilogues.Kernel.pfnSuggestGroupSizeCb = lzt::epilogue_callback;

  init_kernel();

  uint32_t x, y, z;

  ze_result_t initial_result =
      zeKernelSuggestGroupSize(kernel, 1, 1, 1, &x, &y, &z);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeKernelSuggestGroupSize(kernel, 1, 1, 1, &x, &y, &z));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelSetArgumentValueCallbacksWhenCallingzeKernelSetArgumentValueThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnSetArgumentValueCb = lzt::prologue_callback;
  epilogues.Kernel.pfnSetArgumentValueCb = lzt::epilogue_callback;

  init_kernel();

  int val = 1;
  ze_result_t initial_result =
      zeKernelSetArgumentValue(kernel, 1, sizeof(int), &val);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeKernelSetArgumentValue(kernel, 1, sizeof(int), &val));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelGetPropertiesCallbacksWhenCallingzeKernelGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnGetPropertiesCb = lzt::prologue_callback;
  epilogues.Kernel.pfnGetPropertiesCb = lzt::epilogue_callback;

  init_kernel();

  ze_kernel_properties_t kernel_properties = {};
  kernel_properties.stype = ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES;
  ze_result_t initial_result =
      zeKernelGetProperties(kernel, &kernel_properties);
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeKernelGetProperties(kernel, &kernel_properties));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeSamplerCreateCallbacksWhenCallingzeSamplerCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Sampler.pfnCreateCb = lzt::prologue_callback;
  epilogues.Sampler.pfnCreateCb = lzt::epilogue_callback;

  ze_result_t initial_result =
      zeSamplerCreate(context, device, &sampler_desc, &sampler);
  if (initial_result == ZE_RESULT_SUCCESS) {
    zeSamplerDestroy(sampler);
  }
  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeSamplerCreate(context, device, &sampler_desc, &sampler));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeSamplerDestroy(sampler));
}

TEST_F(
    TracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeSamplerDestroyCallbacksWhenCallingzeSamplerDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Sampler.pfnDestroyCb = lzt::prologue_callback;
  epilogues.Sampler.pfnDestroyCb = lzt::epilogue_callback;

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeSamplerCreate(context, device, &sampler_desc, &sampler));

  ze_result_t initial_result = zeSamplerDestroy(sampler);

  ready_tracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeSamplerCreate(context, device, &sampler_desc, &sampler));
  ASSERT_EQ(initial_result, zeSamplerDestroy(sampler));
}

} // namespace
