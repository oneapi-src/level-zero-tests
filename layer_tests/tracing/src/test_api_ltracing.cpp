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
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

namespace lzt = level_zero_tests;
namespace bipc = boost::interprocess;

namespace {

#ifdef USE_RUNTIME_TRACING
class LDynamicTracingCreateTests : public ::testing::Test {};
#define LTRACING_CREATE_TEST_NAME LDynamicTracingCreateTests
#else // USE Tracing ENV
class LTracingCreateTests : public ::testing::Test {};
#define LTRACING_CREATE_TEST_NAME LTracingCreateTests
#endif

TEST(
    LTRACING_CREATE_TEST_NAME,
    GivenValidDeviceAndTracerDescriptionWhenCreatingTracerThenTracerIsNotNull) {
  uint32_t user_data;

  zel_tracer_desc_t tracer_desc = {ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC, nullptr,
                                   &user_data};

  zel_tracer_handle_t tracer_handle = lzt::create_ltracer_handle(tracer_desc);
  EXPECT_NE(tracer_handle, nullptr);

  lzt::destroy_ltracer_handle(tracer_handle);
}

class LTracingCreateMultipleTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<uint32_t> {};

#ifdef USE_RUNTIME_TRACING
class LDynamicTracingCreateMultipleTests : public LTracingCreateMultipleTests {};
#define LTRACING_CREATE_MULTIPLE_TEST_NAME LDynamicTracingCreateMultipleTests
#else // USE Tracing ENV
#define LTRACING_CREATE_MULTIPLE_TEST_NAME LTracingCreateMultipleTests
#endif

TEST_P(LTRACING_CREATE_MULTIPLE_TEST_NAME,
       GivenExistingTracersWhenCreatingNewTracerThenSuccesIsReturned) {

  std::vector<zel_tracer_handle_t> tracers(GetParam());
  std::vector<zel_tracer_desc_t> descs(GetParam());
  uint32_t *user_data = new uint32_t[GetParam()];

  for (uint32_t i = 0; i < GetParam(); ++i) {

    descs[i].stype = ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC;
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

INSTANTIATE_TEST_CASE_P(CreateMultipleTracerTest, LTRACING_CREATE_MULTIPLE_TEST_NAME,
                        ::testing::Values(1, 10, 100, 1000));

#ifdef USE_RUNTIME_TRACING
class LDynamicTracingDestroyTests : public ::testing::Test {};
#define LTRACING_DESTROY_TEST_NAME LDynamicTracingDestroyTests
#else // USE Tracing ENV
class LTracingDestroyTests : public ::testing::Test {};
#define LTRACING_DESTROY_TEST_NAME LTracingDestroyTests
#endif

TEST(LTRACING_DESTROY_TEST_NAME,
     GivenSingleDisabledTracerWhenDestroyingTracerThenSuccessIsReturned) {
  uint32_t user_data;

  zel_tracer_desc_t tracer_desc = {ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC, nullptr,
                                   &user_data};
  zel_tracer_handle_t tracer_handle = lzt::create_ltracer_handle(tracer_desc);

  lzt::destroy_ltracer_handle(tracer_handle);
}

class LTracingPrologueEpilogueTests : public ::testing::Test {
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

    zel_tracer_desc_t tracer_desc = {ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC,
                                     nullptr, &user_data};
    tracer_handle = lzt::create_ltracer_handle(tracer_desc);
  }

  void run_ltracing_ipc_event_test(std::string test_type_name) {
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

    lzt::shared_ipc_event_data_t test_data = {hIpcEventPool};
    bipc::shared_memory_object shm(bipc::create_only, "ipc_ltracing_event_test",
                                   bipc::read_write);
    shm.truncate(sizeof(lzt::shared_ipc_event_data_t));
    bipc::mapped_region region(shm, bipc::read_write);
    std::memcpy(region.get_address(), &test_data,
                sizeof(lzt::shared_ipc_event_data_t));

    #ifdef USE_RUNTIME_TRACING
    // launch child
    boost::process::child c("./tracing/test_ltracing_ipc_event_helper_dynamic",
                            test_type_name.c_str());
    #else
    // launch child
    boost::process::child c("./tracing/test_ltracing_ipc_event_helper",
                            test_type_name.c_str());
    #endif
    lzt::send_ipc_handle(hIpcEventPool);

    c.wait(); // wait for the process to exit

    // cleanup
    bipc::shared_memory_object::remove("ipc_ltracing_event_test");
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

  bool init_image() {
    return ZE_RESULT_SUCCESS ==
           zeImageCreate(context, device, &image_desc, &image);
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
  ze_callbacks_t prologues = {};
  ze_callbacks_t epilogues = {};

  ze_driver_handle_t driver;

  ze_device_handle_t device;
  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES,
                                       nullptr};
  ze_device_compute_properties_t compute_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES, nullptr};

  ze_device_memory_properties_t memory_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES, nullptr};

  ze_device_memory_access_properties_t memory_access_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_MEMORY_ACCESS_PROPERTIES, nullptr};

  ze_device_cache_properties_t cache_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES, nullptr};
  ze_device_image_properties_t device_image_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES, nullptr};
  ze_device_p2p_properties_t p2p_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES, nullptr};

  ze_api_version_t api_version;
  ze_driver_ipc_properties_t ipc_properties = {
      ZE_STRUCTURE_TYPE_DRIVER_IPC_PROPERTIES, nullptr};

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
  ze_image_properties_t image_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES, nullptr};

  void *device_memory, *host_memory, *shared_memory, *memory = nullptr;

  ze_memory_allocation_properties_t mem_alloc_properties = {
      ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES, nullptr};

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
  ze_driver_properties_t driver_properties = {
      ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES, nullptr};
  ze_bool_t can_access;
};

static void ready_ltracer(zel_tracer_handle_t tracer, ze_callbacks_t prologues,
                          ze_callbacks_t epilogues) {
  lzt::set_ltracer_prologues(tracer, prologues);
  lzt::set_ltracer_epilogues(tracer, epilogues);
  lzt::enable_ltracer(tracer);
}

#ifdef USE_RUNTIME_TRACING
class LDynamicTracingPrologueEpilogueTests : public LTracingPrologueEpilogueTests {};
#define LTRACING_TEST_NAME LDynamicTracingPrologueEpilogueTests
#else // USE Tracing ENV
#define LTRACING_TEST_NAME LTracingPrologueEpilogueTests
#endif

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetCallbacksWhenCallingzeDeviceGetThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetCb = lzt::lepilogue_callback;

  ze_result_t initial_result = zeDeviceGet(driver, &num, nullptr);
  ready_ltracer(tracer_handle, prologues, epilogues);
  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGet(driver, &num, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithZeDeviceGetSubDevicesCallbacksWhenCallingzeDeviceGetSubDevicesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetSubDevicesCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetSubDevicesCb = lzt::lepilogue_callback;

  ze_result_t initial_result = zeDeviceGetSubDevices(device, &num, nullptr);
  ready_ltracer(tracer_handle, prologues, epilogues);
  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGetSubDevices(device, &num, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetPropertiesCallbacksWhenCallingzeDeviceGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetPropertiesCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetPropertiesCb = lzt::lepilogue_callback;

  ze_result_t initial_result = zeDeviceGetProperties(device, &properties);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDeviceGetProperties(device, &properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetComputePropertiesCallbacksWhenCallingzeDeviceGetComputePropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetComputePropertiesCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetComputePropertiesCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetComputeProperties(device, &compute_properties);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeDeviceGetComputeProperties(device, &compute_properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetMemoryPropertiesCallbacksWhenCallingzeDeviceGetMemoryPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetMemoryPropertiesCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetMemoryPropertiesCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetMemoryProperties(device, &num, nullptr);
  ready_ltracer(tracer_handle, prologues, epilogues);
  num = 0;
  ASSERT_EQ(initial_result, zeDeviceGetMemoryProperties(device, &num, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetMemoryAccessPropertiesCallbacksWhenCallingzeDeviceGetMemoryAccessPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetMemoryAccessPropertiesCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetMemoryAccessPropertiesCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetMemoryAccessProperties(device, &memory_access_properties);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDeviceGetMemoryAccessProperties(
                                device, &memory_access_properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetCachePropertiesCallbacksWhenCallingzeDeviceGetCachePropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetCachePropertiesCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetCachePropertiesCb = lzt::lepilogue_callback;
  uint32_t count = 0;
  ze_result_t initial_result =
      zeDeviceGetCacheProperties(device, &count, &cache_properties);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeDeviceGetCacheProperties(device, &count, &cache_properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetImagePropertiesCallbacksWhenCallingzeDeviceGetImagePropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetImagePropertiesCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetImagePropertiesCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetImageProperties(device, &device_image_properties);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeDeviceGetImageProperties(device, &device_image_properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetP2PPropertiesCallbacksWhenCallingzeDeviceGetP2PPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnGetP2PPropertiesCb = lzt::lprologue_callback;
  epilogues.Device.pfnGetP2PPropertiesCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeDeviceGetP2PProperties(device, device, &p2p_properties);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeDeviceGetP2PProperties(device, device, &p2p_properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceCanAccessPeerCallbacksWhenCallingzeDeviceCanAccessPeerThenUserDataIsSetAndResultUnchanged) {
  prologues.Device.pfnCanAccessPeerCb = lzt::lprologue_callback;
  epilogues.Device.pfnCanAccessPeerCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeDeviceCanAccessPeer(device, device, &can_access);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDeviceCanAccessPeer(device, device, &can_access));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceSystemBarrierCallbacksWhenCallingzeDeviceSystemBarrierThenUserDataIsSetAndResultUnchanged) {
  prologues.Context.pfnSystemBarrierCb = lzt::lprologue_callback;
  epilogues.Context.pfnSystemBarrierCb = lzt::lepilogue_callback;

  ze_result_t initial_result = zeContextSystemBarrier(context, device);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeContextSystemBarrier(context, device));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceMakeMemoryResidentCallbacksWhenCallingzeDeviceMakeMemoryResidentThenUserDataIsSetAndResultUnchanged) {
  prologues.Context.pfnMakeMemoryResidentCb = lzt::lprologue_callback;
  epilogues.Context.pfnMakeMemoryResidentCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeContextMakeMemoryResident(context, device, memory, 0);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeContextMakeMemoryResident(context, device, memory, 0));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceEvictMemoryCallbacksWhenCallingzeDeviceEvictMemoryThenUserDataIsSetAndResultUnchanged) {
  prologues.Context.pfnEvictMemoryCb = lzt::lprologue_callback;
  epilogues.Context.pfnEvictMemoryCb = lzt::lepilogue_callback;

  init_memory();

  ze_result_t initial_result = zeContextEvictMemory(context, device, memory, 0);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeContextEvictMemory(context, device, memory, 0));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceMakeImageResidentCallbacksWhenCallingzeDeviceMakeImageResidentThenUserDataIsSetAndResultUnchangedAndResultUnchanged) {
  prologues.Context.pfnMakeImageResidentCb = lzt::lprologue_callback;
  epilogues.Context.pfnMakeImageResidentCb = lzt::lepilogue_callback;

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeContextMakeImageResident(context, device, image));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceEvictImageCallbacksWhenCallingzeDeviceEvictImageThenUserDataIsSetAndResultUnchanged) {
  prologues.Context.pfnEvictImageCb = lzt::lprologue_callback;
  epilogues.Context.pfnEvictImageCb = lzt::lepilogue_callback;

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeContextEvictImage(context, device, image));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverGetCallbacksWhenCallingzeDriverGetThenUserDataIsSetAndResultUnchanged) {
  prologues.Driver.pfnGetCb = lzt::lprologue_callback;
  epilogues.Driver.pfnGetCb = lzt::lepilogue_callback;

  ze_result_t initial_result = zeDriverGet(&num, nullptr);
  ready_ltracer(tracer_handle, prologues, epilogues);
  num = 0;
  ASSERT_EQ(initial_result, zeDriverGet(&num, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverGetPropertiesCallbacksWhenCallingzeDriverGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Driver.pfnGetPropertiesCb = lzt::lprologue_callback;
  epilogues.Driver.pfnGetPropertiesCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeDriverGetProperties(driver, &driver_properties);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDriverGetProperties(driver, &driver_properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverGetApiVersionCallbacksWhenCallingzeDriverGetApiVersionThenUserDataIsSetAndResultUnchanged) {
  prologues.Driver.pfnGetApiVersionCb = lzt::lprologue_callback;
  epilogues.Driver.pfnGetApiVersionCb = lzt::lepilogue_callback;

  ze_result_t initial_result = zeDriverGetApiVersion(driver, &api_version);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDriverGetApiVersion(driver, &api_version));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverGetIPCPropertiesCallbacksWhenCallingzeDriverGetIPCPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Driver.pfnGetIpcPropertiesCb = lzt::lprologue_callback;
  epilogues.Driver.pfnGetIpcPropertiesCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeDriverGetIpcProperties(driver, &ipc_properties);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeDriverGetIpcProperties(driver, &ipc_properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverAllocSharedMemCallbacksWhenCallingzeDriverAllocSharedMemThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnAllocSharedCb = lzt::lprologue_callback;
  epilogues.Mem.pfnAllocSharedCb = lzt::lepilogue_callback;

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

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeMemAllocShared(context, &device_desc, &host_desc,
                                             1, 0, device, &shared_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, shared_memory));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverAllocDeviceMemCallbacksWhenCallingzeDriverAllocDeviceMemThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnAllocDeviceCb = lzt::lprologue_callback;
  epilogues.Mem.pfnAllocDeviceCb = lzt::lepilogue_callback;

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.flags = 0;
  device_desc.ordinal = 1;

  ze_result_t initial_result =
      zeMemAllocDevice(context, &device_desc, 1, 1, device, &device_memory);
  lzt::free_memory(device_memory);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeMemAllocDevice(context, &device_desc, 1, 1,
                                             device, &device_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, device_memory));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverAllocHostMemCallbacksWhenCallingzeDriverAllocHostMemThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnAllocHostCb = lzt::lprologue_callback;
  epilogues.Mem.pfnAllocHostCb = lzt::lepilogue_callback;

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  ze_result_t initial_result =
      zeMemAllocHost(context, &host_desc, 1, 1, &host_memory);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeMemAllocHost(context, &host_desc, 1, 1, &host_memory));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, host_memory));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverFreeMemCallbacksWhenCallingzeDriverFreeMemThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnFreeCb = lzt::lprologue_callback;
  epilogues.Mem.pfnFreeCb = lzt::lepilogue_callback;

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;

  zeMemAllocHost(context, &host_desc, 1, 0, &host_memory);
  ze_result_t initial_result = zeMemFree(context, host_memory);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocHost(context, &host_desc, 1, 0, &host_memory));
  ASSERT_EQ(initial_result, zeMemFree(context, host_memory));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverGetMemAllocPropertiesCallbacksWhenCallingzeDriverGetMemAllocPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnGetAllocPropertiesCb = lzt::lprologue_callback;
  epilogues.Mem.pfnGetAllocPropertiesCb = lzt::lepilogue_callback;

  init_memory();

  mem_alloc_properties.pNext = nullptr;
  mem_alloc_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;

  ze_result_t initial_result =
      zeMemGetAllocProperties(context, memory, &mem_alloc_properties, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(
      initial_result,
      zeMemGetAllocProperties(context, memory, &mem_alloc_properties, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverGetMemAddressRangeCallbacksWhenCallingzeDriverGetMemAddressRangeThenUserDataIsSetAndResultUnchanged) {

  prologues.Mem.pfnGetAddressRangeCb = lzt::lprologue_callback;
  epilogues.Mem.pfnGetAddressRangeCb = lzt::lepilogue_callback;

  init_memory();

  ze_result_t initial_result =
      zeMemGetAddressRange(context, memory, nullptr, nullptr);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeMemGetAddressRange(context, memory, nullptr, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverGetMemIpcHandleCallbacksWhenCallingzeDriverGetMemIpcHandleThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnGetIpcHandleCb = lzt::lprologue_callback;
  epilogues.Mem.pfnGetIpcHandleCb = lzt::lepilogue_callback;

  init_memory();

  ze_result_t initial_result = zeMemGetIpcHandle(context, memory, &ipc_handle);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeMemGetIpcHandle(context, memory, &ipc_handle));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverOpenMemIpcHandleCallbacksWhenCallingzeDriverOpenMemIpcHandleThenUserDataIsSetAndResultUnchanged) {
  prologues.Mem.pfnOpenIpcHandleCb = lzt::lprologue_callback;
  epilogues.Mem.pfnOpenIpcHandleCb = lzt::lepilogue_callback;

  init_ipc();

  void *mem = nullptr;

  ze_result_t initial_result =
      zeMemOpenIpcHandle(context, device, ipc_handle, 0, &mem);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeMemOpenIpcHandle(context, device, ipc_handle, 0, &mem));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandQueueCreateCallbacksWhenCallingzeCommandQueueCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandQueue.pfnCreateCb = lzt::lprologue_callback;
  epilogues.CommandQueue.pfnCreateCb = lzt::lepilogue_callback;

  ze_result_t initial_result = zeCommandQueueCreate(
      context, device, &command_queue_desc, &command_queue);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueDestroy(command_queue));
  ASSERT_EQ(initial_result,
            zeCommandQueueCreate(context, device, &command_queue_desc,
                                 &command_queue));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandQueueDestroyCallbacksWhenCallingzeCommandQueueDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandQueue.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.CommandQueue.pfnDestroyCb = lzt::lepilogue_callback;

  init_command_queue();

  ze_result_t initial_result = zeCommandQueueDestroy(command_queue);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeCommandQueueCreate(context, device, &command_queue_desc,
                                 &command_queue));
  ASSERT_EQ(initial_result, zeCommandQueueDestroy(command_queue));
  command_queue = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandQueueExecuteCommandListsCallbacksWhenCallingzeCommandQueueExecuteCommandListsThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandQueue.pfnExecuteCommandListsCb = lzt::lprologue_callback;
  epilogues.CommandQueue.pfnExecuteCommandListsCb = lzt::lepilogue_callback;

  init_command_list();
  init_command_queue();

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(command_list));

  ze_result_t initial_result = zeCommandQueueExecuteCommandLists(
      command_queue, 1, &command_list, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandQueueExecuteCommandLists(
                                command_queue, 1, &command_list, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandQueueSynchronizeCallbacksWhenCallingzeCommandQueueSynchronizeThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandQueue.pfnSynchronizeCb = lzt::lprologue_callback;
  epilogues.CommandQueue.pfnSynchronizeCb = lzt::lepilogue_callback;

  init_command_queue();

  ze_result_t initial_result = zeCommandQueueSynchronize(command_queue, 0);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandQueueSynchronize(command_queue, 0));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListCreateCallbacksWhenCallingzeCommandListCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnCreateCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnCreateCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeCommandListCreate(context, device, &command_list_desc, &command_list);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListDestroy(command_list));
  ASSERT_EQ(
      initial_result,
      zeCommandListCreate(context, device, &command_list_desc, &command_list));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListResetCallbacksWhenCallingzeCommandListResetThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnResetCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnResetCb = lzt::lepilogue_callback;

  init_command_list();

  ze_result_t initial_result = zeCommandListReset(command_list);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListReset(command_list));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListCloseCallbacksWhenCallingzeCommandListCloseThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnCloseCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnCloseCb = lzt::lepilogue_callback;

  init_command_list();

  ze_result_t initial_result = zeCommandListClose(command_list);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListClose(command_list));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListDestroyCallbacksWhenCallingzeCommandListDestroyThenUserDataIsSetAndResultUnchanged) {

  prologues.CommandList.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnDestroyCb = lzt::lepilogue_callback;

  init_command_list();

  ze_result_t initial_result = zeCommandListDestroy(command_list);
  ready_ltracer(tracer_handle, prologues, epilogues);
  init_command_list();

  ASSERT_EQ(initial_result, zeCommandListDestroy(command_list));
  command_list = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListCreateImmediateCallbacksWhenCallingzeCommandListCreateImmediateThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnCreateImmediateCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnCreateImmediateCb = lzt::lepilogue_callback;

  ze_result_t initial_result = zeCommandListCreateImmediate(
      context, device, &command_queue_desc, &command_list);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListDestroy(command_list));
  ASSERT_EQ(initial_result,
            zeCommandListCreateImmediate(context, device, &command_queue_desc,
                                         &command_list));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendBarrierCallbacksWhenCallingzeCommandListAppendBarrierThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendBarrierCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendBarrierCb = lzt::lepilogue_callback;

  init_command_list();

  ze_result_t initial_result =
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendMemoryRangesBarrierCallbacksWhenCallingzeCommandListAppendMemoryRangesBarrierThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryRangesBarrierCb =
      lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendMemoryRangesBarrierCb =
      lzt::lepilogue_callback;

  init_command_list();

  const std::vector<size_t> range_sizes{4096, 8192};
  std::vector<const void *> ranges{
      lzt::allocate_device_memory(range_sizes[0] * 2),
      lzt::allocate_device_memory(range_sizes[1] * 2)};

  ze_result_t initial_result = zeCommandListAppendMemoryRangesBarrier(
      command_list, ranges.size(), range_sizes.data(), ranges.data(), nullptr,
      0, nullptr);

  zeCommandListReset(command_list);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryRangesBarrier(
                                command_list, ranges.size(), range_sizes.data(),
                                ranges.data(), nullptr, 0, nullptr));

  for (auto &range : ranges)
    lzt::free_memory(range);
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendMemoryCopyCallbacksWhenCallingzeCommandListAppendMemoryCopyThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryCopyCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendMemoryCopyCb = lzt::lepilogue_callback;

  init_command_list();

  void *src = lzt::allocate_device_memory(10);
  void *dst = lzt::allocate_device_memory(10);

  ze_result_t initial_result = zeCommandListAppendMemoryCopy(
      command_list, dst, src, 10, nullptr, 0, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendMemoryCopy(command_list, dst, src, 10, nullptr,
                                          0, nullptr));
  lzt::free_memory(src);
  lzt::free_memory(dst);
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendMemoryFillCallbacksWhenCallingzeCommandListAppendMemoryFillThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryFillCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendMemoryFillCb = lzt::lepilogue_callback;

  init_command_list();
  init_memory();

  uint32_t val = 1;
  ze_result_t initial_result = zeCommandListAppendMemoryFill(
      command_list, memory, &val, sizeof(uint32_t), 1, nullptr, 0, nullptr);

  zeCommandListReset(command_list);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryFill(
                                command_list, memory, &val, sizeof(uint32_t), 1,
                                nullptr, 0, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendMemoryCopyFromContextCallbacksWhenCallingzeCommandListAppendMemoryCopyFromContextThenUserDataIsSetAndResultUnchanged) {

  prologues.CommandList.pfnAppendMemoryCopyFromContextCb =
      lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendMemoryCopyFromContextCb =
      lzt::lepilogue_callback;

  init_command_list();

  const size_t size = 4 * 1024;
  ze_context_handle_t src_context = lzt::create_context();
  ze_context_handle_t dflt_context = lzt::get_default_context();
  EXPECT_NE(src_context, dflt_context);

  void *host_memory_src_ctx = lzt::allocate_host_memory(size, 1, src_context);
  void *host_memory_dflt_ctx = lzt::allocate_host_memory(size);

  ze_result_t initial_result = zeCommandListAppendMemoryCopyFromContext(
      command_list, host_memory_dflt_ctx, src_context, host_memory_src_ctx,
      size, nullptr, 0, 0);

  zeCommandListReset(command_list);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryCopyFromContext(
                                command_list, host_memory_dflt_ctx, src_context,
                                host_memory_src_ctx, size, nullptr, 0, 0));

  lzt::free_memory(host_memory_src_ctx);
  lzt::free_memory(host_memory_dflt_ctx);
  lzt::destroy_context(src_context);
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendQueryKernelTimestampsCallbacksWhenCallingzeCommandListAppendQueryKernelTimestampsThenUserDataIsSetAndResultUnchanged) {

  prologues.CommandList.pfnAppendQueryKernelTimestampsCb =
      lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendQueryKernelTimestampsCb =
      lzt::lepilogue_callback;

  void *timestamp_buffer = lzt::allocate_host_memory(
      sizeof(ze_kernel_timestamp_result_t), 8, context);
  ze_kernel_timestamp_result_t *tsResult =
      static_cast<ze_kernel_timestamp_result_t *>(timestamp_buffer);

  init_command_list();

  ze_result_t initial_result = zeCommandListAppendQueryKernelTimestamps(
      command_list, 0, nullptr, &tsResult, nullptr, nullptr, 0, nullptr);

  zeCommandListReset(command_list);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendQueryKernelTimestamps(
                                command_list, 0, nullptr, &tsResult, nullptr,
                                nullptr, 0, nullptr));

  lzt::free_memory(timestamp_buffer);
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendMemoryCopyRegionCallbacksWhenCallingzeCommandListAppendMemoryCopyRegionThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryCopyRegionCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendMemoryCopyRegionCb = lzt::lepilogue_callback;

  init_command_list();

  ze_copy_region_t src_region = {0, 0, 0, 1, 1, 0};
  ze_copy_region_t dst_region = {0, 0, 0, 1, 1, 0};

  void *src = lzt::allocate_device_memory(10);
  void *dst = lzt::allocate_device_memory(10);

  ze_result_t initial_result = zeCommandListAppendMemoryCopyRegion(
      command_list, dst, &dst_region, 0, 0, src, &src_region, 0, 0, nullptr, 0,
      nullptr);
  zeCommandListReset(command_list);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryCopyRegion(
                                command_list, dst, &dst_region, 0, 0, src,
                                &src_region, 0, 0, nullptr, 0, nullptr));
  lzt::free_memory(src);
  lzt::free_memory(dst);
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyCallbacksWhenCallingzeCommandListAppendImageCopyThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendImageCopyCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendImageCopyCb = lzt::lepilogue_callback;

  init_command_list();

  ze_image_handle_t src_image;
  ze_image_handle_t dst_image;

  ze_result_t result = zeImageCreate(context, device, &image_desc, &src_image);
  if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }
  ASSERT_EQ(ZE_RESULT_SUCCESS, result);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(context, device, &image_desc, &dst_image));

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendImageCopy(command_list, dst_image, src_image,
                                         nullptr, 0, nullptr));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(dst_image));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyRegionCallbacksWhenCallingzeCommandListAppendImageCopyRegionThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendImageCopyRegionCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendImageCopyRegionCb = lzt::lepilogue_callback;

  init_command_list();

  ze_image_handle_t src_image;
  ze_image_handle_t dst_image;
  ze_result_t result = zeImageCreate(context, device, &image_desc, &src_image);
  if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }
  ASSERT_EQ(ZE_RESULT_SUCCESS, result);
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeImageCreate(context, device, &image_desc, &dst_image));

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendImageCopyRegion(
                                   command_list, dst_image, src_image, nullptr,
                                   nullptr, nullptr, 0, nullptr));

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(src_image));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeImageDestroy(dst_image));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyFromMemoryCallbacksWhenCallingzeCommandListAppendImageCopyFromMemoryThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendImageCopyFromMemoryCb =
      lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendImageCopyFromMemoryCb =
      lzt::lepilogue_callback;

  init_command_list();
  init_memory();
  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendImageCopyFromMemory(
                command_list, image, memory, nullptr, nullptr, 0, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyToMemoryCallbacksWhenCallingzeCommandListAppendImageCopyToMemoryThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendImageCopyToMemoryCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendImageCopyToMemoryCb = lzt::lepilogue_callback;

  init_command_list();
  init_memory();
  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendImageCopyToMemory(command_list, memory, image,
                                                 nullptr, nullptr, 0, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendMemoryPrefetchCallbacksWhenCallingzeCommandListAppendMemoryPrefetchThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemoryPrefetchCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendMemoryPrefetchCb = lzt::lepilogue_callback;

  init_command_list();

  const size_t size = 1024;
  shared_memory = lzt::allocate_shared_memory(size);

  ze_result_t initial_result =
      zeCommandListAppendMemoryPrefetch(command_list, shared_memory, size / 2);
  zeCommandListReset(command_list);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryPrefetch(
                                command_list, shared_memory, size / 2));
  lzt::free_memory(shared_memory);
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendMemAdviseCallbacksWhenCallingzeCommandListAppendMemAdviseThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendMemAdviseCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendMemAdviseCb = lzt::lepilogue_callback;

  init_command_list();
  init_memory();

  const size_t size = 1024;
  shared_memory = lzt::allocate_shared_memory(size);

  ze_result_t initial_result =
      zeCommandListAppendMemAdvise(command_list, device, shared_memory, size,
                                   ZE_MEMORY_ADVICE_SET_READ_MOSTLY);
  zeCommandListReset(command_list);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendMemAdvise(
                                command_list, device, shared_memory, size,
                                ZE_MEMORY_ADVICE_SET_READ_MOSTLY));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendSignalEventCallbacksWhenCallingzeCommandListAppendSignalEventThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendSignalEventCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendSignalEventCb = lzt::lepilogue_callback;

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendSignalEvent(command_list, event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendSignalEvent(command_list, event));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendWaitOnEventsCallbacksWhenCallingzeCommandListAppendWaitOnEventsThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendWaitOnEventsCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendWaitOnEventsCb = lzt::lepilogue_callback;

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendWaitOnEvents(command_list, 1, &event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendWaitOnEvents(command_list, 1, &event));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendEventResetCallbacksWhenCallingzeCommandListAppendEventResetThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendEventResetCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendEventResetCb = lzt::lepilogue_callback;

  init_command_list();
  init_event();

  ze_result_t initial_result =
      zeCommandListAppendEventReset(command_list, event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeCommandListAppendEventReset(command_list, event));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendLaunchKernelCallbacksWhenCallingzeCommandListAppendLaunchKernelThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendLaunchKernelCb = lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendLaunchKernelCb = lzt::lepilogue_callback;

  init_command_list();
  init_kernel();

  ze_group_count_t args = {1, 0, 0};

  ze_result_t initial_result = zeCommandListAppendLaunchKernel(
      command_list, kernel, &args, nullptr, 0, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchKernel(command_list, kernel, &args,
                                            nullptr, 0, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendLaunchMultipleKernelsIndirectCallbacksWhenCallingzeCommandListAppendLaunchMultipleKernelsIndirectThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendLaunchMultipleKernelsIndirectCb =
      lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendLaunchMultipleKernelsIndirectCb =
      lzt::lepilogue_callback;

  init_command_list();
  init_kernel();

  uint32_t *count =
      static_cast<uint32_t *>(lzt::allocate_device_memory(sizeof(uint32_t)));
  ze_group_count_t *args = static_cast<ze_group_count_t *>(
      lzt::allocate_device_memory(sizeof(ze_group_count_t)));
  ze_result_t initial_result = zeCommandListAppendLaunchMultipleKernelsIndirect(
      command_list, 1, &kernel, count, args, nullptr, 0, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchMultipleKernelsIndirect(
                command_list, 1, &kernel, count, args, nullptr, 0, nullptr));

  lzt::free_memory(count);
  lzt::free_memory(args);
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeFenceCreateCallbacksWhenCallingzeFenceCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnCreateCb = lzt::lprologue_callback;
  epilogues.Fence.pfnCreateCb = lzt::lepilogue_callback;

  init_command_queue();

  ze_result_t initial_result =
      zeFenceCreate(command_queue, &fence_desc, &fence);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeFenceDestroy(fence));
  ASSERT_EQ(initial_result, zeFenceCreate(command_queue, &fence_desc, &fence));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeFenceDestroyCallbacksWhenCallingzeFenceDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.Fence.pfnDestroyCb = lzt::lepilogue_callback;

  init_fence();

  ze_result_t initial_result = zeFenceDestroy(fence);
  ready_ltracer(tracer_handle, prologues, epilogues);
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeFenceCreate(command_queue, &fence_desc, &fence));

  ASSERT_EQ(initial_result, zeFenceDestroy(fence));
  fence = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeFenceHostSynchronizeCallbacksWhenCallingzeFenceHostSynchronizeThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnHostSynchronizeCb = lzt::lprologue_callback;
  epilogues.Fence.pfnHostSynchronizeCb = lzt::lepilogue_callback;

  init_fence();

  ze_result_t initial_result = zeFenceHostSynchronize(fence, 0);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeFenceHostSynchronize(fence, 0));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeFenceQueryStatusCallbacksWhenCallingzeFenceQueryStatusThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnQueryStatusCb = lzt::lprologue_callback;
  epilogues.Fence.pfnQueryStatusCb = lzt::lepilogue_callback;

  init_fence();

  ze_result_t initial_result = zeFenceQueryStatus(fence);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeFenceQueryStatus(fence));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeFenceResetCallbacksWhenCallingzeFenceResetThenUserDataIsSetAndResultUnchanged) {
  prologues.Fence.pfnResetCb = lzt::lprologue_callback;
  epilogues.Fence.pfnResetCb = lzt::lepilogue_callback;

  init_fence();

  ze_result_t initial_result = zeFenceReset(fence);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeFenceReset(fence));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventPoolCreateCallbacksWhenCallingzeEventPoolCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.EventPool.pfnCreateCb = lzt::lprologue_callback;
  epilogues.EventPool.pfnCreateCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeEventPoolCreate(context, &event_pool_desc, 1, &device, &event_pool);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventPoolDestroy(event_pool));
  ASSERT_EQ(initial_result, zeEventPoolCreate(context, &event_pool_desc, 1,
                                              &device, &event_pool));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventPoolDestroyCallbacksWhenCallingzeEventPoolDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.EventPool.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.EventPool.pfnDestroyCb = lzt::lepilogue_callback;

  init_event_pool();

  ze_result_t initial_result = zeEventPoolDestroy(event_pool);
  ready_ltracer(tracer_handle, prologues, epilogues);
  init_event_pool();
  ASSERT_EQ(initial_result, zeEventPoolDestroy(event_pool));
  event = nullptr;
  event_pool = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventCreateCallbacksWhenCallingzeEventCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnCreateCb = lzt::lprologue_callback;
  epilogues.Event.pfnCreateCb = lzt::lepilogue_callback;

  init_event_pool();

  ze_result_t initial_result = zeEventCreate(event_pool, &event_desc, &event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventDestroy(event));
  ASSERT_EQ(initial_result, zeEventCreate(event_pool, &event_desc, &event));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventDestroyCallbacksWhenCallingzeEventDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.Event.pfnDestroyCb = lzt::lepilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventDestroy(event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeEventCreate(event_pool, &event_desc, &event));
  ASSERT_EQ(initial_result, zeEventDestroy(event));
  event = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventPoolGetIpcHandleCallbacksWhenCallingzeEventPoolGetIpcHandleThenUserDataIsSetAndResultUnchanged) {
  prologues.EventPool.pfnGetIpcHandleCb = lzt::lprologue_callback;
  epilogues.EventPool.pfnGetIpcHandleCb = lzt::lepilogue_callback;

  init_event_pool();

  ze_result_t initial_result = zeEventPoolGetIpcHandle(event_pool, &ipc_event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventPoolGetIpcHandle(event_pool, &ipc_event));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventPoolOpenIpcHandleCallbacksWhenCallingzeEventPoolOpenIpcHandleThenUserDataIsSetAndResultUnchanged) {

  run_ltracing_ipc_event_test("TEST_OPEN_IPC_EVENT");
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventPoolCloseIpcHandleCallbacksWhenCallingzeEventPoolCloseIpcHandleThenUserDataIsSetAndResultUnchanged) {

  run_ltracing_ipc_event_test("TEST_CLOSE_IPC_EVENT");
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventHostSignalCallbacksWhenCallingzeEventHostSignalThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnHostSignalCb = lzt::lprologue_callback;
  epilogues.Event.pfnHostSignalCb = lzt::lepilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventHostSignal(event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventHostSignal(event));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventHostSynchronizeCallbacksWhenCallingzeEventHostSynchronizeThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnHostSynchronizeCb = lzt::lprologue_callback;
  epilogues.Event.pfnHostSynchronizeCb = lzt::lepilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventHostSynchronize(event, 0);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventHostSynchronize(event, 0));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventQueryStatusCallbacksWhenCallingzeEventQueryStatusThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnQueryStatusCb = lzt::lprologue_callback;
  epilogues.Event.pfnQueryStatusCb = lzt::lepilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventQueryStatus(event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventQueryStatus(event));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventHostResetCallbacksWhenCallingzeEventHostResetThenUserDataIsSetAndResultUnchanged) {
  prologues.Event.pfnHostResetCb = lzt::lprologue_callback;
  epilogues.Event.pfnHostResetCb = lzt::lepilogue_callback;

  init_event();

  ze_result_t initial_result = zeEventHostReset(event);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeEventHostReset(event));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageCreateCallbacksWhenCallingzeImageCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Image.pfnCreateCb = lzt::lprologue_callback;
  epilogues.Image.pfnCreateCb = lzt::lepilogue_callback;

  ready_ltracer(tracer_handle, prologues, epilogues);

  ze_result_t result = zeImageCreate(context, device, &image_desc, &image);
  ASSERT_TRUE((result == ZE_RESULT_SUCCESS) ||
              (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageGetPropertiesCallbacksWhenCallingzeImageGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Image.pfnGetPropertiesCb = lzt::lprologue_callback;
  epilogues.Image.pfnGetPropertiesCb = lzt::lepilogue_callback;

  ready_ltracer(tracer_handle, prologues, epilogues);

  ze_result_t result =
      zeImageGetProperties(device, &image_desc, &image_properties);
  ASSERT_TRUE((result == ZE_RESULT_SUCCESS) ||
              (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageDestroyCallbacksWhenCallingzeImageDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Image.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.Image.pfnDestroyCb = lzt::lepilogue_callback;

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }
  ze_result_t initial_result = zeImageDestroy(image);
  ready_ltracer(tracer_handle, prologues, epilogues);
  init_image();
  ASSERT_EQ(initial_result, zeImageDestroy(image));
  image = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeModuleCreateCallbacksWhenCallingzeModuleCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnCreateCb = lzt::lprologue_callback;
  epilogues.Module.pfnCreateCb = lzt::lepilogue_callback;

  ze_result_t initial_result =
      zeModuleCreate(context, device, &module_desc, &module, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module));
  ASSERT_EQ(initial_result,
            zeModuleCreate(context, device, &module_desc, &module, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeModuleDestroyCallbacksWhenCallingzeModuleDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.Module.pfnDestroyCb = lzt::lepilogue_callback;

  init_module();

  ze_result_t initial_result = zeModuleDestroy(module);
  ready_ltracer(tracer_handle, prologues, epilogues);
  init_module();
  ASSERT_EQ(initial_result, zeModuleDestroy(module));
  module = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeModuleGetNativeBinaryCallbacksWhenCallingzeModuleGetNativeBinaryThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnGetNativeBinaryCb = lzt::lprologue_callback;
  epilogues.Module.pfnGetNativeBinaryCb = lzt::lepilogue_callback;

  init_module();

  size_t binary_size;

  ze_result_t initial_result =
      zeModuleGetNativeBinary(module, &binary_size, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeModuleGetNativeBinary(module, &binary_size, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeModuleGetGlobalPointerCallbacksWhenCallingzeModuleGetGlobalPointerThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnGetGlobalPointerCb = lzt::lprologue_callback;
  epilogues.Module.pfnGetGlobalPointerCb = lzt::lepilogue_callback;

  init_module();

  void *pointer;

  ze_result_t initial_result =
      zeModuleGetGlobalPointer(module, "xid", nullptr, &pointer);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeModuleGetGlobalPointer(module, "xid", nullptr, &pointer));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeModuleGetFunctionPointerCallbacksWhenCallingzeModuleGetFunctionPointerThenUserDataIsSetAndResultUnchanged) {
  prologues.Module.pfnGetFunctionPointerCb = lzt::lprologue_callback;
  epilogues.Module.pfnGetFunctionPointerCb = lzt::lepilogue_callback;

  init_module();
  void *function_pointer;

  ze_result_t initial_result = zeModuleGetFunctionPointer(
      module, "module_add_constant", &function_pointer);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeModuleGetFunctionPointer(module, "module_add_constant",
                                       &function_pointer));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeModuleBuildLogDestroyCallbacksWhenCallingzeModuleBuildLogDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.ModuleBuildLog.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.ModuleBuildLog.pfnDestroyCb = lzt::lepilogue_callback;

  init_module();

  ze_result_t initial_result = zeModuleBuildLogDestroy(build_log);
  ze_module_handle_t module2;
  ready_ltracer(tracer_handle, prologues, epilogues);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleCreate(context, device, &module_desc,
                                              &module2, &build_log));
  ASSERT_EQ(initial_result, zeModuleBuildLogDestroy(build_log));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module2));
  build_log = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeModuleBuildLogGetStringCallbacksWhenCallingzeModuleBuildLogGetStringThenUserDataIsSetAndResultUnchanged) {
  prologues.ModuleBuildLog.pfnGetStringCb = lzt::lprologue_callback;
  epilogues.ModuleBuildLog.pfnGetStringCb = lzt::lepilogue_callback;

  init_module();

  size_t log_size;
  ze_result_t initial_result =
      zeModuleBuildLogGetString(build_log, &log_size, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeModuleBuildLogGetString(build_log, &log_size, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeKernelCreateCallbacksWhenCallingzeKernelCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnCreateCb = lzt::lprologue_callback;
  epilogues.Kernel.pfnCreateCb = lzt::lepilogue_callback;

  init_module();
  kernel_desc = {ZE_STRUCTURE_TYPE_KERNEL_DESC, nullptr, 0,
                 "module_add_constant"};
  ze_result_t initial_result = zeKernelCreate(module, &kernel_desc, &kernel);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeKernelDestroy(kernel));
  ASSERT_EQ(initial_result, zeKernelCreate(module, &kernel_desc, &kernel));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeKernelDestroyCallbacksWhenCallingzeKernelDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.Kernel.pfnDestroyCb = lzt::lepilogue_callback;

  init_kernel();

  ze_result_t initial_result = zeKernelDestroy(kernel);
  ready_ltracer(tracer_handle, prologues, epilogues);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeKernelCreate(module, &kernel_desc, &kernel));

  ASSERT_EQ(initial_result, zeKernelDestroy(kernel));
  kernel = nullptr;
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendLaunchKernelIndirectCallbacksWhenCallingzeCommandListAppendLaunchKernelIndirectThenUserDataIsSetAndResultUnchanged) {
  prologues.CommandList.pfnAppendLaunchKernelIndirectCb =
      lzt::lprologue_callback;
  epilogues.CommandList.pfnAppendLaunchKernelIndirectCb =
      lzt::lepilogue_callback;

  init_command_list();
  init_kernel();

  ze_group_count_t args = {1, 0, 0};

  ze_result_t initial_result = zeCommandListAppendLaunchKernelIndirect(
      command_list, kernel, &args, nullptr, 0, nullptr);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeCommandListAppendLaunchKernelIndirect(command_list, kernel, &args,
                                                    nullptr, 0, nullptr));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeKernelSetGroupSizeCallbacksWhenCallingzeKernelSetGroupSizeThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnSetGroupSizeCb = lzt::lprologue_callback;
  epilogues.Kernel.pfnSetGroupSizeCb = lzt::lepilogue_callback;

  init_kernel();

  ze_result_t initial_result = zeKernelSetGroupSize(kernel, 1, 1, 1);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeKernelSetGroupSize(kernel, 1, 1, 1));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeKernelSuggestGroupSizeCallbacksWhenCallingzeKernelSuggestGroupSizeThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnSuggestGroupSizeCb = lzt::lprologue_callback;
  epilogues.Kernel.pfnSuggestGroupSizeCb = lzt::lepilogue_callback;

  init_kernel();

  uint32_t x, y, z;

  ze_result_t initial_result =
      zeKernelSuggestGroupSize(kernel, 1, 1, 1, &x, &y, &z);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeKernelSuggestGroupSize(kernel, 1, 1, 1, &x, &y, &z));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeKernelSetArgumentValueCallbacksWhenCallingzeKernelSetArgumentValueThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnSetArgumentValueCb = lzt::lprologue_callback;
  epilogues.Kernel.pfnSetArgumentValueCb = lzt::lepilogue_callback;

  init_kernel();

  int val = 1;
  ze_result_t initial_result =
      zeKernelSetArgumentValue(kernel, 1, sizeof(int), &val);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result,
            zeKernelSetArgumentValue(kernel, 1, sizeof(int), &val));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeKernelGetPropertiesCallbacksWhenCallingzeKernelGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  prologues.Kernel.pfnGetPropertiesCb = lzt::lprologue_callback;
  epilogues.Kernel.pfnGetPropertiesCb = lzt::lepilogue_callback;

  init_kernel();

  ze_kernel_properties_t kernel_properties = {
      ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES, nullptr};
  ze_result_t initial_result =
      zeKernelGetProperties(kernel, &kernel_properties);
  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(initial_result, zeKernelGetProperties(kernel, &kernel_properties));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeSamplerCreateCallbacksWhenCallingzeSamplerCreateThenUserDataIsSetAndResultUnchanged) {
  prologues.Sampler.pfnCreateCb = lzt::lprologue_callback;
  epilogues.Sampler.pfnCreateCb = lzt::lepilogue_callback;

  ready_ltracer(tracer_handle, prologues, epilogues);

  ze_result_t result =
      zeSamplerCreate(context, device, &sampler_desc, &sampler);
  ASSERT_TRUE((result == ZE_RESULT_SUCCESS) ||
              (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) ||
              (result == ZE_RESULT_ERROR_UNINITIALIZED));
}

TEST_F(
    LTRACING_TEST_NAME,
    GivenEnabledTracerWithzeSamplerDestroyCallbacksWhenCallingzeSamplerDestroyThenUserDataIsSetAndResultUnchanged) {
  prologues.Sampler.pfnDestroyCb = lzt::lprologue_callback;
  epilogues.Sampler.pfnDestroyCb = lzt::lepilogue_callback;

  ze_result_t result =
      zeSamplerCreate(context, device, &sampler_desc, &sampler);
  if ((result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) ||
      (result == ZE_RESULT_ERROR_UNINITIALIZED)) {
    LOG_WARNING << "test not executed because "
                   "sampler is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }
  EXPECT_EQ(result, ZE_RESULT_SUCCESS);

  ready_ltracer(tracer_handle, prologues, epilogues);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeSamplerDestroy(sampler));
}

} // namespace
