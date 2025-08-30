/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <thread>
#include <array>
#include <chrono>
#include <condition_variable>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

using lzt::to_u32;

constexpr size_t size_ = 1024;
constexpr size_t num_threads = 16;
constexpr size_t thread_iters = 200;
constexpr size_t size = 5;
std::condition_variable cv;
size_t num_count = 0;
std::mutex mtx;

typedef struct _node {
  uint32_t value;
  struct _node *next;
} node;

void make_resident_device_memory() {
  void *memory_ = nullptr;

  for (uint32_t i = 0; i < thread_iters; i++) {
    memory_ = lzt::allocate_device_memory(size_);
    lzt::make_memory_resident(lzt::zeDevice::get_instance()->get_device(),
                              memory_, size_);
    lzt::free_memory(memory_);
  }
}

void make_resident_evict_device_memory() {
  void *memory_ = nullptr;

  for (uint32_t i = 0; i < thread_iters; i++) {
    memory_ = lzt::allocate_device_memory(size_);
    lzt::make_memory_resident(lzt::zeDevice::get_instance()->get_device(),
                              memory_, size_);
    lzt::evict_memory(lzt::zeDevice::get_instance()->get_device(), memory_,
                      size_);
    lzt::free_memory(memory_);
  }
}

void make_resident_evict_API(ze_module_handle_t module) {
  ze_kernel_handle_t kernel;

  for (uint32_t j = 0U; j < thread_iters; j++) {
    auto context = lzt::get_default_context();
    auto driver = lzt::get_default_driver();
    auto device = lzt::get_default_device(driver);

    // set up
    auto command_list = lzt::create_command_list(device);
    auto command_queue = lzt::create_command_queue(device);
    kernel = lzt::create_function(module, "residency_function");

  ze_device_mem_alloc_flags_t device_flags = 0U;
  ze_host_mem_alloc_flags_t host_flags = 0U;

    node *data = static_cast<node *>(lzt::allocate_shared_memory(
        sizeof(node), 1, device_flags, host_flags, device, context));
    data->value = 0;
    node *temp = data;
    for (size_t i = 0U; i < size; i++) {
      temp->next = static_cast<node *>(lzt::allocate_shared_memory(
          sizeof(node), 1, device_flags, host_flags, device, context));
      temp = temp->next;
      temp->value = to_u32(i + 1);
    }

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
    lzt::set_argument_value(kernel, 1, sizeof(size_t), &size);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::append_barrier(command_list, nullptr, 0, nullptr);
    lzt::close_command_list(command_list);

    temp = data->next;
    node *temp2;
    for (size_t i = 0U; i < size; i++) {
      temp2 = temp->next;
      lzt::make_memory_resident(device, temp, sizeof(node));
      temp = temp2;
    }

    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    temp = data->next;
    for (size_t i = 0U; i < size; i++) {
      lzt::evict_memory(device, temp, sizeof(node));
      temp = temp->next;
    }

    // cleanup
    temp = data;
    // total of size elements linked *after* initial element
    for (size_t i = 0U; i < size + 1; i++) {
      // the kernel increments each node's value by 1
      ASSERT_EQ(temp->value, i + 1);

      temp2 = temp->next;
      lzt::free_memory(context, temp);
      temp = temp2;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
  }
}

void indirect_access_Kernel(ze_module_handle_t module, bool is_immediate) {
  ze_kernel_handle_t kernel;

  auto context = lzt::get_default_context();
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);

  ze_device_mem_alloc_flags_t device_flags = 0U;
  ze_host_mem_alloc_flags_t host_flags = 0U;

  node *data = static_cast<node *>(lzt::allocate_shared_memory(
      sizeof(node), 1, device_flags, host_flags, device, context));
  node *temp = data;
  for (size_t i = 0U; i < size; i++) {
    temp->next = static_cast<node *>(lzt::allocate_shared_memory(
        sizeof(node), 1, device_flags, host_flags, device, context));
    temp = temp->next;
  }

  // init
  temp = data;
  for (size_t i = 0U; i < size + 1; i++) {
    temp->value = to_u32(i);
    temp = temp->next;
  }

  for (size_t iter = 0U; iter < thread_iters; iter++) {
    // set up
    auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);
    kernel = lzt::create_function(module, ZE_KERNEL_FLAG_FORCE_RESIDENCY,
                                  "residency_function");

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::kernel_set_indirect_access(kernel,
                                    ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED);

    lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
    lzt::set_argument_value(kernel, 1, sizeof(size_t), &size);
    lzt::append_launch_function(cmd_bundle.list, kernel, &group_count, nullptr,
                                0, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    // check
    temp = data;
    for (size_t i = 0U; i < size + 1; i++) {
      // kernel should increment each node's value by 1
      ASSERT_EQ(temp->value, i + iter + 1);
      temp = temp->next;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  // wait until all the threads are done accessing allocations indirectly via
  // kernel
  std::unique_lock<std::mutex> lk(mtx);
  num_count++;
  if (num_count < num_threads) {
    cv.wait(lk, [] { return num_count == num_threads; });
  } else {
    cv.notify_all();
  }

  // cleanup
  for (size_t i = 0U; i < size + 1; i++) {
    temp = data;
    data = temp->next;
    lzt::free_memory(context, temp);
  }
}

class zeContextMakeResidentTests : public ::testing::Test {};

LZT_TEST(
    zeContextMakeResidentTests,
    GivenMultipleThreadsWhenMakingTheDeviceMemoryResidentFollowedByFreeThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::vector<std::unique_ptr<std::thread>> threads;

  for (size_t i = 0U; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(
        new std::thread(make_resident_device_memory)));
  }

  for (size_t i = 0U; i < num_threads; i++) {
    threads[i]->join();
  }
}

LZT_TEST(
    zeContextMakeResidentTests,
    GivenMultipleThreadsWhenMakingTheDeviceMemoryResidentFollowedByEvictAndFreeThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::vector<std::unique_ptr<std::thread>> threads;

  for (size_t i = 0U; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(
        new std::thread(make_resident_evict_device_memory)));
  }

  for (size_t i = 0U; i < num_threads; i++) {
    threads[i]->join();
  }
}

LZT_TEST(
    zeContextMakeResidentTests,
    GivenMultipleThreadsWhenMakingTheSharedMemoryResidentFollowedByEvictUsingAPIThenSuccessIsReturned) {

  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::vector<std::unique_ptr<std::thread>> threads;

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  ze_module_handle_t module =
      lzt::create_module(device, "residency_tests_multi_thread.spv");

  for (size_t i = 0U; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(
        new std::thread(make_resident_evict_API, module)));
  }

  for (size_t i = 0U; i < num_threads; i++) {
    threads[i]->join();
  }

  lzt::destroy_module(module);
}

LZT_TEST(
    zeContextMakeResidentTests,
    GivenMultipleThreadsWhenMakingTheSharedMemoryResidentFollowedByEvictUsingIndirectAccessThroughKernelThenSuccessIsReturned) {

  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::vector<std::unique_ptr<std::thread>> threads;

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  ze_module_handle_t module =
      lzt::create_module(device, "residency_tests_multi_thread.spv");

  for (size_t i = 0U; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(
        new std::thread(indirect_access_Kernel, module, false)));
  }

  for (size_t i = 0U; i < num_threads; i++) {
    threads[i]->join();
  }

  lzt::destroy_module(module);
}

LZT_TEST(
    zeContextMakeResidentTests,
    GivenMultipleThreadsWhenMakingTheSharedMemoryResidentFollowedByEvictUsingIndirectAccessThroughKernelOnImmediateCmdListThenSuccessIsReturned) {

  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::vector<std::unique_ptr<std::thread>> threads;

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  ze_module_handle_t module =
      lzt::create_module(device, "residency_tests_multi_thread.spv");

  for (size_t i = 0U; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(
        new std::thread(indirect_access_Kernel, module, true)));
  }

  for (size_t i = 0U; i < num_threads; i++) {
    threads[i]->join();
  }

  lzt::destroy_module(module);
}

void image_make_resident_evict(ze_image_handle_t dflt_device_image_) {
  void *memory_ = nullptr;

  for (uint32_t i = 0; i < thread_iters; i++) {
    memory_ = lzt::allocate_device_memory(size_);
    EXPECT_ZE_RESULT_SUCCESS(

        zeContextMakeImageResident(lzt::get_default_context(),
                                   lzt::zeDevice::get_instance()->get_device(),
                                   dflt_device_image_));
    EXPECT_ZE_RESULT_SUCCESS(zeContextEvictImage(
        lzt::get_default_context(), lzt::zeDevice::get_instance()->get_device(),
        dflt_device_image_));
    lzt::free_memory(memory_);
  }
}

class zeDeviceMakeImageResidentTests : public testing::Test {};

LZT_TEST_F(
    zeDeviceMakeImageResidentTests,
    GivenMultipleThreadsWhenMakingImageResidentFollowedByEvictThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  lzt::zeImageCreateCommon img;
  std::vector<std::unique_ptr<std::thread>> threads;

  for (size_t i = 0U; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(
        new std::thread(image_make_resident_evict, img.dflt_device_image_)));
  }

  for (size_t i = 0U; i < num_threads; i++) {
    threads[i]->join();
  }
}

} // namespace
