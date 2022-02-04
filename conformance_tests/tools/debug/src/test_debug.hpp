/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_DEBUG_HPP
#define TEST_DEBUG_HPP

#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;
namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

constexpr auto device_id_string = "device_id";
constexpr auto use_sub_devices_string = "use_sub_devices";
constexpr auto kernel_string = "kernel";
constexpr auto num_kernels_string = "num_kernels";
constexpr auto test_type_string = "test_type";

const uint16_t eventsTimeoutMS = 30000;
const uint16_t eventsTimeoutS = 30;
const uint16_t timeoutThreshold = 4;

typedef enum {
  BASIC,
  ATTACH_AFTER_MODULE_CREATED,
  MULTIPLE_MODULES_CREATED,
  ATTACH_AFTER_MODULE_DESTROYED,
  LONG_RUNNING_KERNEL_INTERRUPTED,
  KERNEL_RESUME,
  THREAD_STOPPED,
  THREAD_UNAVAILABLE,
  PAGE_FAULT,
  MAX_DEBUG_TEST_TYPE_VALUE = 0xff // Values greater than 0xFF are reserved
} debug_test_type_t;

typedef struct {
  bool debugger_signal;
  bool debugee_signal;
} debug_signals_t;

typedef enum { SINGLE_THREAD, GROUP_OF_THREADS, ALL_THREADS } num_threads_t;

#define CLEAN_AND_ASSERT(condition, debug_session, helper)                     \
  do {                                                                         \
    if (!condition) {                                                          \
      lzt::debug_detach(debug_session);                                        \
      helper.terminate();                                                      \
      ASSERT_TRUE(false);                                                      \
    }                                                                          \
  } while (0)

class zetDebugBaseSetup : public ::testing::Test {
protected:
  void SetUp() override {
    bi::shared_memory_object::remove("debug_bool");
    shm = new bi::shared_memory_object(bi::create_only, "debug_bool",
                                       bi::read_write);

    LOG_DEBUG << "[Debugger] Created shared memory object";
    if (!shm) {
      FAIL()
          << "[Debugger] Could not create condition variable for debug tests";
    }

    shm->truncate(sizeof(debug_signals_t));
    region = new bi::mapped_region(*shm, bi::read_write);
    if (!region || !(region->get_address())) {
      FAIL() << "[Debugger] Could not create signal variables for debug tests";
    }
    LOG_DEBUG << "[Debugger] Created region";

    static_cast<debug_signals_t *>(region->get_address())->debugger_signal =
        false;
    static_cast<debug_signals_t *>(region->get_address())->debugee_signal =
        false;

    bi::named_mutex::remove("debugger_mutex");
    mutex = new bi::named_mutex(bi::create_only, "debugger_mutex");
    LOG_DEBUG << "[Debugger] Created debugger mutex";

    bi::named_condition::remove("debug_bool_set");
    condition = new bi::named_condition(bi::create_only, "debug_bool_set");
    LOG_DEBUG << "[Debugger] Created debug bool set";
  }

  void TearDown() override {
    bi::shared_memory_object::remove("debug_bool");
    bi::named_mutex::remove("debugger_mutex");
    bi::named_condition::remove("debug_bool_set");

    delete shm;
    delete region;
    delete mutex;
    delete condition;
  }

  bp::child debug_helper;
  bi::shared_memory_object *shm;
  bi::mapped_region *region;
  bi::named_mutex *mutex;
  bi::named_condition *condition;
};

#endif // TEST_DEBUG_HPP