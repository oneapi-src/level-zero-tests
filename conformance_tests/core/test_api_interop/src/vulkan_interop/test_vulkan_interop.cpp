/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "vulkan_interop/vulkan_interop_fixture.hpp"

namespace {

#ifdef __linux__
struct VulkanInteroperabilityLinuxTests : public VulkanInteroperabilityTests {};

struct VulkanInteroperabilityWindowsTests : public ::testing::Test {
  void SetUp() override { GTEST_SKIP() << "Not supported on Linux"; }
};
#else
struct VulkanInteroperabilityLinuxTests : public ::testing::Test {
  void SetUp() override { GTEST_SKIP() << "Not supported on Windows"; }
};

struct VulkanInteroperabilityWindowsTests : public VulkanInteroperabilityTests {
};
#endif

LZT_TEST_F(
    VulkanInteroperabilityWindowsTests,
    GivenVulkanSemaphoreWhenImportingExternalSemaphoreAsOpaqueWin32HandleThenIsSuccess) {
#ifndef __linux__
  semaphore_import_test(VK_SEMAPHORE_TYPE_BINARY);
#endif
}

LZT_TEST_F(
    VulkanInteroperabilityWindowsTests,
    GivenVulkanTimelineSemaphoreWhenImportingExternalSemaphoreAsOpaqueWin32HandleThenIsSuccess) {
#ifndef __linux__
  semaphore_import_test(VK_SEMAPHORE_TYPE_TIMELINE);
#endif
}

LZT_TEST_F(
    VulkanInteroperabilityLinuxTests,
    GivenVulkanSemaphoreWhenImportingExternalSemaphoreAsOpaqueFdThenIsSuccess) {
#ifdef __linux__
  semaphore_import_test(VK_SEMAPHORE_TYPE_BINARY);
#endif
}

LZT_TEST_F(
    VulkanInteroperabilityLinuxTests,
    GivenVulkanTimelineSemaphoreWhenImportingExternalSemaphoreAsOpaqueFdThenIsSuccess) {
#ifdef __linux__
  semaphore_import_test(VK_SEMAPHORE_TYPE_TIMELINE);
#endif
}

} // namespace
