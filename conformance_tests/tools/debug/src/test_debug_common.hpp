/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_DEBUG_COMMON_HPP
#define TEST_DEBUG_COMMON_HPP

#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_debug_common.hpp"

namespace lzt = level_zero_tests;
namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

constexpr auto device_id_string = "device_id";
constexpr auto use_sub_devices_string = "use_sub_devices";
constexpr auto module_string = "module";
constexpr auto test_type_string = "test_type";

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
  MAX_DEBUG_TEST_TYPE_VALUE = 0xff, // Values greater than 0xFF are reserved
  DEBUG_TEST_TYPE_FORCE_UINT32 = 0x7fffffff
} debug_test_type_t;

typedef struct {
  bool debugger_signal;
  bool debugee_signal;
  uint64_t gpu_buffer_address;
} debug_signals_t;

class process_synchro {
public:
  process_synchro(bool enable, bool create) {
    enabled = enable;
    if (!enable)
      return;
    if (create) {

      bi::shared_memory_object::remove("debug_bool");
      bi::shared_memory_object shm = bi::shared_memory_object(
          bi::create_only, "debug_bool", bi::read_write);

      shm.truncate(sizeof(debug_signals_t));
      region = new bi::mapped_region(shm, bi::read_write);

      LOG_DEBUG << "[Debugger] Created region";

      debugger_signal = &(static_cast<debug_signals_t *>(region->get_address())
                              ->debugger_signal);
      debugee_signal = &(static_cast<debug_signals_t *>(region->get_address())
                             ->debugee_signal);
      gpu_buffer_address =
          &(static_cast<debug_signals_t *>(region->get_address())
                ->gpu_buffer_address);

      *debugger_signal = false;
      *debugee_signal = false;
      *gpu_buffer_address = 0;

      bi::named_mutex::remove("debugger_mutex");
      mutex = new bi::named_mutex(bi::create_only, "debugger_mutex");
      LOG_DEBUG << "[Debugger] Created debugger mutex";

      bi::named_condition::remove("debug_bool_set");
      condition = new bi::named_condition(bi::create_only, "debug_bool_set");
      LOG_DEBUG << "[Debugger] Created debug bool set";

    } else {
      bi::shared_memory_object shm =
          bi::shared_memory_object(bi::open_only, "debug_bool", bi::read_write);
      region = new bi::mapped_region(shm, bi::read_write);

      mutex = new bi::named_mutex(bi::open_only, "debugger_mutex");
      condition = new bi::named_condition(bi::open_only, "debug_bool_set");

      debugger_signal = &(static_cast<debug_signals_t *>(region->get_address())
                              ->debugger_signal);
      debugee_signal = &(static_cast<debug_signals_t *>(region->get_address())
                             ->debugee_signal);
      gpu_buffer_address =
          &(static_cast<debug_signals_t *>(region->get_address())
                ->gpu_buffer_address);
    }
  }

  ~process_synchro() {
    if (!enabled)
      return;
    delete mutex;
    delete condition;
    delete region;
  }

  // To be used by application only:
  void wait_for_attach() {
    if (!enabled)
      return;
    LOG_INFO << "[Application] Waiting for debugger to attach";
    bi::scoped_lock<bi::named_mutex> lock(*mutex);
    condition->wait(lock, [&] { return *debugger_signal; });
    LOG_INFO << "[Application] Debugged process proceeding";
  }

  // To be used by application only:
  void notify_debugger() {
    if (!enabled)
      return;
    LOG_INFO << "[Application] Notifying Debugger";
    mutex->lock();
    *debugee_signal = true;
    mutex->unlock();
    condition->notify_all();
  }

  // To be used by application only:
  void update_gpu_buffer_address(const uint64_t &gpu_address) {
    if (!enabled)
      return;
    LOG_DEBUG << "[Application] Updating gpu buffer address:" << std::hex
              << gpu_address;
    mutex->lock();
    *gpu_buffer_address = gpu_address;
    mutex->unlock();
  }

  // To be used by Debugger only:
  void notify_attach() {
    if (!enabled)
      return;
    LOG_INFO << "[Debugger] Notifying application after attaching";
    mutex->lock();
    *debugger_signal = true;
    mutex->unlock();
    condition->notify_all();
  }

  // To be used by Debugger only:
  void wait_for_application() {
    if (!enabled)
      return;
    LOG_INFO << "[Debugger] Waiting for Application to notify";
    bi::scoped_lock<bi::named_mutex> lock(*mutex);
    condition->wait(lock, [&] { return *debugee_signal; });
    LOG_INFO << "[Debugger] Received Application notification proceeding";
  }

  // To be used by Debugger only:
  bool get_app_gpu_buffer_address(uint64_t &gpu_address) {
    if ((!enabled) || (*gpu_buffer_address == 0))
      return false;
    mutex->lock();
    gpu_address = *gpu_buffer_address;
    mutex->unlock();
    return true;
  }

private:
  bi::mapped_region *region;
  bi::named_mutex *mutex;
  bi::named_condition *condition;
  bool *debugger_signal;
  bool *debugee_signal;
  uint64_t *gpu_buffer_address;
  bool enabled;
};

#endif // TEST_DEBUG_COMMON_HPP
