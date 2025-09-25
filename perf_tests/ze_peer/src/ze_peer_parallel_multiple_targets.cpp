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
    std::vector<uint32_t> &local_device_ids, size_t buffer_size,
    bool divide_buffers) {

  size_t total_buffer_size = 0;

  size_t num_engines = queues.size();
  size_t chunk = buffer_size / num_engines;

  size_t queue_index_iter = 0;
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

      if (divide_buffers) {
        for (size_t e = 0; e < num_engines; e++) {
          if (transfer_type == PEER_WRITE) {
            SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
                ze_peer_devices[local_device_id].engines[queues[e]].second,
                reinterpret_cast<void *>(reinterpret_cast<uint64_t>(
                                             ze_dst_buffers[remote_device_id]) +
                                         e * chunk),
                reinterpret_cast<void *>(reinterpret_cast<uint64_t>(
                                             ze_src_buffers[local_device_id]) +
                                         e * chunk),
                chunk, nullptr, 1, &event));
            SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
                ze_peer_devices[remote_device_id].engines[queues[e]].second,
                reinterpret_cast<void *>(reinterpret_cast<uint64_t>(
                                             ze_dst_buffers[local_device_id]) +
                                         e * chunk),
                reinterpret_cast<void *>(reinterpret_cast<uint64_t>(
                                             ze_src_buffers[remote_device_id]) +
                                         e * chunk),
                chunk, nullptr, 1, &event));
          } else {
            SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
                ze_peer_devices[local_device_id].engines[queues[e]].second,
                reinterpret_cast<void *>(reinterpret_cast<uint64_t>(
                                             ze_dst_buffers[local_device_id]) +
                                         e * chunk),
                reinterpret_cast<void *>(reinterpret_cast<uint64_t>(
                                             ze_src_buffers[remote_device_id]) +
                                         e * chunk),
                chunk, nullptr, 1, &event));
            SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
                ze_peer_devices[remote_device_id].engines[queues[e]].second,
                reinterpret_cast<void *>(reinterpret_cast<uint64_t>(
                                             ze_dst_buffers[remote_device_id]) +
                                         e * chunk),
                reinterpret_cast<void *>(reinterpret_cast<uint64_t>(
                                             ze_src_buffers[local_device_id]) +
                                         e * chunk),
                chunk, nullptr, 1, &event));
          }
        }
      } else {
        uint32_t queue_index =
            queues[queue_index_iter++ % static_cast<uint32_t>(queues.size())];

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
      }

      total_buffer_size += buffer_size;
    }
  }

  // Close all lists. If the number of queues is less than the number of
  // destinations, copies are batched.
  queue_index_iter = 0;
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

      if (divide_buffers) {
        for (size_t e = 0; e < num_engines; e++) {
          SUCCESS_OR_TERMINATE(zeCommandListClose(
              ze_peer_devices[local_device_id].engines[queues[e]].second));
          SUCCESS_OR_TERMINATE(zeCommandListClose(
              ze_peer_devices[remote_device_id].engines[queues[e]].second));
        }
      } else {
        uint32_t queue_index =
            queues[queue_index_iter++ % static_cast<uint32_t>(queues.size())];

        SUCCESS_OR_TERMINATE(zeCommandListClose(
            ze_peer_devices[local_device_id].engines[queue_index].second));
        SUCCESS_OR_TERMINATE(zeCommandListClose(
            ze_peer_devices[remote_device_id].engines[queue_index].second));
      }
    }
  }

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    queue_index_iter = 0;
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

        SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

        if (divide_buffers) {
          for (size_t e = 0; e < num_engines; e++) {
            SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
                ze_peer_devices[local_device_id].engines[queues[e]].first, 1,
                &ze_peer_devices[local_device_id].engines[queues[e]].second,
                nullptr));
            SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
                ze_peer_devices[remote_device_id].engines[queues[e]].first, 1,
                &ze_peer_devices[remote_device_id].engines[queues[e]].second,
                nullptr));

            SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                ze_peer_devices[local_device_id].engines[queues[e]].first,
                std::numeric_limits<uint64_t>::max()));
            SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                ze_peer_devices[remote_device_id].engines[queues[e]].first,
                std::numeric_limits<uint64_t>::max()));
          }
        } else {
          uint32_t queue_index =
              queues[queue_index_iter++ % static_cast<uint32_t>(queues.size())];
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
        }

        SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      }
    }
  }

  Timer<std::chrono::microseconds::period> timer;
  std::vector<std::vector<Timer<std::chrono::microseconds::period>>> timers(
      ze_peer_devices.size());
  for (size_t i = 0U; i < timers.size(); i++) {
    timers[i].resize(ze_peer_devices.size());
  }

  std::vector<std::vector<long double>> times(ze_peer_devices.size());
  for (size_t i = 0U; i < times.size(); i++) {
    times[i].resize(ze_peer_devices.size());
    for (size_t j = 0U; j < times.size(); j++) {
      times[i][j] = 0;
    }
  }

  do {
    timer.start();
    for (uint32_t i = 0U; i < number_iterations; i++) {
      queue_index_iter = 0;
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

          if (divide_buffers) {
            for (size_t e = 0; e < num_engines; e++) {
              SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
                  ze_peer_devices[local_device_id].engines[queues[e]].first, 1,
                  &ze_peer_devices[local_device_id].engines[queues[e]].second,
                  nullptr));
              SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
                  ze_peer_devices[remote_device_id].engines[queues[e]].first, 1,
                  &ze_peer_devices[remote_device_id].engines[queues[e]].second,
                  nullptr));
            }
          } else {
            uint32_t queue_index = queues[queue_index_iter++ %
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

      queue_index_iter = 0;
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

          if (divide_buffers) {
            for (size_t e = 0; e < num_engines; e++) {
              SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                  ze_peer_devices[local_device_id].engines[queues[e]].first,
                  std::numeric_limits<uint64_t>::max()));
              SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                  ze_peer_devices[remote_device_id].engines[queues[e]].first,
                  std::numeric_limits<uint64_t>::max()));
            }
          } else {
            uint32_t queue_index = queues[queue_index_iter++ %
                                          static_cast<uint32_t>(queues.size())];
            SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                ze_peer_devices[local_device_id].engines[queue_index].first,
                std::numeric_limits<uint64_t>::max()));
            SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                ze_peer_devices[remote_device_id].engines[queue_index].first,
                std::numeric_limits<uint64_t>::max()));
          }
          timer.end();

          timers[local_device_id][remote_device_id].end();
          times[local_device_id][remote_device_id] +=
              timers[local_device_id][remote_device_id].period_minus_overhead();
        }
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    }

    queue_index_iter = 0;
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

        if (divide_buffers) {
          std::cout << "\tDevice " << local_device_id << " - Device "
                    << remote_device_id << " : ";
        } else {
          uint32_t queue_index =
              queues[queue_index_iter++ % static_cast<uint32_t>(queues.size())];

          std::cout << "\tDevice " << local_device_id << " - Device "
                    << remote_device_id << " using queue " << queue_index
                    << ": ";
        }

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
    std::vector<uint32_t> &local_device_ids, size_t buffer_size,
    bool divide_buffers) {

  size_t total_buffer_size = 0;

  size_t num_engines = queues.size();
  size_t chunk = buffer_size / num_engines;

  size_t queue_index_iter = 0;
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

      uint32_t device_id = local_device_id;
      if (use_queue_in_destination) {
        device_id = remote_device_id;
      }

      void *dst_buffer = ze_dst_buffers[remote_device_id];
      void *src_buffer = ze_src_buffers[local_device_id];
      if (transfer_type == PEER_READ) {
        dst_buffer = ze_dst_buffers[local_device_id];
        src_buffer = ze_src_buffers[remote_device_id];
      }

      if (divide_buffers) {
        for (size_t e = 0; e < num_engines; e++) {
          ze_command_list_handle_t command_list =
              ze_peer_devices[device_id].engines[queues[e]].second;
          SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
              command_list,
              reinterpret_cast<void *>(reinterpret_cast<uint64_t>(dst_buffer) +
                                       e * chunk),
              reinterpret_cast<void *>(reinterpret_cast<uint64_t>(src_buffer) +
                                       e * chunk),
              chunk, nullptr, 1, &event));
        }
      } else {
        uint32_t queue_index =
            queues[queue_index_iter++ % static_cast<uint32_t>(queues.size())];
        ze_command_list_handle_t command_list =
            ze_peer_devices[device_id].engines[queue_index].second;
        SUCCESS_OR_TERMINATE(
            zeCommandListAppendMemoryCopy(command_list, dst_buffer, src_buffer,
                                          buffer_size, nullptr, 1, &event));
      }

      total_buffer_size += buffer_size;
    }
  }

  // Close all lists. If the number of queues is less than the number of
  // destinations, copies are batched.
  queue_index_iter = 0;
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

      uint32_t device_id = local_device_id;
      if (use_queue_in_destination) {
        device_id = remote_device_id;
      }

      if (divide_buffers) {
        for (size_t e = 0; e < num_engines; e++) {
          ze_command_list_handle_t command_list =
              ze_peer_devices[device_id].engines[queues[e]].second;
          SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
        }
      } else {
        uint32_t queue_index =
            queues[queue_index_iter++ % static_cast<uint32_t>(queues.size())];
        ze_command_list_handle_t command_list =
            ze_peer_devices[device_id].engines[queue_index].second;
        SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
      }
    }
  }

  /* Warm up */
  queue_index_iter = 0;
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
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

        uint32_t device_id = local_device_id;
        if (use_queue_in_destination) {
          device_id = remote_device_id;
        }

        if (divide_buffers) {
          for (size_t e = 0; e < num_engines; e++) {
            ze_command_list_handle_t command_list =
                ze_peer_devices[device_id].engines[queues[e]].second;
            ze_command_queue_handle_t command_queue =
                ze_peer_devices[device_id].engines[queues[e]].first;
            SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
                command_queue, 1, &command_list, nullptr));
          }

          SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

          for (size_t e = 0; e < num_engines; e++) {
            ze_command_queue_handle_t command_queue =
                ze_peer_devices[device_id].engines[queues[e]].first;
            SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                command_queue, std::numeric_limits<uint64_t>::max()));
          }
        } else {
          uint32_t queue_index =
              queues[queue_index_iter++ % static_cast<uint32_t>(queues.size())];
          ze_command_list_handle_t command_list =
              ze_peer_devices[device_id].engines[queue_index].second;
          ze_command_queue_handle_t command_queue =
              ze_peer_devices[device_id].engines[queue_index].first;

          SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
              command_queue, 1, &command_list, nullptr));
          SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
          SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
              command_queue, std::numeric_limits<uint64_t>::max()));
        }

        SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      }
    }
  }

  Timer<std::chrono::microseconds::period> timer;
  std::vector<std::vector<Timer<std::chrono::microseconds::period>>> timers(
      ze_peer_devices.size());
  for (size_t i = 0U; i < timers.size(); i++) {
    timers[i].resize(ze_peer_devices.size());
  }

  std::vector<std::vector<long double>> times(ze_peer_devices.size());
  for (size_t i = 0U; i < times.size(); i++) {
    times[i].resize(ze_peer_devices.size());
    for (size_t j = 0U; j < times.size(); j++) {
      times[i][j] = 0;
    }
  }

  do {
    timer.start();
    for (uint32_t i = 0U; i < number_iterations; i++) {
      queue_index_iter = 0;
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

          uint32_t device_id = local_device_id;
          if (use_queue_in_destination) {
            device_id = remote_device_id;
          }

          if (divide_buffers) {
            for (size_t e = 0; e < num_engines; e++) {
              ze_command_list_handle_t command_list =
                  ze_peer_devices[device_id].engines[queues[e]].second;
              ze_command_queue_handle_t command_queue =
                  ze_peer_devices[device_id].engines[queues[e]].first;
              SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
                  command_queue, 1, &command_list, nullptr));
            }
          } else {
            uint32_t queue_index = queues[queue_index_iter++ %
                                          static_cast<uint32_t>(queues.size())];
            ze_command_list_handle_t command_list =
                ze_peer_devices[device_id].engines[queue_index].second;
            ze_command_queue_handle_t command_queue =
                ze_peer_devices[device_id].engines[queue_index].first;

            SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
                command_queue, 1, &command_list, nullptr));
          }
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

      queue_index_iter = 0;
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

          uint32_t device_id = local_device_id;
          if (use_queue_in_destination) {
            device_id = remote_device_id;
          }

          if (divide_buffers) {
            for (size_t e = 0; e < num_engines; e++) {
              ze_command_queue_handle_t command_queue =
                  ze_peer_devices[device_id].engines[queues[e]].first;
              SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                  command_queue, std::numeric_limits<uint64_t>::max()));
            }
          } else {
            uint32_t queue_index = queues[queue_index_iter++ %
                                          static_cast<uint32_t>(queues.size())];
            ze_command_queue_handle_t command_queue =
                ze_peer_devices[device_id].engines[queue_index].first;

            SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
                command_queue, std::numeric_limits<uint64_t>::max()));
          }

          timer.end();

          timers[local_device_id][remote_device_id].end();
          times[local_device_id][remote_device_id] +=
              timers[local_device_id][remote_device_id].period_minus_overhead();
        }
      }
      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    }

    queue_index_iter = 0;
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

        if (divide_buffers) {
          std::cout << "\tDevice " << local_device_id << " - Device "
                    << remote_device_id << " : ";
        } else {
          uint32_t queue_index =
              queues[queue_index_iter++ % static_cast<uint32_t>(queues.size())];

          std::cout << "\tDevice " << local_device_id << " - Device "
                    << remote_device_id << " using queue " << queue_index
                    << ": ";
        }
        print_results(false, test_type, buffer_size,
                      times[local_device_id][remote_device_id]);
      }
    }
    print_results(false, test_type, total_buffer_size, timer);
  } while (run_continuously);

  for (auto local_device_id : local_device_ids) {
    if (divide_buffers) {
      for (size_t e = 0; e < num_engines; e++) {
        SUCCESS_OR_TERMINATE(zeCommandListReset(
            ze_peer_devices[local_device_id].engines[queues[e]].second));
      }
    } else {
      for (auto engine_pair : ze_peer_devices[local_device_id].engines) {
        SUCCESS_OR_TERMINATE(zeCommandListReset(engine_pair.second));
      }
    }
  }
  for (auto remote_device_id : remote_device_ids) {
    if (divide_buffers) {
      for (size_t e = 0; e < num_engines; e++) {
        SUCCESS_OR_TERMINATE(zeCommandListReset(
            ze_peer_devices[remote_device_id].engines[queues[e]].second));
      }
    } else {
      for (auto engine_pair : ze_peer_devices[remote_device_id].engines) {
        SUCCESS_OR_TERMINATE(zeCommandListReset(engine_pair.second));
      }
    }
  }
}

void ZePeer::bandwidth_latency_parallel_to_multiple_targets(
    peer_test_t test_type, peer_transfer_t transfer_type,
    size_t number_buffer_elements, std::vector<uint32_t> &remote_device_ids,
    std::vector<uint32_t> &local_device_ids, bool divide_buffers) {

  size_t buffer_size = 0;

  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size);

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  if (bidirectional) {
    perform_bidirectional_parallel_copy_to_multiple_targets(
        test_type, transfer_type, remote_device_ids, local_device_ids,
        buffer_size, divide_buffers);
  } else {
    perform_parallel_copy_to_multiple_targets(
        test_type, transfer_type, remote_device_ids, local_device_ids,
        buffer_size, divide_buffers);
  }

  if (validate_results) {
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
  }

  tear_down(remote_device_ids, local_device_ids);
}
