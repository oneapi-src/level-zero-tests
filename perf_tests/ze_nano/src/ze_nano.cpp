/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "benchmark.hpp"

#include "command_line/command_line.hpp"

#include <algorithm>
#include <iomanip>
#include <utility>
#include <vector>

using namespace ze_api_benchmarks;

namespace {
class ZeNano {
public:
  ZeNano() {
    api_static_probe_init();
    benchmark = new ZeApp("ze_nano_benchmarks.spv");
    benchmark->singleDeviceInit();
    probe_setting.warm_up_iteration = 0;
    probe_setting.measure_iteration = 0;
  }

  ~ZeNano() {
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

void zeNano_zeKernelSetArgumentValue_Buffer() {
  std::cout << "zeNano_zeKernelSetArgumentValue_Buffer" << std::endl;
  ZeNano testInstance;
  testInstance.probe_setting.warm_up_iteration = 1000;
  testInstance.probe_setting.measure_iteration = 9000;

  testInstance.header_print_iteration("Buffer argument",
                                      testInstance.probe_setting);
  latency::parameter_buffer(testInstance.benchmark, testInstance.probe_setting);
  hardware_counter::parameter_buffer(testInstance.benchmark,
                                     testInstance.probe_setting);
  fuction_call_rate::parameter_buffer(testInstance.benchmark,
                                      testInstance.probe_setting);
  std::cout << std::endl;
}

void zeNano_zeKernelSetArgumentValue_Immediate() {
  std::cout << "zeNano_zeKernelSetArgumentValue_Immediate" << std::endl;
  ZeNano testInstance;
  testInstance.probe_setting.warm_up_iteration = 1000;
  testInstance.probe_setting.measure_iteration = 9000;

  testInstance.header_print_iteration("Immediate argument",
                                      testInstance.probe_setting);
  latency::parameter_integer(testInstance.benchmark,
                             testInstance.probe_setting);
  hardware_counter::parameter_integer(testInstance.benchmark,
                                      testInstance.probe_setting);
  fuction_call_rate::parameter_integer(testInstance.benchmark,
                                       testInstance.probe_setting);
  std::cout << std::endl;
}

void zeNano_zeKernelSetArgumentValue_Image() {
  std::cout << "zeNano_zeKernelSetArgumentValue_Image" << std::endl;
  ZeNano testInstance;
  testInstance.probe_setting.warm_up_iteration = 1000;
  testInstance.probe_setting.measure_iteration = 9000;

  testInstance.header_print_iteration("Image argument",
                                      testInstance.probe_setting);
  latency::parameter_image(testInstance.benchmark, testInstance.probe_setting);
  hardware_counter::parameter_image(testInstance.benchmark,
                                    testInstance.probe_setting);
  fuction_call_rate::parameter_image(testInstance.benchmark,
                                     testInstance.probe_setting);
  std::cout << std::endl;
}

void zeNano_zeCommandListAppendLaunchKernel() {
  std::cout << "zeNano_zeCommandListAppendLaunchKernel" << std::endl;
  ZeNano testInstance;
  testInstance.probe_setting.warm_up_iteration = 500;
  testInstance.probe_setting.measure_iteration = 2500;

  testInstance.header_print_iteration("", testInstance.probe_setting);
  latency::launch_function_no_parameter(testInstance.benchmark,
                                        testInstance.probe_setting);
  hardware_counter::launch_function_no_parameter(testInstance.benchmark,
                                                 testInstance.probe_setting);
  std::cout << std::endl;
}

void zeNano_zeCommandQueueExecuteCommandLists() {
  std::cout << "zeNano_zeCommandQueueExecuteCommandLists" << std::endl;
  ZeNano testInstance;
  testInstance.probe_setting.warm_up_iteration = 5;
  testInstance.probe_setting.measure_iteration = 10;
  testInstance.header_print_iteration("", testInstance.probe_setting);
  latency::command_list_empty_execute(testInstance.benchmark,
                                      testInstance.probe_setting);
  hardware_counter::command_list_empty_execute(testInstance.benchmark,
                                               testInstance.probe_setting);
  fuction_call_rate::command_list_empty_execute(testInstance.benchmark,
                                                testInstance.probe_setting);
  std::cout << std::endl;
}

void zeNano_zeDeviceGroupGetMemIpcHandle() {
  std::cout << "zeNano_zeDeviceGroupGetMemIpcHandle" << std::endl;
  ZeNano testInstance;
  testInstance.probe_setting.warm_up_iteration = 1000;
  testInstance.probe_setting.measure_iteration = 9000;
  testInstance.header_print_iteration("", testInstance.probe_setting);
  latency::ipc_memory_handle_get(testInstance.benchmark,
                                 testInstance.probe_setting);
  hardware_counter::ipc_memory_handle_get(testInstance.benchmark,
                                          testInstance.probe_setting);
  fuction_call_rate::ipc_memory_handle_get(testInstance.benchmark,
                                           testInstance.probe_setting);
  std::cout << std::endl;
}

} /* end namespace */

int main(int argc, char **argv) {
  const std::vector<std::pair<const char *, void (*)()>> tests = {
      {"zeKernelSetArgumentValue_Buffer",
       zeNano_zeKernelSetArgumentValue_Buffer},
      {"zeKernelSetArgumentValue_Immediate",
       zeNano_zeKernelSetArgumentValue_Immediate},
      {"zeKernelSetArgumentValue_Image", zeNano_zeKernelSetArgumentValue_Image},
      {"zeCommandListAppendLaunchKernel",
       zeNano_zeCommandListAppendLaunchKernel},
      {"zeCommandQueueExecuteCommandLists",
       zeNano_zeCommandQueueExecuteCommandLists},
      {"zeDeviceGroupGetMemIpcHandle", zeNano_zeDeviceGroupGetMemIpcHandle}};

  // Parse Options
  level_zero_tests::command_line_options options(
      "Select tests to be run. If none specified, all test cases will be "
      "run.\n\nAllowed options");
  options.flag("help", "produce help message");
  for (const auto &test : tests) {
    options.flag(test.first, "enable this test case");
  }
  try {
    options.parse(argc, argv);
  } catch (const level_zero_tests::command_line_error &e) {
    std::cerr << e.what() << std::endl;
    std::cout << options << std::endl;
    return 1;
  }

  // Run Tests
  if (options.given("help")) {
    std::cout << options << std::endl;
    exit(0);
  }
  const bool runAllTests =
      std::none_of(tests.begin(), tests.end(), [&options](const auto &test) {
        return options.given(test.first);
      });
  for (const auto &test : tests) {
    if (runAllTests || options.given(test.first)) {
      test.second();
    }
  }

  std::cout << "All Tests Complete" << std::endl << std::flush;
  return 0;
}
