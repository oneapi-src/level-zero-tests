/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::bidirectional_perform_copy(
    uint32_t remote_device_id, uint32_t local_device_id, uint32_t queue_index,
    peer_test_t test_type, peer_transfer_t transfer_type, size_t buffer_size) {

  ze_command_list_handle_t local_command_list =
      ze_peer_devices[local_device_id].engines[queue_index].second;
  ze_command_queue_handle_t local_command_queue =
      ze_peer_devices[local_device_id].engines[queue_index].first;
  ze_command_list_handle_t remote_command_list =
      ze_peer_devices[remote_device_id].engines[queue_index].second;
  ze_command_queue_handle_t remote_command_queue =
      ze_peer_devices[remote_device_id].engines[queue_index].first;

  if (transfer_type == PEER_WRITE) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        local_command_list, ze_dst_buffers[remote_device_id],
        ze_src_buffers[local_device_id], buffer_size, nullptr, 1, &event));
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        remote_command_list, ze_dst_buffers[local_device_id],
        ze_src_buffers[remote_device_id], buffer_size, nullptr, 1, &event));
  } else {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        local_command_list, ze_dst_buffers[local_device_id],
        ze_src_buffers[remote_device_id], buffer_size, nullptr, 1, &event));
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        remote_command_list, ze_dst_buffers[remote_device_id],
        ze_src_buffers[local_device_id], buffer_size, nullptr, 1, &event));
  }
  SUCCESS_OR_TERMINATE(zeCommandListClose(local_command_list));
  SUCCESS_OR_TERMINATE(zeCommandListClose(remote_command_list));

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        local_command_queue, 1, &local_command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        remote_command_queue, 1, &remote_command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        local_command_queue, std::numeric_limits<uint64_t>::max()));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        remote_command_queue, std::numeric_limits<uint64_t>::max()));
    SUCCESS_OR_TERMINATE(zeEventHostReset(event));
  }

  do {
    long double time_usec = 0;
    for (uint32_t i = 0U; i < number_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          local_command_queue, 1, &local_command_list, nullptr));
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          remote_command_queue, 1, &remote_command_list, nullptr));

      timer.start();
      SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          local_command_queue, std::numeric_limits<uint64_t>::max()));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          remote_command_queue, std::numeric_limits<uint64_t>::max()));
      timer.end();
      time_usec += timer.period_minus_overhead();

      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    }
    print_results(true, test_type, buffer_size, time_usec);
  } while (run_continuously);

  SUCCESS_OR_TERMINATE(zeCommandListReset(local_command_list));
  SUCCESS_OR_TERMINATE(zeCommandListReset(remote_command_list));
}

void ZePeer::bidirectional_bandwidth_latency(peer_test_t test_type,
                                             peer_transfer_t transfer_type,
                                             size_t number_buffer_elements,
                                             uint32_t remote_device_id,
                                             uint32_t local_device_id,
                                             uint32_t queue_index) {

  size_t buffer_size = 0;

  std::vector<uint32_t> remote_device_ids = {remote_device_id};
  std::vector<uint32_t> local_device_ids = {local_device_id};

  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size);

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  bidirectional_perform_copy(remote_device_id, local_device_id, queue_index,
                             test_type, transfer_type, buffer_size);

  if (validate_results) {
    validate_buffer(ze_peer_devices[remote_device_id].engines[0].second,
                    ze_peer_devices[remote_device_id].engines[0].first,
                    ze_host_validate_buffer, ze_dst_buffers[remote_device_id],
                    ze_host_buffer, buffer_size);

    for (size_t k = 0; k < buffer_size; k++) {
      ze_host_validate_buffer[k] = 5;
    }

    validate_buffer(ze_peer_devices[local_device_id].engines[0].second,
                    ze_peer_devices[local_device_id].engines[0].first,
                    ze_host_validate_buffer, ze_dst_buffers[local_device_id],
                    ze_host_buffer, buffer_size);
  }

  tear_down(remote_device_ids, local_device_ids);
}