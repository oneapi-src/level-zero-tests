/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_ZE_WORKLOAD_HPP
#define COMPUTE_API_BENCH_ZE_WORKLOAD_HPP

#define ZE_CHECK_RESULT(result)                                                \
  {                                                                            \
    if (result != ZE_RESULT_SUCCESS) {                                         \
      printf("Fatal : ZeResult is %d in %s at line %d\n", result, __FILE__,    \
             __LINE__);                                                        \
      assert(result == ZE_RESULT_SUCCESS);                                     \
    }                                                                          \
  }

#include <vector>
#include <string>
#include "../common/workload.hpp"

#include <level_zero/ze_api.h>

namespace compute_api_bench {

class ZeWorkload : public Workload {

public:
  ZeWorkload();
  virtual ~ZeWorkload();

protected:
  void create_device();
  void prepare_program();
  virtual void build_program() = 0;
  virtual void create_buffers() = 0;
  virtual void create_cmdlist() = 0;
  virtual void execute_work() = 0;
  virtual bool verify_results() = 0;
  virtual void cleanup() = 0;

  size_t kernel_length;
  std::string kernel_code;
  std::vector<uint8_t> kernel_spv;
  ze_device_handle_t device = nullptr;
  ze_driver_handle_t driver_handle = nullptr;
};

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_ZE_WORKLOAD_HPP
