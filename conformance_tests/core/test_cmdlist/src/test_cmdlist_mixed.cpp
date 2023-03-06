/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <vector>
#include <utility>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

static const size_t vec_len = 1UL << 16;

class zeMixedCMDListsTests : public ::testing::Test {
protected:
  static ze_context_handle_t context;
  static ze_device_handle_t device;
  static ze_module_handle_t module;
  static ze_kernel_handle_t kernel;
  static ze_event_pool_handle_t event_pool;
  // Ordinal and number of physical engines
  static std::vector<std::tuple<uint32_t, uint32_t>> compute_engines;
  static std::vector<std::tuple<uint32_t, uint32_t>> copy_engines;
  static const uint32_t event_pool_sz = 512;

  static void SetUpTestSuite() {
    context = lzt::get_default_context();
    device = lzt::get_default_device(lzt::get_default_driver());

    const auto cmd_queue_group_properties =
        lzt::get_command_queue_group_properties(device);
    EXPECT_NE(cmd_queue_group_properties.size(), 0);

    for (uint32_t i = 0; i < cmd_queue_group_properties.size(); i++) {
      if (cmd_queue_group_properties[i].flags &
          ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
        compute_engines.emplace_back(i,
                                     cmd_queue_group_properties[i].numQueues);
      } else if (cmd_queue_group_properties[i].flags &
                 ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) {
        copy_engines.emplace_back(i, cmd_queue_group_properties[i].numQueues);
      }
    }

    module = lzt::create_module(device, "cmdlist_verify.spv");
    kernel = lzt::create_function(module, "add_one");
    event_pool = lzt::create_event_pool(context, event_pool_sz,
                                        ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  }

  static void TearDownTestSuite() {
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
std::vector<std::tuple<uint32_t, uint32_t>>
    zeMixedCMDListsTests::compute_engines{};
std::vector<std::tuple<uint32_t, uint32_t>>
    zeMixedCMDListsTests::copy_engines{};

class zeTestCMDListRegularOnComputeEngineImmediateOnCopyEngine
    : public zeMixedCMDListsTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flags_t, ze_command_queue_flags_t,
                     ze_command_queue_mode_t, ze_command_list_flags_t>> {};

TEST_P(
    zeTestCMDListRegularOnComputeEngineImmediateOnCopyEngine,
    GivenImmediateCMDListsOnCopyEngineWhenCMDListsRunningOnComputeEngineThenCorrectResultsAreReturned) {
  std::vector<void *> host_vecs_compute_engine;
  std::vector<void *> host_vecs_copy_engine;
  std::vector<void *> device_vecs_compute_engine;
  std::vector<void *> device_vecs_copy_engine;
  std::vector<ze_command_queue_handle_t> cmd_queues_compute_engine;
  std::vector<ze_command_list_handle_t> cmd_lsts_compute_engine;
  std::vector<ze_command_list_handle_t> cmd_lsts_copy_engine_imm;
  std::vector<ze_event_handle_t> events_fill;
  std::vector<ze_event_handle_t> events_copy;
  std::vector<int> patterns_compute_engine;
  std::vector<int> patterns_copy_engine;

  const ze_command_queue_flags_t cmd_queue_flags_compute_engine =
      std::get<0>(GetParam());
  const ze_command_queue_flags_t cmd_queue_flags_copy_engine =
      std::get<1>(GetParam());
  const ze_command_queue_mode_t cmd_queue_mode_copy_engine =
      std::get<2>(GetParam());
  ze_command_list_flags_t cmd_lst_flags_compute_engine =
      std::get<3>(GetParam());

  // Create compute engine CMD queues, lists and host/device buffers
  for (auto &engine : compute_engines) {
    const uint32_t ordinal = std::get<0>(engine);
    const uint32_t num_queues = std::get<1>(engine);
    const bool is_explicit =
        cmd_queue_flags_compute_engine & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY
            ? true
            : false;

    for (uint32_t i = 0; i < num_queues; i++) {
      // Must be ASYNC to overlap with the execution on the copy engine
      cmd_queues_compute_engine.push_back(lzt::create_command_queue(
          context, device, cmd_queue_flags_compute_engine,
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          ordinal, is_explicit ? i : 0));

      if (is_explicit) {
        cmd_lst_flags_compute_engine &= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
      }
      cmd_lsts_compute_engine.push_back(lzt::create_command_list(
          context, device, cmd_lst_flags_compute_engine, ordinal));

      host_vecs_compute_engine.push_back(
          lzt::allocate_host_memory(vec_len * sizeof(int), sizeof(int)));
      device_vecs_compute_engine.push_back(
          lzt::allocate_device_memory(vec_len * sizeof(int), sizeof(int)));

      patterns_compute_engine.push_back(cmd_lsts_compute_engine.size() - 1);
    }
  }

  // Create copy engine immediate CMD lists, events and host/device buffers
  for (auto &engine : copy_engines) {
    const uint32_t ordinal = std::get<0>(engine);
    const uint32_t num_queues = std::get<1>(engine);
    const bool is_explicit =
        cmd_queue_flags_copy_engine & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY
            ? true
            : false;

    for (uint32_t i = 0; i < num_queues; i++) {
      cmd_lsts_copy_engine_imm.push_back(lzt::create_immediate_command_list(
          context, device, cmd_queue_flags_copy_engine,
          cmd_queue_mode_copy_engine, ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal,
          is_explicit ? i : 0));

      ze_event_desc_t ev_desc = {};
      ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
      ev_desc.pNext = nullptr;
      ev_desc.index = cmd_lsts_copy_engine_imm.size() - 1;
      ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
      ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
      events_fill.push_back(lzt::create_event(event_pool, ev_desc));

      ev_desc.index = cmd_lsts_copy_engine_imm.size() - 1 + event_pool_sz / 2;
      ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
      events_copy.push_back(lzt::create_event(event_pool, ev_desc));

      host_vecs_copy_engine.push_back(
          lzt::allocate_host_memory(vec_len * sizeof(int), sizeof(int)));
      device_vecs_copy_engine.push_back(
          lzt::allocate_device_memory(vec_len * sizeof(int), sizeof(int)));

      patterns_copy_engine.push_back(42 + cmd_lsts_copy_engine_imm.size() - 1);
    }
  }

  // Fill compute engine CMD lists with tasks
  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_len / 8, 1, 1};

  for (int i = 0; i < cmd_lsts_compute_engine.size(); i++) {
    const auto cl = cmd_lsts_compute_engine[i];
    void *h_vec = host_vecs_compute_engine[i];
    void *d_vec = device_vecs_compute_engine[i];
    lzt::append_memory_fill(cl, d_vec, &patterns_compute_engine[i], sizeof(int),
                            vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cl);
    lzt::set_argument_value(kernel, 0, sizeof(void *), &d_vec);
    lzt::append_launch_function(cl, kernel, &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, h_vec, d_vec, vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, d_vec, h_vec, vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cl);
    lzt::append_launch_function(cl, kernel, &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, h_vec, d_vec, vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cl);
    lzt::close_command_list(cl);
  }

  // Submit compute engine CMD lists, ASYNC execution
  for (int i = 0; i < cmd_lsts_compute_engine.size(); i++) {
    lzt::execute_command_lists(cmd_queues_compute_engine[i], 1,
                               &cmd_lsts_compute_engine[i], nullptr);
  }

  // Submit tasks to copy engine immediate CMD lists
  const int log_blk_len = 8;
  const int blk_sz = (1u << log_blk_len) * sizeof(int);
  for (int i = 0; i < (vec_len >> log_blk_len); i++) {
    for (int j = 0; j < cmd_lsts_copy_engine_imm.size(); j++) {
      const auto cl = cmd_lsts_copy_engine_imm[j];
      const auto device_ptr =
          reinterpret_cast<uint8_t *>(device_vecs_copy_engine[j]) + i * blk_sz;
      const auto host_ptr =
          reinterpret_cast<uint8_t *>(host_vecs_copy_engine[j]) + i * blk_sz;
      lzt::append_memory_fill(cl, device_ptr, &patterns_copy_engine[j],
                              sizeof(int), blk_sz, events_fill[j]);
      lzt::append_memory_copy(cl, host_ptr, device_ptr, blk_sz, events_copy[j],
                              1, &events_fill[j]);
    }
    for (int j = 0; j < cmd_lsts_copy_engine_imm.size(); j++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeEventHostSynchronize(events_copy[j], UINT64_MAX));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events_copy[j]));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events_fill[j]));
    }
  }

  // Sync with compute engine queues
  for (int i = 0; i < cmd_lsts_compute_engine.size(); i++) {
    lzt::synchronize(cmd_queues_compute_engine[i], UINT64_MAX);
  }

  // Verify compute engine results
  for (int i = 0; i < cmd_lsts_compute_engine.size(); i++) {
    const int *vec = reinterpret_cast<int *>(host_vecs_compute_engine[i]);
    for (size_t j = 0; j < vec_len; j++) {
      if (vec[j] != i + 2) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Verify copy engine results
  for (int i = 0; i < cmd_lsts_copy_engine_imm.size(); i++) {
    const int *vec = reinterpret_cast<int *>(host_vecs_copy_engine[i]);
    for (size_t j = 0; j < vec_len; j++) {
      if (vec[j] != i + 42) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Cleanup
  for (int i = 0; i < cmd_lsts_compute_engine.size(); i++) {
    lzt::free_memory(host_vecs_compute_engine[i]);
    lzt::free_memory(device_vecs_compute_engine[i]);
    lzt::destroy_command_list(cmd_lsts_compute_engine[i]);
    lzt::destroy_command_queue(cmd_queues_compute_engine[i]);
  }

  for (int i = 0; i < cmd_lsts_copy_engine_imm.size(); i++) {
    lzt::free_memory(host_vecs_copy_engine[i]);
    lzt::free_memory(device_vecs_copy_engine[i]);
    lzt::destroy_command_list(cmd_lsts_copy_engine_imm[i]);
    lzt::destroy_event(events_fill[i]);
    lzt::destroy_event(events_copy[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(
    RegularOnComputeEngineImmediateOnCopyEngineParameterization,
    zeTestCMDListRegularOnComputeEngineImmediateOnCopyEngine,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT)));

class zeTestCMDListImmediateOnComputeEngineRegularOnCopyEngine
    : public zeMixedCMDListsTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flags_t, ze_command_queue_flags_t,
                     ze_command_queue_mode_t, ze_command_list_flags_t>> {};

TEST_P(
    zeTestCMDListImmediateOnComputeEngineRegularOnCopyEngine,
    GivenImmediateCMDListsOnComputeEngineWhenRegularCMDListsRunningOnCopyEngineThenCorrectResultsAreReturned) {
  std::vector<void *> host_vecs_compute_engine;
  std::vector<void *> host_vecs_copy_engine;
  std::vector<void *> device_vecs_compute_engine;
  std::vector<void *> device_vecs_copy_engine;
  std::vector<ze_command_queue_handle_t> cmd_queues_copy_engine;
  std::vector<ze_command_list_handle_t> cmd_lsts_copy_engine;
  std::vector<ze_command_list_handle_t> cmd_lsts_compute_engine_imm;
  std::vector<ze_event_handle_t> events;
  std::vector<int> patterns_compute_engine;
  std::vector<int> patterns_copy_engine;

  const ze_command_queue_flags_t cmd_queue_flags_compute_engine =
      std::get<0>(GetParam());
  const ze_command_queue_flags_t cmd_queue_flags_copy_engine =
      std::get<1>(GetParam());
  const ze_command_queue_mode_t cmd_queue_mode_compute_engine =
      std::get<2>(GetParam());
  ze_command_list_flags_t cmd_lst_flags_copy_engine = std::get<3>(GetParam());

  // Create compute engine immediate CMD lists, events and host/device buffers
  for (auto &engine : compute_engines) {
    const uint32_t ordinal = std::get<0>(engine);
    const uint32_t num_queues = std::get<1>(engine);
    const bool is_explicit =
        cmd_queue_flags_compute_engine & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY
            ? true
            : false;

    for (uint32_t i = 0; i < num_queues; i++) {
      cmd_lsts_compute_engine_imm.push_back(lzt::create_immediate_command_list(
          context, device, cmd_queue_flags_compute_engine,
          cmd_queue_mode_compute_engine, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          ordinal, is_explicit ? i : 0));

      ze_event_desc_t ev_desc = {};
      ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
      ev_desc.pNext = nullptr;
      ev_desc.index = cmd_lsts_compute_engine_imm.size() - 1;
      ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
      ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
      events.push_back(lzt::create_event(event_pool, ev_desc));

      host_vecs_compute_engine.push_back(
          lzt::allocate_host_memory(vec_len * sizeof(int), sizeof(int)));
      device_vecs_compute_engine.push_back(
          lzt::allocate_device_memory(vec_len * sizeof(int), sizeof(int)));

      patterns_compute_engine.push_back(cmd_lsts_compute_engine_imm.size() - 1);
    }
  }

  // Create copy engine CMD queues, lists and host/device buffers
  for (auto &engine : copy_engines) {
    const uint32_t ordinal = std::get<0>(engine);
    const uint32_t num_queues = std::get<1>(engine);
    const bool is_explicit =
        cmd_queue_flags_copy_engine & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY
            ? true
            : false;
    for (uint32_t i = 0; i < num_queues; i++) {
      // ASYNC execution for overlapping with compute engine
      cmd_queues_copy_engine.push_back(lzt::create_command_queue(
          context, device, cmd_queue_flags_copy_engine,
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          ordinal, is_explicit ? i : 0));

      if (is_explicit) {
        cmd_lst_flags_copy_engine &= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
      }
      cmd_lsts_copy_engine.push_back(lzt::create_command_list(
          context, device, cmd_lst_flags_copy_engine, ordinal));

      host_vecs_copy_engine.push_back(
          lzt::allocate_host_memory(vec_len * sizeof(int), sizeof(int)));
      device_vecs_copy_engine.push_back(
          lzt::allocate_device_memory(vec_len * sizeof(int), sizeof(int)));

      patterns_copy_engine.push_back(42 + cmd_lsts_copy_engine.size() - 1);
    }
  }

  // Fill copy engine CMD lists with tasks
  const int log_blk_len = 1;
  const int blk_sz = (1u << log_blk_len) * sizeof(int);
  for (int j = 0; j < cmd_lsts_copy_engine.size(); j++) {
    const auto cl = cmd_lsts_copy_engine[j];
    for (int i = 0; i < (vec_len >> log_blk_len); i++) {
      const auto device_ptr =
          reinterpret_cast<uint8_t *>(device_vecs_copy_engine[j]) + i * blk_sz;
      const auto host_ptr =
          reinterpret_cast<uint8_t *>(host_vecs_copy_engine[j]) + i * blk_sz;
      lzt::append_memory_fill(cl, device_ptr, &patterns_copy_engine[j],
                              sizeof(int), blk_sz, nullptr);
      lzt::append_barrier(cl);
      lzt::append_memory_copy(cl, host_ptr, device_ptr, blk_sz, nullptr);
      lzt::append_barrier(cl);
    }
    lzt::close_command_list(cl);
  }

  // Submit copy engine CMD lists, ASYNC execution
  for (int i = 0; i < cmd_lsts_copy_engine.size(); i++) {
    lzt::execute_command_lists(cmd_queues_copy_engine[i], 1,
                               &cmd_lsts_copy_engine[i], nullptr);
  }

  // Submit tasks to compute engine immediate CMD lists
  for (int i = 0; i < cmd_lsts_compute_engine_imm.size(); i++) {
    lzt::append_memory_fill(cmd_lsts_compute_engine_imm[i],
                            device_vecs_compute_engine[i],
                            &patterns_compute_engine[i], sizeof(int),
                            vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cmd_lsts_compute_engine_imm[i]);
  }

  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_len / 8, 1, 1};

  for (int i = 0; i < cmd_lsts_compute_engine_imm.size(); i++) {
    lzt::set_argument_value(kernel, 0, sizeof(void *),
                            &device_vecs_compute_engine[i]);
    lzt::append_launch_function(cmd_lsts_compute_engine_imm[i], kernel,
                                &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cmd_lsts_compute_engine_imm[i]);
  }
  for (int i = 0; i < cmd_lsts_compute_engine_imm.size(); i++) {
    lzt::append_memory_copy(
        cmd_lsts_compute_engine_imm[i], host_vecs_compute_engine[i],
        device_vecs_compute_engine[i], vec_len * sizeof(int), events[i]);
    lzt::append_barrier(cmd_lsts_compute_engine_imm[i]);
  }
  for (int i = 0; i < cmd_lsts_compute_engine_imm.size(); i++) {
    lzt::event_host_synchronize(events[i], UINT64_MAX);
  }

  // Sync with copy engine queues
  for (int i = 0; i < cmd_lsts_copy_engine.size(); i++) {
    lzt::synchronize(cmd_queues_copy_engine[i], UINT64_MAX);
  }

  // Verify copy engine results
  for (int i = 0; i < cmd_lsts_copy_engine.size(); i++) {
    const int *vec = reinterpret_cast<int *>(host_vecs_copy_engine[i]);
    for (size_t j = 0; j < vec_len; j++) {
      if (vec[j] != i + 42) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Verify compute engine results
  for (int i = 0; i < cmd_lsts_compute_engine_imm.size(); i++) {
    const int *vec = reinterpret_cast<int *>(host_vecs_compute_engine[i]);
    for (size_t j = 0; j < vec_len; j++) {
      if (vec[j] != i + 1) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Cleanup
  for (int i = 0; i < cmd_lsts_compute_engine_imm.size(); i++) {
    lzt::free_memory(host_vecs_compute_engine[i]);
    lzt::free_memory(device_vecs_compute_engine[i]);
    lzt::destroy_command_list(cmd_lsts_compute_engine_imm[i]);
    lzt::destroy_event(events[i]);
  }
  for (int i = 0; i < cmd_lsts_copy_engine.size(); i++) {
    lzt::free_memory(host_vecs_copy_engine[i]);
    lzt::free_memory(device_vecs_copy_engine[i]);
    lzt::destroy_command_list(cmd_lsts_copy_engine[i]);
    lzt::destroy_command_queue(cmd_queues_copy_engine[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(
    ImmediateOnComputeEngineRegularOnCopyEngineParameterization,
    zeTestCMDListImmediateOnComputeEngineRegularOnCopyEngine,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT)));

class zeTestCMDListRegularAndImmediateOnComputeEngine
    : public zeMixedCMDListsTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flags_t, ze_command_queue_flags_t,
                     ze_command_queue_mode_t, ze_command_list_flags_t>> {};

TEST_P(
    zeTestCMDListRegularAndImmediateOnComputeEngine,
    GivenRegularAndImmediateCMDListsToRunOnComputeEngineThenCorrectResultsAreReturned) {
  std::vector<void *> host_vecs;
  std::vector<void *> host_vecs_imm;
  std::vector<void *> device_vecs;
  std::vector<void *> device_vecs_imm;
  std::vector<ze_command_queue_handle_t> cmd_queues;
  std::vector<ze_command_list_handle_t> cmd_lsts;
  std::vector<ze_command_list_handle_t> cmd_lsts_imm;
  std::vector<ze_event_handle_t> events;
  std::vector<int> patterns;
  std::vector<int> patterns_imm;

  const ze_command_queue_flags_t cmd_queue_flags = std::get<0>(GetParam());
  const ze_command_queue_flags_t cmd_queue_flags_imm = std::get<1>(GetParam());
  const ze_command_queue_mode_t cmd_queue_mode_imm = std::get<2>(GetParam());
  ze_command_list_flags_t cmd_lst_flags = std::get<3>(GetParam());

  // One regular and one immediate CMD list for each compute engine
  for (auto &engine : compute_engines) {
    const uint32_t ordinal = std::get<0>(engine);
    const uint32_t num_queues = std::get<1>(engine);
    const bool is_explicit =
        cmd_queue_flags & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
    const bool is_explicit_imm =
        cmd_queue_flags_imm & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true
                                                                  : false;

    for (uint32_t i = 0; i < num_queues; i++) {
      // Create the ASYNC CMD queue and the regular CMD lists
      cmd_queues.push_back(lzt::create_command_queue(
          context, device, cmd_queue_flags, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit ? i : 0));

      if (is_explicit) {
        cmd_lst_flags &= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
      }
      cmd_lsts.push_back(
          lzt::create_command_list(context, device, cmd_lst_flags, ordinal));

      // Create the immediate CMD lists and the events
      cmd_lsts_imm.push_back(lzt::create_immediate_command_list(
          context, device, cmd_queue_flags_imm, cmd_queue_mode_imm,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit_imm ? i : 0));

      ze_event_desc_t ev_desc = {};
      ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
      ev_desc.pNext = nullptr;
      ev_desc.index = cmd_lsts_imm.size() - 1;
      ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
      ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
      events.push_back(lzt::create_event(event_pool, ev_desc));

      // Allocate the vectors
      host_vecs.push_back(
          lzt::allocate_host_memory(vec_len * sizeof(int), sizeof(int)));
      host_vecs_imm.push_back(
          lzt::allocate_host_memory(vec_len * sizeof(int), sizeof(int)));
      device_vecs.push_back(
          lzt::allocate_device_memory(vec_len * sizeof(int), sizeof(int)));
      device_vecs_imm.push_back(
          lzt::allocate_device_memory(vec_len * sizeof(int), sizeof(int)));

      patterns.push_back(cmd_lsts.size() - 1);
      patterns_imm.push_back(42 + cmd_lsts_imm.size() - 1);
    }
  }

  lzt::set_group_size(kernel, 8, 1, 1);
  const ze_group_count_t dispatch = {vec_len / 8, 1, 1};

  // Fill regular CMD lists
  for (int i = 0; i < cmd_lsts.size(); i++) {
    const auto cl = cmd_lsts[i];
    void *h_vec = host_vecs[i];
    void *d_vec = device_vecs[i];
    lzt::append_memory_fill(cl, d_vec, &patterns[i], sizeof(int),
                            vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cl);
    lzt::set_argument_value(kernel, 0, sizeof(void *), &d_vec);
    lzt::append_launch_function(cl, kernel, &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, h_vec, d_vec, vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, d_vec, h_vec, vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cl);
    lzt::append_launch_function(cl, kernel, &dispatch, nullptr, 0, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, h_vec, d_vec, vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cl);
    lzt::close_command_list(cl);
  }

  // Submit regular CMD lists, ASYNC execution
  for (int i = 0; i < cmd_lsts.size(); i++) {
    lzt::execute_command_lists(cmd_queues[i], 1, &cmd_lsts[i], nullptr);
  }

  // Submit tasks to the immediate CMD lists
  for (int i = 0; i < cmd_lsts_imm.size(); i++) {
    lzt::append_memory_fill(cmd_lsts_imm[i], device_vecs_imm[i],
                            &patterns_imm[i], sizeof(int),
                            vec_len * sizeof(int), nullptr);
    lzt::append_barrier(cmd_lsts_imm[i]);
  }
  for (int i = 0; i < cmd_lsts_imm.size(); i++) {
    lzt::set_argument_value(kernel, 0, sizeof(void *), &device_vecs_imm[i]);
    lzt::append_launch_function(cmd_lsts_imm[i], kernel, &dispatch, nullptr, 0,
                                nullptr);
    lzt::append_barrier(cmd_lsts_imm[i]);
  }
  for (int i = 0; i < cmd_lsts_imm.size(); i++) {
    lzt::append_memory_copy(cmd_lsts_imm[i], host_vecs_imm[i],
                            device_vecs_imm[i], vec_len * sizeof(int),
                            events[i]);
    lzt::append_barrier(cmd_lsts_imm[i]);
  }
  for (int i = 0; i < cmd_lsts_imm.size(); i++) {
    lzt::event_host_synchronize(events[i], UINT64_MAX);
  }

  // Sync with regular CMD lists
  for (int i = 0; i < cmd_lsts.size(); i++) {
    lzt::synchronize(cmd_queues[i], UINT64_MAX);
  }

  // Verify regular CMD lists' results
  for (int i = 0; i < cmd_lsts.size(); i++) {
    const int *vec = reinterpret_cast<int *>(host_vecs[i]);
    for (size_t j = 0; j < vec_len; j++) {
      if (vec[j] != i + 2) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Verify immediate CMD lists' results
  for (int i = 0; i < cmd_lsts_imm.size(); i++) {
    const int *vec = reinterpret_cast<int *>(host_vecs_imm[i]);
    for (size_t j = 0; j < vec_len; j++) {
      if (vec[j] != i + 43) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Cleanup
  for (int i = 0; i < cmd_lsts.size(); i++) {
    lzt::free_memory(host_vecs[i]);
    lzt::free_memory(host_vecs_imm[i]);
    lzt::free_memory(device_vecs[i]);
    lzt::free_memory(device_vecs_imm[i]);
    lzt::destroy_command_list(cmd_lsts[i]);
    lzt::destroy_command_list(cmd_lsts_imm[i]);
    lzt::destroy_command_queue(cmd_queues[i]);
    lzt::destroy_event(events[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(
    RegularAndImmediateOnComputeEngineParameterization,
    zeTestCMDListRegularAndImmediateOnComputeEngine,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT)));

class zeTestCMDListRegularAndImmediateOnCopyEngine
    : public zeMixedCMDListsTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flags_t, ze_command_queue_flags_t,
                     ze_command_queue_mode_t, ze_command_list_flags_t>> {};

TEST_P(
    zeTestCMDListRegularAndImmediateOnCopyEngine,
    GivenRegularAndImmediateCMDListsToRunOnCopyEngineThenCorrectResultsAreReturned) {
  std::vector<void *> host_vecs;
  std::vector<void *> host_vecs_imm;
  std::vector<void *> device_vecs;
  std::vector<void *> device_vecs_imm;
  std::vector<ze_command_queue_handle_t> cmd_queues;
  std::vector<ze_command_list_handle_t> cmd_lsts;
  std::vector<ze_command_list_handle_t> cmd_lsts_imm;
  std::vector<ze_event_handle_t> events_fill;
  std::vector<ze_event_handle_t> events_copy;
  std::vector<int> patterns;
  std::vector<int> patterns_imm;

  const ze_command_queue_flags_t cmd_queue_flags = std::get<0>(GetParam());
  const ze_command_queue_flags_t cmd_queue_flags_imm = std::get<1>(GetParam());
  const ze_command_queue_mode_t cmd_queue_mode_imm = std::get<2>(GetParam());
  ze_command_list_flags_t cmd_lst_flags = std::get<3>(GetParam());

  // One regular and one immediate CMD list for each copy engine
  for (auto &engine : copy_engines) {
    const uint32_t ordinal = std::get<0>(engine);
    const uint32_t num_queues = std::get<1>(engine);
    const bool is_explicit =
        cmd_queue_flags & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true : false;
    const bool is_explicit_imm =
        cmd_queue_flags_imm & ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY ? true
                                                                  : false;

    for (uint32_t i = 0; i < num_queues; i++) {
      // Create the ASYNC CMD queue and the regular CMD lists
      cmd_queues.push_back(lzt::create_command_queue(
          context, device, cmd_queue_flags, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit ? i : 0));

      if (is_explicit) {
        cmd_lst_flags &= ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY;
      }
      cmd_lsts.push_back(
          lzt::create_command_list(context, device, cmd_lst_flags, ordinal));

      // Create the immediate CMD lists and the events
      cmd_lsts_imm.push_back(lzt::create_immediate_command_list(
          context, device, cmd_queue_flags_imm, cmd_queue_mode_imm,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, ordinal, is_explicit_imm ? i : 0));

      ze_event_desc_t ev_desc = {};
      ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
      ev_desc.pNext = nullptr;
      ev_desc.index = cmd_lsts_imm.size() - 1;
      ev_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
      ev_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
      events_fill.push_back(lzt::create_event(event_pool, ev_desc));

      ev_desc.index = cmd_lsts_imm.size() - 1 + event_pool_sz / 2;
      ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
      events_copy.push_back(lzt::create_event(event_pool, ev_desc));

      // Allocate the vectors
      host_vecs.push_back(
          lzt::allocate_host_memory(vec_len * sizeof(int), sizeof(int)));
      host_vecs_imm.push_back(
          lzt::allocate_host_memory(vec_len * sizeof(int), sizeof(int)));
      device_vecs.push_back(
          lzt::allocate_device_memory(vec_len * sizeof(int), sizeof(int)));
      device_vecs_imm.push_back(
          lzt::allocate_device_memory(vec_len * sizeof(int), sizeof(int)));

      patterns.push_back(cmd_lsts.size());
      patterns_imm.push_back(42 + cmd_lsts_imm.size() - 1);
    }
  }

  const int log_blk_len = 6;
  const int blk_sz = (1u << log_blk_len) * sizeof(int);

  // Fill regular CMD lists
  for (int j = 0; j < cmd_lsts.size(); j++) {
    const auto cl = cmd_lsts[j];
    for (int i = 0; i < (vec_len >> log_blk_len); i++) {
      const auto device_ptr =
          reinterpret_cast<uint8_t *>(device_vecs[j]) + i * blk_sz;
      const auto host_ptr =
          reinterpret_cast<uint8_t *>(host_vecs[j]) + i * blk_sz;
      lzt::append_memory_fill(cl, device_ptr, &patterns[j], sizeof(int), blk_sz,
                              nullptr);
      lzt::append_barrier(cl);
      lzt::append_memory_copy(cl, host_ptr, device_ptr, blk_sz, nullptr);
      lzt::append_barrier(cl);
    }
    lzt::close_command_list(cl);
  }

  // Submit regular CMD lists, ASYNC execution
  for (int i = 0; i < cmd_lsts.size(); i++) {
    lzt::execute_command_lists(cmd_queues[i], 1, &cmd_lsts[i], nullptr);
  }

  // Submit tasks to immediate CMD lists
  for (int i = 0; i < (vec_len >> log_blk_len); i++) {
    for (int j = 0; j < cmd_lsts_imm.size(); j++) {
      const auto cl = cmd_lsts_imm[j];
      const auto device_ptr =
          reinterpret_cast<uint8_t *>(device_vecs_imm[j]) + i * blk_sz;
      const auto host_ptr =
          reinterpret_cast<uint8_t *>(host_vecs_imm[j]) + i * blk_sz;
      lzt::append_memory_fill(cl, device_ptr, &patterns_imm[j], sizeof(int),
                              blk_sz, events_fill[j]);
      lzt::append_memory_copy(cl, host_ptr, device_ptr, blk_sz, events_copy[j],
                              1, &events_fill[j]);
    }
    for (int j = 0; j < cmd_lsts_imm.size(); j++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeEventHostSynchronize(events_copy[j], UINT64_MAX));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events_copy[j]));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(events_fill[j]));
    }
  }

  // Sync with regular CMD lists' queues
  for (int i = 0; i < cmd_lsts.size(); i++) {
    lzt::synchronize(cmd_queues[i], UINT64_MAX);
  }

  // Verify regular CMD lists' results
  for (int i = 0; i < cmd_lsts.size(); i++) {
    const int *vec = reinterpret_cast<int *>(host_vecs[i]);
    for (size_t j = 0; j < vec_len; j++) {
      if (vec[j] != i + 1) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Verify immediate CMD lists' results
  for (int i = 0; i < cmd_lsts_imm.size(); i++) {
    const int *vec = reinterpret_cast<int *>(host_vecs_imm[i]);
    for (size_t j = 0; j < vec_len; j++) {
      if (vec[j] != i + 42) {
        EXPECT_TRUE(false);
        break;
      }
    }
  }

  // Cleanup
  for (int i = 0; i < cmd_lsts.size(); i++) {
    lzt::free_memory(host_vecs[i]);
    lzt::free_memory(host_vecs_imm[i]);
    lzt::free_memory(device_vecs[i]);
    lzt::free_memory(device_vecs_imm[i]);
    lzt::destroy_command_list(cmd_lsts[i]);
    lzt::destroy_command_list(cmd_lsts_imm[i]);
    lzt::destroy_command_queue(cmd_queues[i]);
    lzt::destroy_event(events_fill[i]);
    lzt::destroy_event(events_copy[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(
    RegularAndImmediateOnCopyEngineParameterization,
    zeTestCMDListRegularAndImmediateOnCopyEngine,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(static_cast<ze_command_list_flag_t>(0),
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING |
                              ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT)));
} // namespace
