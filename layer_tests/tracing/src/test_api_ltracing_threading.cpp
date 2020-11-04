/*
 *
 * Copyright (C) 2020 Intel Corporation
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

class LTracingThreadTests : public ::testing::Test {
protected:
  void SetUp() override {
    callback_exit_invocations = 0;
    callback_enter_invocations = 0;
    ready = false;
    signaling_thread = false;
    zel_tracer_desc_t tracer_desc = {};

    tracer_desc.pNext = nullptr;
    tracer_desc.pUserData = nullptr;
    tracer = lzt::create_ltracer_handle(tracer_desc);

    prologues.Mem.pfnAllocDeviceCb = OnEnterMemAllocDevice;
    epilogues.Mem.pfnAllocDeviceCb = OnExitMemAllocDevice;

    prologues.Mem.pfnAllocHostCb = OnEnterMemAllocHost;
    epilogues.Mem.pfnAllocHostCb = OnExitMemAllocHost;

    lzt::set_ltracer_prologues(tracer, prologues);
    lzt::set_ltracer_epilogues(tracer, epilogues);
    lzt::enable_ltracer(tracer);
  }

  void TearDown() {
    lzt::disable_ltracer(tracer);
    lzt::destroy_ltracer_handle(tracer);
  }

  zel_tracer_handle_t tracer;
  ze_callbacks_t prologues = {};
  ze_callbacks_t epilogues = {};
};

TEST_F(
    LTracingThreadTests,
    GivenSingleTracingEnabledThreadWhenCallingDifferentAPIFunctionThenCallbacksCalledOnce) {

  std::thread child_thread(allocate_then_deallocate_device_memory);
  void *memory = lzt::allocate_host_memory(100);
  ready = true;

  lzt::free_memory(memory);
  child_thread.join();

  EXPECT_EQ(callback_enter_invocations, 1);
  EXPECT_EQ(callback_exit_invocations, 1);
}

TEST_F(
    LTracingThreadTests,
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

TEST_F(
    LTracingThreadTests,
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

TEST_F(
    LTracingThreadTests,
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

class LTracingThreadTestsDisabling : public LTracingThreadTests {
protected:
  void TearDown() override {}
};

TEST_F(
    LTracingThreadTestsDisabling,
    GivenInvokedPrologueWhenDisablingTracerInSeparateThreadThenEpilogueIsCalled) {

  std::thread child_thread(allocate_then_deallocate_device_memory);
  // we need to ensure that child thread has started and that the API
  // callback has been invoked before disabling the tracer
  while (!callback_enter_invocations) {
  }
  lzt::disable_ltracer(tracer);
  ready = true;
  child_thread.join();
  EXPECT_EQ(callback_enter_invocations, 1);
  EXPECT_EQ(callback_exit_invocations, 1);
  lzt::destroy_ltracer_handle(tracer);
}

} // namespace
