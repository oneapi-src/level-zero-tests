/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::bandwidth_latency_all_engines(peer_test_t test_type,
                                           peer_transfer_t transfer_type,
                                           int number_buffer_elements,
                                           uint32_t remote_device_id,
                                           uint32_t local_device_id,
                                           bool validate) {

  size_t buffer_size = 0;
  ze_command_list_handle_t command_list =
      ze_peer_devices[local_device_id].command_lists[0];
  ze_command_queue_handle_t command_queue =
      ze_peer_devices[local_device_id].command_queues[0];

  set_up(number_buffer_elements, remote_device_id, local_device_id, buffer_size,
         validate);

  void *dst_buffer = ze_dst_buffers[remote_device_id];
  void *src_buffer = ze_src_buffers[local_device_id];
  if (transfer_type == PEER_READ) {
    dst_buffer = ze_dst_buffers[local_device_id];
    src_buffer = ze_src_buffers[remote_device_id];
  }

  initialize_buffers(remote_device_id, local_device_id, ze_host_buffer,
                     buffer_size);

  perform_copy_all_engines(test_type, local_device_id, dst_buffer, src_buffer,
                           buffer_size);

  if (validate) {
    validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                    dst_buffer, ze_host_buffer, buffer_size);
  }

  tear_down(remote_device_id, local_device_id);
}

void ZePeer::bandwidth_latency(peer_test_t test_type,
                               peer_transfer_t transfer_type,
                               int number_buffer_elements,
                               uint32_t remote_device_id,
                               uint32_t local_device_id, bool validate) {

  size_t buffer_size = 0;
  ze_command_list_handle_t command_list =
      ze_peer_devices[local_device_id].command_lists[0];
  ze_command_queue_handle_t command_queue =
      ze_peer_devices[local_device_id].command_queues[0];

  set_up(number_buffer_elements, remote_device_id, local_device_id, buffer_size,
         validate);

  void *dst_buffer = ze_dst_buffers[remote_device_id];
  void *src_buffer = ze_src_buffers[local_device_id];
  if (transfer_type == PEER_READ) {
    dst_buffer = ze_dst_buffers[local_device_id];
    src_buffer = ze_src_buffers[remote_device_id];
  }

  initialize_buffers(remote_device_id, local_device_id, ze_host_buffer,
                     buffer_size);

  perform_copy(test_type, command_list, command_queue, dst_buffer, src_buffer,
               buffer_size);

  if (validate) {
    validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                    dst_buffer, ze_host_buffer, buffer_size);
  }

  tear_down(remote_device_id, local_device_id);
}
