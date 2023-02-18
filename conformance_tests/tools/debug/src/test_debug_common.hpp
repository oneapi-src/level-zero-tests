/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_DEBUG_COMMON_HPP
#define TEST_DEBUG_COMMON_HPP

#ifdef _WIN32
#define BOOST_INTERPROCESS_SHARED_DIR_FUNC
#endif
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
constexpr auto module_string = "module";
constexpr auto test_type_string = "test_type";
#ifdef _WIN32
static void
boost::interprocess::ipcdetail::get_shared_dir(std::string &shared_dir) {
  shared_dir = ".";
}
#endif

typedef enum {
  BASIC,
  ATTACH_AFTER_MODULE_CREATED,
  MULTIPLE_MODULES_CREATED,
  ATTACH_AFTER_MODULE_DESTROYED,
  LONG_RUNNING_KERNEL_INTERRUPTED,
  LONG_RUNNING_KERNEL_INTERRUPTED_SLM,
  PAGE_FAULT,
  MULTIPLE_THREADS,
  MULTIPLE_CQ,
  MULTIPLE_IMM_CL,
  USE_TWO_DEVICES,
  MULTI_DEVICE_RESOURCE_STRESS,
  MAX_DEBUG_TEST_TYPE_VALUE = 0xff, // Values greater than 0xFF are reserved
  DEBUG_TEST_TYPE_FORCE_UINT32 = 0x7fffffff
} debug_test_type_t;

typedef struct {
  bool debugger_signal;
  bool debugee_signal;
  uint64_t gpu_buffer_address;
  uint32_t gpu_thread_count;
  uint32_t child_application_pid;
  uint32_t spill_mem_size;
} debug_signals_t;

class process_synchro {
public:
  process_synchro(bool enable, bool create, int index = 0) {
    enabled = enable;
    if (!enable)
      return;

    auto debug_bool_name = "debug_bool" + std::to_string(index);
    auto debug_mutex_name = "debugger_mutex" + std::to_string(index);
    auto debug_bool_set_name = "debug_bool_set" + std::to_string(index);

    if (create) {
      bi::shared_memory_object::remove(debug_bool_name.c_str());
      bi::shared_memory_object shm = bi::shared_memory_object(
          bi::create_only, debug_bool_name.c_str(), bi::read_write);

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
      gpu_thread_count = &(static_cast<debug_signals_t *>(region->get_address())
                               ->gpu_thread_count);
      child_application_pid =
          &(static_cast<debug_signals_t *>(region->get_address())
                ->child_application_pid);

      spill_mem_size = &(static_cast<debug_signals_t *>(region->get_address())
                             ->spill_mem_size);

      *debugger_signal = false;
      *debugee_signal = false;
      *gpu_buffer_address = 0;
      *gpu_thread_count = 0;
      *child_application_pid = 0;
      *spill_mem_size = 0;

      bi::named_mutex::remove(debug_mutex_name.c_str());
      mutex = new bi::named_mutex(bi::create_only, debug_mutex_name.c_str());
      LOG_DEBUG << "[Debugger] Created debugger mutex";

      bi::named_condition::remove(debug_bool_set_name.c_str());
      condition =
          new bi::named_condition(bi::create_only, debug_bool_set_name.c_str());
      LOG_DEBUG << "[Debugger] Created debug bool set";

    } else {
      bi::shared_memory_object shm = bi::shared_memory_object(
          bi::open_only, debug_bool_name.c_str(), bi::read_write);
      region = new bi::mapped_region(shm, bi::read_write);

      mutex = new bi::named_mutex(bi::open_only, debug_mutex_name.c_str());
      condition =
          new bi::named_condition(bi::open_only, debug_bool_set_name.c_str());

      debugger_signal = &(static_cast<debug_signals_t *>(region->get_address())
                              ->debugger_signal);
      debugee_signal = &(static_cast<debug_signals_t *>(region->get_address())
                             ->debugee_signal);
      gpu_buffer_address =
          &(static_cast<debug_signals_t *>(region->get_address())
                ->gpu_buffer_address);
      gpu_thread_count = &(static_cast<debug_signals_t *>(region->get_address())
                               ->gpu_thread_count);
      child_application_pid =
          &(static_cast<debug_signals_t *>(region->get_address())
                ->child_application_pid);

      spill_mem_size = &(static_cast<debug_signals_t *>(region->get_address())
                             ->spill_mem_size);
    }
  }

  ~process_synchro() {
    if (!enabled)
      return;
    delete mutex;
    delete condition;
    delete region;
  }

  void notify_child() { notify_application("[Parent] Notifying child"); }
  void wait_for_parent_signal() {
    wait_for_debugger_signal("[Child Debugger] Waiting for parent to notify");
  }
  void clear_parent_signal() { clear_debugger_signal(); }

  void notify_parent() { notify_debugger("[Child Debugger] Notifying parent"); }
  void wait_for_child_signal() {
    wait_for_application_signal("[Parent] Waiting for child to notify");
  }
  void clear_child_signal() { clear_application_signal(); }

  // To be used by Debugger only:
  void notify_application(const std::string &message = "") {
    if (!enabled)
      return;
    LOG_INFO << (message != "" ? message
                               : " [Debugger] Notifying application signal");
    mutex->lock();
    *debugger_signal = true;
    mutex->unlock();
    condition->notify_all();
  }

  // To be used by application only:
  void wait_for_debugger_signal(const std::string &message = "") {
    if (!enabled)
      return;
    LOG_INFO << (message != ""
                     ? message
                     : "[Application] Waiting for debugger to notify");
    bi::scoped_lock<bi::named_mutex> lock(*mutex);

    condition->wait(lock, [&] { return *debugger_signal; });
    LOG_INFO << "Received notification proceeding";
  }

  void clear_debugger_signal() {
    if (!enabled)
      return;
    LOG_INFO << "Clearing debugger signal";
    mutex->lock();
    *debugger_signal = false;
    mutex->unlock();
    condition->notify_all();
  }

  // To be used by application only:
  void notify_debugger(const std::string &message = "") {
    if (!enabled)
      return;
    LOG_INFO << (message != "" ? message
                               : "[Application] Notifying debugger signal");
    mutex->lock();
    *debugee_signal = true;
    mutex->unlock();
    condition->notify_all();
  }

  // To be used by Debugger only:
  void wait_for_application_signal(const std::string &message = "") {
    if (!enabled)
      return;
    LOG_INFO << (message != ""
                     ? message
                     : "[Debugger] Waiting for application to notify");
    bi::scoped_lock<bi::named_mutex> lock(*mutex);
    condition->wait(lock, [&] { return *debugee_signal; });
    LOG_INFO << "Received notification proceeding";
  }

  void clear_application_signal() {
    if (!enabled)
      return;
    LOG_INFO << "Clearing application signal";
    mutex->lock();
    *debugee_signal = false;
    mutex->unlock();
    condition->notify_all();
  }

  // To be used by application only:
  void update_gpu_buffer_address(const uint64_t &gpu_address) {
    if (!enabled)
      return;
    LOG_INFO << "[Application] Updating gpu buffer address:" << std::hex
             << gpu_address;
    mutex->lock();
    *gpu_buffer_address = gpu_address;
    mutex->unlock();
  }

  // To be used by application only:
  void update_gpu_thread_count(const uint32_t &gpu_threads) {
    if (!enabled)
      return;
    LOG_INFO << "[Application] Sharing number gpu thread to run: "
             << gpu_threads;
    mutex->lock();
    *gpu_thread_count = gpu_threads;
    mutex->unlock();
  }

  // To be used by application only:
  void update_spill_mem_size(const uint32_t &spill_mem) {
    if (!enabled)
      return;
    LOG_INFO << "[Application] Sharing spill memory size: " << spill_mem;
    mutex->lock();
    *spill_mem_size = spill_mem;
    mutex->unlock();
  }

  // To be used by Debugger only:
  bool get_app_gpu_buffer_address(uint64_t &gpu_address) {
    if ((!enabled) || (*gpu_buffer_address == 0))
      return false;
    mutex->lock();
    gpu_address = *gpu_buffer_address;
    mutex->unlock();
    LOG_INFO << "[Debugger] Received Application buffer address: " << std::hex
             << gpu_address;

    return true;
  }

  // To be used by Debugger only:
  bool get_app_gpu_thread_count(uint32_t &gpu_threads) {
    if ((!enabled) || (*gpu_thread_count == 0))
      return false;
    mutex->lock();
    gpu_threads = *gpu_thread_count;
    mutex->unlock();
    LOG_INFO << "[Debugger] Received number gpu thread to run: " << gpu_threads;

    return true;
  }

  bool get_app_scratch_size(uint32_t &scratch_size) {
    if ((!enabled) || (*spill_mem_size == 0))
      return false;
    mutex->lock();
    scratch_size = *spill_mem_size;
    mutex->unlock();
    LOG_INFO << "[Debugger] Received scratch space size: " << scratch_size;

    return true;
  }

  void update_child_application_pid(uint32_t child_pid) {
    if (!enabled)
      return;
    LOG_INFO << "[Child Debugger] Updating child application pid: "
             << child_pid;
    mutex->lock();
    *child_application_pid = child_pid;
    mutex->unlock();
  }

  void get_child_application_pid(uint32_t &child_pid) {
    if (!enabled)
      return;
    mutex->lock();
    child_pid = *child_application_pid;
    mutex->unlock();
    LOG_INFO << "[Debugger] Received child application pid: " << child_pid;
  }

private:
  bi::mapped_region *region;
  bi::named_mutex *mutex;
  bi::named_condition *condition;
  bool *debugger_signal;
  bool *debugee_signal;
  uint64_t *gpu_buffer_address;
  uint32_t *gpu_thread_count;
  uint32_t *child_application_pid;
  bool enabled;
  uint32_t *spill_mem_size;
};

#endif // TEST_DEBUG_COMMON_HPP
