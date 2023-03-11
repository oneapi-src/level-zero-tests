/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "gtest/gtest.h"

#include <vector>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace bp = boost::process;

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "net/test_ipc_comm.hpp"

#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace {

class zeIpcMemHandleTests : public ::testing::Test {
protected:
  void *memory_ = nullptr;
  void *ipc_memory_ = nullptr;
  ze_ipc_mem_handle_t ipc_mem_handle_;
  ze_context_handle_t context_;
};

TEST_F(
    zeIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenGettingIpcMemHandleThenSuccessIsReturned) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();
  memory_ = lzt::allocate_device_memory(1);
  context_ = lzt::create_context();

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  lzt::destroy_context(context_);
  lzt::free_memory(memory_);
}

TEST_F(
    zeIpcMemHandleTests,
    GivenSameIpcMemoryHandleWhenOpeningIpcMemHandleMultipleTimesThenUniquePointersAreReturned) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();

  ze_driver_handle_t driver = lzt::get_default_driver();
  ze_device_handle_t device = lzt::get_default_device(driver);
  context_ = lzt::create_context();
  memory_ = lzt::allocate_device_memory(1, 1, 0, device, context_);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));

  const int numIters = 2000;
  std::vector<void *> ipcPointers(numIters);
  ze_ipc_memory_flags_t ipcMemFlags;
  for (int i = 0; i < numIters; i++) {
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemOpenIpcHandle(context_, device, ipc_mem_handle_, 0,
                                 &ipcPointers[i]));
    EXPECT_NE(nullptr, ipcPointers[i]);
  }

  std::sort(ipcPointers.begin(), ipcPointers.end());
  EXPECT_EQ(std::adjacent_find(ipcPointers.begin(), ipcPointers.end()),
            ipcPointers.end());

  lzt::free_memory(context_, memory_);
  lzt::destroy_context(context_);
}

class zeIpcMemHandleCloseTests : public zeIpcMemHandleTests {
protected:
  void SetUp() override {
    ze_result_t result = zeInit(0);
    if (result) {
      throw std::runtime_error("zeInit failed: " +
                               level_zero_tests::to_string(result));
    }
    LOG_TRACE << "Driver initialized";
    level_zero_tests::print_platform_overview();
    context_ = lzt::create_context();
    lzt::allocate_mem_and_get_ipc_handle(context_, &ipc_mem_handle_, &memory_,
                                         ZE_MEMORY_TYPE_DEVICE);
  }

  void TearDown() {
    lzt::free_memory(context_, memory_);
    lzt::destroy_context(context_);
  }
};

TEST_F(
    zeIpcMemHandleCloseTests,
    GivenValidPointerToDeviceMemoryAllocationWhenClosingIpcHandleThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context_,
                               lzt::zeDevice::get_instance()->get_device(),
                               ipc_mem_handle_,
                               ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, &ipc_memory_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context_, ipc_memory_));
}

TEST_F(
    zeIpcMemHandleCloseTests,
    GivenValidPointerToDeviceMemoryAllocationBiasCachedWhenClosingIpcHandleThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(
                context_, lzt::zeDevice::get_instance()->get_device(),
                ipc_mem_handle_, ZE_IPC_MEMORY_FLAG_BIAS_CACHED, &ipc_memory_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context_, ipc_memory_));
}

} // namespace
