/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

/*
 * If function signatures are updated, benchmark_template/set_parameter.hpp
 * needs to be updated.
 */
void parameter_buffer(ZeApp *benchmark, probe_config_t &probe_setting) {
  ze_kernel_handle_t function;
  void *input_buffer = nullptr;
  const std::vector<int8_t> input = {72, 101, 108, 108, 111, 32,
                                     87, 111, 114, 108, 100, 33};
  benchmark->memoryAlloc(size_in_bytes(input), &input_buffer);

  benchmark->functionCreate(&function, "function_parameter_buffers");

  /* Warm up */
  for (int i = 0; i < probe_setting.warm_up_iteration; i++) {
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        function, 0, sizeof(input_buffer), &input_buffer));
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        function, 1, sizeof(input_buffer), &input_buffer));
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        function, 2, sizeof(input_buffer), &input_buffer));
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        function, 3, sizeof(input_buffer), &input_buffer));
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        function, 4, sizeof(input_buffer), &input_buffer));
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        function, 5, sizeof(input_buffer), &input_buffer));
  }

  NANO_PROBE(" Argument index 0\t", probe_setting, zeKernelSetArgumentValue,
             function, 0U, sizeof(input_buffer), &input_buffer);

  NANO_PROBE(" Argument index 5\t", probe_setting, zeKernelSetArgumentValue,
             function, 5U, sizeof(input_buffer), &input_buffer);

  benchmark->functionDestroy(function);
  benchmark->memoryFree(input_buffer);
}

void parameter_integer(ZeApp *benchmark, probe_config_t &probe_setting) {
  ze_kernel_handle_t function;
  int input_a = 1;

  benchmark->functionCreate(&function, "function_parameter_integer");

  /* Warm up */
  for (int i = 0; i < probe_setting.warm_up_iteration; i++) {
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 0, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 1, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 2, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 3, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 4, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 5, sizeof(input_a), &input_a));
  }

  NANO_PROBE(" Argument index 0\t", probe_setting, zeKernelSetArgumentValue,
             function, 0U, sizeof(input_a), &input_a);

  NANO_PROBE(" Argument index 5\t", probe_setting, zeKernelSetArgumentValue,
             function, 5U, sizeof(input_a), &input_a);

  benchmark->functionDestroy(function);
}

void parameter_image(ZeApp *benchmark, probe_config_t &probe_setting) {
  ze_kernel_handle_t function;
  ze_image_handle_t input_a;

  benchmark->functionCreate(&function, "function_parameter_image");
  benchmark->imageCreate(&input_a, 128, 128, 0);

  /* Warm up */
  for (int i = 0; i < probe_setting.warm_up_iteration; i++) {
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 0, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 1, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 2, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 3, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 4, sizeof(input_a), &input_a));
    SUCCESS_OR_TERMINATE(
        zeKernelSetArgumentValue(function, 5, sizeof(input_a), &input_a));
  }

  NANO_PROBE(" Argument index 0\t", probe_setting, zeKernelSetArgumentValue,
             function, 0U, sizeof(input_a), &input_a);

  NANO_PROBE(" Argument index 5\t", probe_setting, zeKernelSetArgumentValue,
             function, 5U, sizeof(input_a), &input_a);

  benchmark->functionDestroy(function);
}
