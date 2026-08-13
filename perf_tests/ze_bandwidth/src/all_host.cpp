/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/ze_api.h>

#include "../../common/include/common.hpp"
#include "ze_app.hpp"
#include "ze_bandwidth.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>

//---------------------------------------------------------------------
// Multi device host transfers. One device at a time is the target, every
// other device generates interfering traffic in the same direction, and only
// the target's bandwidth is reported. The interfering copies are twice the
// size so that they outlive the measured copy.
//
// The measured direction runs on the engine named for that direction, and the
// opposite direction, used only by the bidirectional variants, runs on the
// other one. The kernel driven bidirectional variant needs a single queue,
// because one split direction kernel already produces traffic both ways.
//---------------------------------------------------------------------

void ZeBandwidth::allocate_all_host_buffers(size_t size, uint32_t target,
                                            bool second_pair) {
  for (auto device_id : device_ids) {
    const size_t device_size = (device_id == target) ? size : 2 * size;

    benchmark->memoryAlloc(device_id, device_size, &device_buffers[device_id]);
    benchmark->memoryAllocHost(device_size, &host_buffers[device_id]);

    if (second_pair) {
      // The opposite direction always interferes, so it is never the measured
      // copy and always runs at the larger size.
      benchmark->memoryAlloc(device_id, 2 * size,
                             &device_buffers_bidir[device_id]);
      benchmark->memoryAllocHost(2 * size, &host_buffers_bidir[device_id]);
    }
  }
}

void ZeBandwidth::free_all_host_buffers(bool second_pair) {
  for (auto device_id : device_ids) {
    benchmark->memoryFree(device_buffers[device_id]);
    benchmark->memoryFree(host_buffers[device_id]);

    if (second_pair) {
      benchmark->memoryFree(device_buffers_bidir[device_id]);
      benchmark->memoryFree(host_buffers_bidir[device_id]);
    }
  }
}

//---------------------------------------------------------------------
// A device holds the measured pair and, for the bidirectional variants, the
// opposite direction pair. An interfering device holds twice the measured
// size, so it is the one that decides whether a transfer size fits.
//---------------------------------------------------------------------
bool ZeBandwidth::all_host_size_fits(size_t size, bool second_pair) {
  const size_t target_bytes = size + (second_pair ? 2 * size : 0);
  const size_t other_bytes = 2 * size + (second_pair ? 2 * size : 0);
  const size_t needed = (device_ids.size() > 1)
                            ? std::max(target_bytes, other_bytes)
                            : target_bytes;

  const std::string label = std::to_string(size) + " bytes";
  const size_t largest = (device_ids.size() > 1) ? 2 * size : size;
  if (!device_size_fits(largest, needed, label.c_str())) {
    return false;
  }

  // Every selected device holds a host allocation of its own, so the host
  // side grows with the device count while the device side does not.
  size_t host_bytes = 0;
  for (auto device_id : device_ids) {
    host_bytes +=
        (device_id == device_ids.front()) ? target_bytes : other_bytes;
  }

  return host_size_fits(host_bytes, label.c_str());
}

//---------------------------------------------------------------------
// Only the target device is verified. The interfering devices move data too,
// but their transfers are not the measurement and are left unchecked.
//---------------------------------------------------------------------
void ZeBandwidth::all_host_verify_target(uint32_t target, size_t bytes_moved,
                                         bool split_kernel,
                                         bool source_is_host) {
  if (split_kernel) {
    kernel_verify_split(bytes_moved, host_buffers[target],
                        device_buffers[target], target);
    return;
  }

  kernel_check_pattern(
      target, source_is_host ? device_buffers[target] : host_buffers[target],
      bytes_moved);
}

void ZeBandwidth::test_all_host(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  const bool source_is_host = all_host_source_is_host;
  const bool split_kernel = all_host_use_kernel && all_host_bidirectional;
  const bool second_queue = all_host_bidirectional && !split_kernel;
  const bool measured_on_h2d = all_host_measures_on_h2d_slot();

  // The measured direction runs on the engine named for it; the opposite
  // direction, when there is one, runs on the other engine.
  auto &measured_list = measured_on_h2d ? command_list : command_list1;
  auto &measured_queue = measured_on_h2d ? command_queue : command_queue1;
  auto &measured_event = measured_on_h2d ? event : event1;
  auto &other_list = measured_on_h2d ? command_list1 : command_list;
  auto &other_queue = measured_on_h2d ? command_queue1 : command_queue;
  auto &other_event = measured_on_h2d ? event1 : event;

  info() << std::endl;
  info() << (source_is_host ? "HOST-TO-ALL" : "ALL-TO-HOST")
         << (all_host_bidirectional ? " BIDIRECTIONAL" : "")
         << " BANDWIDTH AND LATENCY"
         << (all_host_use_kernel ? " VIA COMPUTE KERNEL" : "") << std::endl;
  info() << "Each row is the measured device; every other device generates "
            "interfering traffic at the same time."
         << std::endl;
  if (split_kernel) {
    info() << "Reported bandwidth is the aggregate of both directions on the "
              "measured device."
           << std::endl;
  }
  if (device_ids.size() < 2) {
    std::cerr << "NOTE: only one device selected, so there is no interfering "
                 "traffic and this is not a multi device measurement."
              << std::endl;
  }
  // Coverage compares the measured copy against the opposite direction on the
  // same device. Interference from the other devices cannot be timed this way,
  // because each device's timestamps come from its own free running clock.
  const bool report_coverage = second_queue && use_event_timer;

  if (csv_output) {
    print_csv_header(report_coverage ? "Coverage" : nullptr);
  }

  for (auto size : transfer_size) {
    if (!all_host_size_fits(size, second_queue)) {
      continue;
    }

    for (auto target : device_ids) {
      long double measured_nsec = 0.0L;
      long double overlap_nsec = 0.0L;
      size_t measured_bytes = 0;
      std::vector<uint32_t> silent_devices;

      allocate_all_host_buffers(size, target, second_queue);

      if (verify) {
        if (split_kernel) {
          kernel_fill_split_patterns(target, host_buffers[target],
                                     device_buffers[target], size);
        } else {
          kernel_fill_pattern(target,
                              source_is_host ? host_buffers[target]
                                             : device_buffers[target],
                              size);
        }
      }

      // Build the command lists once, then release them all together.
      for (auto device_id : device_ids) {
        const size_t device_size = (device_id == target) ? size : 2 * size;
        ze_event_handle_t signal =
            use_event_timer ? measured_event[device_id] : nullptr;

        if (split_kernel) {
          const size_t moved = append_kernel_copy(
              measured_list[device_id], device_id, true,
              host_buffers[device_id], device_buffers[device_id], device_size,
              signal);
          if (device_id == target) {
            measured_bytes = moved;
          } else if (moved == 0) {
            silent_devices.push_back(device_id);
          }
        } else {
          void *destination = source_is_host ? device_buffers[device_id]
                                             : host_buffers[device_id];
          void *source = source_is_host ? host_buffers[device_id]
                                        : device_buffers[device_id];

          if (all_host_use_kernel) {
            const size_t moved =
                append_kernel_copy(measured_list[device_id], device_id, false,
                                   destination, source, device_size, signal);
            if (device_id == target) {
              measured_bytes = moved;
            } else if (moved == 0) {
              silent_devices.push_back(device_id);
            }
          } else {
            SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
                measured_list[device_id], destination, source, device_size,
                signal, 1, &wait_event));
            if (device_id == target) {
              measured_bytes = device_size;
            }
          }
        }

        benchmark->commandListClose(measured_list[device_id]);

        if (second_queue) {
          // Opposite direction, always interfering, always the larger size.
          void *destination = source_is_host ? host_buffers_bidir[device_id]
                                             : device_buffers_bidir[device_id];
          void *source = source_is_host ? device_buffers_bidir[device_id]
                                        : host_buffers_bidir[device_id];
          SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
              other_list[device_id], destination, source, 2 * size,
              use_event_timer ? other_event[device_id] : nullptr, 1,
              &wait_event));
          benchmark->commandListClose(other_list[device_id]);
        }
      }

      if (measured_bytes == 0) {
        for (auto device_id : device_ids) {
          benchmark->commandListReset(measured_list[device_id]);
          if (second_queue) {
            benchmark->commandListReset(other_list[device_id]);
          }
        }
        free_all_host_buffers(second_queue);
        std::cerr << "\t[skipped " << size
                  << " bytes: smaller than the kernel granularity]\n";
        continue;
      }

      // An interfering device whose transfer rounded away contributes
      // nothing, and the row would otherwise still read as contended.
      if (!silent_devices.empty()) {
        std::cerr << "\tWARNING: no interfering traffic from device";
        for (auto device_id : silent_devices) {
          std::cerr << " " << device_id;
        }
        std::cerr << ", the transfer rounded below the kernel granularity\n";
      }

      Timer<std::chrono::nanoseconds::period> timer;

      for (uint32_t i = 0; i < warmup_iterations + number_iterations; i++) {
        for (auto device_id : device_ids) {
          if (use_event_timer) {
            SUCCESS_OR_TERMINATE(zeEventHostReset(measured_event[device_id]));
            if (second_queue) {
              SUCCESS_OR_TERMINATE(zeEventHostReset(other_event[device_id]));
            }
          }
          benchmark->commandQueueExecuteCommandList(
              measured_queue[device_id], 1, &measured_list[device_id]);
          if (second_queue) {
            benchmark->commandQueueExecuteCommandList(other_queue[device_id], 1,
                                                      &other_list[device_id]);
          }
        }

        timer.start();
        SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

        // The target is drained first so that its host timer is not extended
        // by the longer interfering copies on the other devices.
        benchmark->commandQueueSynchronize(measured_queue[target]);
        timer.end();

        for (auto device_id : device_ids) {
          if (device_id != target) {
            benchmark->commandQueueSynchronize(measured_queue[device_id]);
          }
          if (second_queue) {
            benchmark->commandQueueSynchronize(other_queue[device_id]);
          }
        }

        if (i >= warmup_iterations) {
          if (use_event_timer) {
            measured_nsec += event_time_nsec(measured_event[target], target);
          } else {
            measured_nsec += timer.period_minus_overhead();
          }
          if (report_coverage) {
            overlap_nsec += event_overlap_nsec(measured_event[target],
                                               other_event[target], target);
          }
        }

        SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
      }

      for (auto device_id : device_ids) {
        benchmark->commandListReset(measured_list[device_id]);
        if (second_queue) {
          benchmark->commandListReset(other_list[device_id]);
        }
      }

      if (verify) {
        all_host_verify_target(target, measured_bytes, split_kernel,
                               source_is_host);
      }

      free_all_host_buffers(second_queue);

      calculate_metrics(measured_nsec,
                        static_cast<long double>(measured_bytes) *
                            number_iterations,
                        total_bandwidth, total_latency);
      long double coverage = 0.0L;
      if (report_coverage && measured_nsec > 0.0L) {
        coverage = overlap_nsec / measured_nsec;
      }
      print_results(measured_bytes, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(target) + " ",
                    report_coverage ? "coverage" : nullptr, coverage);
      if (report_coverage && coverage < 0.95L) {
        std::cerr << "\tWARNING: the measured copy was contended by the "
                     "opposite direction for only "
                  << std::setprecision(3) << coverage << " of its duration\n";
      }
    }
  }

  info() << "Measured direction: "
         << (source_is_host ? "Host->Device" : "Device->Host")
         << (split_kernel ? " (aggregate of both directions)" : "")
         << std::endl;
}
