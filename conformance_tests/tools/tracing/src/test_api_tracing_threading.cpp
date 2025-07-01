/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <thread>
#include <atomic>

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "test_harness/test_harness.hpp"
#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace lzt = level_zero_tests;

namespace {

std::atomic_bool ready(false);
std::atomic_bool signaling_thread(false);
std::atomic_int callback_enter_invocations(0);
std::atomic_int callback_exit_invocations(0);

void OnEnterMemAllocDevice(ze_mem_alloc_device_params_t *tracer_params,
                           ze_result_t result, void *pTraceUserData,
                           void **ppTracerInstanceUserData) {

  callback_enter_invocations++;
  while (!signaling_thread && !ready) {
  };
}

void OnExitMemAllocDevice(ze_mem_alloc_device_params_t *tracer_params,
                          ze_result_t result, void *pTraceUserData,
                          void **ppTracerInstanceUserData) {

  callback_exit_invocations++;
}

void OnEnterMemAllocHost(ze_mem_alloc_host_params_t *tracer_params,
                         ze_result_t result, void *pTraceUserData,
                         void **ppTracerInstanceUserData) {}

void OnExitMemAllocHost(ze_mem_alloc_host_params_t *tracer_params,
                        ze_result_t result, void *pTraceUserData,
                        void **ppTracerInstanceUserData) {}

void allocate_then_deallocate_device_memory() {
  void *memory = lzt::allocate_device_memory(1);
  lzt::free_memory(memory);
}

class TracingThreadTests : public ::testing::Test {
protected:
  void SetUp() override {
    callback_exit_invocations = 0;
    callback_enter_invocations = 0;
    ready = false;
    signaling_thread = false;
    zet_tracer_exp_desc_t tracer_desc = {ZET_STRUCTURE_TYPE_TRACER_EXP_DESC,
                                         nullptr, nullptr};
    tracer = lzt::create_tracer_handle(tracer_desc);

    prologues.Mem.pfnAllocDeviceCb = OnEnterMemAllocDevice;
    epilogues.Mem.pfnAllocDeviceCb = OnExitMemAllocDevice;

    prologues.Mem.pfnAllocHostCb = OnEnterMemAllocHost;
    epilogues.Mem.pfnAllocHostCb = OnExitMemAllocHost;

    lzt::set_tracer_prologues(tracer, prologues);
    lzt::set_tracer_epilogues(tracer, epilogues);
    lzt::enable_tracer(tracer);
  }

  void TearDown() {
    lzt::disable_tracer(tracer);
    lzt::destroy_tracer_handle(tracer);
  }

  zet_tracer_exp_handle_t tracer;
  ze_callbacks_t prologues = {};
  ze_callbacks_t epilogues = {};
};

LZT_TEST_F(
    TracingThreadTests,
    GivenSingleTracingEnabledThreadWhenCallingDifferentAPIFunctionThenCallbacksCalledOnce) {

  std::thread child_thread(allocate_then_deallocate_device_memory);
  void *memory = lzt::allocate_host_memory(100);
  ready = true;

  lzt::free_memory(memory);
  child_thread.join();

  EXPECT_EQ(callback_enter_invocations, 1);
  EXPECT_EQ(callback_exit_invocations, 1);
}

LZT_TEST_F(
    TracingThreadTests,
    GivenSingleTracingEnabledThreadWhenCallingSameAPIFunctionThenCallbackCalledTwice) {

  std::thread child_thread(allocate_then_deallocate_device_memory);
  signaling_thread = true;
  void *memory = lzt::allocate_device_memory(100);
  ready = true;

  lzt::free_memory(memory);
  child_thread.join();

  EXPECT_EQ(2, callback_enter_invocations);
  EXPECT_EQ(2, callback_exit_invocations);
}

void trace_memory_allocation_then_deallocate(ze_memory_type_t memory_type) {

  void *memory;
  if (memory_type == ZE_MEMORY_TYPE_DEVICE) {
    memory = lzt::allocate_device_memory(1);
  } else if (memory_type == ZE_MEMORY_TYPE_HOST) {
    memory = lzt::allocate_host_memory(1);
  } else {
    abort();
  }

  lzt::free_memory(memory);
}

LZT_TEST_F(
    TracingThreadTests,
    GivenTwoThreadsWhenTracingEnabledCallingDifferentAPIFunctionThenCallbackCalledOnce) {

  std::thread second_trace_thread(trace_memory_allocation_then_deallocate,
                                  ZE_MEMORY_TYPE_HOST);
  signaling_thread = true;
  void *memory = lzt::allocate_device_memory(100);
  ready = true;

  lzt::free_memory(memory);
  second_trace_thread.join();

  EXPECT_EQ(1, callback_enter_invocations);
  EXPECT_EQ(1, callback_exit_invocations);
}

LZT_TEST_F(
    TracingThreadTests,
    GivenTwoThreadsWhenTracingEnabledCallingSameAPIFunctionThenCallbacksCalledTwice) {

  std::thread second_child_thread(trace_memory_allocation_then_deallocate,
                                  ZE_MEMORY_TYPE_DEVICE);
  signaling_thread = true;
  void *memory = lzt::allocate_device_memory(100);
  ready = true;

  lzt::free_memory(memory);
  second_child_thread.join();

  EXPECT_EQ(2, callback_enter_invocations);
  EXPECT_EQ(2, callback_exit_invocations);
}

class TracingThreadTestsDisabling : public TracingThreadTests {
protected:
  void TearDown() override {}
};

LZT_TEST_F(
    TracingThreadTestsDisabling,
    GivenInvokedPrologueWhenDisablingTracerInSeparateThreadThenEpilogueIsCalled) {

  std::thread child_thread(allocate_then_deallocate_device_memory);
  // we need to ensure that child thread has started and that the API
  // callback has been invoked before disabling the tracer
  while (!callback_enter_invocations) {
  }
  lzt::disable_tracer(tracer);
  ready = true;
  child_thread.join();
  EXPECT_EQ(callback_enter_invocations, 1);
  EXPECT_EQ(callback_exit_invocations, 1);
  lzt::destroy_tracer_handle(tracer);
}

} // namespace
