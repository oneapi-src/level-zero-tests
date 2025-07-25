/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

#include <algorithm>
#include <cstdlib>
#include <array>
#include <string>
#include <chrono>
#include <thread>

using namespace level_zero_tests;

namespace {

class zeCommandListAppendMemoryPrefetchDataVerificationTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<bool, bool, bool, bool, size_t, float>> {
protected:
  uint64_t GetPageFaultCount() {
    uint64_t page_fault_count = 0;
#ifdef __linux__
    FILE *fp;
    char buffer[1024];
    fp =
        popen("sudo grep svm_pagefault_count /sys/kernel/debug/dri/0/gt0/stats "
              "| sed 's/svm_pagefault_count: //'",
              "r");
    if (fp == NULL) {
      perror("Failed to run command");
      exit(EXIT_FAILURE);
    }
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
      page_fault_count = atoi(buffer);
    }
    pclose(fp);
#endif
    return (page_fault_count);
  }

  void RunAppendMemoryCopyWithPrefetch(bool is_src_shared_system,
                                       bool is_dst_shared_system,
                                       bool is_immediate, bool isCopyOnly,
                                       size_t size, float prefetch_ratio) {
    FILE *fp_err;
    fp_err = freopen("myfile.txt", "w", stderr);

    const uint8_t value = rand() & 0xFF;
    const uint8_t init = (value + 0x22) & 0xFF;
    uint64_t expected_ioctl_count = 0;

    void *src_memory = lzt::allocate_shared_memory_with_allocator_selector(
        size, is_src_shared_system);
    void *dst_memory = lzt::allocate_shared_memory_with_allocator_selector(
        size, is_dst_shared_system);

    uint32_t ordinal = 0;
    if (isCopyOnly) {
      ordinal = 1;
    }
    size_t prefetch_size = (prefetch_ratio * size);

    memset(src_memory, value, size);
    memset(dst_memory, init, size);

    const char *src_memory_type = is_src_shared_system ? "SVM" : "USM";
    const char *dst_memory_type = is_dst_shared_system ? "SVM" : "USM";

    //    NOTE:  cannot use LOG_DEBUG or LOG_INFO because redirecting stderr to
    //    file
    //
    //    printf("src_memory (%s)= %p dst_memory (%s)= %p immediate = %d
    //    isCopyOnly "
    //           "= %d size = %ld  prefetch_size = %ld\n",
    //           src_memory_type, src_memory, dst_memory_type, dst_memory,
    //           is_immediate, isCopyOnly, size, prefetch_size);

    uint64_t page_fault_count1 = GetPageFaultCount();

    auto cmd_bundle = lzt::create_command_bundle(
        lzt::get_default_context(), zeDevice::get_instance()->get_device(), 0,
        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0,
        ordinal, 0, is_immediate);

    const auto t0 = std::chrono::steady_clock::now();

    if (prefetch_size) {
      if (is_src_shared_system) {
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryPrefetch(
            cmd_bundle.list, src_memory, prefetch_size));
        expected_ioctl_count++;
      }
      if (is_dst_shared_system) {
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryPrefetch(
            cmd_bundle.list, dst_memory, prefetch_size));
        expected_ioctl_count++;
      }
    }
    lzt::append_memory_copy(cmd_bundle.list, static_cast<void *>(dst_memory),
                            static_cast<void *>(src_memory), size);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    const auto t1 = std::chrono::steady_clock::now();
    execution_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    //    printf("time(nsec) = %ld\n", execution_time);

    EXPECT_EQ(0, memcmp(src_memory, dst_memory, size));

    uint64_t page_fault_count2 = GetPageFaultCount();
    page_faults_iteration = page_fault_count2 - page_fault_count1;

    //  printf("total page faults per iteration = %ld\n",
    //  page_faults_iteration);
    if (!is_src_shared_system && !is_dst_shared_system) {
      EXPECT_EQ(0, page_faults_iteration);
    }
#ifdef __linux__
    fflush(stderr);
    freopen("/dev/tty", "w", stderr);
    fclose(fp_err);
    FILE *fp;
    fp = popen("grep -c setVmPrefetch myfile.txt", "r");
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
      prefetch_ioctl_count = atoi(buffer);
    }
    pclose(fp);

    //  printf("Prefetch Ioctl Count = %ld\n", prefetch_ioctl_count);
    EXPECT_EQ(expected_ioctl_count, prefetch_ioctl_count);
#endif
    free_memory_with_allocator_selector(src_memory, is_src_shared_system);
    free_memory_with_allocator_selector(dst_memory, is_dst_shared_system);
    lzt::destroy_command_bundle(cmd_bundle);
  }

public:
  uint64_t prefetch_ioctl_count = 0;
  uint64_t page_faults_iteration = 0;
  uint64_t execution_time = 0;
};

LZT_TEST_F(zeCommandListAppendMemoryPrefetchDataVerificationTests,
           GivenNoPrefetchCaseVerifyDecreasingPageFaultsWithPrefetchCases) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  std::vector<std::vector<uint64_t>> page_fault(3, std::vector<uint64_t>(3, 0));
  std::vector<size_t> transfer_size{32 * 1024 * 1024, 512 * 1024 * 1024};
  float step = 0.5;

  for (auto size : transfer_size) {
    bool src_shared = true;
    bool dst_shared = true;
    for (uint32_t i = 0; i < 3; i++) {
      if (i == 1) {
        src_shared = false;
      }
      if (i == 2) {
        src_shared = true;
        dst_shared = false;
      }
      for (uint32_t j = 0; j < 3; j++) {
        RunAppendMemoryCopyWithPrefetch(src_shared, dst_shared, false, false,
                                        size, j * step);
        page_fault[i][j] = page_faults_iteration;
      }
      for (uint32_t j = 1; j < 3; j++) {
        EXPECT_LT(page_fault[i][j], page_fault[i][j - 1]);
      }
      // Expect all prefetch (no page faults) for 100% case
      EXPECT_EQ(0, page_fault[i][2]);
    }
  }
}

LZT_TEST_F(zeCommandListAppendMemoryPrefetchDataVerificationTests,
           GivenNoPrefetchCaseVerifyDecreasingExecTimeWithPrefetchCases) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  float step = 0.5;

  size_t size = 32 * 1024 * 1024;
  std::vector<uint64_t> duration(3, 0);
  bool src_shared = true;
  bool dst_shared = true;
  for (uint32_t i = 0; i < 3; i++) {
    std::vector<uint64_t> duration(3, 0);
    if (i == 1) {
      src_shared = false;
    }
    if (i == 2) {
      src_shared = true;
      dst_shared = false;
    }
    for (uint32_t j = 0; j < 20; j++) {
      for (uint32_t k = 0; k < 3; k++) {
        RunAppendMemoryCopyWithPrefetch(src_shared, dst_shared, false, false,
                                        size, k * step);
        duration[k] += execution_time;
      }
    }

    for (uint32_t k = 0; k < 3; k++) {
      printf("duration[%2d] = %ld\n", k, duration[k] / 20);
      if (k > 0) {
        EXPECT_LT(duration[k], duration[k - 1]);
      }
    }
  }
}

LZT_TEST_P(
    zeCommandListAppendMemoryPrefetchDataVerificationTests,
    GivenAllUsmAndSvmPermutationsConfirmSuccessfulExecutionPrefetchWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunAppendMemoryCopyWithPrefetch(
      std::get<0>(GetParam()), std::get<1>(GetParam()), std::get<2>(GetParam()),
      std::get<3>(GetParam()), std::get<4>(GetParam()),
      std::get<5>(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    ParameterizedTests, zeCommandListAppendMemoryPrefetchDataVerificationTests,
    ::testing::Combine(::testing::Bool(), ::testing::Bool(), ::testing::Bool(),
                       ::testing::Bool(),
                       ::testing::Values(32, 32 * 1024, 32 * 1024 * 1024),
                       ::testing::Values(0.0, 0.25, 0.5, 0.75, 1.0)));

} // namespace
