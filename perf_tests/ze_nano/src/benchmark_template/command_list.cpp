/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

void launch_function_no_parameter(ZeApp *benchmark,
                                  probe_config_t &probe_setting) {
  ze_kernel_handle_t function;
  ze_command_list_handle_t command_list;
  benchmark->commandListCreate(&command_list);

  benchmark->functionCreate(&function, "function_no_parameter");

  ze_group_count_t group_count;
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  /* Warm up */
  for (int i = 0; i < probe_setting.warm_up_iteration; i++) {
    zeCommandListAppendLaunchKernel(command_list, function, &group_count,
                                    nullptr, 0, nullptr);
  }

  NANO_PROBE(" Function with no parameters\t", probe_setting,
             zeCommandListAppendLaunchKernel, command_list, function,
             &group_count, nullptr, 0, nullptr);

  benchmark->functionDestroy(function);
  benchmark->commandListDestroy(command_list);
}

void command_list_empty_execute(ZeApp *benchmark,
                                probe_config_t &probe_setting) {
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue;

  benchmark->commandQueueCreate(0, /*command_queue_id */
                                &command_queue);
  benchmark->commandListCreate(&command_list);
  benchmark->commandListClose(command_list);

  /* Warm up */
  for (int i = 0; i < probe_setting.warm_up_iteration; i++) {
    zeCommandQueueExecuteCommandLists(command_queue, 1, &command_list, nullptr);
  }

  NANO_PROBE(" Empty command list\t", probe_setting,
             zeCommandQueueExecuteCommandLists, command_queue, 1, &command_list,
             nullptr);

  benchmark->commandListDestroy(command_list);
  benchmark->commandQueueDestroy(command_queue);
}
