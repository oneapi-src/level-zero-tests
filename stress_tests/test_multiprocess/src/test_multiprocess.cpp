/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "stress_common_func.hpp"
namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

#include <cerrno>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#ifdef _WIN32
#include <windows.h>
#include <process.h> // _getpid()
#define getpid _getpid
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace bipc = boost::interprocess;

struct ChildResults {
  int values[1024]; // supports up to 1024 child processes
};

static constexpr int default_child_count = 48;

using ChildWorkerFn = std::function<void()>;

static std::map<std::string, ChildWorkerFn> &worker_registry() {
  static std::map<std::string, ChildWorkerFn> registry;
  return registry;
}

void register_child_worker(const std::string &name, ChildWorkerFn fn) {
  worker_registry()[name] = std::move(fn);
}

// RAII helper: registers a worker via static initializer so it is available
// in every process (parent and re-launched child).
struct WorkerRegistrar {
  WorkerRegistrar(const char *name, ChildWorkerFn fn) {
    register_child_worker(name, std::move(fn));
  }
};

static WorkerRegistrar s_device_properties_worker("device_properties", []() {
  lzt::ze_init(ZE_INIT_FLAG_GPU_ONLY);
  ze_driver_handle_t driver = lzt::get_default_driver();
  ze_device_handle_t device = lzt::get_default_device(driver);
  ze_device_properties_t props = lzt::get_device_properties(device);
});

static void run_kernel_worker(bool is_immediate) {
  lzt::ze_init(ZE_INIT_FLAG_GPU_ONLY);

  ze_driver_handle_t driver = lzt::get_default_driver();
  ze_device_handle_t device = lzt::get_default_device(driver);
  ze_context_handle_t context = lzt::create_context(driver);
  constexpr uint32_t elem_count = 256;
  constexpr uint64_t event_sync_timeout =
      60'000'000'000ULL; // 60seconds in nanoseconds
  constexpr size_t buf_size = elem_count * sizeof(uint32_t);

  uint32_t *src = static_cast<uint32_t *>(
      lzt::allocate_shared_memory(buf_size, 1, 0, 0, device, context));
  uint32_t *dst = static_cast<uint32_t *>(
      lzt::allocate_shared_memory(buf_size, 1, 0, 0, device, context));

  const uint32_t fill_value = 0xDEADBEEF;
  for (uint32_t i = 0; i < elem_count; ++i)
    src[i] = fill_value;
  memset(dst, 0, buf_size);

  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
                                          nullptr,
                                          ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1};

  ze_event_pool_handle_t event_pool =
      lzt::create_event_pool(context, event_pool_desc);
  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, 0,
                                ZE_EVENT_SCOPE_FLAG_HOST};
  ze_event_handle_t event = lzt::create_event(event_pool, event_desc);

  ze_module_handle_t module =
      lzt::create_module(context, device, "test_multiprocess.spv");
  ze_kernel_handle_t kernel = lzt::create_function(module, "simple_test");

  uint32_t group_size_x = 0, group_size_y = 0, group_size_z = 0;
  lzt::suggest_group_size(kernel, elem_count, 1, 1, group_size_x, group_size_y,
                          group_size_z);

  lzt::set_group_size(kernel, group_size_x, 1, 1);
  lzt::set_argument_value(kernel, 0, sizeof(src), &src);
  lzt::set_argument_value(kernel, 1, sizeof(dst), &dst);

  ze_group_count_t dispatch = {};
  dispatch.groupCountX = elem_count / group_size_x;
  dispatch.groupCountY = 1;
  dispatch.groupCountZ = 1;

  lzt::zeCommandBundle bundle =
      lzt::create_command_bundle(context, device, 0, is_immediate);

  lzt::append_launch_function(bundle.list, kernel, &dispatch, event, 0,
                              nullptr);

  if (is_immediate) {
    lzt::synchronize_command_list_host(bundle.list, event_sync_timeout);
  } else {
    lzt::close_command_list(bundle.list);
    lzt::execute_and_sync_command_bundle(bundle, event_sync_timeout);
  }

  lzt::event_host_synchronize(event, event_sync_timeout);

  bool data_ok = true;
  for (uint32_t i = 0; i < elem_count; ++i) {
    if (dst[i] != fill_value) {
      LOG_INFO << "Data mismatch at index " << i << ": expected 0x" << std::hex
               << fill_value << ", got 0x" << dst[i];
      data_ok = false;
      break;
    }
  }

  lzt::destroy_event(event);
  lzt::destroy_event_pool(event_pool);
  lzt::free_memory(context, src);
  lzt::free_memory(context, dst);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_bundle(bundle);
  lzt::destroy_context(context);

  if (!data_ok) {
    throw std::runtime_error("data verification failed");
  }
}

static WorkerRegistrar s_kernel_immediate_worker("kernel_immediate", []() {
  run_kernel_worker(true);
});

static WorkerRegistrar s_kernel_registered_worker("kernel_registered", []() {
  run_kernel_worker(false);
});

int child_work(int child_index, ChildResults *shm,
               const std::string &worker_name) {
  try {
    auto it = worker_registry().find(worker_name);
    if (it == worker_registry().end()) {
      LOG_ERROR << "[child " << child_index
                << "] unknown worker: " << worker_name;
      return 1;
    }
    it->second();
  } catch (const std::exception &e) {
    LOG_ERROR << "[child " << child_index << "] exception: " << e.what();
    return 1;
  } catch (...) {
    LOG_ERROR << "[child " << child_index << "] unknown exception";
    return 1;
  }

  shm->values[child_index] = 1;
  return 0;
}

static void run_children_and_verify(int num_children,
                                    const std::string &worker_name) {
  LOG_INFO << "Parent process started (pid: " << getpid() << ")";
  const char *shm_name = "lzt_multiprocess_contexts_results";
  bipc::shared_memory_object::remove(shm_name);

  bipc::shared_memory_object shm_obj(bipc::create_only, shm_name,
                                     bipc::read_write);
  shm_obj.truncate(static_cast<bipc::offset_t>(sizeof(ChildResults)));
  bipc::mapped_region region(shm_obj, bipc::read_write);
  ChildResults *shm = static_cast<ChildResults *>(region.get_address());
  memset(shm, 0, sizeof(ChildResults));

#ifdef _WIN32
  char exe_path[MAX_PATH];
  GetModuleFileNameA(nullptr, exe_path, MAX_PATH);

  std::vector<HANDLE> proc_handles(num_children);
  for (int i = 0; i < num_children; ++i) {
    std::string cmd =
        std::string(exe_path) + " --lzt-child-worker=" + std::to_string(i) +
        " --lzt-shm-name=" + shm_name + " --lzt-child-worker-fn=" + worker_name;
    std::vector<char> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back('\0');

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessA(nullptr, cmd_buf.data(), nullptr, nullptr, FALSE,
                             0, nullptr, nullptr, &si, &pi);
    if (!ok) {
      ADD_FAILURE() << "CreateProcess failed for child " << i << ": "
                    << GetLastError();
      for (int j = 0; j < i; ++j)
        CloseHandle(proc_handles[j]);
      bipc::shared_memory_object::remove(shm_name);
      return;
    }
    LOG_INFO << "Child " << i << " started: pid=" << pi.dwProcessId
             << ", worker=" << worker_name;
    proc_handles[i] = pi.hProcess;
    CloseHandle(pi.hThread);
  }

  DWORD wait_res = WaitForMultipleObjects(static_cast<DWORD>(num_children),
                                          proc_handles.data(), TRUE, INFINITE);
  EXPECT_NE(WAIT_FAILED, wait_res)
      << "WaitForMultipleObjects failed: " << GetLastError();

  for (int i = 0; i < num_children; ++i) {
    DWORD exit_code = 0;
    if (GetExitCodeProcess(proc_handles[i], &exit_code)) {
      LOG_INFO << "Child " << i << " completed with exit code " << exit_code;
      EXPECT_EQ(0u, exit_code)
          << "Child process " << i << " exited with non-zero status";
    } else {
      ADD_FAILURE() << "GetExitCodeProcess failed for child " << i;
    }
    CloseHandle(proc_handles[i]);
  }

#else  // POSIX
  std::vector<pid_t> pids(num_children);
  for (int i = 0; i < num_children; ++i) {
    pid_t pid = fork();
    if (pid == -1) {
      ADD_FAILURE() << "fork() failed for child " << i << ": "
                    << strerror(errno);
      for (int j = 0; j < i; ++j)
        waitpid(pids[j], nullptr, 0);
      bipc::shared_memory_object::remove(shm_name);
      return;
    }
    if (pid == 0) {
      _exit(child_work(i, shm, worker_name));
    }
    LOG_INFO << "Child " << i << " started: pid=" << pid
             << ", worker=" << worker_name;
    pids[i] = pid;
  }

  for (int i = 0; i < num_children; ++i) {
    int status = 0;
    pid_t waited = waitpid(pids[i], &status, 0);
    EXPECT_NE(-1, waited) << "waitpid failed for child " << i << ": "
                          << strerror(errno);
    if (WIFEXITED(status)) {
      LOG_INFO << "Child " << i << " (pid=" << pids[i]
               << ") completed with exit code " << WEXITSTATUS(status);
      EXPECT_EQ(0, WEXITSTATUS(status))
          << "Child process " << i << " exited with non-zero status";
    } else {
      ADD_FAILURE() << "Child process " << i << " did not exit normally";
    }
  }
#endif // _WIN32

  for (int i = 0; i < num_children; ++i) {
    EXPECT_EQ(1, shm->values[i])
        << "Child " << i << " did not set its success flag in shared memory";
  }

  bipc::shared_memory_object::remove(shm_name);
}

namespace {

class RunMultiProcessTest : public ::testing::Test {};

LZT_TEST_F(
    RunMultiProcessTest,
    GivenNChildProcessesWhenEachRunsZeInitAndDevicePropertiesThenAllSucceed) {
  ASSERT_LE(default_child_count,
            static_cast<int>(sizeof(ChildResults::values) /
                             sizeof(ChildResults::values[0])))
      << "kDefaultChildCount exceeds shared-memory capacity";
  run_children_and_verify(default_child_count, "device_properties");
}

LZT_TEST_F(
    RunMultiProcessTest,
    GivenNChildProcessesWhenKernelExecutionAndHostEventSynchornizedSetThenNotTimeoutsAndErrorsOnImmediateCommandList) {
  ASSERT_LE(default_child_count,
            static_cast<int>(sizeof(ChildResults::values) /
                             sizeof(ChildResults::values[0])))
      << "kDefaultChildCount exceeds shared-memory capacity";
  run_children_and_verify(default_child_count, "kernel_immediate");
}

LZT_TEST_F(
    RunMultiProcessTest,
    GivenNChildProcessesWhenKernelExecutionAndHostEventSynchornizedSetThenNotTimeoutsAndErrorsOnRegisteredCommandList) {
  ASSERT_LE(default_child_count,
            static_cast<int>(sizeof(ChildResults::values) /
                             sizeof(ChildResults::values[0])))
      << "kDefaultChildCount exceeds shared-memory capacity";
  run_children_and_verify(default_child_count, "kernel_registered");
}

} // namespace
