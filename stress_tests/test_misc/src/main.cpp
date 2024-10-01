/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gmock/gmock.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#if defined(unix) || defined(__unix__) || defined(__unix)
#include <sys/mman.h>
#endif
#include "stress_common_func.hpp"

void reserve_memory(bool release) {
  size_t page_size = get_page_size();
  size_t single_alloc_size = 1024UL * 1024UL * get_page_size();
  size_t initial_alloc_size = single_alloc_size;
  size_t total_alloc_size = 0;
  uint64_t *ptr;
  size_t offset = 0;
  size_t counter = 0;
  std::vector<uint64_t *> r_ptr;

#if defined(unix) || defined(__unix__) || defined(__unix)
  while (single_alloc_size > 0) {
    ptr = (uint64_t *)mmap((void *)offset, single_alloc_size, PROT_NONE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (counter < 10) {
      r_ptr.emplace_back(ptr);
    }
    if (ptr == MAP_FAILED) {
      single_alloc_size = single_alloc_size - page_size;
    } else {
      total_alloc_size = total_alloc_size + single_alloc_size;
      counter++;
    }
  }
  LOG_INFO << "Memory reservation completed.";
  LOG_INFO << "Memory page size: " << page_size << " B";
  LOG_INFO << "Initial single reservation size: "
           << initial_alloc_size / (1024UL * 1024UL * 1024UL) << " GB";
  LOG_INFO << "Total reservation size: "
           << total_alloc_size / (1024UL * 1024UL * 1024UL) << " GB";
  LOG_INFO << "First 10 reserved addresses:";
  for (auto address : r_ptr) {
    LOG_INFO << address;
  }
  if (release) {
    // verification case (unreserve memory)
    if (munmap(r_ptr[0], initial_alloc_size) == -1) {
      LOG_INFO << "Error while removing reservation offset = " << r_ptr[0]
               << " length = "
               << initial_alloc_size / (1024UL * 1024UL * 1024UL) << " GB";
    } else {
      LOG_INFO << "Removed reservation in offset = " << r_ptr[0] << " length = "
               << initial_alloc_size / (1024UL * 1024UL * 1024UL) << " GB";
    }
  }
#else
  throw std::runtime_error("The test is not supported on the Windows system.");
#endif
}

int main(int argc, char **argv) {
  try {
    ::testing::InitGoogleMock(&argc, argv);
  } catch (const std::exception &e) {
    LOG_ERROR << "Failed to init google mock: " << e.what();
    return 1;
  } catch (...) {
    LOG_ERROR << "Failed to init google mock";
    return 1;
  }
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);
  std::string user_arg = "release_memory";
  bool release = false;
  if (std::find(command_line.begin(), command_line.end(), user_arg) !=
      command_line.end()) {
    release = true;
  }
  reserve_memory(release);
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();

  try {
    auto result = RUN_ALL_TESTS();
  } catch (const std::runtime_error &e) {
    LOG_ERROR << "Failed to run tests: " << e.what();
    return 1;
  }
}
