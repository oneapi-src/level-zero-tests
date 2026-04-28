/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "utils/utils.hpp"

#ifdef __linux__
struct DX11InteroperabilityTests : ::testing::Test {
  void SetUp() override { GTEST_SKIP() << "Not supported on Linux"; }
};
#else
#include "dx11_interop_fixture.hpp"
#endif

namespace {

LZT_TEST_F(DX11InteroperabilityTests,
           GivenDX11SharedFenceWhenImportingExternalSemaphoreThenIsSuccess) {
#ifndef __linux__
  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence);

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

  lzt::release_external_semaphore(external_semaphore_handle);
  CloseHandle(fence_shared_handle);
#endif
}

LZT_TEST_F(DX11InteroperabilityTests,
           GivenDX11SharedFenceWhenSignalingByL0ThenIsSuccessfullySignaled) {
#ifndef __linux__
  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence);

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

  EXPECT_EQ(fence->GetCompletedValue(), 0);

  uint64_t wait_value = 1;

  auto l0_cmd_bundle =
      lzt::create_command_bundle(l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
                                 ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                 ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, true);

  ze_external_semaphore_signal_params_ext_t semaphore_signal_params = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS_EXT,
      .value = wait_value};
  lzt::append_signal_external_semaphore(
      l0_cmd_bundle.list, 1, &external_semaphore_handle,
      &semaphore_signal_params, nullptr, 0, nullptr);

  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  dx11::wait_for_fence(fence, wait_value);

  EXPECT_EQ(fence->GetCompletedValue(), wait_value);

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::release_external_semaphore(external_semaphore_handle);
  CloseHandle(fence_shared_handle);
#endif
}

} // namespace
