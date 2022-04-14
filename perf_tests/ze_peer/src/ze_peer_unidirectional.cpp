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

  std::vector<uint32_t> remote_device_ids = {remote_device_id};
  std::vector<uint32_t> local_device_ids = {local_device_id};
  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size, validate);

  void *dst_buffer = ze_dst_buffers[remote_device_id];
  void *src_buffer = ze_src_buffers[local_device_id];
  if (transfer_type == PEER_READ) {
    dst_buffer = ze_dst_buffers[local_device_id];
    src_buffer = ze_src_buffers[remote_device_id];
  }

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  perform_copy_all_engines(test_type, local_device_id, dst_buffer, src_buffer,
                           buffer_size);

  if (validate) {
    validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                    dst_buffer, ze_host_buffer, buffer_size);
  }

  tear_down(remote_device_ids, local_device_ids);
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

  std::vector<uint32_t> remote_device_ids = {remote_device_id};
  std::vector<uint32_t> local_device_ids = {local_device_id};
  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size, validate);

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

  if (validate) {
    validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                    dst_buffer, ze_host_buffer, buffer_size);
  }

  tear_down(remote_device_ids, local_device_ids);
}

void ZePeer::parallel_bandwidth_latency(
    peer_test_t test_type, peer_transfer_t transfer_type,
    int number_buffer_elements, std::vector<uint32_t> &remote_device_ids,
    std::vector<uint32_t> &local_device_ids, bool validate) {

  size_t buffer_size = 0;

  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size, validate);

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  perform_parallel_copy(test_type, transfer_type, remote_device_ids,
                        local_device_ids, buffer_size);

  if (validate) {
    for (auto local_device_id : remote_device_ids) {
      for (auto remote_device_id : remote_device_ids) {
        void *dst_buffer = ze_dst_buffers[remote_device_id];
        ze_command_list_handle_t command_list =
            ze_peer_devices[local_device_id].command_lists[0];
        ze_command_queue_handle_t command_queue =
            ze_peer_devices[local_device_id].command_queues[0];

        validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                        dst_buffer, ze_host_buffer, buffer_size);
        for (size_t k = 0; k < buffer_size; k++) {
          ze_host_validate_buffer[k] = 0;
        }
      }
    }
  }

  tear_down(remote_device_ids, local_device_ids);
}
