/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::perform_bidirectional_parallel_copy_to_multiple_targets(
    peer_test_t test_type, peer_transfer_t transfer_type,
    std::vector<uint32_t> &remote_device_ids,
    std::vector<uint32_t> &local_device_ids, size_t buffer_size) {

  size_t total_buffer_size = 0;

  for (size_t local_device_id_iter = 0;
       local_device_id_iter < local_device_ids.size(); local_device_id_iter++) {
    for (size_t remote_device_id_iter = 0;
         remote_device_id_iter < remote_device_ids.size();
         remote_device_id_iter++) {

      auto local_device_id = local_device_ids[local_device_id_iter];
      auto remote_device_id = remote_device_ids[remote_device_id_iter];

      if (local_device_id == remote_device_id) {
        continue;
      }

      uint32_t queue_index =
          queues[remote_device_id_iter % static_cast<uint32_t>(queues.size())];

      if (transfer_type == PEER_WRITE) {
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            ze_peer_devices[local_device_id].engines[queue_index].second,
            ze_dst_buffers[remote_device_id], ze_src_buffers[local_device_id],
            buffer_size, nullptr, 1, &event));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            ze_peer_devices[remote_device_id].engines[queue_index].second,
            ze_dst_buffers[local_device_id], ze_src_buffers[remote_device_id],
            buffer_size, nullptr, 1, &event));
      } else {
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            ze_peer_devices[local_device_id].engines[queue_index].second,
            ze_dst_buffers[local_device_id], ze_src_buffers[remote_device_id],
            buffer_size, nullptr, 1, &event));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            ze_peer_devices[remote_device_id].engines[queue_index].second,
            ze_dst_buffers[remote_device_id], ze_src_buffers[local_device_id],
            buffer_size, nullptr, 1, &event));
      }

      total_buffer_size += buffer_size;
    }
  }

  // Close all lists. If the number of queues is less than the number of
  // destinations, copies are batched.
  for (size_t local_device_id_iter = 0;
       local_device_id_iter < local_device_ids.size(); local_device_id_iter++) {
    for (size_t remote_device_id_iter = 0;
         remote_device_id_iter < remote_device_ids.size();
         remote_device_id_iter++) {

      auto local_device_id = local_device_ids[local_device_id_iter];
      auto remote_device_id = remote_device_ids[remote_device_id_iter];

      if (local_device_id == remote_device_id) {
        continue;
      }

      uint32_t queue_index =
          queues[remote_device_id_iter % static_cast<uint32_t>(queues.size())];

      SUCCESS_OR_TERMINATE(zeCommandListClose(
          ze_peer_devices[local_device_id].engines[queue_index].second));
      SUCCESS_OR_TERMINATE(zeCommandListClose(
          ze_peer_devices[remote_device_id].engines[queue_index].second));
    }
  }

  /* Warm up */
  for (int i = 0; i < warm_up_iterations; i++) {
    for (size_t local_device_id_iter = 0;
         local_device_id_iter < local_device_ids.size();
         local_device_id_iter++) {
      for (size_t remote_device_id_iter = 0;
           remote_device_id_iter < remote_device_ids.size();
           remote_device_id_iter++) {

        auto local_device_id = local_device_ids[local_device_id_iter];
        auto remote_device_id = remote_device_ids[remote_device_id_iter];

        if (local_device_id == remote_device_id) {
          continue;
        }

        uint32_t queue_index = queues[remote_device_id_iter %
                                      static_cast<uint32_t>(queues.size())];

        SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

        SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
            ze_peer_devices[local_device_id].engines[queue_index].first, 1,
            &ze_peer_devices[local_device_id].engines[queue_index].second,
            nullptr));
        SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
            ze_peer_devices[local_device_id].engines[queue_index].first,
            std::numeric_limits<uint64_t>::max()));

        SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
            ze_peer_devices[remote_device_id].engines[queue_index].first, 1,
            &ze_peer_devices[remote_device_id].engines[queue_index].second,
            nullptr));
        SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
            ze_peer_devices[remote_device_id].engines[queue_index].first,
            std::numeric_limits<uint64_t>::max()));

        SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      }
    }
  }

  Timer<std::chrono::microseconds::period> timer;
  std::vector<std::vector<Timer<std::chrono::microseconds::period>>> timers(
      ze_peer_devices.size());
  for (int i = 0; i < timers.size(); i++) {
    timers[i].resize(ze_peer_devices.size());
  }

  std::vector<std::vector<long double>> times(ze_peer_devices.size());
  for (int i = 0; i < times.size(); i++) {
    times[i].resize(ze_peer_devices.size());
    for (int j = 0; j < times.size(); j++) {
      times[i][j] = 0;
    }
  }

  do {
    timer.start();
    for (int i = 0; i < number_iterations; i++) {
      for (size_t local_device_id_iter = 0;
           local_device_id_iter < local_device_ids.size();
           local_device_id_iter++) {
        for (size_t remote_device_id_iter = 0;
             remote_device_id_iter < remote_device_ids.size();
             remote_device_id_iter++) {

          auto local_device_id = local_device_ids[local_device_id_iter];
          auto remote_device_id = remote_device_ids[remote_device_id_iter];

          if (local_device_id == remote_device_id) {
            continue;
          }

          uint32_t queue_index = queues[remote_device_id_iter %
                                        static_cast<uint32_t>(queues.size())];

          SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
              ze_peer_devices[local_device_id].engines[queue_index].first, 1,
              &ze_peer_devices[local_device_id].engines[queue_index].second,
              nullptr));
          SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
              ze_peer_devices[remote_device_id].engines[queue_index].first, 1,
              &ze_peer_devices[remote_device_id].engines[queue_index].second,
              nullptr));
        }
      }

      for (size_t local_device_id_iter = 0;
           local_device_id_iter < local_device_ids.size();
           local_device_id_iter++) {
        for (size_t remote_device_id_iter = 0;
             remote_device_id_iter < remote_device_ids.size();
             remote_device_id_iter++) {

          auto local_device_id = local_device_ids[local_device_id_iter];
          auto remote_device_id = remote_device_ids[remote_device_id_iter];

          if (local_device_id == remote_device_id) {
            continue;
          }

          timers[local_device_id][remote_device_id].start();
        }
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

      for (size_t local_device_id_iter = 0;
           local_device_id_iter < local_device_ids.size();
           local_device_id_iter++) {
        for (size_t remote_device_id_iter = 0;
             remote_device_id_iter < remote_device_ids.size();
             remote_device_id_iter++) {

          auto local_device_id = local_device_ids[local_device_id_iter];
          auto remote_device_id = remote_device_ids[remote_device_id_iter];

          if (local_device_id == remote_device_id) {
            continue;
          }

          uint32_t queue_index = queues[remote_device_id_iter %
                                        static_cast<uint32_t>(queues.size())];

          SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
              ze_peer_devices[local_device_id].engines[queue_index].first,
              std::numeric_limits<uint64_t>::max()));

          SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
              ze_peer_devices[remote_device_id].engines[queue_index].first,
              std::numeric_limits<uint64_t>::max()));

          timers[local_device_id][remote_device_id].end();
          times[local_device_id][remote_device_id] +=
              timers[local_device_id][remote_device_id].period_minus_overhead();
        }
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    }
    timer.end();

    for (size_t local_device_id_iter = 0;
         local_device_id_iter < local_device_ids.size();
         local_device_id_iter++) {
      for (size_t remote_device_id_iter = 0;
           remote_device_id_iter < remote_device_ids.size();
           remote_device_id_iter++) {
        auto local_device_id = local_device_ids[local_device_id_iter];
        auto remote_device_id = remote_device_ids[remote_device_id_iter];

        if (local_device_id == remote_device_id) {
          continue;
        }

        uint32_t queue_index = queues[remote_device_id_iter %
                                      static_cast<uint32_t>(queues.size())];

        std::cout << "\tDevice " << local_device_id << " - Device "
                  << remote_device_id << " using queue " << queue_index << ": ";
        print_results(true, test_type, buffer_size,
                      times[local_device_id][remote_device_id]);
      }
    }
    print_results(true, test_type, total_buffer_size, timer);
  } while (run_continuously);

  for (auto local_device_id : local_device_ids) {
    for (auto engine_pair : ze_peer_devices[local_device_id].engines) {
      SUCCESS_OR_TERMINATE(zeCommandListReset(engine_pair.second));
    }
  }
  for (auto remote_device_id : remote_device_ids) {
    for (auto engine_pair : ze_peer_devices[remote_device_id].engines) {
      SUCCESS_OR_TERMINATE(zeCommandListReset(engine_pair.second));
    }
  }
}

void ZePeer::perform_parallel_copy_to_multiple_targets(
    peer_test_t test_type, peer_transfer_t transfer_type,
    std::vector<uint32_t> &remote_device_ids,
    std::vector<uint32_t> &local_device_ids, size_t buffer_size) {

  size_t total_buffer_size = 0;

  for (size_t local_device_id_iter = 0;
       local_device_id_iter < local_device_ids.size(); local_device_id_iter++) {
    for (size_t remote_device_id_iter = 0;
         remote_device_id_iter < remote_device_ids.size();
         remote_device_id_iter++) {

      auto local_device_id = local_device_ids[local_device_id_iter];
      auto remote_device_id = remote_device_ids[remote_device_id_iter];

      if (local_device_id == remote_device_id) {
        continue;
      }

      uint32_t queue_index =
          queues[remote_device_id_iter % static_cast<uint32_t>(queues.size())];
      uint32_t device_id = local_device_id;
      if (use_queue_in_destination) {
        device_id = remote_device_id;
      }
      ze_command_list_handle_t command_list =
          ze_peer_devices[device_id].engines[queue_index].second;

      void *dst_buffer = ze_dst_buffers[remote_device_id];
      void *src_buffer = ze_src_buffers[local_device_id];
      if (transfer_type == PEER_READ) {
        dst_buffer = ze_dst_buffers[local_device_id];
        src_buffer = ze_src_buffers[remote_device_id];
      }

      SUCCESS_OR_TERMINATE(
          zeCommandListAppendMemoryCopy(command_list, dst_buffer, src_buffer,
                                        buffer_size, nullptr, 1, &event));

      total_buffer_size += buffer_size;
    }
  }

  // Close all lists. If the number of queues is less than the number of
  // destinations, copies are batched.
  for (size_t local_device_id_iter = 0;
       local_device_id_iter < local_device_ids.size(); local_device_id_iter++) {
    for (size_t remote_device_id_iter = 0;
         remote_device_id_iter < remote_device_ids.size();
         remote_device_id_iter++) {

      auto local_device_id = local_device_ids[local_device_id_iter];
      auto remote_device_id = remote_device_ids[remote_device_id_iter];

      if (local_device_id == remote_device_id) {
        continue;
      }

      uint32_t queue_index =
          queues[remote_device_id_iter % static_cast<uint32_t>(queues.size())];
      uint32_t device_id = local_device_id;
      if (use_queue_in_destination) {
        device_id = remote_device_id;
      }
      ze_command_list_handle_t command_list =
          ze_peer_devices[device_id].engines[queue_index].second;

      SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
    }
  }

  /* Warm up */
  for (int i = 0; i < warm_up_iterations; i++) {
    for (size_t local_device_id_iter = 0;
         local_device_id_iter < local_device_ids.size();
         local_device_id_iter++) {
      for (size_t remote_device_id_iter = 0;
           remote_device_id_iter < remote_device_ids.size();
           remote_device_id_iter++) {
        auto local_device_id = local_device_ids[local_device_id_iter];
        auto remote_device_id = remote_device_ids[remote_device_id_iter];

        if (local_device_id == remote_device_id) {
          continue;
        }

        uint32_t queue_index = queues[remote_device_id_iter %
                                      static_cast<uint32_t>(queues.size())];
        uint32_t device_id = local_device_id;
        if (use_queue_in_destination) {
          device_id = remote_device_id;
        }
        ze_command_list_handle_t command_list =
            ze_peer_devices[device_id].engines[queue_index].second;
        ze_command_queue_handle_t command_queue =
            ze_peer_devices[device_id].engines[queue_index].first;

        SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

        SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
            command_queue, 1, &command_list, nullptr));
        SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
            command_queue, std::numeric_limits<uint64_t>::max()));

        SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      }
    }
  }

  Timer<std::chrono::microseconds::period> timer;
  std::vector<std::vector<Timer<std::chrono::microseconds::period>>> timers(
      ze_peer_devices.size());
  for (int i = 0; i < timers.size(); i++) {
    timers[i].resize(ze_peer_devices.size());
  }

  std::vector<std::vector<long double>> times(ze_peer_devices.size());
  for (int i = 0; i < times.size(); i++) {
    times[i].resize(ze_peer_devices.size());
    for (int j = 0; j < times.size(); j++) {
      times[i][j] = 0;
    }
  }

  do {
    timer.start();
    for (int i = 0; i < number_iterations; i++) {
      for (size_t local_device_id_iter = 0;
           local_device_id_iter < local_device_ids.size();
           local_device_id_iter++) {
        for (size_t remote_device_id_iter = 0;
             remote_device_id_iter < remote_device_ids.size();
             remote_device_id_iter++) {

          auto local_device_id = local_device_ids[local_device_id_iter];
          auto remote_device_id = remote_device_ids[remote_device_id_iter];

          if (local_device_id == remote_device_id) {
            continue;
          }

          uint32_t queue_index = queues[remote_device_id_iter %
                                        static_cast<uint32_t>(queues.size())];
          uint32_t device_id = local_device_id;
          if (use_queue_in_destination) {
            device_id = remote_device_id;
          }
          ze_command_list_handle_t command_list =
              ze_peer_devices[device_id].engines[queue_index].second;
          ze_command_queue_handle_t command_queue =
              ze_peer_devices[device_id].engines[queue_index].first;

          SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
              command_queue, 1, &command_list, nullptr));
        }
      }

      for (size_t local_device_id_iter = 0;
           local_device_id_iter < local_device_ids.size();
           local_device_id_iter++) {
        for (size_t remote_device_id_iter = 0;
             remote_device_id_iter < remote_device_ids.size();
             remote_device_id_iter++) {

          auto local_device_id = local_device_ids[local_device_id_iter];
          auto remote_device_id = remote_device_ids[remote_device_id_iter];

          if (local_device_id == remote_device_id) {
            continue;
          }

          timers[local_device_id][remote_device_id].start();
        }
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

      for (size_t local_device_id_iter = 0;
           local_device_id_iter < local_device_ids.size();
           local_device_id_iter++) {
        for (size_t remote_device_id_iter = 0;
             remote_device_id_iter < remote_device_ids.size();
             remote_device_id_iter++) {

          auto local_device_id = local_device_ids[local_device_id_iter];
          auto remote_device_id = remote_device_ids[remote_device_id_iter];

          if (local_device_id == remote_device_id) {
            continue;
          }

          uint32_t queue_index = queues[remote_device_id_iter %
                                        static_cast<uint32_t>(queues.size())];
          uint32_t device_id = local_device_id;
          if (use_queue_in_destination) {
            device_id = remote_device_id;
          }
          ze_command_queue_handle_t command_queue =
              ze_peer_devices[device_id].engines[queue_index].first;

          SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
              command_queue, std::numeric_limits<uint64_t>::max()));

          timers[local_device_id][remote_device_id].end();
          times[local_device_id][remote_device_id] +=
              timers[local_device_id][remote_device_id].period_minus_overhead();
        }
      }
      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    }
    timer.end();

    for (size_t local_device_id_iter = 0;
         local_device_id_iter < local_device_ids.size();
         local_device_id_iter++) {
      for (size_t remote_device_id_iter = 0;
           remote_device_id_iter < remote_device_ids.size();
           remote_device_id_iter++) {
        auto local_device_id = local_device_ids[local_device_id_iter];
        auto remote_device_id = remote_device_ids[remote_device_id_iter];

        if (local_device_id == remote_device_id) {
          continue;
        }

        uint32_t queue_index = queues[remote_device_id_iter %
                                      static_cast<uint32_t>(queues.size())];

        std::cout << "\tDevice " << local_device_id << " - Device "
                  << remote_device_id << " using queue " << queue_index << ": ";
        print_results(false, test_type, buffer_size,
                      times[local_device_id][remote_device_id]);
      }
    }
    print_results(false, test_type, total_buffer_size, timer);
  } while (run_continuously);

  for (auto local_device_id : local_device_ids) {
    for (auto engine_pair : ze_peer_devices[local_device_id].engines) {
      SUCCESS_OR_TERMINATE(zeCommandListReset(engine_pair.second));
    }
  }
  for (auto remote_device_id : remote_device_ids) {
    for (auto engine_pair : ze_peer_devices[remote_device_id].engines) {
      SUCCESS_OR_TERMINATE(zeCommandListReset(engine_pair.second));
    }
  }
}

void ZePeer::bandwidth_latency_parallel_to_multiple_targets(
    peer_test_t test_type, peer_transfer_t transfer_type,
    int number_buffer_elements, std::vector<uint32_t> &remote_device_ids,
    std::vector<uint32_t> &local_device_ids) {

  size_t buffer_size = 0;

  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size);

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  if (bidirectional) {
    perform_bidirectional_parallel_copy_to_multiple_targets(
        test_type, transfer_type, remote_device_ids, local_device_ids,
        buffer_size);
  } else {
    perform_parallel_copy_to_multiple_targets(test_type, transfer_type,
                                              remote_device_ids,
                                              local_device_ids, buffer_size);
  }

  for (size_t local_device_id_iter = 0;
       local_device_id_iter < local_device_ids.size(); local_device_id_iter++) {
    for (size_t remote_device_id_iter = 0;
         remote_device_id_iter < remote_device_ids.size();
         remote_device_id_iter++) {

      auto local_device_id = local_device_ids[local_device_id_iter];
      auto remote_device_id = remote_device_ids[remote_device_id_iter];

      if (local_device_id == remote_device_id) {
        continue;
      }

      void *dst_buffer = ze_dst_buffers[remote_device_id];
      if (transfer_type == PEER_READ) {
        dst_buffer = ze_dst_buffers[local_device_id];
      }

      ze_command_queue_handle_t command_queue =
          ze_peer_devices[local_device_id].engines[0].first;
      ze_command_list_handle_t command_list =
          ze_peer_devices[local_device_id].engines[0].second;

      validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                      dst_buffer, ze_host_buffer, buffer_size);
      for (size_t k = 0; k < buffer_size; k++) {
        ze_host_validate_buffer[k] = 0;
      }
    }
  }

  tear_down(remote_device_ids, local_device_ids);
}
