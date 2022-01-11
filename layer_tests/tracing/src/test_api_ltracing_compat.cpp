/*
 *
 * Copyright (C) 2020-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "test_harness/test_harness.hpp"
#include "net/test_ipc_comm.hpp"
#include <boost/process.hpp>
#include <level_zero/ze_api.h>
#include <level_zero/layers/zel_tracing_api.h>
#include <level_zero/layers/zel_tracing_register_cb.h>

namespace lzt = level_zero_tests;

namespace {

TEST(
    LCTracingCreateTests,
    GivenValidDeviceAndTracerDescriptionWhenCreatingTracerThenTracerIsNotNull) {
  uint32_t user_data;

  zel_tracer_desc_t tracer_desc = {};

  tracer_desc.pNext = nullptr;
  tracer_desc.pUserData = &user_data;

  zel_tracer_handle_t tracer_handle = lzt::create_ltracer_handle(tracer_desc);
  EXPECT_NE(tracer_handle, nullptr);

  lzt::destroy_ltracer_handle(tracer_handle);
}

class LCTracingCreateMultipleTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<uint32_t> {};

TEST_P(LCTracingCreateMultipleTests,
       GivenExistingTracersWhenCreatingNewTracerThenSuccesIsReturned) {

  std::vector<zel_tracer_handle_t> tracers(GetParam());
  std::vector<zel_tracer_desc_t> descs(GetParam());
  uint32_t *user_data = new uint32_t[GetParam()];

  for (uint32_t i = 0; i < GetParam(); ++i) {

    descs[i].pNext = nullptr;
    descs[i].pUserData = &user_data[i];

    tracers[i] = lzt::create_ltracer_handle(descs[i]);
    EXPECT_NE(tracers[i], nullptr);
  }

  for (auto tracer : tracers) {
    lzt::destroy_ltracer_handle(tracer);
  }
  delete[] user_data;
}

INSTANTIATE_TEST_CASE_P(LCCreateMultipleTracerTest,
                        LCTracingCreateMultipleTests,
                        ::testing::Values(1, 10, 100, 1000));

TEST(LCTracingDestroyTests,
     GivenSingleDisabledTracerWhenDestroyingTracerThenSuccessIsReturned) {
  uint32_t user_data;

  zel_tracer_desc_t tracer_desc = {};

  tracer_desc.pNext = nullptr;
  tracer_desc.pUserData = &user_data;
  zel_tracer_handle_t tracer_handle = lzt::create_ltracer_handle(tracer_desc);

  lzt::destroy_ltracer_handle(tracer_handle);
}

class LCTracingPrologueEpilogueTests : public ::testing::Test {
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

    zel_tracer_desc_t tracer_desc = {};

    tracer_desc.pUserData = &user_data;
    tracer_handle = lzt::create_ltracer_handle(tracer_desc);
  }

  void run_ltracing_compat_ipc_event_test(std::string test_type_name) {
#ifdef __linux__

    lzt::zeEventPool ep;
    ze_device_handle_t device =
        lzt::get_default_device(lzt::get_default_driver());

    ze_event_pool_desc_t defaultEventPoolDesc = {
        ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
        (ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC), 10};
    ep.InitEventPool(defaultEventPoolDesc);

    ze_ipc_event_pool_handle_t hIpcEventPool;
    ep.get_ipc_handle(&hIpcEventPool);
    if (testing::Test::HasFatalFailure())
      exit(EXIT_FAILURE); // Abort test if IPC Event handle failed

    static const ze_event_desc_t defaultEventDesc = {
        ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 5, 0,
        ZE_EVENT_SCOPE_FLAG_HOST // ensure memory coherency across device
                                 // and Host after event signalled
    };
    ze_event_handle_t hEvent;
    ep.create_event(hEvent, defaultEventDesc);

    // launch child
    boost::process::child c("./tracing/test_ltracing_compat_ipc_event_helper",
                            test_type_name.c_str());
    lzt::send_ipc_handle(hIpcEventPool);

    c.wait(); // wait for the process to exit

    // cleanup
    ep.destroy_event(hEvent);

    // hack to be able to use this class for ipc event test
    if (c.exit_code() == EXIT_SUCCESS) {
      user_data.prologue_called = true;
      user_data.epilogue_called = true;
    }
#else
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
#endif /* __linux__ */
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
    lzt::disable_ltracer(tracer_handle);
    lzt::destroy_ltracer_handle(tracer_handle);
  }

  ze_context_handle_t context;
  zel_tracer_handle_t tracer_handle;
  lzt::test_api_ltracing_user_data user_data = {};

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
  ze_image_properties_t image_properties = {};

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

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeInitCallbacksWhenCallingzeInitThenUserDataIsSetAndResultUnchanged) {

  zelTracerInitRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                lzt::lprologue_callback);
  zelTracerInitRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                lzt::lepilogue_callback);

  ze_result_t initial_result = zeInit(0);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeInit(0));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetCallbacksWhenCallingzeDeviceGetThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                     lzt::lprologue_callback);
  zelTracerDeviceGetRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                     lzt::lepilogue_callback);

  ze_result_t initial_result = zeDeviceGet(driver, &num, nullptr);

  lzt::enable_ltracer(tracer_handle);

  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGet(driver, &num, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithZeDeviceGetSubDevicesCallbacksWhenCallingzeDeviceGetSubDevicesThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetSubDevicesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetSubDevicesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result = zeDeviceGetSubDevices(device, &num, nullptr);

  lzt::enable_ltracer(tracer_handle);

  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGetSubDevices(device, &num, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetPropertiesCallbacksWhenCallingzeDeviceGetPropertiesThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result = zeDeviceGetProperties(device, &properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeDeviceGetProperties(device, &properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetComputePropertiesCallbacksWhenCallingzeDeviceGetComputePropertiesThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetComputePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetComputePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeDeviceGetComputeProperties(device, &compute_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeDeviceGetComputeProperties(device, &compute_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetModulePropertiesCallbacksWhenCallingzeDeviceGetModulePropertiesThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetModulePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetModulePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_device_module_properties_t moduleProperties = {};

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeDeviceGetModuleProperties(device, &moduleProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetCommandQueueGroupPropertieszeDeviceGetCommandQueueGroupPropertiesThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetCommandQueueGroupPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetCommandQueueGroupPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  uint32_t count = 0;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeDeviceGetCommandQueueGroupProperties(device, &count, nullptr);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetMemoryPropertiesCallbacksWhenCallingzeDeviceGetMemoryPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerDeviceGetMemoryPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetMemoryPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeDeviceGetMemoryProperties(device, &num, nullptr);

  lzt::enable_ltracer(tracer_handle);

  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGetMemoryProperties(device, &num, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetMemoryAccessPropertiesCallbacksWhenCallingzeDeviceGetMemoryAccessPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerDeviceGetMemoryAccessPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetMemoryAccessPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeDeviceGetMemoryAccessProperties(device, &memory_access_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeDeviceGetMemoryAccessProperties(
                                device, &memory_access_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetCachePropertiesCallbacksWhenCallingzeDeviceGetCachePropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerDeviceGetCachePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetCachePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  uint32_t count = 0;
  ze_result_t initial_result =
      zeDeviceGetCacheProperties(device, &count, &cache_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeDeviceGetCacheProperties(device, &count, &cache_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetImagePropertiesCallbacksWhenCallingzeDeviceGetImagePropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerDeviceGetImagePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetImagePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeDeviceGetImageProperties(device, &device_image_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeDeviceGetImageProperties(device, &device_image_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetExternalMemoryPropertiesCallbacksWhenCallingzeDeviceGetExternalMemoryPropertiesThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetExternalMemoryPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetExternalMemoryPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_device_external_memory_properties_t externalMemoryProperties;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDeviceGetExternalMemoryProperties(device, &externalMemoryProperties);


  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetP2PPropertiesCallbacksWhenCallingzeDeviceGetP2PPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerDeviceGetP2PPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetP2PPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeDeviceGetP2PProperties(device, device, &p2p_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeDeviceGetP2PProperties(device, device, &p2p_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceCanAccessPeerCallbacksWhenCallingzeDeviceCanAccessPeerThenUserDataIsSetAndResultUnchanged) {
  zelTracerDeviceCanAccessPeerRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceCanAccessPeerRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeDeviceCanAccessPeer(device, device, &can_access);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeDeviceCanAccessPeer(device, device, &can_access));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetStatusCallbacksWhenCallingzeDeviceGetStatusThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDeviceGetStatus(device);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceGetGlobalTimestampsCallbacksWhenCallingzeDeviceGetGlobalTimestampsThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetGlobalTimestampsRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetGlobalTimestampsRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  uint64_t hostTimestamp, deviceTimestamp;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDeviceGetGlobalTimestamps(device, &hostTimestamp, &deviceTimestamp);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceReserveCacheExtCallbacksWhenCallingzeDeviceReserveCacheExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceReserveCacheExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceReserveCacheExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDeviceReserveCacheExt(device, 0, 0);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceSetCacheAdviceExtCallbacksWhenCallingzeDeviceSetCacheAdviceExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceSetCacheAdviceExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceSetCacheAdviceExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_cache_ext_region_t cacheRegion = {ZE_CACHE_EXT_REGION_ZE_CACHE_REGION_DEFAULT};

  lzt::enable_ltracer(tracer_handle);

  const size_t memoryRegionSize = 8192;
  void *memoryRegion = lzt::allocate_device_memory(memoryRegionSize);

  ze_result_t result = zeDeviceSetCacheAdviceExt(device, memoryRegion, memoryRegionSize, cacheRegion);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  lzt::free_memory(memoryRegion);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDevicePciGetPropertiesExtCallbacksWhenCallingzeDevicePciGetPropertiesExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerDevicePciGetPropertiesExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDevicePciGetPropertiesExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_pci_ext_properties_t devicePciProperties = {ZE_STRUCTURE_TYPE_PCI_EXT_PROPERTIES, nullptr};

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDevicePciGetPropertiesExt(device, &devicePciProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeContextCreateCallbacksWhenCallingzeContextCreateThenUserDataIsSetAndResultUnchanged) {

  zelTracerContextCreateRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextCreateRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_context_desc_t contextDescriptor = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
  ze_context_handle_t context;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeContextCreate(driver, &contextDescriptor, &context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  result = zeContextDestroy(context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeContextCreateExCallbacksWhenCallingzeContextCreateExThenUserDataIsSetAndResultUnchanged) {

  zelTracerContextCreateExRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextCreateExRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_context_desc_t contextDescriptor = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
  ze_context_handle_t context;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeContextCreateEx(driver, &contextDescriptor, 0, nullptr, &context);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  result = zeContextDestroy(context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeContextDestroyCallbacksWhenCallingzeContextDestroyThenUserDataIsSetAndResultUnchanged) {

  zelTracerContextDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_context_desc_t contextDescriptor = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
  ze_context_handle_t context;

  ze_result_t result = zeContextCreate(driver, &contextDescriptor, &context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  lzt::enable_ltracer(tracer_handle);

  result = zeContextDestroy(context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeContextGetStatusCallbacksWhenCallingzeContextGetStatusThenUserDataIsSetAndResultUnchanged) {

  zelTracerContextGetStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextGetStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_context_desc_t contextDescriptor = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
  ze_context_handle_t context;

  ze_result_t result = zeContextCreate(driver, &contextDescriptor, &context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  lzt::enable_ltracer(tracer_handle);

  result = zeContextGetStatus(context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  result = zeContextDestroy(context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceSystemBarrierCallbacksWhenCallingzeDeviceSystemBarrierThenUserDataIsSetAndResultUnchanged) {
  zelTracerContextSystemBarrierRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextSystemBarrierRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result = zeContextSystemBarrier(context, device);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeContextSystemBarrier(context, device));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceMakeMemoryResidentCallbacksWhenCallingzeDeviceMakeMemoryResidentThenUserDataIsSetAndResultUnchanged) {
  zelTracerContextMakeMemoryResidentRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextMakeMemoryResidentRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeContextMakeMemoryResident(context, device, memory, 0);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeContextMakeMemoryResident(context, device, memory, 0));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceEvictMemoryCallbacksWhenCallingzeDeviceEvictMemoryThenUserDataIsSetAndResultUnchanged) {
  zelTracerContextEvictMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextEvictMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_memory();

  ze_result_t initial_result = zeContextEvictMemory(context, device, memory, 0);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeContextEvictMemory(context, device, memory, 0));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceMakeImageResidentCallbacksWhenCallingzeDeviceMakeImageResidentThenUserDataIsSetAndResultUnchangedAndResultUnchanged) {
  zelTracerContextMakeImageResidentRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextMakeImageResidentRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_image();

  ze_result_t initial_result =
      zeContextMakeImageResident(context, device, image);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeContextMakeImageResident(context, device, image));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDeviceEvictImageCallbacksWhenCallingzeDeviceEvictImageThenUserDataIsSetAndResultUnchanged) {
  zelTracerContextEvictImageRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextEvictImageRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_image();

  ze_result_t initial_result = zeContextEvictImage(context, device, image);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeContextEvictImage(context, device, image));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetCallbacksWhenCallingzeDriverGetThenUserDataIsSetAndResultUnchanged) {
  zelTracerDriverGetRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                     lzt::lprologue_callback);
  zelTracerDriverGetRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                     lzt::lepilogue_callback);

  ze_result_t initial_result = zeDriverGet(&num, nullptr);

  lzt::enable_ltracer(tracer_handle);

  num = 0;
  ASSERT_EQ(initial_result, zeDriverGet(&num, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetApiVersionCallbacksWhenCallingzeDriverGetApiVersionThenUserDataIsSetAndResultUnchanged) {
  zelTracerDriverGetApiVersionRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDriverGetApiVersionRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result = zeDriverGetApiVersion(driver, &api_version);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeDriverGetApiVersion(driver, &api_version));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetPropertiesCallbacksWhenCallingzeDriverGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerDriverGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDriverGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeDriverGetProperties(driver, &driver_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeDriverGetProperties(driver, &driver_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetIPCPropertiesCallbacksWhenCallingzeDriverGetIPCPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerDriverGetIpcPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDriverGetIpcPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeDriverGetIpcProperties(driver, &ipc_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeDriverGetIpcProperties(driver, &ipc_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverzeDriverGetExtensionPropertiesCallbacksWhenCallingzeDriverGetExtensionPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerDriverGetExtensionPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDriverGetExtensionPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  lzt::enable_ltracer(tracer_handle);

  uint32_t count = 0;

  ze_result_t result = zeDriverGetExtensionProperties(driver, &count, nullptr);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverzeDriverGetExtensionFunctionAddressCallbacksWhenCallingzeDriverGetExtensionFunctionAddressThenUserDataIsSetAndResultUnchanged) {
  zelTracerDriverGetExtensionFunctionAddressRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDriverGetExtensionFunctionAddressRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  typedef ze_result_t (*pFnzexDriverImportExternalPointer)(ze_driver_handle_t, void *, size_t);
  pFnzexDriverImportExternalPointer zexDriverImportExternalPointer = nullptr;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDriverGetExtensionFunctionAddress(driver, "zexDriverImportExternalPointer", reinterpret_cast<void **>(&zexDriverImportExternalPointer));

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverAllocSharedMemCallbacksWhenCallingzeDriverAllocSharedMemThenUserDataIsSetAndResultUnchanged) {
  zelTracerMemAllocSharedRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                          lzt::lprologue_callback);
  zelTracerMemAllocSharedRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                          lzt::lepilogue_callback);

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

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeMemAllocShared(context, &device_desc, &host_desc,
                                             1, 0, device, &shared_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, shared_memory));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverAllocDeviceMemCallbacksWhenCallingzeDriverAllocDeviceMemThenUserDataIsSetAndResultUnchanged) {
  zelTracerMemAllocDeviceRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                          lzt::lprologue_callback);
  zelTracerMemAllocDeviceRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                          lzt::lepilogue_callback);

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.flags = 0;
  device_desc.ordinal = 1;

  ze_result_t initial_result =
      zeMemAllocDevice(context, &device_desc, 1, 1, device, &device_memory);
  lzt::free_memory(device_memory);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeMemAllocDevice(context, &device_desc, 1, 1,
                                             device, &device_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, device_memory));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverAllocHostMemCallbacksWhenCallingzeDriverAllocHostMemThenUserDataIsSetAndResultUnchanged) {
  zelTracerMemAllocHostRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                        lzt::lprologue_callback);
  zelTracerMemAllocHostRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                        lzt::lepilogue_callback);

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  ze_result_t initial_result =
      zeMemAllocHost(context, &host_desc, 1, 1, &host_memory);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeMemAllocHost(context, &host_desc, 1, 1, &host_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, host_memory));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverFreeMemCallbacksWhenCallingzeDriverFreeMemThenUserDataIsSetAndResultUnchanged) {
  zelTracerMemFreeRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                   lzt::lprologue_callback);
  zelTracerMemFreeRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                   lzt::lepilogue_callback);

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  zeMemAllocHost(context, &host_desc, 1, 0, &host_memory);
  ze_result_t initial_result = zeMemFree(context, host_memory);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocHost(context, &host_desc, 1, 0, &host_memory));
  ASSERT_EQ(initial_result, zeMemFree(context, host_memory));
}


TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeMemFreeExtCallbacksWhenCallingzeMemFreeExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerMemFreeExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerMemFreeExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_memory_free_ext_desc_t memFreeDesc = {ZE_STRUCTURE_TYPE_MEMORY_FREE_EXT_DESC, nullptr, ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_BLOCKING_FREE};

  init_memory();

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeMemFreeExt(context, &memFreeDesc, memory);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  memory = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetMemAllocPropertiesCallbacksWhenCallingzeDriverGetMemAllocPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerMemGetAllocPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerMemGetAllocPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_memory();

  mem_alloc_properties.pNext = nullptr;
  mem_alloc_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;

  ze_result_t initial_result =
      zeMemGetAllocProperties(context, memory, &mem_alloc_properties, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(
      initial_result,
      zeMemGetAllocProperties(context, memory, &mem_alloc_properties, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetMemAddressRangeCallbacksWhenCallingzeDriverGetMemAddressRangeThenUserDataIsSetAndResultUnchanged) {

  zelTracerMemGetAddressRangeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerMemGetAddressRangeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_memory();

  ze_result_t initial_result =
      zeMemGetAddressRange(context, memory, nullptr, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeMemGetAddressRange(context, memory, nullptr, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverGetMemIpcHandleCallbacksWhenCallingzeDriverGetMemIpcHandleThenUserDataIsSetAndResultUnchanged) {
  zelTracerMemGetIpcHandleRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                           lzt::lprologue_callback);
  zelTracerMemGetIpcHandleRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                           lzt::lepilogue_callback);

  init_memory();

  ze_result_t initial_result = zeMemGetIpcHandle(context, memory, &ipc_handle);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeMemGetIpcHandle(context, memory, &ipc_handle));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeDriverOpenMemIpcHandleCallbacksWhenCallingzeDriverOpenMemIpcHandleThenUserDataIsSetAndResultUnchanged) {
  zelTracerMemOpenIpcHandleRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerMemOpenIpcHandleRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_ipc();

  void *mem = nullptr;

  ze_result_t initial_result =
      zeMemOpenIpcHandle(context, device, ipc_handle, 0, &mem);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeMemOpenIpcHandle(context, device, ipc_handle, 0, &mem));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandQueueCreateCallbacksWhenCallingzeCommandQueueCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandQueueCreateRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandQueueCreateRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result = zeCommandQueueCreate(
      context, device, &command_queue_desc, &command_queue);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueDestroy(command_queue));
  ASSERT_EQ(initial_result,
            zeCommandQueueCreate(context, device, &command_queue_desc,
                                 &command_queue));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandQueueDestroyCallbacksWhenCallingzeCommandQueueDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandQueueDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandQueueDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_queue();

  ze_result_t initial_result = zeCommandQueueDestroy(command_queue);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeCommandQueueCreate(context, device, &command_queue_desc,
                                 &command_queue));
  ASSERT_EQ(initial_result, zeCommandQueueDestroy(command_queue));
  command_queue = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandQueueExecuteCommandListsCallbacksWhenCallingzeCommandQueueExecuteCommandListsThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandQueueExecuteCommandListsRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandQueueExecuteCommandListsRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_command_queue();

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(command_list));

  ze_result_t initial_result = zeCommandQueueExecuteCommandLists(
      command_queue, 1, &command_list, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandQueueExecuteCommandLists(
                                command_queue, 1, &command_list, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandQueueSynchronizeCallbacksWhenCallingzeCommandQueueSynchronizeThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandQueueSynchronizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandQueueSynchronizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_queue();

  ze_result_t initial_result = zeCommandQueueSynchronize(command_queue, 0);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandQueueSynchronize(command_queue, 0));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListCreateCallbacksWhenCallingzeCommandListCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListCreateRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListCreateRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeCommandListCreate(context, device, &command_list_desc, &command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListDestroy(command_list));
  ASSERT_EQ(
      initial_result,
      zeCommandListCreate(context, device, &command_list_desc, &command_list));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListResetCallbacksWhenCallingzeCommandListResetThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListResetRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListResetRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  ze_result_t initial_result = zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListReset(command_list));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListCloseCallbacksWhenCallingzeCommandListCloseThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListCloseRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListCloseRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  ze_result_t initial_result = zeCommandListClose(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListClose(command_list));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListDestroyCallbacksWhenCallingzeCommandListDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  ze_result_t initial_result = zeCommandListDestroy(command_list);

  lzt::enable_ltracer(tracer_handle);

  init_command_list();

  ASSERT_EQ(initial_result, zeCommandListDestroy(command_list));
  command_list = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListCreateImmediateCallbacksWhenCallingzeCommandListCreateImmediateThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListCreateImmediateRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListCreateImmediateRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result = zeCommandListCreateImmediate(
      context, device, &command_queue_desc, &command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListDestroy(command_list));
  ASSERT_EQ(initial_result,
            zeCommandListCreateImmediate(context, device, &command_queue_desc,
                                         &command_list));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendBarrierCallbacksWhenCallingzeCommandListAppendBarrierThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendBarrierRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendBarrierRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  ze_result_t initial_result =
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryRangesBarrierCallbacksWhenCallingzeCommandListAppendMemoryRangesBarrierThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendMemoryRangesBarrierRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendMemoryRangesBarrierRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  const std::vector<size_t> range_sizes{4096, 8192};
  std::vector<const void *> ranges{
      lzt::allocate_device_memory(range_sizes[0] * 2),
      lzt::allocate_device_memory(range_sizes[1] * 2)};

  ze_result_t initial_result = zeCommandListAppendMemoryRangesBarrier(
      command_list, ranges.size(), range_sizes.data(), ranges.data(), nullptr,
      0, nullptr);

  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryRangesBarrier(
                                command_list, ranges.size(), range_sizes.data(),
                                ranges.data(), nullptr, 0, nullptr));

  for (auto &range : ranges)
    lzt::free_memory(range);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryCopyCallbacksWhenCallingzeCommandListAppendMemoryCopyThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendMemoryCopyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendMemoryCopyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  void *src = lzt::allocate_device_memory(10);
  void *dst = lzt::allocate_device_memory(10);

  ze_result_t initial_result = zeCommandListAppendMemoryCopy(
      command_list, dst, src, 10, nullptr, 0, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendMemoryCopy(command_list, dst, src, 10, nullptr,
                                          0, nullptr));
  lzt::free_memory(src);
  lzt::free_memory(dst);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryFillCallbacksWhenCallingzeCommandListAppendMemoryFillThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendMemoryFillRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendMemoryFillRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_memory();

  uint32_t val = 1;
  ze_result_t initial_result = zeCommandListAppendMemoryFill(
      command_list, memory, &val, sizeof(uint32_t), 1, nullptr, 0, nullptr);

  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryFill(
                                command_list, memory, &val, sizeof(uint32_t), 1,
                                nullptr, 0, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryCopyRegionCallbacksWhenCallingzeCommandListAppendMemoryCopyRegionThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendMemoryCopyRegionRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendMemoryCopyRegionRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  ze_copy_region_t src_region = {0, 0, 0, 1, 1, 0};
  ze_copy_region_t dst_region = {0, 0, 0, 1, 1, 0};

  void *src = lzt::allocate_device_memory(10);
  void *dst = lzt::allocate_device_memory(10);

  ze_result_t initial_result = zeCommandListAppendMemoryCopyRegion(
      command_list, dst, &dst_region, 0, 0, src, &src_region, 0, 0, nullptr, 0,
      nullptr);
  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryCopyRegion(
                                command_list, dst, &dst_region, 0, 0, src,
                                &src_region, 0, 0, nullptr, 0, nullptr));
  lzt::free_memory(src);
  lzt::free_memory(dst);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListappendWriteGlobalTImestampCallbacksWhenCallingzeCommandListappendWriteGlobalTImestampThenUserDataIsSetAndResultUnchanged) {

  zelTracerCommandListAppendWriteGlobalTimestampRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendWriteGlobalTimestampRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_event_handle_t event = nullptr;
  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, 1};
  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
  lzt::zeEventPool ep;
  ep.InitEventPool(event_pool_desc);
  ep.create_event(event, event_desc);

  auto command_list = lzt::create_command_list();
  uint64_t dst_timestamp;
  ze_event_handle_t wait_event_before, wait_event_after;
  auto wait_event_initial = event;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeCommandListAppendWriteGlobalTimestamp(command_list, &dst_timestamp, nullptr, 1, &event);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
  ASSERT_EQ(event, wait_event_initial);

  ep.destroy_event(event);
  lzt::destroy_command_list(command_list);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyCallbacksWhenCallingzeCommandListAppendImageCopyThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendImageCopyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  ze_image_handle_t src_image;
  ze_image_handle_t dst_image;
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(context, device, &image_desc, &src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(context, device, &image_desc, &dst_image));

  ze_result_t initial_result = zeCommandListAppendImageCopy(
      command_list, dst_image, src_image, nullptr, 0, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendImageCopy(command_list, dst_image, src_image,
                                         nullptr, 0, nullptr));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(dst_image));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyRegionCallbacksWhenCallingzeCommandListAppendImageCopyRegionThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendImageCopyRegionRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyRegionRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

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

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendImageCopyRegion(
                                command_list, dst_image, src_image, nullptr,
                                nullptr, nullptr, 0, nullptr));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(dst_image));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyFromMemoryCallbacksWhenCallingzeCommandListAppendImageCopyFromMemoryThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendImageCopyFromMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyFromMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_memory();
  init_image();

  ze_result_t initial_result = zeCommandListAppendImageCopyFromMemory(
      command_list, image, memory, nullptr, nullptr, 0, nullptr);
  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendImageCopyFromMemory(
                command_list, image, memory, nullptr, nullptr, 0, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyToMemoryCallbacksWhenCallingzeCommandListAppendImageCopyToMemoryThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendImageCopyToMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyToMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_memory();
  init_image();

  ze_result_t initial_result = zeCommandListAppendImageCopyToMemory(
      command_list, memory, image, nullptr, nullptr, 0, nullptr);
  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendImageCopyToMemory(command_list, memory, image,
                                                 nullptr, nullptr, 0, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyToMemoryExtCallbacksWhenCallingzezeCommandListAppendImageCopyToMemoryExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerCommandListAppendImageCopyToMemoryExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyToMemoryExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_memory();
  init_image();

  uint32_t destRowPitch = 0;
  uint32_t destSlicePitch = 0;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeCommandListAppendImageCopyToMemoryExt(command_list, memory, image, nullptr, destRowPitch, destSlicePitch, nullptr, 0, 0);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendImageCopyFromMemoryExtCallbacksWhenCallingzeCommandListAppendImageCopyFromMemoryExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerCommandListAppendImageCopyFromMemoryExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyFromMemoryExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_image();

  uint32_t srcRowPitch = 0;
  uint32_t srcSlicePitch = 0;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeCommandListAppendImageCopyFromMemoryExt(command_list, image, memory, nullptr, srcRowPitch, srcSlicePitch, nullptr, 0, nullptr);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageGetAllocPropertiesExtCallbacksWhenCallingzeDriverGetApiVersionThenUserDataIsSetAndResultUnchanged) {

  zelTracerImageGetAllocPropertiesExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerImageGetAllocPropertiesExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_image_allocation_ext_properties_t imageAllocProperties = {ZE_STRUCTURE_TYPE_IMAGE_ALLOCATION_EXT_PROPERTIES, nullptr};

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeImageGetAllocPropertiesExt(context, image, &imageAllocProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageGetMemoryPropertiesExpCallbacksWhenCallingzeImageGetMemoryPropertiesExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerImageGetMemoryPropertiesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerImageGetMemoryPropertiesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_image();

  ze_image_memory_properties_exp_t imageMemoryProperties = {ZE_STRUCTURE_TYPE_IMAGE_MEMORY_EXP_PROPERTIES, nullptr};

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeImageGetMemoryPropertiesExp(image, &imageMemoryProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageViewCreateExpCallbacksWhenCallingzeImageViewCreateExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerImageViewCreateExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerImageViewCreateExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_image();

  ze_image_handle_t imageView;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeImageViewCreateExp(context, device, &image_desc, image, &imageView);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  zeImageDestroy(imageView);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemoryPrefetchCallbacksWhenCallingzeCommandListAppendMemoryPrefetchThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendMemoryPrefetchRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendMemoryPrefetchRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();

  const size_t size = 1024;
  shared_memory = lzt::allocate_shared_memory(size);

  ze_result_t initial_result =
      zeCommandListAppendMemoryPrefetch(command_list, shared_memory, size / 2);
  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryPrefetch(
                                command_list, shared_memory, size / 2));
  lzt::free_memory(shared_memory);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendMemAdviseCallbacksWhenCallingzeCommandListAppendMemAdviseThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendMemAdviseRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendMemAdviseRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_memory();

  const size_t size = 1024;
  shared_memory = lzt::allocate_shared_memory(size);

  ze_result_t initial_result =
      zeCommandListAppendMemAdvise(command_list, device, shared_memory, size,
                                   ZE_MEMORY_ADVICE_SET_READ_MOSTLY);
  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendMemAdvise(
                                command_list, device, shared_memory, size,
                                ZE_MEMORY_ADVICE_SET_READ_MOSTLY));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendSignalEventCallbacksWhenCallingzeCommandListAppendSignalEventThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendSignalEventRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendSignalEventRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendSignalEvent(command_list, event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendSignalEvent(command_list, event));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendWaitOnEventsCallbacksWhenCallingzeCommandListAppendWaitOnEventsThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendWaitOnEventsRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendWaitOnEventsRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendWaitOnEvents(command_list, 1, &event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendWaitOnEvents(command_list, 1, &event));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendEventResetCallbacksWhenCallingzeCommandListAppendEventResetThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendEventResetRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendEventResetRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendEventReset(command_list, event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendEventReset(command_list, event));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendLaunchKernelCallbacksWhenCallingzeCommandListAppendLaunchKernelThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendLaunchKernelRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendLaunchKernelRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_kernel();

  ze_group_count_t args = {1, 0, 0};

  ze_result_t initial_result = zeCommandListAppendLaunchKernel(
      command_list, kernel, &args, nullptr, 0, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchKernel(command_list, kernel, &args,
                                            nullptr, 0, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendLaunchMultipleKernelsIndirectCallbacksWhenCallingzeCommandListAppendLaunchMultipleKernelsIndirectThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendLaunchMultipleKernelsIndirectRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendLaunchMultipleKernelsIndirectRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_kernel();

  uint32_t *count =
      static_cast<uint32_t *>(lzt::allocate_device_memory(sizeof(uint32_t)));
  ze_group_count_t *args = static_cast<ze_group_count_t *>(
      lzt::allocate_device_memory(sizeof(ze_group_count_t)));
  ze_result_t initial_result = zeCommandListAppendLaunchMultipleKernelsIndirect(
      command_list, 1, &kernel, count, args, nullptr, 0, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchMultipleKernelsIndirect(
                command_list, 1, &kernel, count, args, nullptr, 0, nullptr));

  lzt::free_memory(count);
  lzt::free_memory(args);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceCreateCallbacksWhenCallingzeFenceCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerFenceCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                       lzt::lprologue_callback);
  zelTracerFenceCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                       lzt::lepilogue_callback);

  init_command_queue();

  ze_result_t initial_result =
      zeFenceCreate(command_queue, &fence_desc, &fence);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeFenceDestroy(fence));
  ASSERT_EQ(initial_result, zeFenceCreate(command_queue, &fence_desc, &fence));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceDestroyCallbacksWhenCallingzeFenceDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerFenceDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                        lzt::lprologue_callback);
  zelTracerFenceDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                        lzt::lepilogue_callback);

  init_fence();

  ze_result_t initial_result = zeFenceDestroy(fence);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeFenceCreate(command_queue, &fence_desc, &fence));

  ASSERT_EQ(initial_result, zeFenceDestroy(fence));
  fence = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceHostSynchronizeCallbacksWhenCallingzeFenceHostSynchronizeThenUserDataIsSetAndResultUnchanged) {
  zelTracerFenceHostSynchronizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerFenceHostSynchronizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_fence();

  ze_result_t initial_result = zeFenceHostSynchronize(fence, 0);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeFenceHostSynchronize(fence, 0));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceQueryStatusCallbacksWhenCallingzeFenceQueryStatusThenUserDataIsSetAndResultUnchanged) {
  zelTracerFenceQueryStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerFenceQueryStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_fence();

  ze_result_t initial_result = zeFenceQueryStatus(fence);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeFenceQueryStatus(fence));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeFenceResetCallbacksWhenCallingzeFenceResetThenUserDataIsSetAndResultUnchanged) {
  zelTracerFenceResetRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                      lzt::lprologue_callback);
  zelTracerFenceResetRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                      lzt::lepilogue_callback);

  init_fence();

  ze_result_t initial_result = zeFenceReset(fence);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeFenceReset(fence));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolCreateCallbacksWhenCallingzeEventPoolCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventPoolCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                           lzt::lprologue_callback);
  zelTracerEventPoolCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                           lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeEventPoolCreate(context, &event_pool_desc, 1, &device, &event_pool);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolDestroy(event_pool));
  ASSERT_EQ(initial_result, zeEventPoolCreate(context, &event_pool_desc, 1,
                                              &device, &event_pool));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolDestroyCallbacksWhenCallingzeEventPoolDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventPoolDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerEventPoolDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_event_pool();

  ze_result_t initial_result = zeEventPoolDestroy(event_pool);

  lzt::enable_ltracer(tracer_handle);

  init_event_pool();
  ASSERT_EQ(initial_result, zeEventPoolDestroy(event_pool));
  event = nullptr;
  event_pool = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventCreateCallbacksWhenCallingzeEventCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                       lzt::lprologue_callback);
  zelTracerEventCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                       lzt::lepilogue_callback);

  init_event_pool();

  ze_result_t initial_result = zeEventCreate(event_pool, &event_desc, &event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventDestroy(event));
  ASSERT_EQ(initial_result, zeEventCreate(event_pool, &event_desc, &event));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventDestroyCallbacksWhenCallingzeEventDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                        lzt::lprologue_callback);
  zelTracerEventDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                        lzt::lepilogue_callback);

  init_event();

  ze_result_t initial_result = zeEventDestroy(event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventCreate(event_pool, &event_desc, &event));
  ASSERT_EQ(initial_result, zeEventDestroy(event));
  event = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolGetIpcHandleCallbacksWhenCallingzeEventPoolGetIpcHandleThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventPoolGetIpcHandleRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerEventPoolGetIpcHandleRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_event_pool();

  ze_result_t initial_result = zeEventPoolGetIpcHandle(event_pool, &ipc_event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeEventPoolGetIpcHandle(event_pool, &ipc_event));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolOpenIpcHandleCallbacksWhenCallingzeEventPoolOpenIpcHandleThenUserDataIsSetAndResultUnchanged) {

  run_ltracing_compat_ipc_event_test("TEST_OPEN_IPC_EVENT");
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventPoolCloseIpcHandleCallbacksWhenCallingzeEventPoolCloseIpcHandleThenUserDataIsSetAndResultUnchanged) {

  run_ltracing_compat_ipc_event_test("TEST_CLOSE_IPC_EVENT");
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventHostSignalCallbacksWhenCallingzeEventHostSignalThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventHostSignalRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                           lzt::lprologue_callback);
  zelTracerEventHostSignalRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                           lzt::lepilogue_callback);

  init_event();

  ze_result_t initial_result = zeEventHostSignal(event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeEventHostSignal(event));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventHostSynchronizeCallbacksWhenCallingzeEventHostSynchronizeThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventHostSynchronizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerEventHostSynchronizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_event();

  ze_result_t initial_result = zeEventHostSynchronize(event, 0);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeEventHostSynchronize(event, 0));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventQueryStatusCallbacksWhenCallingzeEventQueryStatusThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventQueryStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerEventQueryStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_event();

  ze_result_t initial_result = zeEventQueryStatus(event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeEventQueryStatus(event));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeEventHostResetCallbacksWhenCallingzeEventHostResetThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventHostResetRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                          lzt::lprologue_callback);
  zelTracerEventHostResetRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                          lzt::lepilogue_callback);

  init_event();

  ze_result_t initial_result = zeEventHostReset(event);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeEventHostReset(event));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageCreateCallbacksWhenCallingzeImageCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerImageCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                       lzt::lprologue_callback);
  zelTracerImageCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                       lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeImageCreate(context, device, &image_desc, &image);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(image));
  ASSERT_EQ(initial_result,
            zeImageCreate(context, device, &image_desc, &image));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageGetPropertiesCallbacksWhenCallingzeImageGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerImageGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerImageGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeImageGetProperties(device, &image_desc, &image_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeImageGetProperties(device, &image_desc, &image_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeImageDestroyCallbacksWhenCallingzeImageDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerImageDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                        lzt::lprologue_callback);
  zelTracerImageDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                        lzt::lepilogue_callback);

  init_image();
  ze_result_t initial_result = zeImageDestroy(image);

  lzt::enable_ltracer(tracer_handle);

  init_image();
  ASSERT_EQ(initial_result, zeImageDestroy(image));
  image = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleCreateCallbacksWhenCallingzeModuleCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                        lzt::lprologue_callback);
  zelTracerModuleCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                        lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeModuleCreate(context, device, &module_desc, &module, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module));
  ASSERT_EQ(initial_result,
            zeModuleCreate(context, device, &module_desc, &module, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleDestroyCallbacksWhenCallingzeModuleDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                         lzt::lprologue_callback);
  zelTracerModuleDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                         lzt::lepilogue_callback);

  init_module();

  ze_result_t initial_result = zeModuleDestroy(module);

  lzt::enable_ltracer(tracer_handle);

  init_module();
  ASSERT_EQ(initial_result, zeModuleDestroy(module));
  module = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleGetNativeBinaryCallbacksWhenCallingzeModuleGetNativeBinaryThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleGetNativeBinaryRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleGetNativeBinaryRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_module();

  size_t binary_size;

  ze_result_t initial_result =
      zeModuleGetNativeBinary(module, &binary_size, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeModuleGetNativeBinary(module, &binary_size, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleGetGlobalPointerCallbacksWhenCallingzeModuleGetGlobalPointerThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleGetGlobalPointerRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleGetGlobalPointerRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_module();

  void *pointer;

  ze_result_t initial_result =
      zeModuleGetGlobalPointer(module, "xid", nullptr, &pointer);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeModuleGetGlobalPointer(module, "xid", nullptr, &pointer));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleGetFunctionPointerCallbacksWhenCallingzeModuleGetFunctionPointerThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleGetFunctionPointerRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleGetFunctionPointerRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_module();
  void *function_pointer;

  ze_result_t initial_result = zeModuleGetFunctionPointer(
      module, "module_add_constant", &function_pointer);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeModuleGetFunctionPointer(module, "module_add_constant",
                                       &function_pointer));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleBuildLogDestroyCallbacksWhenCallingzeModuleBuildLogDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleBuildLogDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleBuildLogDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_module();

  ze_result_t initial_result = zeModuleBuildLogDestroy(build_log);
  ze_module_handle_t module2;

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleCreate(context, device, &module_desc,
                                              &module2, &build_log));
  ASSERT_EQ(initial_result, zeModuleBuildLogDestroy(build_log));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module2));
  build_log = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleBuildLogGetStringCallbacksWhenCallingzeModuleBuildLogGetStringThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleBuildLogGetStringRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleBuildLogGetStringRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_module();

  size_t log_size;
  ze_result_t initial_result =
      zeModuleBuildLogGetString(build_log, &log_size, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeModuleBuildLogGetString(build_log, &log_size, nullptr));
}


TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeModuleInspectLinkageExtCallbacksWhenCallingzeModuleInspectLinkageExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerModuleInspectLinkageExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleInspectLinkageExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_linkage_inspection_ext_desc_t linkageInspectDesc = {ZE_STRUCTURE_TYPE_LINKAGE_INSPECTION_EXT_DESC, nullptr, 0};
  uint32_t numModules = 1;

  init_module();

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeModuleInspectLinkageExt(&linkageInspectDesc, numModules, &module, &build_log);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelCreateCallbacksWhenCallingzeKernelCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                        lzt::lprologue_callback);
  zelTracerKernelCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                        lzt::lepilogue_callback);

  init_module();
  kernel_desc = {ZE_STRUCTURE_TYPE_KERNEL_DESC, nullptr, 0,
                 "module_add_constant"};
  ze_result_t initial_result = zeKernelCreate(module, &kernel_desc, &kernel);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeKernelDestroy(kernel));
  ASSERT_EQ(initial_result, zeKernelCreate(module, &kernel_desc, &kernel));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelDestroyCallbacksWhenCallingzeKernelDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                         lzt::lprologue_callback);
  zelTracerKernelDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                         lzt::lepilogue_callback);

  init_kernel();

  ze_result_t initial_result = zeKernelDestroy(kernel);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeKernelCreate(module, &kernel_desc, &kernel));

  ASSERT_EQ(initial_result, zeKernelDestroy(kernel));
  kernel = nullptr;
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeCommandListAppendLaunchKernelIndirectCallbacksWhenCallingzeCommandListAppendLaunchKernelIndirectThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendLaunchKernelIndirectRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendLaunchKernelIndirectRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_kernel();

  ze_group_count_t args = {1, 0, 0};

  ze_result_t initial_result = zeCommandListAppendLaunchKernelIndirect(
      command_list, kernel, &args, nullptr, 0, nullptr);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchKernelIndirect(command_list, kernel, &args,
                                                    nullptr, 0, nullptr));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelSetGroupSizeCallbacksWhenCallingzeKernelSetGroupSizeThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelSetGroupSizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelSetGroupSizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();

  ze_result_t initial_result = zeKernelSetGroupSize(kernel, 1, 1, 1);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeKernelSetGroupSize(kernel, 1, 1, 1));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelSetGlobalOffsetExpCallbacksWhenCallingzeKernelSetGlobalOffsetExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerKernelSetGlobalOffsetExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelSetGlobalOffsetExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();
  uint32_t offsetX = 0;
  uint32_t offsetY = 0;
  uint32_t offsetZ = 0;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeKernelSetGlobalOffsetExp(kernel, offsetX, offsetY, offsetZ);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelSchedulingHintExpCallbacksWhenCallingzeKernelSchedulingHintExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerKernelSchedulingHintExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelSchedulingHintExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_scheduling_hint_exp_desc_t schedulingHint = {ZE_STRUCTURE_TYPE_SCHEDULING_HINT_EXP_DESC, nullptr, ZE_SCHEDULING_HINT_EXP_FLAG_OLDEST_FIRST};

  init_kernel();

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeKernelSchedulingHintExp(kernel, &schedulingHint);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelSuggestGroupSizeCallbacksWhenCallingzeKernelSuggestGroupSizeThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelSuggestGroupSizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelSuggestGroupSizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();

  uint32_t x, y, z;

  ze_result_t initial_result =
      zeKernelSuggestGroupSize(kernel, 1, 1, 1, &x, &y, &z);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeKernelSuggestGroupSize(kernel, 1, 1, 1, &x, &y, &z));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelSetArgumentValueCallbacksWhenCallingzeKernelSetArgumentValueThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelSetArgumentValueRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelSetArgumentValueRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();

  int val = 1;
  ze_result_t initial_result =
      zeKernelSetArgumentValue(kernel, 1, sizeof(int), &val);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeKernelSetArgumentValue(kernel, 1, sizeof(int), &val));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeKernelGetPropertiesCallbacksWhenCallingzeKernelGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();

  ze_kernel_properties_t kernel_properties = {};
  kernel_properties.stype = ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES;
  ze_result_t initial_result =
      zeKernelGetProperties(kernel, &kernel_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeKernelGetProperties(kernel, &kernel_properties));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeSamplerCreateCallbacksWhenCallingzeSamplerCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerSamplerCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                         lzt::lprologue_callback);
  zelTracerSamplerCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                         lzt::lepilogue_callback);

  ze_result_t initial_result =
      zeSamplerCreate(context, device, &sampler_desc, &sampler);
  if (initial_result == ZE_RESULT_SUCCESS) {
    zeSamplerDestroy(sampler);
  }

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeSamplerCreate(context, device, &sampler_desc, &sampler));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeSamplerDestroy(sampler));
}

TEST_F(
    LCTracingPrologueEpilogueTests,
    GivenEnabledTracerWithzeSamplerDestroyCallbacksWhenCallingzeSamplerDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerSamplerDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                          lzt::lprologue_callback);
  zelTracerSamplerDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                          lzt::lepilogue_callback);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeSamplerCreate(context, device, &sampler_desc, &sampler));

  ze_result_t initial_result = zeSamplerDestroy(sampler);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeSamplerCreate(context, device, &sampler_desc, &sampler));
  ASSERT_EQ(initial_result, zeSamplerDestroy(sampler));
}
} // namespace
