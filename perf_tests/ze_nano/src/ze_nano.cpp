/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "benchmark.hpp"
#include "gmock/gmock.h"

#include <iomanip>

using namespace ze_api_benchmarks;

namespace {
class ZeNano : public ::testing::Test {
protected:
  ZeNano() {
    api_static_probe_init();
    benchmark = new ZeApp("ze_nano_benchmarks.spv");
    benchmark->singleDeviceInit();
    probe_setting.warm_up_iteration = 0;
    probe_setting.measure_iteration = 0;
  }

  ~ZeNano() override {
    api_static_probe_cleanup();
    benchmark->singleDeviceCleanup();
    delete benchmark;
  }

  void header_print_iteration(std::string prefix,
                              probe_config_t &probe_setting) {
    std::cout << " All measurements are averaged per call except the function "
                 "call rate metric"
              << std::endl;
    std::cout << std::left << std::setw(25) << " " + prefix << std::internal
              << "Warm up iterations " << probe_setting.warm_up_iteration
              << std::setw(30) << " Measured iterations "
              << probe_setting.measure_iteration << std::endl;
  }

  ZeApp *benchmark;
  probe_config_t probe_setting;
};

TEST_F(ZeNano, zeKernelSetArgumentValue_Buffer) {
  probe_setting.warm_up_iteration = 1000;
  probe_setting.measure_iteration = 9000;

  header_print_iteration("Buffer argument", probe_setting);
  latency::parameter_buffer(benchmark, probe_setting);
  hardware_counter::parameter_buffer(benchmark, probe_setting);
  fuction_call_rate::parameter_buffer(benchmark, probe_setting);
  std::cout << std::endl;
}

TEST_F(ZeNano, zeKernelSetArgumentValue_Immediate) {
  probe_setting.warm_up_iteration = 1000;
  probe_setting.measure_iteration = 9000;

  header_print_iteration("Immediate argument", probe_setting);
  latency::parameter_integer(benchmark, probe_setting);
  hardware_counter::parameter_integer(benchmark, probe_setting);
  fuction_call_rate::parameter_integer(benchmark, probe_setting);
  std::cout << std::endl;
}

TEST_F(ZeNano, zeKernelSetArgumentValue_Image) {
  probe_setting.warm_up_iteration = 1000;
  probe_setting.measure_iteration = 9000;

  header_print_iteration("Image argument", probe_setting);
  latency::parameter_image(benchmark, probe_setting);
  hardware_counter::parameter_image(benchmark, probe_setting);
  fuction_call_rate::parameter_image(benchmark, probe_setting);
  std::cout << std::endl;
}

TEST_F(ZeNano, zeCommandListAppendLaunchKernel) {
  probe_setting.warm_up_iteration = 500;
  probe_setting.measure_iteration = 2500;

  header_print_iteration("", probe_setting);
  latency::launch_function_no_parameter(benchmark, probe_setting);
  hardware_counter::launch_function_no_parameter(benchmark, probe_setting);
  std::cout << std::endl;
}

TEST_F(ZeNano, zeCommandQueueExecuteCommandLists) {
  probe_setting.warm_up_iteration = 5;
  probe_setting.measure_iteration = 10;
  header_print_iteration("", probe_setting);
  latency::command_list_empty_execute(benchmark, probe_setting);
  hardware_counter::command_list_empty_execute(benchmark, probe_setting);
  fuction_call_rate::command_list_empty_execute(benchmark, probe_setting);
  std::cout << std::endl;
}

TEST_F(ZeNano, zeDeviceGroupGetMemIpcHandle) {
  probe_setting.warm_up_iteration = 1000;
  probe_setting.measure_iteration = 9000;
  header_print_iteration("", probe_setting);
  latency::ipc_memory_handle_get(benchmark, probe_setting);
  hardware_counter::ipc_memory_handle_get(benchmark, probe_setting);
  fuction_call_rate::ipc_memory_handle_get(benchmark, probe_setting);
  std::cout << std::endl;
}
} /* end namespace */

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
  std::cout << std::flush;
}
