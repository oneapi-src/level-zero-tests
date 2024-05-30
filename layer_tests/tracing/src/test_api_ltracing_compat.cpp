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
#include "test_harness/test_harness_memory.hpp"
#include "test_harness/test_harness_driver.hpp"
#include "net/test_ipc_comm.hpp"
#include <boost/process.hpp>
#include <level_zero/ze_api.h>
#include <level_zero/layers/zel_tracing_api.h>
#include <level_zero/layers/zel_tracing_register_cb.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

namespace lzt = level_zero_tests;
namespace bipc = boost::interprocess;

namespace {

#ifdef USE_RUNTIME_TRACING
class LCDynamicTracingCreateTests : public ::testing::Test {};
#define LCTRACING_CREATE_TEST_NAME LCDynamicTracingCreateTests
#else // USE Tracing ENV
class LCTracingCreateTests : public ::testing::Test {};
#define LCTRACING_CREATE_TEST_NAME LCTracingCreateTests
#endif

TEST(
    LCTRACING_CREATE_TEST_NAME,
    GivenValidDeviceAndTracerDescriptionWhenCreatingTracerThenTracerIsNotNull) {
  uint32_t user_data;

  zel_tracer_desc_t tracer_desc = {ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC, nullptr,
                                   &user_data};

  zel_tracer_handle_t tracer_handle = lzt::create_ltracer_handle(tracer_desc);
  EXPECT_NE(tracer_handle, nullptr);

  lzt::destroy_ltracer_handle(tracer_handle);
}

class LCTracingCreateMultipleTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<uint32_t> {};


#ifdef USE_RUNTIME_TRACING
class LCDynamicTracingCreateMultipleTests : public LCTracingCreateMultipleTests {};
#define LCTRACING_CREATE_MULTIPLE_TEST_NAME LCDynamicTracingCreateMultipleTests
#else // USE Tracing ENV
#define LCTRACING_CREATE_MULTIPLE_TEST_NAME LCTracingCreateMultipleTests
#endif

TEST_P(LCTRACING_CREATE_MULTIPLE_TEST_NAME,
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

INSTANTIATE_TEST_CASE_P(LCCreateMultipleTracerTest,
                        LCTRACING_CREATE_MULTIPLE_TEST_NAME,
                        ::testing::Values(1, 10, 100, 1000));

#ifdef USE_RUNTIME_TRACING
class LCDynamicTracingDestroyTests : public ::testing::Test {};
#define LCTRACING_DESTROY_TEST_NAME LCDynamicTracingDestroyTests
#else // USE Tracing ENV
class LCTracingDestroyTests : public ::testing::Test {};
#define LCTRACING_DESTROY_TEST_NAME LCTracingDestroyTests
#endif

TEST(LCTRACING_DESTROY_TEST_NAME,
     GivenSingleDisabledTracerWhenDestroyingTracerThenSuccessIsReturned) {
  uint32_t user_data;

  zel_tracer_desc_t tracer_desc = {ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC, nullptr,
                                   &user_data};
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

    zel_tracer_desc_t tracer_desc = {ZEL_STRUCTURE_TYPE_TRACER_EXP_DESC,
                                     nullptr, &user_data};

    tracer_handle = lzt::create_ltracer_handle(tracer_desc);
  }

  void run_ltracing_compat_ipc_event_test(std::string test_type_name) {
#ifdef __linux__

    lzt::zeEventPool ep;
    ze_device_handle_t device =
        lzt::get_default_device(lzt::get_default_driver());

    ze_event_pool_desc_t defaultEventPoolDesc = {
        ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
        (ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC |
         ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP),
        10};
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
    bipc::shared_memory_object shm(
        bipc::create_only, "ipc_ltracing_compat_event_test", bipc::read_write);
    shm.truncate(sizeof(lzt::shared_ipc_event_data_t));
    bipc::mapped_region region(shm, bipc::read_write);
    std::memcpy(region.get_address(), &test_data,
                sizeof(lzt::shared_ipc_event_data_t));

    #ifdef USE_RUNTIME_TRACING
    // launch child
    boost::process::child c("./tracing/test_ltracing_compat_ipc_event_helper_dynamic",
                            test_type_name.c_str());
    #else
    // launch child
    boost::process::child c("./tracing/test_ltracing_compat_ipc_event_helper",
                            test_type_name.c_str());
    #endif
    lzt::send_ipc_handle(hIpcEventPool);

    c.wait(); // wait for the process to exit

    // cleanup
    bipc::shared_memory_object::remove("ipc_ltracing_compat_event_test");
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
      ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES, nullptr};

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
  ze_image_properties_t image_properties = {ZE_STRUCTURE_TYPE_IMAGE_PROPERTIES,
                                            nullptr};

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

#ifdef USE_RUNTIME_TRACING
class LCDynamicTracingPrologueEpilogueTests : public LCTracingPrologueEpilogueTests {};
#define LCTRACING_TEST_NAME LCDynamicTracingPrologueEpilogueTests
#else // USE Tracing ENV
#define LCTRACING_TEST_NAME LCTracingPrologueEpilogueTests
#endif

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetModulePropertiesCallbacksWhenCallingzeDeviceGetModulePropertiesThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetModulePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetModulePropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_device_module_properties_t moduleProperties = {
      ZE_STRUCTURE_TYPE_MODULE_PROPERTIES, nullptr};

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDeviceGetModuleProperties(device, &moduleProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetExternalMemoryPropertiesCallbacksWhenCallingzeDeviceGetExternalMemoryPropertiesThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetExternalMemoryPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetExternalMemoryPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_device_external_memory_properties_t externalMemoryProperties = {
      ZE_STRUCTURE_TYPE_DEVICE_EXTERNAL_MEMORY_PROPERTIES, nullptr};

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeDeviceGetExternalMemoryProperties(device, &externalMemoryProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetStatusCallbacksWhenCallingzeDeviceGetStatusThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetStatusRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                           lzt::lprologue_callback);
  zelTracerDeviceGetStatusRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                           lzt::lepilogue_callback);

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDeviceGetStatus(device);

  ASSERT_TRUE((result == ZE_RESULT_SUCCESS) ||
              (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceGetGlobalTimestampsCallbacksWhenCallingzeDeviceGetGlobalTimestampsThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceGetGlobalTimestampsRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetGlobalTimestampsRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  uint64_t hostTimestamp, deviceTimestamp;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeDeviceGetGlobalTimestamps(device, &hostTimestamp, &deviceTimestamp);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingzeDeviceGetFabricVertexExpThenUserDataIsSetAndResultUnchanged) {
  zelTracerDeviceGetFabricVertexExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceGetFabricVertexExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_fabric_vertex_handle_t vertex{};
  ze_result_t initial_result = zeDeviceGetFabricVertexExp(device, &vertex);

  lzt::enable_ltracer(tracer_handle);

  EXPECT_EQ(initial_result, zeDeviceGetFabricVertexExp(device, &vertex));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceReserveCacheExtCallbacksWhenCallingzeDeviceReserveCacheExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceReserveCacheExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceReserveCacheExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!lzt::check_if_extension_supported(driver,
                                         "ZE_extension_cache_reservation")) {
    LOG_WARNING << "test not executed because ZE_extension_cache_reservation "
                   "is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDeviceReserveCacheExt(device, 0, 0);

  ASSERT_TRUE((result == ZE_RESULT_SUCCESS) ||
              (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceSetCacheAdviceExtCallbacksWhenCallingzeDeviceSetCacheAdviceExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerDeviceSetCacheAdviceExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDeviceSetCacheAdviceExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!lzt::check_if_extension_supported(driver,
                                         "ZE_extension_cache_reservation")) {
    LOG_WARNING << "test not executed because ZE_extension_cache_reservation "
                   "is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_cache_ext_region_t cacheRegion = {
      ZE_CACHE_EXT_REGION_ZE_CACHE_REGION_DEFAULT};

  lzt::enable_ltracer(tracer_handle);

  const size_t memoryRegionSize = 8192;
  void *memoryRegion = lzt::allocate_device_memory(memoryRegionSize);

  ze_result_t result = zeDeviceSetCacheAdviceExt(device, memoryRegion,
                                                 memoryRegionSize, cacheRegion);

  ASSERT_TRUE((result == ZE_RESULT_SUCCESS) ||
              (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE));

  lzt::free_memory(memoryRegion);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDevicePciGetPropertiesExtCallbacksWhenCallingzeDevicePciGetPropertiesExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerDevicePciGetPropertiesExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDevicePciGetPropertiesExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_pci_ext_properties_t devicePciProperties = {
      ZE_STRUCTURE_TYPE_PCI_EXT_PROPERTIES, nullptr};

  if (!lzt::check_if_extension_supported(driver,
                                         "ZE_extension_pci_properties")) {
    LOG_WARNING << "test not executed because ZE_extension_pci_properties is "
                   "not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeDevicePciGetPropertiesExt(device, &devicePciProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeContextCreateCallbacksWhenCallingzeContextCreateThenUserDataIsSetAndResultUnchanged) {

  zelTracerContextCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                         lzt::lprologue_callback);
  zelTracerContextCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                         lzt::lepilogue_callback);

  ze_context_desc_t contextDescriptor = {ZE_STRUCTURE_TYPE_CONTEXT_DESC,
                                         nullptr, 0};
  ze_context_handle_t context;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeContextCreate(driver, &contextDescriptor, &context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  result = zeContextDestroy(context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeContextCreateExCallbacksWhenCallingzeContextCreateExThenUserDataIsSetAndResultUnchanged) {

  zelTracerContextCreateExRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                           lzt::lprologue_callback);
  zelTracerContextCreateExRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                           lzt::lepilogue_callback);

  ze_context_desc_t contextDescriptor = {ZE_STRUCTURE_TYPE_CONTEXT_DESC,
                                         nullptr, 0};
  ze_context_handle_t context;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeContextCreateEx(driver, &contextDescriptor, 0, nullptr, &context);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  result = zeContextDestroy(context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeContextDestroyCallbacksWhenCallingzeContextDestroyThenUserDataIsSetAndResultUnchanged) {

  zelTracerContextDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                          lzt::lprologue_callback);
  zelTracerContextDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                          lzt::lepilogue_callback);

  ze_context_desc_t contextDescriptor = {ZE_STRUCTURE_TYPE_CONTEXT_DESC,
                                         nullptr, 0};
  ze_context_handle_t context;

  ze_result_t result = zeContextCreate(driver, &contextDescriptor, &context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  lzt::enable_ltracer(tracer_handle);

  result = zeContextDestroy(context);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeContextGetStatusCallbacksWhenCallingzeContextGetStatusThenUserDataIsSetAndResultUnchanged) {

  zelTracerContextGetStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextGetStatusRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_context_desc_t contextDescriptor = {ZE_STRUCTURE_TYPE_CONTEXT_DESC,
                                         nullptr, 0};
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceMakeImageResidentCallbacksWhenCallingzeDeviceMakeImageResidentThenUserDataIsSetAndResultUnchangedAndResultUnchanged) {
  zelTracerContextMakeImageResidentRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextMakeImageResidentRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeContextMakeImageResident(context, device, image);
  EXPECT_EQ(ZE_RESULT_SUCCESS, result);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDeviceEvictImageCallbacksWhenCallingzeDeviceEvictImageThenUserDataIsSetAndResultUnchanged) {
  zelTracerContextEvictImageRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerContextEvictImageRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_result_t initial_result = zeContextEvictImage(context, device, image);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeContextEvictImage(context, device, image));
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeDriverzeDriverGetExtensionFunctionAddressCallbacksWhenCallingzeDriverGetExtensionFunctionAddressThenUserDataIsSetAndResultUnchanged) {
  zelTracerDriverGetExtensionFunctionAddressRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerDriverGetExtensionFunctionAddressRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  typedef ze_result_t (*pFnzexDriverImportExternalPointer)(ze_driver_handle_t,
                                                           void *, size_t);
  pFnzexDriverImportExternalPointer zexDriverImportExternalPointer = nullptr;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeDriverGetExtensionFunctionAddress(
      driver, "zexDriverImportExternalPointer",
      reinterpret_cast<void **>(&zexDriverImportExternalPointer));

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeMemFreeExtCallbacksWhenCallingzeMemFreeExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerMemFreeExtRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                      lzt::lprologue_callback);
  zelTracerMemFreeExtRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                      lzt::lepilogue_callback);

  if (!lzt::check_if_extension_supported(driver,
                                         "ZE_extension_memory_free_policies")) {
    LOG_WARNING << "test not executed because "
                   "ZE_extension_memory_free_policies is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_memory_free_ext_desc_t memFreeDesc = {
      ZE_STRUCTURE_TYPE_MEMORY_FREE_EXT_DESC, nullptr,
      ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_BLOCKING_FREE};

  init_memory();

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeMemFreeExt(context, &memFreeDesc, memory);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  memory = nullptr;
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendMemoryCopyFromContextCallbacksWhenCallingzeCommandListAppendMemoryCopyFromContextThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendMemoryCopyFromContextRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendMemoryCopyFromContextRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_context_handle_t src_context = lzt::create_context();
  ze_context_handle_t dflt_context = lzt::get_default_context();
  EXPECT_NE(src_context, dflt_context);

  const size_t size = 4 * 1024;
  void *host_memory_src_ctx = lzt::allocate_host_memory(size, 1, src_context);
  void *host_memory_dflt_ctx = lzt::allocate_host_memory(size);

  init_command_list();

  ze_result_t initial_result = zeCommandListAppendMemoryCopyFromContext(
      command_list, host_memory_dflt_ctx, src_context, host_memory_src_ctx,
      size, nullptr, 0, 0);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendMemoryCopyFromContext(
                                command_list, host_memory_dflt_ctx, src_context,
                                host_memory_src_ctx, size, nullptr, 0, 0));

  lzt::free_memory(host_memory_src_ctx);
  lzt::free_memory(host_memory_dflt_ctx);
  lzt::destroy_context(src_context);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendQueryKernelTimestampsCallbacksWhenCallingzeCommandListAppendQueryKernelTimestampsThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendQueryKernelTimestampsRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendQueryKernelTimestampsRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  void *timestamp_buffer = lzt::allocate_host_memory(
      sizeof(ze_kernel_timestamp_result_t), 8, context);
  ze_kernel_timestamp_result_t *tsResult =
      static_cast<ze_kernel_timestamp_result_t *>(timestamp_buffer);

  init_command_list();

  ze_result_t initial_result = zeCommandListAppendQueryKernelTimestamps(
      command_list, 0, nullptr, &tsResult, nullptr, nullptr, 0, nullptr);

  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeCommandListAppendQueryKernelTimestamps(
                                command_list, 0, nullptr, &tsResult, nullptr,
                                nullptr, 0, nullptr));

  lzt::free_memory(timestamp_buffer);
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListappendWriteGlobalTImestampCallbacksWhenCallingzeCommandListappendWriteGlobalTImestampThenUserDataIsSetAndResultUnchanged) {

  zelTracerCommandListAppendWriteGlobalTimestampRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendWriteGlobalTimestampRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_event_handle_t event = nullptr;
  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
                                          nullptr, 0, 1};
  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0,
                                ZE_EVENT_SCOPE_FLAG_HOST,
                                ZE_EVENT_SCOPE_FLAG_HOST};
  lzt::zeEventPool ep;
  ep.InitEventPool(event_pool_desc);
  ep.create_event(event, event_desc);

  auto command_list = lzt::create_command_list();
  uint64_t dst_timestamp;
  ze_event_handle_t wait_event_before, wait_event_after;
  auto wait_event_initial = event;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeCommandListAppendWriteGlobalTimestamp(
      command_list, &dst_timestamp, nullptr, 1, &event);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
  ASSERT_EQ(event, wait_event_initial);

  ep.destroy_event(event);
  lzt::destroy_command_list(command_list);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyCallbacksWhenCallingzeCommandListAppendImageCopyThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendImageCopyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyRegionCallbacksWhenCallingzeCommandListAppendImageCopyRegionThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendImageCopyRegionRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyRegionRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyFromMemoryCallbacksWhenCallingzeCommandListAppendImageCopyFromMemoryThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendImageCopyFromMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyFromMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_memory();
  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_result_t initial_result = zeCommandListAppendImageCopyFromMemory(
      command_list, image, memory, nullptr, nullptr, 0, nullptr);
  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendImageCopyFromMemory(
                command_list, image, memory, nullptr, nullptr, 0, nullptr));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyToMemoryCallbacksWhenCallingzeCommandListAppendImageCopyToMemoryThenUserDataIsSetAndResultUnchanged) {
  zelTracerCommandListAppendImageCopyToMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyToMemoryRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_command_list();
  init_memory();
  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_result_t initial_result = zeCommandListAppendImageCopyToMemory(
      command_list, memory, image, nullptr, nullptr, 0, nullptr);
  zeCommandListReset(command_list);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result,
            zeCommandListAppendImageCopyToMemory(command_list, memory, image,
                                                 nullptr, nullptr, 0, nullptr));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyToMemoryExtCallbacksWhenCallingzezeCommandListAppendImageCopyToMemoryExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerCommandListAppendImageCopyToMemoryExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyToMemoryExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!lzt::check_if_extension_supported(driver, "ZE_extension_image_copy")) {
    LOG_WARNING
        << "test not executed because ZE_extension_image_copy is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  init_command_list();
  init_memory();
  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  uint32_t destRowPitch = 0;
  uint32_t destSlicePitch = 0;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeCommandListAppendImageCopyToMemoryExt(
      command_list, memory, image, nullptr, destRowPitch, destSlicePitch,
      nullptr, 0, 0);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCommandListAppendImageCopyFromMemoryExtCallbacksWhenCallingzeCommandListAppendImageCopyFromMemoryExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerCommandListAppendImageCopyFromMemoryExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerCommandListAppendImageCopyFromMemoryExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!lzt::check_if_extension_supported(driver, "ZE_extension_image_copy")) {
    LOG_WARNING
        << "test not executed because ZE_extension_image_copy is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  init_command_list();
  init_memory();
  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  uint32_t srcRowPitch = 0;
  uint32_t srcSlicePitch = 0;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeCommandListAppendImageCopyFromMemoryExt(
      command_list, image, memory, nullptr, srcRowPitch, srcSlicePitch, nullptr,
      0, nullptr);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageGetAllocPropertiesExtCallbacksWhenCallingzeDriverGetApiVersionThenUserDataIsSetAndResultUnchanged) {

  zelTracerImageGetAllocPropertiesExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerImageGetAllocPropertiesExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!lzt::check_if_extension_supported(
          driver, "ZE_extension_image_query_alloc_properties")) {
    LOG_WARNING << "test not executed because "
                   "ZE_extension_image_query_alloc_properties is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_image_allocation_ext_properties_t imageAllocProperties;
  imageAllocProperties.stype =
      ZE_STRUCTURE_TYPE_IMAGE_ALLOCATION_EXT_PROPERTIES;
  imageAllocProperties.pNext = nullptr;
  imageAllocProperties.id = 0;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeImageGetAllocPropertiesExt(context, image, &imageAllocProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageGetMemoryPropertiesExpCallbacksWhenCallingzeImageGetMemoryPropertiesExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerImageGetMemoryPropertiesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerImageGetMemoryPropertiesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_image_memory_properties_exp_t imageMemoryProperties = {
      ZE_STRUCTURE_TYPE_IMAGE_MEMORY_EXP_PROPERTIES, nullptr};

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeImageGetMemoryPropertiesExp(image, &imageMemoryProperties);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageViewCreateExtCallbacksWhenCallingzeImageViewCreateExtThenUserDataIsSetAndResultUnchanged) {

  zelTracerImageViewCreateExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerImageViewCreateExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!lzt::check_if_extension_supported(driver, "ZE_extension_image_view")) {
    LOG_WARNING
        << "test not executed because ZE_extension_image_view is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_image_handle_t imageView;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeImageViewCreateExt(context, device, &image_desc, image, &imageView);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  zeImageDestroy(imageView);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageViewCreateExpCallbacksWhenCallingzeImageViewCreateExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerImageViewCreateExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerImageViewCreateExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_image_handle_t imageView;

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeImageViewCreateExp(context, device, &image_desc, image, &imageView);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  zeImageDestroy(imageView);
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventPoolOpenIpcHandleCallbacksWhenCallingzeEventPoolOpenIpcHandleThenUserDataIsSetAndResultUnchanged) {

  run_ltracing_compat_ipc_event_test("TEST_OPEN_IPC_EVENT");
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeEventPoolCloseIpcHandleCallbacksWhenCallingzeEventPoolCloseIpcHandleThenUserDataIsSetAndResultUnchanged) {

  run_ltracing_compat_ipc_event_test("TEST_CLOSE_IPC_EVENT");
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageCreateCallbacksWhenCallingzeImageCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerImageCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                       lzt::lprologue_callback);
  zelTracerImageCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                       lzt::lepilogue_callback);
  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeImageCreate(context, device, &image_desc, &image);

  ASSERT_TRUE((result == ZE_RESULT_SUCCESS) ||
              (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE));
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeImageDestroyCallbacksWhenCallingzeImageDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerImageDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                        lzt::lprologue_callback);
  zelTracerImageDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                        lzt::lepilogue_callback);

  if (!init_image()) {
    LOG_WARNING << "test not executed because "
                   "images are not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }
  ze_result_t initial_result = zeImageDestroy(image);

  lzt::enable_ltracer(tracer_handle);

  init_image();
  ASSERT_EQ(initial_result, zeImageDestroy(image));
  image = nullptr;
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeModuleInspectLinkageExtCallbacksWhenCallingzeModuleInspectLinkageExtThenUserDataIsSetAndResultUnchanged) {

  if (!lzt::check_if_extension_supported(driver,
                                         "ZE_extension_linkage_inspection")) {
    LOG_WARNING << "test not executed because ZE_extension_linkage_inspection "
                   "is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  zelTracerModuleInspectLinkageExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleInspectLinkageExtRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_linkage_inspection_ext_desc_t linkageInspectDesc = {
      ZE_STRUCTURE_TYPE_LINKAGE_INSPECTION_EXT_DESC, nullptr, 0};
  uint32_t numModules = 1;

  init_module();

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeModuleInspectLinkageExt(
      &linkageInspectDesc, numModules, &module, &build_log);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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

  ze_result_t result =
      zeKernelSetGlobalOffsetExp(kernel, offsetX, offsetY, offsetZ);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeKernelSchedulingHintExpCallbacksWhenCallingzeKernelSchedulingHintExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerKernelSchedulingHintExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelSchedulingHintExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_scheduling_hint_exp_desc_t schedulingHint = {
      ZE_STRUCTURE_TYPE_SCHEDULING_HINT_EXP_DESC, nullptr,
      ZE_SCHEDULING_HINT_EXP_FLAG_OLDEST_FIRST};

  init_kernel();

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result = zeKernelSchedulingHintExp(kernel, &schedulingHint);

  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
}

TEST_F(
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
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
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeKernelGetPropertiesCallbacksWhenCallingzeKernelGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();

  ze_kernel_properties_t kernel_properties = {
      ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES, nullptr};
  ze_result_t initial_result =
      zeKernelGetProperties(kernel, &kernel_properties);

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(initial_result, zeKernelGetProperties(kernel, &kernel_properties));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeSamplerCreateCallbacksWhenCallingzeSamplerCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerSamplerCreateRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                         lzt::lprologue_callback);
  zelTracerSamplerCreateRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                         lzt::lepilogue_callback);

  lzt::enable_ltracer(tracer_handle);

  ze_result_t result =
      zeSamplerCreate(context, device, &sampler_desc, &sampler);

  ASSERT_TRUE((result == ZE_RESULT_SUCCESS) ||
              (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) ||
              (result == ZE_RESULT_ERROR_UNINITIALIZED));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeSamplerDestroyCallbacksWhenCallingzeSamplerDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerSamplerDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                          lzt::lprologue_callback);
  zelTracerSamplerDestroyRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                          lzt::lepilogue_callback);

  ze_result_t result =
      zeSamplerCreate(context, device, &sampler_desc, &sampler);

  if (result == (ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) ||
      (result == ZE_RESULT_ERROR_UNINITIALIZED)) {
    LOG_WARNING << "test not executed because fabric is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  lzt::enable_ltracer(tracer_handle);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeSamplerDestroy(sampler));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingzeFabricVertexGetDeviceExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerFabricVertexGetDeviceExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerFabricVertexGetDeviceExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_fabric_vertex_handle_t vertex{};
  ze_device_handle_t device_vertex{};

  ze_result_t result = zeDeviceGetFabricVertexExp(device, &vertex);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_WARNING << "test not executed because fabric is not supported";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }
  EXPECT_NE(vertex, nullptr);

  lzt::enable_ltracer(tracer_handle);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFabricVertexGetDeviceExp(vertex, &device_vertex));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingzeFabricVertexGetExpThenUserDataIsSetAndResultUnchanged) {
  zelTracerFabricVertexGetExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerFabricVertexGetExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  uint32_t count = 0;
  ze_driver_handle_t driverHandle = lzt::get_default_driver();

  ze_result_t initial_result =
      zeFabricVertexGetExp(driverHandle, &count, nullptr);
  count = 0;

  lzt::enable_ltracer(tracer_handle);

  EXPECT_EQ(initial_result,
            zeFabricVertexGetExp(driverHandle, &count, nullptr));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingzeFabricVertexGetPropertiesExpThenUserDataIsSetAndResultUnchanged) {

  zelTracerFabricVertexGetPropertiesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);

  zelTracerFabricVertexGetPropertiesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  auto vertices = lzt::get_ze_fabric_vertices();
  if (vertices.size() == 0) {
    LOG_WARNING << "There are no vertices";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  ze_fabric_vertex_exp_properties_t vertexProperties = {
      ZE_STRUCTURE_TYPE_FABRIC_VERTEX_EXP_PROPERTIES, nullptr};

  lzt::enable_ltracer(tracer_handle);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFabricVertexGetPropertiesExp(vertices[0], &vertexProperties));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingzeFabricVertexGetSubVerticesExpThenUserDataIsSetAndResultUnchanged) {
  zelTracerFabricVertexGetSubVerticesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerFabricVertexGetSubVerticesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  auto vertices = lzt::get_ze_fabric_vertices();
  if (vertices.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough vertices";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  lzt::enable_ltracer(tracer_handle);

  uint32_t count;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFabricVertexGetSubVerticesExp(vertices[0], &count, nullptr));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingzeFabricEdgeGetExpThenUserDataIsSetAndResultUnchanged) {
  zelTracerFabricEdgeGetExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerFabricEdgeGetExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  auto vertices = lzt::get_ze_fabric_vertices();
  if (vertices.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough vertices";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  lzt::enable_ltracer(tracer_handle);

  uint32_t count;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFabricEdgeGetExp(vertices[0], vertices[0], &count, nullptr));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingzeFabricEdgeGetPropertiesExpThenUserDataIsSetAndResultUnchanged) {
  zelTracerFabricEdgeGetPropertiesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerFabricEdgeGetPropertiesExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  auto vertices = lzt::get_ze_fabric_vertices();
  if (vertices.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough vertices";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  uint32_t edgeCount;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFabricEdgeGetExp(vertices[0], vertices[0], &edgeCount, nullptr));

  if (edgeCount == 0) {
    LOG_WARNING << "Test not executed due to not enough edges";
    user_data.prologue_called = true;
    user_data.epilogue_called = true;
    GTEST_SKIP();
  }

  std::vector<ze_fabric_edge_handle_t> edgeHandles(edgeCount);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFabricEdgeGetExp(vertices[0], vertices[0], &edgeCount,
                               edgeHandles.data()));

  lzt::enable_ltracer(tracer_handle);

  ze_fabric_edge_exp_properties_t edgeProperties = {
      ZE_STRUCTURE_TYPE_FABRIC_EDGE_EXP_PROPERTIES, nullptr};

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeFabricEdgeGetPropertiesExp(edgeHandles[0], &edgeProperties));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingEventQueryKernelTimestampThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventQueryKernelTimestampRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerEventQueryKernelTimestampRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_event_pool();
  init_event();

  lzt::enable_ltracer(tracer_handle);

  ze_kernel_timestamp_result_t kernelTimestamps;

  EXPECT_EQ(ZE_RESULT_NOT_READY,
            zeEventQueryKernelTimestamp(event, &kernelTimestamps));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingEventQueryTimestampsExpThenUserDataIsSetAndResultUnchanged) {
  zelTracerEventQueryTimestampsExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerEventQueryTimestampsExpRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_event_pool();
  init_event();

  lzt::enable_ltracer(tracer_handle);

  uint32_t count = 0;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeEventQueryTimestampsExp(event, device, &count, nullptr));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingKernelSetIndirectAccessThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelSetIndirectAccessRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelSetIndirectAccessRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();

  lzt::enable_ltracer(tracer_handle);

  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeKernelSetIndirectAccess(kernel, ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingKernelGetSourceAttributesThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelGetSourceAttributesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelGetSourceAttributesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();

  lzt::enable_ltracer(tracer_handle);

  uint32_t size = 0;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelGetSourceAttributes(kernel, &size, nullptr));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingKernelSuggestMaxCooperativeGroupCountThenUserDataIsSetAndResultUnchanged) {
  zelTracerKernelSuggestMaxCooperativeGroupCountRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerKernelSuggestMaxCooperativeGroupCountRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_kernel();

  lzt::enable_ltracer(tracer_handle);

  uint32_t groups_x;

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSuggestMaxCooperativeGroupCount(kernel, &groups_x));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingMemCloseIpcHandleThenUserDataIsSetAndResultUnchanged) {
  zelTracerMemCloseIpcHandleRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerMemCloseIpcHandleRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_ipc();

  void *mem = nullptr;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, ipc_handle, 0, &mem));

  lzt::enable_ltracer(tracer_handle);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, mem));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingModuleGetKernelNamesThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleGetKernelNamesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleGetKernelNamesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_module();

  lzt::enable_ltracer(tracer_handle);

  uint32_t kernel_count = 0;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeModuleGetKernelNames(module, &kernel_count, nullptr));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingModuleGetPropertiesThenUserDataIsSetAndResultUnchanged) {
  zelTracerModuleGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerModuleGetPropertiesRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  init_module();

  lzt::enable_ltracer(tracer_handle);

  ze_module_properties_t moduleProperties;
  moduleProperties.stype = ZE_STRUCTURE_TYPE_MODULE_PROPERTIES;
  moduleProperties.pNext = nullptr;
  moduleProperties.flags = ZE_MODULE_PROPERTY_FLAG_FORCE_UINT32;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeModuleGetProperties(module, &moduleProperties));
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingPhysicalMemCreateThenUserDataIsSetAndResultUnchanged) {
  zelTracerPhysicalMemCreateRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerPhysicalMemCreateRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  ze_physical_mem_handle_t reservedPhysicalMemory = nullptr;

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);

  lzt::enable_ltracer(tracer_handle);

  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);
  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingPhysicalMemDestroyThenUserDataIsSetAndResultUnchanged) {
  zelTracerPhysicalMemDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerPhysicalMemDestroyRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  ze_physical_mem_handle_t reservedPhysicalMemory = nullptr;

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);

  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);

  lzt::enable_ltracer(tracer_handle);

  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingVirtualMemoryReserveThenUserDataIsSetAndResultUnchanged) {

  zelTracerVirtualMemReserveRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerVirtualMemReserveRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  void *reservedVirtualMemory = nullptr;
  ze_physical_mem_handle_t reservedPhysicalMemory;

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);

  lzt::enable_ltracer(tracer_handle);

  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingVirtualMemoryFreeThenUserDataIsSetAndResultUnchanged) {

  zelTracerVirtualMemFreeRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                          lzt::lprologue_callback);
  zelTracerVirtualMemFreeRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                          lzt::lepilogue_callback);

  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  void *reservedVirtualMemory = nullptr;
  ze_physical_mem_handle_t reservedPhysicalMemory;

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  lzt::enable_ltracer(tracer_handle);

  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingVirtualMemoryGetAccessAttributesThenUserDataIsSetAndResultUnchanged) {

  zelTracerVirtualMemGetAccessAttributeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerVirtualMemGetAccessAttributeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  ze_memory_access_attribute_t access;
  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  void *reservedVirtualMemory = nullptr;
  ze_physical_mem_handle_t reservedPhysicalMemory;

  ze_memory_access_attribute_t previousAccess;
  size_t memorySize = 0;
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);

  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  lzt::enable_ltracer(tracer_handle);

  lzt::virtual_memory_reservation_get_access(
      context, reservedVirtualMemory, allocationSize, &access, &memorySize);
  EXPECT_EQ(access, ZE_MEMORY_ACCESS_ATTRIBUTE_NONE);
  EXPECT_EQ(memorySize, allocationSize);

  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingVirtualMemoryQueryPageSizeThenUserDataIsSetAndResultUnchanged) {
  zelTracerVirtualMemQueryPageSizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerVirtualMemQueryPageSizeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  ze_physical_mem_handle_t reservedPhysicalMemory = nullptr;

  lzt::enable_ltracer(tracer_handle);
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);

  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);
  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingVirtualMemorySetAccessAttributesThenUserDataIsSetAndResultUnchanged) {
  zelTracerVirtualMemSetAccessAttributeRegisterCallback(
      tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
  zelTracerVirtualMemSetAccessAttributeRegisterCallback(
      tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  size_t memorySize = 0;
  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  void *reservedVirtualMemory = nullptr;

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  lzt::enable_ltracer(tracer_handle);

  lzt::virtual_memory_reservation_set_access(
      context, reservedVirtualMemory, allocationSize,
      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);

  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingVirtualMemoryMapThenUserDataIsSetAndResultUnchanged) {

  zelTracerVirtualMemMapRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                         lzt::lprologue_callback);
  zelTracerVirtualMemMapRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                         lzt::lepilogue_callback);

  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  void *reservedVirtualMemory = nullptr;
  ze_physical_mem_handle_t reservedPhysicalMemory;

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  std::vector<ze_memory_access_attribute_t> memoryAccessFlags = {
      ZE_MEMORY_ACCESS_ATTRIBUTE_NONE, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE,
      ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY};

  lzt::enable_ltracer(tracer_handle);

  lzt::virtual_memory_map(context, reservedVirtualMemory, allocationSize,
                          reservedPhysicalMemory, 0,
                          ZE_MEMORY_ACCESS_ATTRIBUTE_NONE);
  lzt::virtual_memory_unmap(context, reservedVirtualMemory, allocationSize);

  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

TEST_F(
    LCTRACING_TEST_NAME,
    GivenEnabledTracerWithzeCallbacksWhenCallingVirtualMemoryUnmapThenUserDataIsSetAndResultUnchanged) {

  zelTracerVirtualMemUnmapRegisterCallback(tracer_handle, ZEL_REGISTER_PROLOGUE,
                                           lzt::lprologue_callback);
  zelTracerVirtualMemUnmapRegisterCallback(tracer_handle, ZEL_REGISTER_EPILOGUE,
                                           lzt::lepilogue_callback);

  size_t pageSize = 0;
  size_t allocationSize = (1024 * 1024);
  void *reservedVirtualMemory = nullptr;
  ze_physical_mem_handle_t reservedPhysicalMemory;

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_memory_allocation(context, device, allocationSize,
                                  &reservedPhysicalMemory);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  std::vector<ze_memory_access_attribute_t> memoryAccessFlags = {
      ZE_MEMORY_ACCESS_ATTRIBUTE_NONE, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE,
      ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY};

  lzt::virtual_memory_map(context, reservedVirtualMemory, allocationSize,
                          reservedPhysicalMemory, 0,
                          ZE_MEMORY_ACCESS_ATTRIBUTE_NONE);
  lzt::enable_ltracer(tracer_handle);
  lzt::virtual_memory_unmap(context, reservedVirtualMemory, allocationSize);

  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

} // namespace
