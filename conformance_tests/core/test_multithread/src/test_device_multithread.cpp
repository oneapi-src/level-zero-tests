/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <chrono>
#include <thread>
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace {

const int thread_iters = 1000;
const size_t num_threads = 16;

void driver_thread_test() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID ::" << thread_id;

  auto driver = lzt::get_default_driver();
  auto driver_version = lzt::get_driver_version(driver);
  auto driver_api_version = lzt::get_api_version(driver);
  auto driver_ipc_props = lzt::get_ipc_properties(driver);

  for (int i = 0; i < thread_iters; i++) {

    ASSERT_EQ(driver, lzt::get_default_driver());
    ASSERT_EQ(driver_version, lzt::get_driver_version(driver));
    ASSERT_EQ(driver_api_version, lzt::get_api_version(driver));
    auto new_driver_ipc_props = lzt::get_ipc_properties(driver);
    ASSERT_EQ(0, memcmp(&driver_ipc_props, &new_driver_ipc_props,
                        sizeof(driver_ipc_props)));
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void device_thread_test(ze_driver_handle_t driver) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID ::" << thread_id;

  auto device = lzt::get_default_device(driver);
  auto sub_device = lzt::get_ze_sub_devices(device);
  auto device_props = lzt::get_device_properties(device);
  auto device_compute_props = lzt::get_compute_properties(device);
  auto device_module_props = lzt::get_device_module_properties(device);
  auto device_mem_prop_count = lzt::get_memory_properties_count(device);
  auto device_mem_props = lzt::get_memory_properties(device);
  auto device_mem_access_props = lzt::get_memory_access_properties(device);
  auto device_cache_props = lzt::get_cache_properties(device);
  auto device_image_props = lzt::get_image_properties(device);

  for (int i = 0; i < thread_iters; i++) {
    ASSERT_EQ(device, lzt::get_default_device(driver));
    ASSERT_EQ(sub_device, lzt::get_ze_sub_devices(device));
    auto new_device_props = lzt::get_device_properties(device);
    ASSERT_EQ(0,
              memcmp(&device_props, &new_device_props, sizeof(device_props)));
    auto new_compute_props = lzt::get_compute_properties(device);
    ASSERT_EQ(0, memcmp(&device_compute_props, &new_compute_props,
                        sizeof(device_compute_props)));
    auto new_device_module_props = lzt::get_device_module_properties(device);
    ASSERT_EQ(0, memcmp(&device_module_props, &new_device_module_props,
                        sizeof(device_module_props)));
    ASSERT_EQ(device_mem_prop_count, lzt::get_memory_properties_count(device));
    auto new_mem_props = lzt::get_memory_properties(device);
    int j = 0;
    for (auto mem_prop : device_mem_props) {
      ASSERT_EQ(0, memcmp(&mem_prop, &new_mem_props[j++], sizeof(mem_prop)));
    }
    auto new_mem_access_props = lzt::get_memory_access_properties(device);
    ASSERT_EQ(0, memcmp(&device_mem_access_props, &new_mem_access_props,
                        sizeof(device_mem_access_props)));
    auto new_cache_props = lzt::get_cache_properties(device);
    ASSERT_EQ(device_cache_props.size(), new_cache_props.size());
    auto new_image_props = lzt::get_image_properties(device);
    ASSERT_EQ(0, memcmp(&device_image_props, &new_image_props,
                        sizeof(device_image_props)));
  }
}

class zeDeviceMultithreadTests : public ::testing::Test {};

TEST(zeDeviceMultithreadTests,
     GivenMultipleThreadsWhenUsingDriverAPIsThenResultsAreConsistent) {
  std::vector<std::thread *> threads;

  for (int i = 0; i < num_threads; i++)
    threads.push_back(new std::thread(driver_thread_test));

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (int i = 0; i < num_threads; i++) {
    delete threads[i];
  }
}

TEST(zeDeviceMultithreadTests,
     GivenMultipleThreadsWhenUsingDeviceAPIsThenResultsAreConsistent) {
  std::vector<std::thread *> threads;
  auto driver = lzt::get_default_driver();
  for (int i = 0; i < num_threads; i++)
    threads.push_back(new std::thread(device_thread_test, driver));

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (int i = 0; i < num_threads; i++) {
    delete threads[i];
  }
}

} // namespace
