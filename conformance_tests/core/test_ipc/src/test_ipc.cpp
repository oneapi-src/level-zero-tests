/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace bp = boost::process;

#include "gtest/gtest.h"
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
};

TEST_F(
    zeIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenGettingIpcMemHandleThenSuccessIsReturned) {
  memory_ = lzt::allocate_device_memory(1);

  const ze_driver_handle_t driver = lzt::get_default_driver();
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDriverGetMemIpcHandle(driver, memory_, &ipc_mem_handle_));

  lzt::free_memory(memory_);
}

class zeIpcMemHandleCloseTests : public zeIpcMemHandleTests {
protected:
  void SetUp() override {
    lzt::allocate_mem_and_get_ipc_handle(&ipc_mem_handle_, &memory_,
                                         ZE_MEMORY_TYPE_DEVICE);

    ze_ipc_memory_flag_t flags = ZE_IPC_MEMORY_FLAG_NONE;
    EXPECT_EQ(
        ZE_RESULT_SUCCESS,
        zeDriverOpenMemIpcHandle(lzt::get_default_driver(),
                                 lzt::zeDevice::get_instance()->get_device(),
                                 ipc_mem_handle_, flags, &ipc_memory_));
  }

  void TearDown() { lzt::free_memory(memory_); }
};

TEST_F(
    zeIpcMemHandleCloseTests,
    GivenValidPointerToDeviceMemoryAllocationWhenClosingIpcHandleThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDriverCloseMemIpcHandle(lzt::get_default_driver(), ipc_memory_));
}

} // namespace
