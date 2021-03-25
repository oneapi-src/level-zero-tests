/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "gtest/gtest.h"

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace bp = boost::process;

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_comm.hpp"

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
  memory_ = lzt::allocate_device_memory(1);
  context_ = lzt::create_context();

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  lzt::destroy_context(context_);
  lzt::free_memory(memory_);
}

class zeIpcMemHandleCloseTests : public zeIpcMemHandleTests {
protected:
  void SetUp() override {
    context_ = lzt::create_context();
    lzt::allocate_mem_and_get_ipc_handle(context_, &ipc_mem_handle_, &memory_,
                                         ZE_MEMORY_TYPE_DEVICE);

    ze_ipc_memory_flags_t flags = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemOpenIpcHandle(context_,
                                 lzt::zeDevice::get_instance()->get_device(),
                                 ipc_mem_handle_, flags, &ipc_memory_));
  }

  void TearDown() { lzt::destroy_context(context_); }
};

TEST_F(
    zeIpcMemHandleCloseTests,
    GivenValidPointerToDeviceMemoryAllocationWhenClosingIpcHandleThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context_, ipc_memory_));
}

} // namespace
