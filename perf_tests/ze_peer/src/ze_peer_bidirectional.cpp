/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::bidirectional_perform_copy(uint32_t remote_device_id,
                                        uint32_t local_device_id,
                                        peer_test_t test_type,
                                        peer_transfer_t transfer_type,
                                        size_t buffer_size) {

  ze_command_list_handle_t local_command_list =
      ze_peer_devices[local_device_id].command_lists[0];
  ze_command_queue_handle_t local_command_queue =
      ze_peer_devices[local_device_id].command_queues[0];
  ze_command_list_handle_t remote_command_list =
      ze_peer_devices[remote_device_id].command_lists[0];
  ze_command_queue_handle_t remote_command_queue =
      ze_peer_devices[remote_device_id].command_queues[0];

  if (transfer_type == PEER_WRITE) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        local_command_list, ze_dst_buffers[remote_device_id],
        ze_src_buffers[local_device_id], buffer_size, nullptr, 0, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        remote_command_list, ze_dst_buffers[local_device_id],
        ze_src_buffers[remote_device_id], buffer_size, nullptr, 0, nullptr));
  } else {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        local_command_list, ze_dst_buffers[local_device_id],
        ze_src_buffers[remote_device_id], buffer_size, nullptr, 0, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        remote_command_list, ze_dst_buffers[remote_device_id],
        ze_src_buffers[local_device_id], buffer_size, nullptr, 0, nullptr));
  }
  SUCCESS_OR_TERMINATE(zeCommandListClose(local_command_list));
  SUCCESS_OR_TERMINATE(zeCommandListClose(remote_command_list));

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (int i = 0; i < warm_up_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        local_command_queue, 1, &local_command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        remote_command_queue, 1, &remote_command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        local_command_queue, std::numeric_limits<uint64_t>::max()));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        remote_command_queue, std::numeric_limits<uint64_t>::max()));
  }

  timer.start();
  for (int i = 0; i < number_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        local_command_queue, 1, &local_command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        remote_command_queue, 1, &remote_command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        local_command_queue, std::numeric_limits<uint64_t>::max()));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        remote_command_queue, std::numeric_limits<uint64_t>::max()));
  }
  timer.end();

  print_results(true, test_type, buffer_size, timer);

  SUCCESS_OR_TERMINATE(zeCommandListReset(local_command_list));
  SUCCESS_OR_TERMINATE(zeCommandListReset(remote_command_list));
}

void ZePeer::bidirectional_bandwidth_latency(peer_test_t test_type,
                                             peer_transfer_t transfer_type,
                                             int number_buffer_elements,
                                             uint32_t remote_device_id,
                                             uint32_t local_device_id,
                                             bool validate) {

  size_t buffer_size = 0;

  set_up(number_buffer_elements, remote_device_id, local_device_id, buffer_size,
         validate);

  initialize_buffers(remote_device_id, local_device_id, ze_host_buffer,
                     buffer_size);

  bidirectional_perform_copy(remote_device_id, local_device_id, test_type,
                             transfer_type, buffer_size);

  if (validate) {
    validate_buffer(ze_peer_devices[remote_device_id].command_lists[0],
                    ze_peer_devices[remote_device_id].command_queues[0],
                    ze_host_validate_buffer, ze_dst_buffers[remote_device_id],
                    ze_host_buffer, buffer_size);

    for (size_t k = 0; k < buffer_size; k++) {
      ze_host_validate_buffer[k] = 5;
    }

    validate_buffer(ze_peer_devices[local_device_id].command_lists[0],
                    ze_peer_devices[local_device_id].command_queues[0],
                    ze_host_validate_buffer, ze_dst_buffers[local_device_id],
                    ze_host_buffer, buffer_size);
  }

  tear_down(remote_device_id, local_device_id);
}