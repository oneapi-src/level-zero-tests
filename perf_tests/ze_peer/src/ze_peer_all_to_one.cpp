/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

namespace {
struct PeerCopy {
  uint32_t submitter = 0;
  uint32_t src_device = 0;
  uint32_t dst_device = 0;
  void *src = nullptr;
  void *dst = nullptr;
  ze_command_list_handle_t command_list = nullptr;
  ze_event_pool_handle_t timestamp_pool = nullptr;
  ze_event_handle_t timestamp_event = nullptr;
  long double time_us = 0;
};
} // namespace

bool ZePeer::bandwidth_all_to_one(peer_transfer_t transfer_type,
                                  size_t buffer_size, uint32_t one_device_id,
                                  const std::vector<uint32_t> &all_device_ids) {
  const bool write = transfer_type == PEER_WRITE;

  std::vector<PeerCopy> copies(all_device_ids.size());
  std::unordered_map<uint32_t, size_t> device_buffers;
  for (size_t i = 0; i < all_device_ids.size(); i++) {
    PeerCopy &copy = copies[i];
    const uint32_t all_device_id = all_device_ids[i];
    copy.submitter = one_to_all ? one_device_id : all_device_id;
    const uint32_t peer = one_to_all ? all_device_id : one_device_id;
    copy.src_device = write ? copy.submitter : peer;
    copy.dst_device = write ? peer : copy.submitter;
    device_buffers[copy.src_device]++;
    device_buffers[copy.dst_device]++;
  }
  check_sizes(buffer_size, device_buffers);

  const size_t host_size = std::min(buffer_size, host_chunk_size);
  benchmark->memoryAllocHost(host_size,
                             reinterpret_cast<void **>(&ze_host_buffer));
  benchmark->memoryAllocHost(
      host_size, reinterpret_cast<void **>(&ze_host_validate_buffer));

  // Copies submitted by the same device take the engines of -u in turn.
  std::unordered_map<uint32_t, size_t> copies_of_device;
  for (PeerCopy &copy : copies) {
    const uint32_t engine =
        queues[copies_of_device[copy.submitter]++ % queues.size()];
    if (engine >= ze_peer_devices[copy.submitter].engines.size()) {
      std::cerr << "ERROR: engine " << engine << " is not available on device "
                << copy.submitter << " (see -q)\n";
      exit(1);
    }
    set_up_kernel_copy(copy.submitter, engine, buffer_size);
    const auto ordinal =
        ze_peer_devices[copy.submitter].engine_ordinals[engine];
    benchmark->immediateCommandListCreate(copy.submitter, ordinal.first,
                                          ordinal.second, &copy.command_list);

    benchmark->memoryAlloc(copy.src_device, buffer_size, &copy.src);
    benchmark->memoryAlloc(copy.dst_device, buffer_size, &copy.dst);
    initialize_src_buffer_immediate(
        ze_peer_devices[copy.src_device].engines[0].second, copy.src,
        ze_host_buffer, buffer_size);

    if (use_event_timer) {
      create_timestamp_event(copy.submitter, copy.timestamp_pool,
                             copy.timestamp_event);
    }
  }

  // Every copy waits for the host to signal the gate event, so the copies
  // start together and their submission is not timed.
  auto run_copies = [&]() -> long double {
    for (PeerCopy &copy : copies) {
      append_copy(copy.command_list, copy.submitter, copy.dst, copy.src,
                  buffer_size, copy.timestamp_event, 1, &event);
    }
    Timer<std::chrono::microseconds::period> timer;
    timer.start();
    SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
    for (PeerCopy &copy : copies) {
      SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
          copy.command_list, std::numeric_limits<uint64_t>::max()));
    }
    timer.end();
    SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    for (PeerCopy &copy : copies) {
      if (copy.timestamp_event) {
        copy.time_us +=
            timestamp_event_time_us(copy.timestamp_event, copy.submitter);
        SUCCESS_OR_TERMINATE(zeEventHostReset(copy.timestamp_event));
      }
    }
    return timer.period_minus_overhead();
  };

  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    run_copies();
  }

  do {
    for (PeerCopy &copy : copies) {
      copy.time_us = 0;
    }
    long double total_time_us = 0;
    for (uint32_t i = 0U; i < number_iterations; i++) {
      total_time_us += run_copies();
    }

    const long double iterations = static_cast<long double>(number_iterations);
    print_value(PEER_BANDWIDTH, buffer_size,
                copies.size() *
                    bandwidth_gbps(buffer_size, total_time_us / iterations),
                "");
    if (use_event_timer) {
      for (const PeerCopy &copy : copies) {
        print_value(PEER_BANDWIDTH, buffer_size,
                    bandwidth_gbps(buffer_size, copy.time_us / iterations),
                    "Device( " + std::to_string(copy.src_device) +
                        " )->Device( " + std::to_string(copy.dst_device) +
                        " )");
      }
    }
  } while (run_continuously);

  bool passed = true;
  for (PeerCopy &copy : copies) {
    if (validate_results &&
        !validate_buffer_immediate(
            ze_peer_devices[copy.dst_device].engines[0].second,
            ze_host_validate_buffer, copy.dst, ze_host_buffer, buffer_size)) {
      passed = false;
    }
    if (copy.timestamp_event) {
      SUCCESS_OR_TERMINATE(zeEventDestroy(copy.timestamp_event));
      SUCCESS_OR_TERMINATE(zeEventPoolDestroy(copy.timestamp_pool));
    }
    benchmark->commandListDestroy(copy.command_list);
    benchmark->memoryFree(copy.src);
    benchmark->memoryFree(copy.dst);
  }
  benchmark->memoryFree(ze_host_buffer);
  benchmark->memoryFree(ze_host_validate_buffer);
  ze_host_buffer = nullptr;
  ze_host_validate_buffer = nullptr;

  return passed;
}
