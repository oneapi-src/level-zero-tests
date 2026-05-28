/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gmock/gmock.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"

#ifdef _WIN32
#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

// Forward declarations - definitions live in test_multiprocess.cpp.
struct ChildResults {
  int values[64];
};
int child_work(int child_index, ChildResults *shm,
               const std::string &worker_name);

// Returns the child exit code when running as a Windows child worker,
// or -1 when this is the normal parent/test-runner invocation.
static int maybe_run_as_child_worker(int argc, char **argv) {
  int child_index = -1;
  const char *shm_name = nullptr;
  const char *worker_fn = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--lzt-child-worker=", 19) == 0)
      child_index = std::atoi(argv[i] + 19);
    else if (strncmp(argv[i], "--lzt-shm-name=", 15) == 0)
      shm_name = argv[i] + 15;
    else if (strncmp(argv[i], "--lzt-child-worker-fn=", 22) == 0)
      worker_fn = argv[i] + 22;
  }
  if (child_index < 0 || !shm_name || !worker_fn)
    return -1;

  namespace bipc = boost::interprocess;
  try {
    bipc::shared_memory_object shm_obj(bipc::open_only, shm_name,
                                       bipc::read_write);
    bipc::mapped_region region(shm_obj, bipc::read_write);
    ChildResults *shm = static_cast<ChildResults *>(region.get_address());
    return child_work(child_index, shm, std::string(worker_fn));
  } catch (...) {
    return 1;
  }
}
#endif // _WIN32

int main(int argc, char **argv) {
#ifdef _WIN32
  // On Windows, child processes re-enter main() with special arguments
  // instead of fork(). Intercept before zeInit is called by the parent path.
  {
    int child_rc = maybe_run_as_child_worker(argc, argv);
    if (child_rc >= 0)
      return child_rc;
  }
#endif
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);

  bool run_ze_init = true;

  for (const auto &arg : command_line) {
    if (arg == "--no-ze-init") {
      run_ze_init = false;
      break;
    }
  }

  if (run_ze_init) {
    ze_result_t result = zeInit(0);
    if (result) {
      throw std::runtime_error("zeInit failed: " +
                               level_zero_tests::to_string(result));
    }
    LOG_TRACE << "Driver initialized";
    level_zero_tests::print_platform_overview();
  } else {
    LOG_TRACE << "zeInit skipped (--no-ze-init)";
  }

  try {
    auto result = RUN_ALL_TESTS();
    return result;
  } catch (const std::exception &e) {
    LOG_ERROR << "Error: " << e.what();
    return 1;
  }
}
