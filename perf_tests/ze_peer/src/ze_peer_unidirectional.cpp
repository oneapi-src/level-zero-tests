/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::perform_copy(peer_test_t test_type,
                          ze_command_list_handle_t command_list,
                          ze_command_queue_handle_t command_queue,
                          void *dst_buffer, void *src_buffer,
                          size_t buffer_size) {

  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
      command_list, dst_buffer, src_buffer, buffer_size, nullptr, 0, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        command_queue, 1, &command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        command_queue, std::numeric_limits<uint64_t>::max()));
  }

  do {
    timer.start();
    for (uint32_t i = 0U; i < number_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          command_queue, 1, &command_list, nullptr));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          command_queue, std::numeric_limits<uint64_t>::max()));
    }
    timer.end();

    print_results(false, test_type, buffer_size, timer);
  } while (run_continuously);

  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));
}

void ZePeer::bandwidth_latency(peer_test_t test_type,
                               peer_transfer_t transfer_type,
                               int number_buffer_elements,
                               uint32_t remote_device_id,
                               uint32_t local_device_id, uint32_t queue_index) {

  size_t buffer_size = 0;

  ze_command_queue_handle_t command_queue =
      ze_peer_devices[local_device_id].engines[queue_index].first;
  ze_command_list_handle_t command_list =
      ze_peer_devices[local_device_id].engines[queue_index].second;

  std::vector<uint32_t> remote_device_ids = {remote_device_id};
  std::vector<uint32_t> local_device_ids = {local_device_id};
  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size);

  void *dst_buffer = ze_dst_buffers[remote_device_id];
  void *src_buffer = ze_src_buffers[local_device_id];
  if (transfer_type == PEER_READ) {
    dst_buffer = ze_dst_buffers[local_device_id];
    src_buffer = ze_src_buffers[remote_device_id];
  }

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  perform_copy(test_type, command_list, command_queue, dst_buffer, src_buffer,
               buffer_size);

  if (validate_results) {
    validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                    dst_buffer, ze_host_buffer, buffer_size);
  }

  tear_down(remote_device_ids, local_device_ids);
}
