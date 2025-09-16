/*
 *
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "random/random.hpp"
#include <vector>
#include <utility>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

static const size_t vec_sz = 1UL << 16;

class zeMixedCMDListsTests : public ::testing::Test {
protected:
  static ze_context_handle_t context;
  static ze_device_handle_t device;
  static ze_module_handle_t module;
  static ze_kernel_handle_t kernel;
  static ze_event_pool_handle_t event_pool;
  // Ordinal and index of all physical engines
  static std::vector<std::pair<uint32_t, uint32_t>> compute_engines;
  static std::vector<std::pair<uint32_t, uint32_t>> copy_engines;
  static const uint32_t event_pool_sz = 512;

  static void SetUpTestSuite() {
    context = lzt::get_default_context();
    device = lzt::get_default_device(lzt::get_default_driver());

    const auto cq_group_properties =
        lzt::get_command_queue_group_properties(device);
    EXPECT_NE(cq_group_properties.size(), 0);

    for (uint32_t i = 0; i < cq_group_properties.size(); i++) {
      if (cq_group_properties[i].flags &
          ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
        for (uint32_t j = 0; j < cq_group_properties[i].numQueues; j++) {
          compute_engines.emplace_back(i, j);
        }
      } else if (cq_group_properties[i].flags &
                 ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) {
        for (uint32_t j = 0; j < cq_group_properties[i].numQueues; j++) {
          copy_engines.emplace_back(i, j);
        }
      }
    }

    module = lzt::create_module(device, "cmdlist_verify.spv");
    kernel = lzt::create_function(module, "add_one");
    event_pool = lzt::create_event_pool(context, event_pool_sz,
                                        ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  }

  static void TearDownTestSuite() {
    compute_engines.clear();
    copy_engines.clear();
    lzt::destroy_event_pool(event_pool);
    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
  }
};

ze_context_handle_t zeMixedCMDListsTests::context = nullptr;
ze_device_handle_t zeMixedCMDListsTests::device = nullptr;
ze_module_handle_t zeMixedCMDListsTests::module = nullptr;
ze_kernel_handle_t zeMixedCMDListsTests::kernel = nullptr;
ze_event_pool_handle_t zeMixedCMDListsTests::event_pool = nullptr;
std::vector<std::pair<uint32_t, uint32_t>>
    zeMixedCMDListsTests::compute_engines{};
std::vector<std::pair<uint32_t, uint32_t>> zeMixedCMDListsTests::copy_engines{};

class zeTestMixedCMDListsIndependentOverlapping
    : public zeMixedCMDListsTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flags_t, ze_command_queue_flags_t,
                     ze_command_queue_mode_t, ze_command_list_flags_t, bool>> {
};

LZT_TEST_P(
    zeTestMixedCMDListsIndependentOverlapping,
    GivenRegularCMDListOnComputeEngineAndImmCMDListOnCopyEngineThenCorrectResultsAreReturned) {
  std::vector<void *> host_vecs_compute;
  std::vector<void *> host_vecs_copy;
  std::vector<void *> device_vecs_compute;
  std::vector<void *> device_vecs_copy;
  std::vector<ze_command_queue_handle_t> cqs_compute;
  std::vector<ze_command_list_handle_t> cls_compute;
  std::vector<ze_command_list_handle_t> cls_copy_imm;
  std::vector<ze_event_handle_t> events_fill;
  std::vector<ze_event_handle_t> events_copy;
  std::vector<uint8_t> patterns_compute;
  std::vector<uint8_t> patterns_copy;

  const ze_command_queue_flags_t cq_flags_compute = std::get<0>(GetParam());
  const ze_command_queue_flags_t cq_flags_copy = std::get<1>(GetParam());
  const ze_command_queue_mode_t cq_mode_copy = std::get<2>(GetParam());
  ze_command_list_flags_t cl_flags_compute = std::get<3>(GetParam());
  const auto use_events_sync = std::get<4>(GetParam());

  // Create compute engine CMD queues, lists and host/device buffers
  for (auto &engine : compute_engines) {
    const uint32_t ordinal = engine.first;
    const uint32_t index = engine.second;
    const bool is_explicit =
        cq_flags_compute & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

    // Must be ASYNC to overlap with the execution on the copy engine
    cqs_compute.push_back(lzt::create_command_queue(
        context, device, cq_flags_compute, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit ? index : 0));

    if (is_explicit) {
      cl_flags_compute |= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
    }
    cls_compute.push_back(
        lzt::create_command_list(context, device, cl_flags_compute, ordinal));

    host_vecs_compute.push_back(lzt::allocate_host_memory(vec_sz));
    device_vecs_compute.push_back(lzt::allocate_device_memory(vec_sz));

    patterns_compute.push_back(cls_compute.size() - 1);
  }

  // Create copy engine immediate CMD lists, events and host/device buffers
  for (auto &engine : copy_engines) {
    const uint32_t ordinal = engine.first;
    const uint32_t index = engine.second;
    const bool is_explicit =
        cq_flags_copy & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

    cls_copy_imm.push_back(lzt::create_immediate_command_list(
        context, device, cq_flags_copy, cq_mode_copy,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit ? index : 0));

    ze_event_desc_t ev_desc = {};
    ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    ev_desc.pNext = nullptr;
    ev_desc.index = cls_copy_imm.size() - 1;
    ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    events_fill.push_back(lzt::create_event(event_pool, ev_desc));

    ev_desc.index = cls_copy_imm.size() - 1 + event_pool_sz / 2;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    events_copy.push_back(lzt::create_event(event_pool, ev_desc));

    host_vecs_copy.push_back(lzt::allocate_host_memory(vec_sz));
    device_vecs_copy.push_back(lzt::allocate_device_memory(vec_sz));

    patterns_copy.push_back(42 + cls_copy_imm.size() - 1);
  }

  // Fill compute engine CMD lists with tasks
  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_sz / 8, 1, 1};

  for (int i = 0; i < cls_compute.size(); i++) {
    const auto cl = cls_compute[i];
    void *h_vec = host_vecs_compute[i];
    void *d_vec = device_vecs_compute[i];
    lzt::append_memory_fill(cl, d_vec, &patterns_compute[i], 1, vec_sz,
                            nullptr);
    lzt::append_barrier(cl);
    lzt::set_argument_value(kernel, 0, sizeof(void *), &d_vec);
    lzt::append_launch_function(cl, kernel, &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, h_vec, d_vec, vec_sz, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, d_vec, h_vec, vec_sz, nullptr);
    lzt::append_barrier(cl);
    lzt::append_launch_function(cl, kernel, &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, h_vec, d_vec, vec_sz, nullptr);
    lzt::append_barrier(cl);
    lzt::close_command_list(cl);
  }

  // Submit compute engine CMD lists, ASYNC execution
  for (int i = 0; i < cls_compute.size(); i++) {
    lzt::execute_command_lists(cqs_compute[i], 1, &cls_compute[i], nullptr);
  }

  // Submit tasks to copy engine immediate CMD lists
  const int log_blk_len = 8;
  const int blk_sz = (1u << log_blk_len);
  for (int i = 0; i < (vec_sz >> log_blk_len); i++) {
    for (int j = 0; j < cls_copy_imm.size(); j++) {
      const auto cl = cls_copy_imm[j];
      const auto device_ptr =
          reinterpret_cast<uint8_t *>(device_vecs_copy[j]) + i * blk_sz;
      const auto host_ptr =
          reinterpret_cast<uint8_t *>(host_vecs_copy[j]) + i * blk_sz;
      lzt::append_memory_fill(cl, device_ptr, &patterns_copy[j], 1, blk_sz,
                              events_fill[j]);
      lzt::append_memory_copy(cl, host_ptr, device_ptr, blk_sz, events_copy[j],
                              1, &events_fill[j]);
    }
    for (int j = 0; j < cls_copy_imm.size(); j++) {
      if (use_events_sync) {
        EXPECT_ZE_RESULT_SUCCESS(
            zeEventHostSynchronize(events_copy[j], UINT64_MAX));
      } else {
        EXPECT_ZE_RESULT_SUCCESS(
            zeCommandListHostSynchronize(cls_copy_imm[j], UINT64_MAX));
      }
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(events_copy[j]));
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(events_fill[j]));
    }
  }

  // Sync with compute engine queues
  for (int i = 0; i < cls_compute.size(); i++) {
    lzt::synchronize(cqs_compute[i], UINT64_MAX);
  }

  // Verify compute engine results
  for (uint8_t i = 0; i < cls_compute.size(); i++) {
    const uint8_t *vec = reinterpret_cast<uint8_t *>(host_vecs_compute[i]);
    for (size_t j = 0; j < vec_sz; j++) {
      if (vec[j] != i + 2) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Verify copy engine results
  for (uint8_t i = 0; i < cls_copy_imm.size(); i++) {
    const uint8_t *vec = reinterpret_cast<uint8_t *>(host_vecs_copy[i]);
    for (size_t j = 0; j < vec_sz; j++) {
      if (vec[j] != i + 42) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Cleanup
  for (int i = 0; i < cls_compute.size(); i++) {
    lzt::free_memory(host_vecs_compute[i]);
    lzt::free_memory(device_vecs_compute[i]);
    lzt::destroy_command_list(cls_compute[i]);
    lzt::destroy_command_queue(cqs_compute[i]);
  }

  for (int i = 0; i < cls_copy_imm.size(); i++) {
    lzt::free_memory(host_vecs_copy[i]);
    lzt::free_memory(device_vecs_copy[i]);
    lzt::destroy_command_list(cls_copy_imm[i]);
    lzt::destroy_event(events_fill[i]);
    lzt::destroy_event(events_copy[i]);
  }
}

LZT_TEST_P(
    zeTestMixedCMDListsIndependentOverlapping,
    GivenRegularCMDListOnCopyEngineAndImmCMDListOnComputeEngineThenCorrectResultsAreReturned) {
  std::vector<void *> host_vecs_compute;
  std::vector<void *> host_vecs_copy;
  std::vector<void *> device_vecs_compute;
  std::vector<void *> device_vecs_copy;
  std::vector<ze_command_queue_handle_t> cqs_copy;
  std::vector<ze_command_list_handle_t> cls_copy;
  std::vector<ze_command_list_handle_t> cls_compute_imm;
  std::vector<ze_event_handle_t> events;
  std::vector<uint8_t> patterns_compute;
  std::vector<uint8_t> patterns_copy;

  const ze_command_queue_flags_t cq_flags_compute = std::get<0>(GetParam());
  const ze_command_queue_flags_t cq_flags_copy = std::get<1>(GetParam());
  const ze_command_queue_mode_t cq_mode_compute = std::get<2>(GetParam());
  ze_command_list_flags_t cl_flags_copy = std::get<3>(GetParam());
  const auto use_events_sync = std::get<4>(GetParam());

  // Create compute engine immediate CMD lists, events and host/device buffers
  for (auto &engine : compute_engines) {
    const uint32_t ordinal = engine.first;
    const uint32_t index = engine.second;
    const bool is_explicit =
        cq_flags_compute & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

    cls_compute_imm.push_back(lzt::create_immediate_command_list(
        context, device, cq_flags_compute, cq_mode_compute,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit ? index : 0));

    ze_event_desc_t ev_desc = {};
    ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    ev_desc.pNext = nullptr;
    ev_desc.index = cls_compute_imm.size() - 1;
    ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    events.push_back(lzt::create_event(event_pool, ev_desc));

    host_vecs_compute.push_back(lzt::allocate_host_memory(vec_sz));
    device_vecs_compute.push_back(lzt::allocate_device_memory(vec_sz));

    patterns_compute.push_back(cls_compute_imm.size() - 1);
  }

  // Create copy engine CMD queues, lists and host/device buffers
  for (auto &engine : copy_engines) {
    const uint32_t ordinal = engine.first;
    const uint32_t index = engine.second;
    const bool is_explicit =
        cq_flags_copy & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
    // ASYNC execution for overlapping with compute engine
    cqs_copy.push_back(lzt::create_command_queue(
        context, device, cq_flags_copy, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit ? index : 0));

    if (is_explicit) {
      cl_flags_copy |= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
    }
    cls_copy.push_back(
        lzt::create_command_list(context, device, cl_flags_copy, ordinal));

    host_vecs_copy.push_back(lzt::allocate_host_memory(vec_sz));
    device_vecs_copy.push_back(lzt::allocate_device_memory(vec_sz));

    patterns_copy.push_back(42 + cls_copy.size() - 1);
  }

  // Fill copy engine CMD lists with tasks
  const int log_blk_len = 1;
  const int blk_sz = (1u << log_blk_len);
  for (int j = 0; j < cls_copy.size(); j++) {
    const auto cl = cls_copy[j];
    for (int i = 0; i < (vec_sz >> log_blk_len); i++) {
      const auto device_ptr =
          reinterpret_cast<uint8_t *>(device_vecs_copy[j]) + i * blk_sz;
      const auto host_ptr =
          reinterpret_cast<uint8_t *>(host_vecs_copy[j]) + i * blk_sz;
      lzt::append_memory_fill(cl, device_ptr, &patterns_copy[j], 1, blk_sz,
                              nullptr);
      lzt::append_barrier(cl);
      lzt::append_memory_copy(cl, host_ptr, device_ptr, blk_sz, nullptr);
      lzt::append_barrier(cl);
    }
    lzt::close_command_list(cl);
  }

  // Submit copy engine CMD lists, ASYNC execution
  for (int i = 0; i < cls_copy.size(); i++) {
    lzt::execute_command_lists(cqs_copy[i], 1, &cls_copy[i], nullptr);
  }

  // Submit tasks to compute engine immediate CMD lists
  for (int i = 0; i < cls_compute_imm.size(); i++) {
    lzt::append_memory_fill(cls_compute_imm[i], device_vecs_compute[i],
                            &patterns_compute[i], 1, vec_sz, nullptr);
    lzt::append_barrier(cls_compute_imm[i]);
  }

  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_sz / 8, 1, 1};

  for (int i = 0; i < cls_compute_imm.size(); i++) {
    lzt::set_argument_value(kernel, 0, sizeof(void *), &device_vecs_compute[i]);
    lzt::append_launch_function(cls_compute_imm[i], kernel, &dispatch, nullptr,
                                0, nullptr);
    lzt::append_barrier(cls_compute_imm[i]);
  }
  for (int i = 0; i < cls_compute_imm.size(); i++) {
    lzt::append_memory_copy(cls_compute_imm[i], host_vecs_compute[i],
                            device_vecs_compute[i], vec_sz, events[i]);
    lzt::append_barrier(cls_compute_imm[i]);
  }
  for (int i = 0; i < cls_compute_imm.size(); i++) {
    if (use_events_sync) {
      lzt::event_host_synchronize(events[i], UINT64_MAX);
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cls_compute_imm[i], UINT64_MAX));
    }
  }

  // Sync with copy engine queues
  for (int i = 0; i < cls_copy.size(); i++) {
    lzt::synchronize(cqs_copy[i], UINT64_MAX);
  }

  // Verify copy engine results
  for (uint8_t i = 0; i < cls_copy.size(); i++) {
    const uint8_t *vec = reinterpret_cast<uint8_t *>(host_vecs_copy[i]);
    for (size_t j = 0; j < vec_sz; j++) {
      if (vec[j] != i + 42) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Verify compute engine results
  for (uint8_t i = 0; i < cls_compute_imm.size(); i++) {
    const uint8_t *vec = reinterpret_cast<uint8_t *>(host_vecs_compute[i]);
    for (size_t j = 0; j < vec_sz; j++) {
      if (vec[j] != i + 1) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Cleanup
  for (int i = 0; i < cls_compute_imm.size(); i++) {
    lzt::free_memory(host_vecs_compute[i]);
    lzt::free_memory(device_vecs_compute[i]);
    lzt::destroy_command_list(cls_compute_imm[i]);
    lzt::destroy_event(events[i]);
  }
  for (int i = 0; i < cls_copy.size(); i++) {
    lzt::free_memory(host_vecs_copy[i]);
    lzt::free_memory(device_vecs_copy[i]);
    lzt::destroy_command_list(cls_copy[i]);
    lzt::destroy_command_queue(cqs_copy[i]);
  }
}

LZT_TEST_P(
    zeTestMixedCMDListsIndependentOverlapping,
    GivenRegularAndImmCMDListsOnComputeEngineThenCorrectResultsAreReturned) {
  std::vector<void *> host_vecs;
  std::vector<void *> host_vecs_imm;
  std::vector<void *> device_vecs;
  std::vector<void *> device_vecs_imm;
  std::vector<ze_command_queue_handle_t> cqs;
  std::vector<ze_command_list_handle_t> cls;
  std::vector<ze_command_list_handle_t> cls_imm;
  std::vector<ze_event_handle_t> events;
  std::vector<uint8_t> patterns;
  std::vector<uint8_t> patterns_imm;

  const ze_command_queue_flags_t cq_flags = std::get<0>(GetParam());
  const ze_command_queue_flags_t cq_flags_imm = std::get<1>(GetParam());
  const ze_command_queue_mode_t cq_mode_imm = std::get<2>(GetParam());
  ze_command_list_flags_t cl_flags = std::get<3>(GetParam());
  const auto use_events_sync = std::get<4>(GetParam());

  // One regular and one immediate CMD list for each compute engine
  for (auto &engine : compute_engines) {
    const uint32_t ordinal = engine.first;
    const uint32_t index = engine.second;
    const bool is_explicit =
        cq_flags & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
    const bool is_explicit_imm =
        cq_flags_imm & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

    // Create the ASYNC CMD queue and the regular CMD lists
    cqs.push_back(lzt::create_command_queue(
        context, device, cq_flags, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit ? index : 0));

    if (is_explicit) {
      cl_flags |= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
    }
    cls.push_back(lzt::create_command_list(context, device, cl_flags, ordinal));

    // Create the immediate CMD lists and the events
    cls_imm.push_back(lzt::create_immediate_command_list(
        context, device, cq_flags_imm, cq_mode_imm,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal,
        is_explicit_imm ? index : 0));

    ze_event_desc_t ev_desc = {};
    ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    ev_desc.pNext = nullptr;
    ev_desc.index = cls_imm.size() - 1;
    ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    events.push_back(lzt::create_event(event_pool, ev_desc));

    // Allocate the vectors
    host_vecs.push_back(lzt::allocate_host_memory(vec_sz));
    host_vecs_imm.push_back(lzt::allocate_host_memory(vec_sz));
    device_vecs.push_back(lzt::allocate_device_memory(vec_sz));
    device_vecs_imm.push_back(lzt::allocate_device_memory(vec_sz));

    patterns.push_back(cls.size() - 1);
    patterns_imm.push_back(42 + cls_imm.size() - 1);
  }

  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_sz / 8, 1, 1};

  // Fill regular CMD lists
  for (int i = 0; i < cls.size(); i++) {
    const auto cl = cls[i];
    void *h_vec = host_vecs[i];
    void *d_vec = device_vecs[i];
    lzt::append_memory_fill(cl, d_vec, &patterns[i], 1, vec_sz, nullptr);
    lzt::append_barrier(cl);
    lzt::set_argument_value(kernel, 0, sizeof(void *), &d_vec);
    lzt::append_launch_function(cl, kernel, &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, h_vec, d_vec, vec_sz, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, d_vec, h_vec, vec_sz, nullptr);
    lzt::append_barrier(cl);
    lzt::append_launch_function(cl, kernel, &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, h_vec, d_vec, vec_sz, nullptr);
    lzt::append_barrier(cl);
    lzt::close_command_list(cl);
  }

  // Submit regular CMD lists, ASYNC execution
  for (int i = 0; i < cls.size(); i++) {
    lzt::execute_command_lists(cqs[i], 1, &cls[i], nullptr);
  }

  // Submit tasks to the immediate CMD lists
  for (int i = 0; i < cls_imm.size(); i++) {
    lzt::append_memory_fill(cls_imm[i], device_vecs_imm[i], &patterns_imm[i], 1,
                            vec_sz, nullptr);
    lzt::append_barrier(cls_imm[i]);
  }
  for (int i = 0; i < cls_imm.size(); i++) {
    lzt::set_argument_value(kernel, 0, sizeof(void *), &device_vecs_imm[i]);
    lzt::append_launch_function(cls_imm[i], kernel, &dispatch, nullptr, 0,
                                nullptr);
    lzt::append_barrier(cls_imm[i]);
  }
  for (int i = 0; i < cls_imm.size(); i++) {
    lzt::append_memory_copy(cls_imm[i], host_vecs_imm[i], device_vecs_imm[i],
                            vec_sz, events[i]);
    lzt::append_barrier(cls_imm[i]);
  }
  for (int i = 0; i < cls_imm.size(); i++) {
    if (use_events_sync) {
      lzt::event_host_synchronize(events[i], UINT64_MAX);
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cls_imm[i], UINT64_MAX));
    }
  }

  // Sync with regular CMD lists
  for (int i = 0; i < cls.size(); i++) {
    lzt::synchronize(cqs[i], UINT64_MAX);
  }

  // Verify regular CMD lists' results
  for (uint8_t i = 0; i < cls.size(); i++) {
    const uint8_t *vec = reinterpret_cast<uint8_t *>(host_vecs[i]);
    for (size_t j = 0; j < vec_sz; j++) {
      if (vec[j] != i + 2) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Verify immediate CMD lists' results
  for (uint8_t i = 0; i < cls_imm.size(); i++) {
    const uint8_t *vec = reinterpret_cast<uint8_t *>(host_vecs_imm[i]);
    for (size_t j = 0; j < vec_sz; j++) {
      if (vec[j] != i + 43) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Cleanup
  for (int i = 0; i < cls.size(); i++) {
    lzt::free_memory(host_vecs[i]);
    lzt::free_memory(host_vecs_imm[i]);
    lzt::free_memory(device_vecs[i]);
    lzt::free_memory(device_vecs_imm[i]);
    lzt::destroy_command_list(cls[i]);
    lzt::destroy_command_list(cls_imm[i]);
    lzt::destroy_command_queue(cqs[i]);
    lzt::destroy_event(events[i]);
  }
}

LZT_TEST_P(
    zeTestMixedCMDListsIndependentOverlapping,
    GivenRegularAndImmCMDListsOnCopyEngineThenCorrectResultsAreReturned) {
  std::vector<void *> host_vecs;
  std::vector<void *> host_vecs_imm;
  std::vector<void *> device_vecs;
  std::vector<void *> device_vecs_imm;
  std::vector<ze_command_queue_handle_t> cqs;
  std::vector<ze_command_list_handle_t> cls;
  std::vector<ze_command_list_handle_t> cls_imm;
  std::vector<ze_event_handle_t> events_fill;
  std::vector<ze_event_handle_t> events_copy;
  std::vector<uint8_t> patterns;
  std::vector<uint8_t> patterns_imm;

  const ze_command_queue_flags_t cq_flags = std::get<0>(GetParam());
  const ze_command_queue_flags_t cq_flags_imm = std::get<1>(GetParam());
  const ze_command_queue_mode_t cq_mode_imm = std::get<2>(GetParam());
  ze_command_list_flags_t cl_flags = std::get<3>(GetParam());
  const auto use_events_sync = std::get<4>(GetParam());

  // One regular and one immediate CMD list for each copy engine
  for (auto &engine : copy_engines) {
    const uint32_t ordinal = engine.first;
    const uint32_t index = engine.second;
    const bool is_explicit =
        cq_flags & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
    const bool is_explicit_imm =
        cq_flags_imm & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

    // Create the ASYNC CMD queue and the regular CMD lists
    cqs.push_back(lzt::create_command_queue(
        context, device, cq_flags, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit ? index : 0));

    if (is_explicit) {
      cl_flags |= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
    }
    cls.push_back(lzt::create_command_list(context, device, cl_flags, ordinal));

    // Create the immediate CMD lists and the events
    cls_imm.push_back(lzt::create_immediate_command_list(
        context, device, cq_flags_imm, cq_mode_imm,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal,
        is_explicit_imm ? index : 0));

    ze_event_desc_t ev_desc = {};
    ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    ev_desc.pNext = nullptr;
    ev_desc.index = cls_imm.size() - 1;
    ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    events_fill.push_back(lzt::create_event(event_pool, ev_desc));

    ev_desc.index = cls_imm.size() - 1 + event_pool_sz / 2;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    events_copy.push_back(lzt::create_event(event_pool, ev_desc));

    // Allocate the vectors
    host_vecs.push_back(lzt::allocate_host_memory(vec_sz));
    host_vecs_imm.push_back(lzt::allocate_host_memory(vec_sz));
    device_vecs.push_back(lzt::allocate_device_memory(vec_sz));
    device_vecs_imm.push_back(lzt::allocate_device_memory(vec_sz));

    patterns.push_back(cls.size());
    patterns_imm.push_back(42 + cls_imm.size() - 1);
  }

  const int log_blk_len = 6;
  const int blk_sz = (1u << log_blk_len);

  // Fill regular CMD lists
  for (int j = 0; j < cls.size(); j++) {
    const auto cl = cls[j];
    for (int i = 0; i < (vec_sz >> log_blk_len); i++) {
      const auto device_ptr =
          reinterpret_cast<uint8_t *>(device_vecs[j]) + i * blk_sz;
      const auto host_ptr =
          reinterpret_cast<uint8_t *>(host_vecs[j]) + i * blk_sz;
      lzt::append_memory_fill(cl, device_ptr, &patterns[j], 1, blk_sz, nullptr);
      lzt::append_barrier(cl);
      lzt::append_memory_copy(cl, host_ptr, device_ptr, blk_sz, nullptr);
      lzt::append_barrier(cl);
    }
    lzt::close_command_list(cl);
  }

  // Submit regular CMD lists, ASYNC execution
  for (int i = 0; i < cls.size(); i++) {
    lzt::execute_command_lists(cqs[i], 1, &cls[i], nullptr);
  }

  // Submit tasks to immediate CMD lists
  for (int i = 0; i < (vec_sz >> log_blk_len); i++) {
    for (int j = 0; j < cls_imm.size(); j++) {
      const auto cl = cls_imm[j];
      const auto device_ptr =
          reinterpret_cast<uint8_t *>(device_vecs_imm[j]) + i * blk_sz;
      const auto host_ptr =
          reinterpret_cast<uint8_t *>(host_vecs_imm[j]) + i * blk_sz;
      lzt::append_memory_fill(cl, device_ptr, &patterns_imm[j], 1, blk_sz,
                              events_fill[j]);
      lzt::append_memory_copy(cl, host_ptr, device_ptr, blk_sz, events_copy[j],
                              1, &events_fill[j]);
    }
    for (int j = 0; j < cls_imm.size(); j++) {
      if (use_events_sync) {
        EXPECT_ZE_RESULT_SUCCESS(
            zeEventHostSynchronize(events_copy[j], UINT64_MAX));
      } else {
        EXPECT_ZE_RESULT_SUCCESS(
            zeCommandListHostSynchronize(cls_imm[j], UINT64_MAX));
      }

      EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(events_copy[j]));
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(events_fill[j]));
    }
  }

  // Sync with regular CMD lists' queues
  for (int i = 0; i < cls.size(); i++) {
    lzt::synchronize(cqs[i], UINT64_MAX);
  }

  // Verify regular CMD lists' results
  for (uint8_t i = 0; i < cls.size(); i++) {
    const uint8_t *vec = reinterpret_cast<uint8_t *>(host_vecs[i]);
    for (size_t j = 0; j < vec_sz; j++) {
      if (vec[j] != i + 1) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Verify immediate CMD lists' results
  for (uint8_t i = 0; i < cls_imm.size(); i++) {
    const uint8_t *vec = reinterpret_cast<uint8_t *>(host_vecs_imm[i]);
    for (size_t j = 0; j < vec_sz; j++) {
      if (vec[j] != i + 42) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Cleanup
  for (int i = 0; i < cls.size(); i++) {
    lzt::free_memory(host_vecs[i]);
    lzt::free_memory(host_vecs_imm[i]);
    lzt::free_memory(device_vecs[i]);
    lzt::free_memory(device_vecs_imm[i]);
    lzt::destroy_command_list(cls[i]);
    lzt::destroy_command_list(cls_imm[i]);
    lzt::destroy_command_queue(cqs[i]);
    lzt::destroy_event(events_fill[i]);
    lzt::destroy_event(events_copy[i]);
  }
}

std::string event_to_str(bool use_event) {
  return use_event ? "using_event" : "not_using_event";
}

std::string command_list_flag_to_str(ze_command_list_flag_t flag) {
  std::stringstream ss;
  ss << "_cmd_list_flag_" << static_cast<uint32_t>(flag);
  return ss.str();
}

std::string command_queue_flag_to_str(ze_command_queue_flag_t flag) {
  std::stringstream ss;
  ss << "_cmd_queue_flag_" << static_cast<uint32_t>(flag);
  return ss.str();
}

std::string queue_mode_to_str(ze_command_queue_mode_t queue_mode) {
  std::stringstream ss;
  ss << "_queue_mode_" << static_cast<uint32_t>(queue_mode);
  return ss.str();
}

struct zeTestMixedCMDListsIndependentOverlappingTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    ss << "compute"
       << command_queue_flag_to_str(
              static_cast<ze_command_queue_flag_t>(std::get<0>(info.param)));
    ss << "_copy"
       << command_queue_flag_to_str(
              static_cast<ze_command_queue_flag_t>(std::get<1>(info.param)));
    ss << queue_mode_to_str(
        static_cast<ze_command_queue_mode_t>(std::get<2>(info.param)));
    ss << command_list_flag_to_str(
        static_cast<ze_command_list_flag_t>(std::get<3>(info.param)));
    ss << "_" << event_to_str(std::get<4>(info.param));
    return ss.str();
  }
};

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationSyncQueueWithEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationCopyExplicitSyncQueueWithEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationComputeExplicitSyncQueueWithEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationComputeExplicitCopyExplicithSyncWithEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationAsyncQueueWithEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationCopyExplicitAsyncQueueWithEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationComputeExplicitAsyncQueueWithEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationComputeExplicitCopyExplicithAsyncWithEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationSyncQueueWithoutEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(false)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationCopyExplicitSyncQueueWithoutEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(false)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationComputeExplicitSyncQueueWithoutEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(false)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationComputeExplicitCopyExplicithSyncWithoutEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(false)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationAsyncQueueWithoutEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(false)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationCopyExplicitAsyncQueueWithoutEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(false)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationComputeExplicitAsyncQueueWithoutEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0)),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(false)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());

INSTANTIATE_TEST_SUITE_P(
    IndependentCMDListsOverlappingParameterizationComputeExplicitCopyExplicitAsyncQueueWithoutEvents,
    zeTestMixedCMDListsIndependentOverlapping,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(false)),
    zeTestMixedCMDListsIndependentOverlappingTestNameSuffix());
class zeTestMixedCMDListsInterdependPairSameEngineType
    : public zeMixedCMDListsTests,
      public ::testing::WithParamInterface<std::tuple<
          bool, ze_command_queue_mode_t, ze_command_list_flag_t, bool>> {
protected:
  void SetUp() override {
    ze_event_desc_t ev_desc = {};
    ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    ev_desc.pNext = nullptr;
    ev_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ev_desc.index = 0;
    ev0 = lzt::create_event(event_pool, ev_desc);
    ev_desc.index = 1;
    ev1 = lzt::create_event(event_pool, ev_desc);

    use_compute_engine = std::get<0>(GetParam());
    engines = use_compute_engine ? compute_engines : copy_engines;

    if (engines.size() < 2) {
      GTEST_SKIP() << (use_compute_engine
                           ? "Not enough physical compute engines"
                           : "Not enough physical copy engines");
    }
  }

  void TearDown() override {
    lzt::destroy_event(ev1);
    lzt::destroy_event(ev0);
  }

  bool use_compute_engine;
  std::vector<std::pair<uint32_t, uint32_t>> engines;
  ze_event_handle_t ev0;
  ze_event_handle_t ev1;
  const int n_iters = 10000;
};

LZT_TEST_P(
    zeTestMixedCMDListsInterdependPairSameEngineType,
    GivenRegularAndImmCMDListPairWithEventDependenciesAndRegularCMDListExecutedFirstThenExecuteSuccessfully) {
  const ze_command_queue_mode_t cq_mode_imm = std::get<1>(GetParam());
  const ze_command_list_flags_t cl_flags =
      std::get<2>(GetParam()) & ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
  const auto use_events_sync = std::get<3>(GetParam());

  const uint32_t ordinal = engines[0].first;
  const uint32_t index = engines[0].second;
  auto cq = lzt::create_command_queue(
      context, device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY,
      ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
      ordinal, index);
  auto cl = lzt::create_command_list(context, device, cl_flags, ordinal);

  const uint32_t ordinal_imm = engines[1].first;
  const uint32_t index_imm = engines[1].second;
  auto cl_imm = lzt::create_immediate_command_list(
      context, device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY, cq_mode_imm,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal_imm, index_imm);

  for (int i = 0; i < n_iters; i++) {
    lzt::append_wait_on_events(cl, 1, &ev0);
    lzt::append_reset_event(cl, ev0);
    lzt::append_signal_event(cl, ev1);
  }

  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);

  lzt::signal_event_from_host(ev0);

  for (int i = 0; i < n_iters; i++) {
    lzt::append_wait_on_events(cl_imm, 1, &ev1);
    lzt::append_reset_event(cl_imm, ev1);
    lzt::append_signal_event(cl_imm, ev0);
  }

  lzt::synchronize(cq, UINT64_MAX);
  if (use_events_sync) {
    lzt::event_host_synchronize(ev0, UINT64_MAX);
  } else {
    zeCommandListHostSynchronize(cl_imm, UINT64_MAX);
  }

  lzt::query_event(ev0, ZE_RESULT_SUCCESS);
  lzt::query_event(ev1, ZE_RESULT_NOT_READY);

  lzt::destroy_command_list(cl_imm);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
}

LZT_TEST_P(
    zeTestMixedCMDListsInterdependPairSameEngineType,
    GivenRegularAndImmCMDListPairWithEventDependenciesAndImmCMDListExecutedFirstThenExecuteSuccessfully) {
  const ze_command_queue_mode_t cq_mode = std::get<1>(GetParam());
  const ze_command_list_flags_t cl_flags =
      std::get<2>(GetParam()) & ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
  const auto use_events_sync = std::get<3>(GetParam());

  const uint32_t ordinal = engines[0].first;
  const uint32_t index = engines[0].second;
  auto cq = lzt::create_command_queue(
      context, device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY, cq_mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, index);
  auto cl = lzt::create_command_list(context, device, cl_flags, ordinal);

  const uint32_t ordinal_imm = engines[1].first;
  const uint32_t index_imm = engines[1].second;
  auto cl_imm = lzt::create_immediate_command_list(
      context, device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY,
      ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
      ordinal_imm, index_imm);

  for (int i = 0; i < n_iters; i++) {
    lzt::append_wait_on_events(cl_imm, 1, &ev0);
    lzt::append_reset_event(cl_imm, ev0);
    lzt::append_signal_event(cl_imm, ev1);

    lzt::append_wait_on_events(cl, 1, &ev1);
    lzt::append_reset_event(cl, ev1);
    lzt::append_signal_event(cl, ev0);
  }

  lzt::close_command_list(cl);
  lzt::signal_event_from_host(ev0);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);

  if (!use_events_sync) {
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cl_imm, UINT64_MAX));
  }
  lzt::synchronize(cq, UINT64_MAX);
  lzt::event_host_synchronize(ev0, UINT64_MAX);

  lzt::query_event(ev0, ZE_RESULT_SUCCESS);
  lzt::query_event(ev1, ZE_RESULT_NOT_READY);

  lzt::destroy_command_list(cl_imm);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
}

struct zeTestMixedCMDListsInterdependPairSameEngineTypeTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    ss << "cmd_queue_mode_" << std::get<0>(info.param);
    ss << queue_mode_to_str(
        static_cast<ze_command_queue_mode_t>(std::get<1>(info.param)));
    ss << command_list_flag_to_str(
        static_cast<ze_command_list_flag_t>(std::get<2>(info.param)));
    ss << "_" << event_to_str(std::get<3>(info.param));
    return ss.str();
  }
};

INSTANTIATE_TEST_SUITE_P(
    InterdependCMDListsPairSameEngineTypeParameterization,
    zeTestMixedCMDListsInterdependPairSameEngineType,
    ::testing::Combine(
        ::testing::Values(true, false),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true, false)),
    zeTestMixedCMDListsInterdependPairSameEngineTypeTestNameSuffix());

class zeTestMixedCMDListsInterdependPipelining
    : public zeMixedCMDListsTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flags_t, ze_command_queue_flags_t,
                     ze_command_list_flags_t, bool>> {
protected:
  void SetUp() override {
    vec_hst = static_cast<uint8_t *>(lzt::allocate_host_memory(vec_sz));
    vec_dev = static_cast<uint8_t *>(lzt::allocate_device_memory(vec_sz));
    std::fill_n(vec_hst, vec_sz, 0);
  }

  void TearDown() override {
    lzt::free_memory(vec_dev);
    lzt::free_memory(vec_hst);
  }

  uint8_t *vec_hst;
  uint8_t *vec_dev;
  const int n_iters = 8;
};

LZT_TEST_P(
    zeTestMixedCMDListsInterdependPipelining,
    GivenRegularCMDListOnComputeEngineAndImmCMDListOnCopyEngineThenCorrectResultsAreReturned) {
  // A single copy engine is enough to serve the Copy-Compute-Copy tuple because
  // of our interleaved submission pattern using the immediate cmdlists
  if ((compute_engines.size() == 0) || (copy_engines.size() == 0)) {
    GTEST_SKIP() << "Not enough physical engines";
  }

  const ze_command_queue_flags_t cq_flags_compute = std::get<0>(GetParam());
  const ze_command_queue_flags_t cq_flags_copy = std::get<1>(GetParam());
  ze_command_list_flags_t cl_flags_compute = std::get<2>(GetParam());
  const auto use_events_sync = std::get<3>(GetParam());

  const bool is_explicit_compute =
      cq_flags_compute & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
  if (is_explicit_compute) {
    cl_flags_compute |= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
  }

  const auto ord_compute = compute_engines[0].first;
  const auto idx_compute = compute_engines[0].second;

  auto cq_compute = lzt::create_command_queue(
      context, device, cq_flags_compute, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_compute,
      is_explicit_compute ? idx_compute : 0);
  auto cl_compute =
      lzt::create_command_list(context, device, cl_flags_compute, ord_compute);

  const bool is_explicit_copy =
      cq_flags_copy & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

  const auto ord_copy_h2d_imm = copy_engines[0].first;
  const auto idx_copy_h2d_imm = copy_engines[0].second;
  auto cl_h2d_imm = lzt::create_immediate_command_list(
      context, device, cq_flags_copy, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_copy_h2d_imm,
      is_explicit_copy ? idx_copy_h2d_imm : 0);

  const auto ord_copy_d2h_imm = copy_engines[1 % copy_engines.size()].first;
  const auto idx_copy_d2h_imm = copy_engines[1 % copy_engines.size()].second;
  auto cl_d2h_imm = lzt::create_immediate_command_list(
      context, device, cq_flags_copy, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_copy_d2h_imm,
      is_explicit_copy ? idx_copy_d2h_imm : 0);

  ze_event_desc_t ev_desc = {};
  ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  ev_desc.pNext = nullptr;

  ev_desc.index = 0;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto ev_h2d = lzt::create_event(event_pool, ev_desc);

  ev_desc.index = 1;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto ev_compute = lzt::create_event(event_pool, ev_desc);

  ev_desc.index = 2;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto ev_d2h = lzt::create_event(event_pool, ev_desc);

  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_sz / 8, 1, 1};
  lzt::set_argument_value(kernel, 0, sizeof(void *), &vec_dev);

  for (int i = 0; i < n_iters; i++) {
    // Copy to device and signal kernel launch
    lzt::append_memory_copy(cl_h2d_imm, vec_dev, vec_hst, vec_sz, nullptr, 1,
                            &ev_h2d);
    lzt::append_barrier(cl_h2d_imm);
    lzt::append_reset_event(cl_h2d_imm, ev_h2d);
    lzt::append_signal_event(cl_h2d_imm, ev_compute);

    // Compute and signal d2h copy
    lzt::append_launch_function(cl_compute, kernel, &dispatch, nullptr, 1,
                                &ev_compute);
    lzt::append_barrier(cl_compute);
    lzt::append_reset_event(cl_compute, ev_compute);
    lzt::append_signal_event(cl_compute, ev_d2h);

    // Copy to host and signal the next CCC stage
    lzt::append_memory_copy(cl_d2h_imm, vec_hst, vec_dev, vec_sz, nullptr, 1,
                            &ev_d2h);
    lzt::append_barrier(cl_d2h_imm);
    lzt::append_reset_event(cl_d2h_imm, ev_d2h);
    lzt::append_signal_event(cl_d2h_imm, ev_h2d);
  }

  lzt::close_command_list(cl_compute);
  lzt::execute_command_lists(cq_compute, 1, &cl_compute, nullptr);

  // Kickstart the pipeline
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSignal(ev_h2d));

  // Once the last kernel launch is finished, the results should be back soon
  EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cq_compute, UINT64_MAX));

  if (use_events_sync) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(ev_h2d, UINT64_MAX));
  } else {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cl_d2h_imm, UINT64_MAX));
  }

  lzt::query_event(ev_h2d, ZE_RESULT_SUCCESS);
  lzt::query_event(ev_compute, ZE_RESULT_NOT_READY);
  lzt::query_event(ev_d2h, ZE_RESULT_NOT_READY);

  for (int i = 0; i < vec_sz; i++) {
    if (vec_hst[i] != n_iters) {
      EXPECT_TRUE(false);
      break;
    }
  }

  lzt::destroy_event(ev_d2h);
  lzt::destroy_event(ev_compute);
  lzt::destroy_event(ev_h2d);
  lzt::destroy_command_list(cl_d2h_imm);
  lzt::destroy_command_list(cl_compute);
  lzt::destroy_command_list(cl_h2d_imm);
  lzt::destroy_command_queue(cq_compute);
}

LZT_TEST_P(
    zeTestMixedCMDListsInterdependPipelining,
    GivenRegularCMDListOnCopyEngineAndImmCMDListOnComputeEngineThenCorrectResultsAreReturned) {
  // We need two physical copy engines because the H2D and D2H commands are
  // submitted in two different batches, a single engine would cause deadlocks
  if ((compute_engines.size() == 0) || (copy_engines.size() < 2)) {
    GTEST_SKIP() << "Not enough physical engines";
  }

  const ze_command_queue_flags_t cq_flags_compute = std::get<0>(GetParam());
  const ze_command_queue_flags_t cq_flags_copy = std::get<1>(GetParam());
  ze_command_list_flags_t cl_flags_copy = std::get<2>(GetParam());
  const auto use_events_sync = std::get<3>(GetParam());

  const bool is_explicit_compute =
      cq_flags_compute & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

  const auto ord_compute_imm = compute_engines[0].first;
  const auto idx_compute_imm = compute_engines[0].second;

  auto cl_compute_imm = lzt::create_immediate_command_list(
      context, device, cq_flags_compute, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_compute_imm,
      is_explicit_compute ? idx_compute_imm : 0);

  const bool is_explicit_copy =
      cq_flags_copy & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
  if (is_explicit_copy) {
    cl_flags_copy |= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
  }

  const auto ord_copy_h2d = copy_engines[0].first;
  const auto idx_copy_h2d = copy_engines[0].second;
  auto cq_h2d = lzt::create_command_queue(
      context, device, cq_flags_copy, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_copy_h2d,
      is_explicit_copy ? idx_copy_h2d : 0);
  auto cl_h2d =
      lzt::create_command_list(context, device, cl_flags_copy, ord_copy_h2d);

  const auto ord_copy_d2h = copy_engines[1].first;
  const auto idx_copy_d2h = copy_engines[1].second;
  auto cq_d2h = lzt::create_command_queue(
      context, device, cq_flags_copy, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_copy_d2h,
      is_explicit_copy ? idx_copy_d2h : 0);
  auto cl_d2h =
      lzt::create_command_list(context, device, cl_flags_copy, ord_copy_d2h);

  ze_event_desc_t ev_desc = {};
  ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  ev_desc.pNext = nullptr;

  ev_desc.index = 0;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto ev_h2d = lzt::create_event(event_pool, ev_desc);

  ev_desc.index = 1;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto ev_compute = lzt::create_event(event_pool, ev_desc);

  ev_desc.index = 2;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto ev_d2h = lzt::create_event(event_pool, ev_desc);

  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_sz / 8, 1, 1};
  lzt::set_argument_value(kernel, 0, sizeof(void *), &vec_dev);

  for (int i = 0; i < n_iters; i++) {
    // Copy to device and signal kernel launch
    lzt::append_memory_copy(cl_h2d, vec_dev, vec_hst, vec_sz, nullptr, 1,
                            &ev_h2d);
    lzt::append_barrier(cl_h2d);
    lzt::append_reset_event(cl_h2d, ev_h2d);
    lzt::append_signal_event(cl_h2d, ev_compute);

    // Compute and signal d2h copy
    lzt::append_launch_function(cl_compute_imm, kernel, &dispatch, nullptr, 1,
                                &ev_compute);
    lzt::append_barrier(cl_compute_imm);
    lzt::append_reset_event(cl_compute_imm, ev_compute);
    lzt::append_signal_event(cl_compute_imm, ev_d2h);

    // Copy to host and signal the next CCC stage
    lzt::append_memory_copy(cl_d2h, vec_hst, vec_dev, vec_sz, nullptr, 1,
                            &ev_d2h);
    lzt::append_barrier(cl_d2h);
    lzt::append_reset_event(cl_d2h, ev_d2h);
    lzt::append_signal_event(cl_d2h, ev_h2d);
  }

  // Submit in reverse order to verify that the copy cmdlists are independent
  lzt::close_command_list(cl_d2h);
  lzt::close_command_list(cl_h2d);
  lzt::execute_command_lists(cq_d2h, 1, &cl_d2h, nullptr);
  lzt::execute_command_lists(cq_h2d, 1, &cl_h2d, nullptr);

  // Kickstart the pipeline
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSignal(ev_h2d));

  // The results should be back once the last copy queue is done
  EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cq_d2h, UINT64_MAX));
  if (!use_events_sync) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cl_compute_imm, UINT64_MAX));
  }

  lzt::query_event(ev_h2d, ZE_RESULT_SUCCESS);
  lzt::query_event(ev_compute, ZE_RESULT_NOT_READY);
  lzt::query_event(ev_d2h, ZE_RESULT_NOT_READY);

  for (int i = 0; i < vec_sz; i++) {
    if (vec_hst[i] != n_iters) {
      EXPECT_TRUE(false);
      break;
    }
  }

  lzt::destroy_event(ev_d2h);
  lzt::destroy_event(ev_compute);
  lzt::destroy_event(ev_h2d);
  lzt::destroy_command_list(cl_d2h);
  lzt::destroy_command_list(cl_compute_imm);
  lzt::destroy_command_list(cl_h2d);
  lzt::destroy_command_queue(cq_d2h);
  lzt::destroy_command_queue(cq_h2d);
}

LZT_TEST_P(
    zeTestMixedCMDListsInterdependPipelining,
    GivenRegularAndImmCMDListsOnComputeEngineThenCorrectResultsAreReturned) {
  // Alternating cmdlist types, so we need at least two physical compute engines
  // to avoid deadlocks
  if (compute_engines.size() < 2) {
    GTEST_SKIP() << "Not enough physical compute engines";
  }

  ze_command_queue_flags_t cq_flags = std::get<0>(GetParam());
  ze_command_queue_flags_t cq_flags_imm = std::get<1>(GetParam());
  ze_command_list_flags_t cl_flags = std::get<2>(GetParam());
  const auto use_events_sync = std::get<3>(GetParam());

  const auto ord_h2d_imm = compute_engines[0].first;
  const auto idx_h2d_imm = compute_engines[0].second;
  const auto ord_compute = compute_engines[1].first;
  const auto idx_compute = compute_engines[1].second;
  const auto ord_d2h_imm = compute_engines[2 % compute_engines.size()].first;
  const auto idx_d2h_imm = compute_engines[2 % compute_engines.size()].second;

  // Force explicit mode to avoid deadlocks if the compute queue could affect
  // the copy queues
  if ((ord_compute == ord_h2d_imm) || (ord_compute == ord_d2h_imm)) {
    cq_flags |= ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY;
    cq_flags_imm |= ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY;
  }

  const bool is_explicit =
      cq_flags & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
  if (is_explicit) {
    cl_flags |= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
  }

  auto cq_compute = lzt::create_command_queue(
      context, device, cq_flags, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_compute,
      is_explicit ? idx_compute : 0);
  auto cl_compute =
      lzt::create_command_list(context, device, cl_flags, ord_compute);

  const bool is_explicit_imm =
      cq_flags_imm & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

  auto cl_h2d_imm = lzt::create_immediate_command_list(
      context, device, cq_flags_imm, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_h2d_imm,
      is_explicit_imm ? idx_h2d_imm : 0);

  auto cl_d2h_imm = lzt::create_immediate_command_list(
      context, device, cq_flags_imm, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_d2h_imm,
      is_explicit_imm ? idx_d2h_imm : 0);

  ze_event_desc_t ev_desc = {};
  ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  ev_desc.pNext = nullptr;

  ev_desc.index = 0;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto ev_h2d = lzt::create_event(event_pool, ev_desc);

  ev_desc.index = 1;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto ev_compute = lzt::create_event(event_pool, ev_desc);

  ev_desc.index = 2;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto ev_d2h = lzt::create_event(event_pool, ev_desc);

  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_sz / 8, 1, 1};
  lzt::set_argument_value(kernel, 0, sizeof(void *), &vec_dev);

  for (int i = 0; i < n_iters; i++) {
    // Copy to device and signal kernel launch
    lzt::append_memory_copy(cl_h2d_imm, vec_dev, vec_hst, vec_sz, nullptr, 1,
                            &ev_h2d);
    lzt::append_barrier(cl_h2d_imm);
    lzt::append_reset_event(cl_h2d_imm, ev_h2d);
    lzt::append_signal_event(cl_h2d_imm, ev_compute);

    // Compute and signal d2h copy
    lzt::append_launch_function(cl_compute, kernel, &dispatch, nullptr, 1,
                                &ev_compute);
    lzt::append_barrier(cl_compute);
    lzt::append_reset_event(cl_compute, ev_compute);
    lzt::append_signal_event(cl_compute, ev_d2h);

    // Copy to host and signal the next CCC stage
    lzt::append_memory_copy(cl_d2h_imm, vec_hst, vec_dev, vec_sz, nullptr, 1,
                            &ev_d2h);
    lzt::append_barrier(cl_d2h_imm);
    lzt::append_reset_event(cl_d2h_imm, ev_d2h);
    lzt::append_signal_event(cl_d2h_imm, ev_h2d);
  }

  lzt::close_command_list(cl_compute);
  lzt::execute_command_lists(cq_compute, 1, &cl_compute, nullptr);

  // Kickstart the pipeline
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSignal(ev_h2d));

  // Once the last kernel launch is finished, the results should be back soon
  EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cq_compute, UINT64_MAX));
  if (use_events_sync) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(ev_h2d, UINT64_MAX));
  } else {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cl_d2h_imm, UINT64_MAX));
  }

  lzt::query_event(ev_h2d, ZE_RESULT_SUCCESS);
  lzt::query_event(ev_compute, ZE_RESULT_NOT_READY);
  lzt::query_event(ev_d2h, ZE_RESULT_NOT_READY);

  for (int i = 0; i < vec_sz; i++) {
    if (vec_hst[i] != n_iters) {
      EXPECT_TRUE(false);
      break;
    }
  }

  lzt::destroy_event(ev_d2h);
  lzt::destroy_event(ev_compute);
  lzt::destroy_event(ev_h2d);
  lzt::destroy_command_list(cl_d2h_imm);
  lzt::destroy_command_list(cl_compute);
  lzt::destroy_command_list(cl_h2d_imm);
  lzt::destroy_command_queue(cq_compute);
}

LZT_TEST_P(
    zeTestMixedCMDListsInterdependPipelining,
    GivenRegularAndImmCMDListsOnCopyEngineThenCorrectResultsAreReturned) {
  // Alternating cmdlist types for fill-copy stages, so we need at least two
  // physical copy engines to avoid deadlocks
  if (copy_engines.size() < 2) {
    GTEST_SKIP() << "Not enough physical copy engines";
  }

  ze_command_queue_flags_t cq_flags = std::get<0>(GetParam());
  ze_command_queue_flags_t cq_flags_imm = std::get<1>(GetParam());
  ze_command_list_flags_t cl_flags = std::get<2>(GetParam());
  const auto use_events_sync = std::get<3>(GetParam());

  const auto ord_fill = copy_engines[0].first;
  const auto idx_fill = copy_engines[0].second;
  const auto ord_copy = copy_engines[1].first;
  const auto idx_copy = copy_engines[1].second;

  // Force explicit mode to avoid deadlocks if the fill queue could affect the
  // copy queue
  if (ord_fill == ord_copy) {
    cq_flags |= ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY;
    cq_flags_imm |= ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY;
  }

  const bool is_explicit =
      cq_flags & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
  if (is_explicit) {
    cl_flags |= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
  }

  auto cq_fill = lzt::create_command_queue(
      context, device, cq_flags, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_fill, is_explicit ? idx_fill : 0);
  auto cl_fill = lzt::create_command_list(context, device, cl_flags, ord_fill);

  const bool is_explicit_imm =
      cq_flags_imm & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;

  auto cl_copy = lzt::create_immediate_command_list(
      context, device, cq_flags_imm, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ord_copy,
      is_explicit_imm ? idx_copy : 0);

  ze_event_desc_t ev_desc = {};
  ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  ev_desc.pNext = nullptr;

  ev_desc.index = 0;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  auto ev_fill = lzt::create_event(event_pool, ev_desc);

  ev_desc.index = 1;
  ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto ev_copy = lzt::create_event(event_pool, ev_desc);

  const int blk_sz = vec_sz / n_iters;

  for (int i = 0; i < n_iters; i++) {
    // Fill the device buffer block
    lzt::append_memory_fill(cl_fill, vec_dev + i * blk_sz, &n_iters, 1, blk_sz,
                            nullptr, 1, &ev_fill);
    lzt::append_barrier(cl_fill);
    lzt::append_reset_event(cl_fill, ev_fill);
    lzt::append_signal_event(cl_fill, ev_copy);

    // Copy the filled block back to the host
    lzt::append_memory_copy(cl_copy, vec_hst + i * blk_sz, vec_dev + i * blk_sz,
                            blk_sz, nullptr, 1, &ev_copy);

    lzt::append_barrier(cl_copy);
    lzt::append_reset_event(cl_copy, ev_copy);
    lzt::append_signal_event(cl_copy, ev_fill);
  }

  lzt::close_command_list(cl_fill);
  lzt::execute_command_lists(cq_fill, 1, &cl_fill, nullptr);

  // Kickstart the pipeline
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSignal(ev_fill));

  // Once the last copy is finished, the results should be back soon
  EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cq_fill, UINT64_MAX));

  if (use_events_sync) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(ev_fill, UINT64_MAX));
  } else {
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cl_copy, UINT64_MAX));
  }

  lzt::query_event(ev_fill, ZE_RESULT_SUCCESS);
  lzt::query_event(ev_copy, ZE_RESULT_NOT_READY);

  for (int i = 0; i < blk_sz * n_iters; i++) {
    if (vec_hst[i] != n_iters) {
      EXPECT_TRUE(false);
      break;
    }
  }

  lzt::destroy_event(ev_copy);
  lzt::destroy_event(ev_fill);
  lzt::destroy_command_list(cl_copy);
  lzt::destroy_command_list(cl_fill);
  lzt::destroy_command_queue(cq_fill);
}

struct zeTestMixedCMDListsInterdependPipeliningTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    ss << "compute"
       << command_queue_flag_to_str(
              static_cast<ze_command_queue_flag_t>(std::get<0>(info.param)));
    ss << "_copy"
       << command_queue_flag_to_str(
              static_cast<ze_command_queue_flag_t>(std::get<1>(info.param)));
    ss << command_list_flag_to_str(
        static_cast<ze_command_list_flag_t>(std::get<2>(info.param)));
    ss << "_" << event_to_str(std::get<3>(info.param));
    return ss.str();
  }
};

INSTANTIATE_TEST_SUITE_P(
    InterdependCMDListsPipeliningParameterization,
    zeTestMixedCMDListsInterdependPipelining,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT),
        ::testing::Values(true, false)),
    zeTestMixedCMDListsInterdependPipeliningTestNameSuffix());

class zeMixedCommandListEventCounterTests : public lzt::zeEventPoolTests {
protected:
  void SetUp() override {
    ze_event_pool_desc_t eventPoolDesc = {};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 10;

    ze_event_pool_counter_based_exp_desc_t counterBasedExtension = {
        ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC};
    counterBasedExtension.flags =
        ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_IMMEDIATE |
        ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_NON_IMMEDIATE;
    eventPoolDesc.pNext = &counterBasedExtension;
    ep.InitEventPool(eventPoolDesc);
    ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, 0);

    cmdlist.push_back(
        lzt::create_command_list(lzt::zeDevice::get_instance()->get_device(),
                                 ZE_COMMAND_LIST_FLAG_IN_ORDER));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(
        lzt::create_command_list(lzt::zeDevice::get_instance()->get_device(),
                                 ZE_COMMAND_LIST_FLAG_IN_ORDER));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(
        lzt::create_command_list(lzt::zeDevice::get_instance()->get_device(),
                                 ZE_COMMAND_LIST_FLAG_IN_ORDER));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
  }

  void TearDown() override {
    ep.destroy_event(event0);
    for (auto cl : cmdlist) {
      lzt::destroy_command_list(cl);
    }
    for (auto cq : cmdqueue) {
      lzt::destroy_command_queue(cq);
    }
  }
  std::vector<ze_command_list_handle_t> cmdlist;
  std::vector<ze_command_queue_handle_t> cmdqueue;
  ze_event_handle_t event0 = nullptr;
};

static void
RunAppendLaunchKernelEvent(std::vector<ze_command_list_handle_t> cmdlist,
                           std::vector<ze_command_queue_handle_t> cmdqueue,
                           ze_event_handle_t event, int num_cmdlist,
                           bool is_shared_system) {
  const size_t size = 16;
  const int addval = 10;
  const int num_iterations = 100;
  int addval2 = 0;
  const uint64_t timeout = UINT64_MAX - 1;

  void *buffer = lzt::allocate_shared_memory_with_allocator_selector(
      num_cmdlist * size * sizeof(int), is_shared_system);

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int totalVal[10];

  memset(buffer, 0x0, num_cmdlist * size * sizeof(int));

  for (int n = 0; n < num_cmdlist; n++) {
    int *p_dev = static_cast<int *>(buffer);
    p_dev += (n * size);
    lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
    lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
    ze_group_count_t tg;
    tg.groupCountX = static_cast<uint32_t>(size);
    tg.groupCountY = 1;
    tg.groupCountZ = 1;

    lzt::append_launch_function(cmdlist[n], kernel, &tg, nullptr, 0, nullptr);

    totalVal[n] = 0;
    for (int i = 0; i < (num_iterations - 1); i++) {
      addval2 = lzt::generate_value<int>() & 0xFFFF;
      totalVal[n] += addval2;
      lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

      lzt::append_launch_function(cmdlist[n], kernel, &tg, nullptr, 0, nullptr);
    }
    addval2 = lzt::generate_value<int>() & 0xFFFF;
    ;
    totalVal[n] += addval2;
    lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

    if (n == 0) {
      lzt::append_launch_function(cmdlist[n], kernel, &tg, event, 0, nullptr);
    } else {
      lzt::append_launch_function(cmdlist[n], kernel, &tg, event, 1, &event);
    }

    if ((n % 2) == 0) {
      lzt::close_command_list(cmdlist[n]);
      lzt::execute_command_lists(cmdqueue[n / 2], 1, &cmdlist[n], nullptr);
      lzt::synchronize(cmdqueue[n / 2], UINT64_MAX);
    }
  }

  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, timeout));

  for (int n = 0; n < num_cmdlist; n++) {
    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<int *>(buffer)[(n * size) + i],
                (addval + totalVal[n]));
    }

    if ((n % 2) == 0) {
      lzt::reset_command_list(cmdlist[n]);
    }
  }
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);

  lzt::free_memory_with_allocator_selector(buffer, is_shared_system);
}

LZT_TEST_F(
    zeMixedCommandListEventCounterTests,
    GivenInOrderMixedCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecution) {
  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }
  for (int i = 1; i <= cmdlist.size(); i++) {
    LOG_INFO << "Testing " << i << " command list(s)";
    RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event0, i, false);
  }
}

LZT_TEST_F(
    zeMixedCommandListEventCounterTests,
    GivenInOrderMixedCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecutionWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }
  for (int i = 1; i <= cmdlist.size(); i++) {
    LOG_INFO << "Testing " << i << " command list(s)";
    RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event0, i, true);
  }
}

class zeOutOfOrderCommandListEventCounterTests : public lzt::zeEventPoolTests {
protected:
  void SetUp() override {
    /* must have two event pools:  first is for counter based events and second
     * is not */
    ze_event_pool_desc_t eventPoolDesc = {};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 10;

    ze_event_pool_counter_based_exp_desc_t counterBasedExtension = {
        ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC};
    counterBasedExtension.flags =
        ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_IMMEDIATE |
        ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_NON_IMMEDIATE;
    eventPoolDesc.pNext = &counterBasedExtension;
    ep.InitEventPool(eventPoolDesc);
    ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, 0);

    ze_event_pool_desc_t eventPoolDesc1 = {};
    eventPoolDesc1.count = 1;
    ze_event_desc_t eventDesc1 = {};
    eventDesc1.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    eventDesc1.index = 0;
    eventDesc1.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc1.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ep1.InitEventPool(eventPoolDesc1);
    ep1.create_event(event1, eventDesc1);

    cmdlist_reg.push_back(
        lzt::create_command_list(lzt::zeDevice::get_instance()->get_device(),
                                 ZE_COMMAND_LIST_FLAG_IN_ORDER));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist_reg.push_back(lzt::create_command_list(
        lzt::zeDevice::get_instance()->get_device(), 0));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(), 0,
        ZE_COMMAND_QUEUE_MODE_DEFAULT, ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist_imm.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist_imm.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(), 0,
        ZE_COMMAND_QUEUE_MODE_DEFAULT, ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
  }

  void TearDown() override {
    ep.destroy_event(event0);
    ep1.destroy_event(event1);
    for (auto cl : cmdlist_reg) {
      lzt::destroy_command_list(cl);
    }
    for (auto cl : cmdlist_imm) {
      lzt::destroy_command_list(cl);
    }
    for (auto cq : cmdqueue) {
      lzt::destroy_command_queue(cq);
    }
  }
  std::vector<ze_command_list_handle_t> cmdlist_reg;
  std::vector<ze_command_list_handle_t> cmdlist_imm;
  std::vector<ze_command_queue_handle_t> cmdqueue;
  lzt::zeEventPool ep1;
  ze_event_handle_t event0 = nullptr;
  ze_event_handle_t event1 = nullptr;
};

static void RunOutOfOrderAppendLaunchKernelEvent(
    std::vector<ze_command_list_handle_t> cmdlist,
    std::vector<ze_command_queue_handle_t> cmdqueue,
    ze_event_handle_t counter_event, ze_event_handle_t event, bool is_immediate,
    bool is_shared_system) {
  int num_cmdlist = 2;
  const size_t size = 16;
  const int addval = 10;
  const int num_iterations = 100;
  int addval2 = 0;
  const uint64_t timeout = UINT64_MAX - 1;

  void *buffer = lzt::allocate_shared_memory_with_allocator_selector(
      num_cmdlist * size * sizeof(int), is_shared_system);

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int totalVal[10];

  memset(buffer, 0x0, num_cmdlist * size * sizeof(int));

  int *p_dev = static_cast<int *>(buffer);

  lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
  ze_group_count_t tg;
  tg.groupCountX = static_cast<uint32_t>(size);
  tg.groupCountY = 1;
  tg.groupCountZ = 1;

  /* First command list is In-order and signals counter event */
  lzt::append_launch_function(cmdlist[0], kernel, &tg, nullptr, 0, nullptr);

  totalVal[0] = 0;
  for (int i = 0; i < (num_iterations - 1); i++) {
    addval2 = lzt::generate_value<int>() & 0xFFFF;
    totalVal[0] += addval2;
    lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

    lzt::append_launch_function(cmdlist[0], kernel, &tg, nullptr, 0, nullptr);
  }
  addval2 = lzt::generate_value<int>() & 0xFFFF;
  ;
  totalVal[0] += addval2;
  lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

  lzt::append_launch_function(cmdlist[0], kernel, &tg, counter_event, 0,
                              nullptr);

  if (!is_immediate) {
    lzt::close_command_list(cmdlist[0]);
    lzt::execute_command_lists(cmdqueue[0], 1, &cmdlist[0], nullptr);
    lzt::synchronize(cmdqueue[0], UINT64_MAX);
  }

  /* second command list is Out-of-order and may only wait on counter event */
  p_dev += size;
  lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);

  lzt::append_launch_function(cmdlist[1], kernel, &tg, nullptr, 0, nullptr);
  lzt::append_barrier(cmdlist[1]);

  totalVal[1] = 0;
  for (int i = 0; i < (num_iterations - 1); i++) {
    addval2 = lzt::generate_value<int>() & 0xFFFF;
    totalVal[1] += addval2;
    lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

    lzt::append_launch_function(cmdlist[1], kernel, &tg, nullptr, 0, nullptr);
    lzt::append_barrier(cmdlist[1]);
  }
  addval2 = lzt::generate_value<int>() & 0xFFFF;
  totalVal[1] += addval2;
  lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);
  lzt::append_launch_function(cmdlist[1], kernel, &tg, event, 1,
                              &counter_event);
  lzt::append_barrier(cmdlist[1]);
  if (!is_immediate) {
    lzt::close_command_list(cmdlist[1]);
    lzt::execute_command_lists(cmdqueue[1], 1, &cmdlist[1], nullptr);
    lzt::synchronize(cmdqueue[1], UINT64_MAX);
  }
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, timeout));

  for (int n = 0; n < num_cmdlist; n++) {
    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<int *>(buffer)[(n * size) + i],
                (addval + totalVal[n]));
    }
    if (!is_immediate) {
      lzt::reset_command_list(cmdlist[n]);
    }
  }
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);

  lzt::free_memory_with_allocator_selector(buffer, is_shared_system);
}

LZT_TEST_F(
    zeOutOfOrderCommandListEventCounterTests,
    GivenOutOfOrderRegularCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyWaitForEvent) {
  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }
  RunOutOfOrderAppendLaunchKernelEvent(cmdlist_reg, cmdqueue, event0, event1,
                                       false, false);
}

LZT_TEST_F(
    zeOutOfOrderCommandListEventCounterTests,
    GivenOutOfOrderRegularCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyWaitForEventWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();

  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }

  RunOutOfOrderAppendLaunchKernelEvent(cmdlist_reg, cmdqueue, event0, event1,
                                       false, true);
}

LZT_TEST_F(
    zeOutOfOrderCommandListEventCounterTests,
    GivenOutOfOrderImmediateCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyWaitForEvent) {

  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }

  RunOutOfOrderAppendLaunchKernelEvent(cmdlist_imm, cmdqueue, event0, event1,
                                       true, false);
}

LZT_TEST_F(
    zeOutOfOrderCommandListEventCounterTests,
    GivenOutOfOrderImmediateCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyWaitForEventWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();

  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }

  RunOutOfOrderAppendLaunchKernelEvent(cmdlist_imm, cmdqueue, event0, event1,
                                       true, true);
}

} // namespace
