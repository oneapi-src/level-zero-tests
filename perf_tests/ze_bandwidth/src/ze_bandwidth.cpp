/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/ze_api.h>

#include "../../common/include/common.hpp"
#include "ze_app.hpp"
#include "ze_bandwidth.hpp"

#include <assert.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

//---------------------------------------------------------------------
// Where the device sits on the host, read from sysfs through its PCI
// address. Reported so that a host side asymmetry between sockets shows up
// in the output instead of having to be inferred from the bandwidth: host
// allocations carry no NUMA hint, so a device on a remote node crosses the
// interconnect on every transfer.
//---------------------------------------------------------------------
struct DeviceLocation {
  std::string pci_address;
  int numa_node;
};

static DeviceLocation device_location(ze_device_handle_t device) {
  DeviceLocation location = {"", -1};

#ifdef __linux__
  ze_pci_ext_properties_t pci = {};
  pci.stype = ZE_STRUCTURE_TYPE_PCI_EXT_PROPERTIES;
  if (zeDevicePciGetPropertiesExt(device, &pci) != ZE_RESULT_SUCCESS) {
    return location;
  }

  char address[16] = {};
  std::snprintf(address, sizeof(address), "%04x:%02x:%02x.%x",
                pci.address.domain, pci.address.bus, pci.address.device,
                pci.address.function);
  location.pci_address = address;

  const std::string path =
      "/sys/bus/pci/devices/" + location.pci_address + "/numa_node";
  std::ifstream node_file(path);
  int node = -1;
  if (node_file >> node) {
    location.numa_node = node;
  }
#else
  (void)device;
#endif

  return location;
}

// Leave room for the driver's own allocations rather than filling the device.
static const uint64_t device_memory_headroom_percent = 80;

//---------------------------------------------------------------------
// Queue group classification. These mirror the rule already used by the
// engine listing below: a group advertising compute is a CCS, a group
// advertising copy without compute is a BCS.
//---------------------------------------------------------------------
static bool queue_is_ccs(ze_command_queue_group_property_flags_t flags) {
  return (flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) != 0;
}

static bool queue_is_bcs(ze_command_queue_group_property_flags_t flags) {
  return (flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0 &&
         (flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) != 0;
}

//---------------------------------------------------------------------
// Flatten every usable queue of every usable group into a named list, so
// that CCS0/BCS1 style names can be mapped to an ordinal and index pair.
//---------------------------------------------------------------------
static std::vector<EngineId> enumerate_engines(
    const std::vector<ze_command_queue_group_properties_t> &properties) {
  std::vector<EngineId> engines;
  uint32_t ccs_count = 0;
  uint32_t bcs_count = 0;

  for (uint32_t ordinal = 0; ordinal < properties.size(); ordinal++) {
    const bool is_ccs = queue_is_ccs(properties[ordinal].flags);
    const bool is_bcs = queue_is_bcs(properties[ordinal].flags);
    if (!is_ccs && !is_bcs) {
      continue;
    }

    for (uint32_t index = 0; index < properties[ordinal].numQueues; index++) {
      uint32_t &counter = is_ccs ? ccs_count : bcs_count;
      EngineId engine = {(is_ccs ? "CCS" : "BCS") + std::to_string(counter),
                         ordinal, index};
      engines.push_back(engine);
      counter++;
    }
  }

  return engines;
}

static std::string to_upper(const std::string &text) {
  std::string result = text;
  for (auto &character : result) {
    character =
        static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
  }
  return result;
}

static void print_engine_list(const std::vector<EngineId> &engines) {
  std::cerr << " available engines:";
  for (const auto &engine : engines) {
    std::cerr << " " << engine.name;
  }
  std::cerr << std::endl;
}

//---------------------------------------------------------------------
// Accepts bcs, bcs0, BCS1, ccs0 and so on. A bare class name selects the
// first engine of that class.
//---------------------------------------------------------------------
static bool find_engine(const std::string &name,
                        const std::vector<EngineId> &engines, uint32_t &ordinal,
                        uint32_t &index) {
  std::string wanted = to_upper(name);
  if (wanted == "BCS" || wanted == "CCS") {
    wanted += "0";
  }

  for (const auto &engine : engines) {
    if (engine.name == wanted) {
      ordinal = engine.ordinal;
      index = engine.index;
      return true;
    }
  }

  return false;
}

static std::string engine_label(const std::vector<EngineId> &engines,
                                uint32_t ordinal, uint32_t index) {
  for (const auto &engine : engines) {
    if (engine.ordinal == ordinal && engine.index == index) {
      return engine.name;
    }
  }
  return "unknown";
}

ZeBandwidth::ZeBandwidth() {
  benchmark = new ZeApp();

  benchmark->allDevicesInit();
}

ZeBandwidth::~ZeBandwidth() {
  if (!query_engines) {
    for (auto device_id : device_ids) {
      if (device_id < command_list_verify.size() &&
          command_list_verify[device_id]) {
        benchmark->commandListDestroy(command_list_verify[device_id]);
      }
      if (device_id < command_queue_verify.size() &&
          command_queue_verify[device_id]) {
        benchmark->commandQueueDestroy(command_queue_verify[device_id]);
      }
    }
    for (auto device_id : device_ids) {
      if (command_list[device_id]) {
        benchmark->commandListDestroy(command_list[device_id]);
      }
      if (command_list1[device_id]) {
        benchmark->commandListDestroy(command_list1[device_id]);
      }
      if (command_queue[device_id]) {
        benchmark->commandQueueDestroy(command_queue[device_id]);
      }
      if (command_queue1[device_id]) {
        benchmark->commandQueueDestroy(command_queue1[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventDestroy(event[device_id]));
      SUCCESS_OR_TERMINATE(zeEventDestroy(event1[device_id]));
    }

    SUCCESS_OR_TERMINATE(zeEventDestroy(wait_event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(event_pool));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(wait_event_pool));
  }

  benchmark->singleDeviceCleanup();

  delete benchmark;
}

bool ZeBandwidth::any_kernel_test(void) const {
  return run_host2dev_kernel || run_dev2host_kernel ||
         run_bidirectional_kernel || (run_all_host && all_host_use_kernel);
}

//---------------------------------------------------------------------
// GPU timestamp helpers. timerResolution is nanoseconds per cycle for
// ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, which is the stype requested in
// ze_bandwidth_query_engines(); ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2
// reports cycles per second instead and would need a division here.
//---------------------------------------------------------------------
long double ZeBandwidth::ticks_to_nsec(uint64_t ticks, uint32_t device_id) {
  return static_cast<long double>(ticks) *
         static_cast<long double>(device_properties[device_id].timerResolution);
}

long double ZeBandwidth::timestamp_delta_nsec(uint64_t start, uint64_t end,
                                              uint32_t device_id) {
  const uint32_t valid_bits =
      device_properties[device_id].kernelTimestampValidBits;
  const uint64_t max_value =
      (valid_bits >= 64) ? ~0ULL : ~(~0ULL << valid_bits);
  const uint64_t ticks =
      (end >= start) ? (end - start) : ((max_value - start) + end + 1);

  return ticks_to_nsec(ticks, device_id);
}

void ZeBandwidth::event_timestamps(ze_event_handle_t event_handle,
                                   uint64_t &start, uint64_t &end) {
  ze_kernel_timestamp_result_t timestamp = {};
  SUCCESS_OR_TERMINATE(zeEventQueryKernelTimestamp(event_handle, &timestamp));

  // The global domain is a free running device clock, so it is the only one
  // comparable between two different engines.
  start = timestamp.global.kernelStart;
  end = timestamp.global.kernelEnd;
}

//---------------------------------------------------------------------
// How long two operations on one device were in flight together. Both
// timestamps must come from the same device: the global domain is a free
// running per device clock, so spans from different devices cannot be
// compared and cross device interference cannot be measured this way.
//---------------------------------------------------------------------
long double ZeBandwidth::event_overlap_nsec(ze_event_handle_t first,
                                            ze_event_handle_t second,
                                            uint32_t device_id) {
  uint64_t start0 = 0;
  uint64_t end0 = 0;
  uint64_t start1 = 0;
  uint64_t end1 = 0;
  event_timestamps(first, start0, end0);
  event_timestamps(second, start1, end1);

  const int64_t end0_offset = tick_offset(end0, start0, device_id);
  const int64_t start1_offset = tick_offset(start1, start0, device_id);
  const int64_t end1_offset = tick_offset(end1, start0, device_id);

  // A pair the device could not have produced is dropped, not guessed.
  if (end0_offset < 0 || end1_offset < start1_offset) {
    return 0.0L;
  }

  const int64_t zero = 0;
  const int64_t overlap = std::max(zero, std::min(end0_offset, end1_offset) -
                                             std::max(zero, start1_offset));
  return ticks_to_nsec(static_cast<uint64_t>(overlap), device_id);
}

long double ZeBandwidth::event_time_nsec(ze_event_handle_t event_handle,
                                         uint32_t device_id) {
  uint64_t start = 0;
  uint64_t end = 0;
  event_timestamps(event_handle, start, end);
  return timestamp_delta_nsec(start, end, device_id);
}

void ZeBandwidth::calculate_metrics(
    long double total_time_nsec,     /* Units in nanoseconds */
    long double total_data_transfer, /* Units in bytes */
    long double &total_bandwidth, long double &total_latency,
    uint32_t iterations) {
  long double total_time_s;
  const uint32_t divisor = (iterations > 0) ? iterations : number_iterations;

  total_time_s = total_time_nsec / 1e9;
  total_bandwidth = (total_data_transfer / total_time_s) / ONE_GB;
  total_latency = total_time_nsec / (1e3 * divisor);
}

std::ostream &ZeBandwidth::info(void) {
  return csv_output ? std::cerr : std::cout;
}

void ZeBandwidth::print_csv_header(const char *extra_column) {
  csv_extra_column = (extra_column != nullptr);

  std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)";
  if (csv_extra_column) {
    std::cout << "," << extra_column;
  }
  std::cout << std::endl;
}

void ZeBandwidth::print_results(size_t buffer_size, long double total_bandwidth,
                                long double total_latency,
                                std::string direction_string,
                                const char *extra_label,
                                long double extra_value) {
  if (csv_output) {
    std::cout << buffer_size << "," << std::setprecision(6) << total_bandwidth
              << "," << std::setprecision(2) << total_latency;
    // Keep the column count fixed: rows with no value for the extra column
    // still emit the separator so that positional parsers stay aligned.
    if (csv_extra_column) {
      std::cout << ",";
      if (extra_label != nullptr) {
        std::cout << std::setprecision(3) << extra_value;
      }
    }
    std::cout << std::endl;
  } else {
    std::cout << direction_string << std::fixed << std::setw(10) << buffer_size
              << "]:  BW = " << std::setw(9) << std::setprecision(6)
              << total_bandwidth << " GBPS  Latency = " << std::setw(9)
              << std::setprecision(2) << total_latency << " usec";
    if (extra_label != nullptr) {
      std::cout << "  " << extra_label << " = " << std::setprecision(3)
                << extra_value;
    }
    std::cout << std::endl;
  }
}

//---------------------------------------------------------------------
// Stages one copy through the verification bundle of the device that owns the
// memory. Every verification path goes through here, so none of them can end
// up issuing a copy on another device's engine.
//---------------------------------------------------------------------
void ZeBandwidth::run_verify_copy(uint32_t device_id, void *destination,
                                  void *source, size_t bytes) {
  ze_command_list_handle_t list = command_list_verify[device_id];
  ze_command_queue_handle_t queue = command_queue_verify[device_id];

  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
      list, destination, source, bytes, nullptr, 0, nullptr));
  benchmark->commandListClose(list);
  benchmark->commandQueueExecuteCommandList(queue, 1, &list);
  benchmark->commandQueueSynchronize(queue);
  benchmark->commandListReset(list);
}

void ZeBandwidth::transfer_size_test(
    size_t size, std::vector<void *> &destination_buffer,
    std::vector<void *> &source_buffer,
    std::vector<long double> &device_times_nsec, long double &total_time_nsec,
    bool use_h2d_slot) {
  size_t element_size = sizeof(uint8_t);
  size_t buffer_size = element_size * size;

  // The measured direction picks the slot, so --h2dEngine and --d2hEngine
  // always name the engine that carries the copy they are named for.
  auto &queue = use_h2d_slot ? command_queue : command_queue1;
  auto &list = use_h2d_slot ? command_list : command_list1;

  if (verify) {
    benchmark->memoryAllocHost(buffer_size, &host_buffer_verify1);
    char *host_buffer_verify_char1 = static_cast<char *>(host_buffer_verify1);
    for (uint32_t i = 0; i < buffer_size; i++) {
      host_buffer_verify_char1[i] = static_cast<char>(i);
    }

    for (auto device_id : device_ids) {
      run_verify_copy(device_id, source_buffer[device_id], host_buffer_verify1,
                      buffer_size);
    }
  }

  if (use_immediate_command_list == false) {
    for (auto device_id : device_ids) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          list[device_id], destination_buffer[device_id],
          source_buffer[device_id], buffer_size,
          use_event_timer ? event[device_id] : nullptr, 1, &wait_event));
    }

    for (auto device_id : device_ids) {
      benchmark->commandListClose(list[device_id]);
    }

    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      for (auto device_id : device_ids) {
        if (use_event_timer) {
          SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        }
        benchmark->commandQueueExecuteCommandList(queue[device_id], 1,
                                                  &list[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        benchmark->commandQueueSynchronize(queue[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    Timer<std::chrono::nanoseconds::period> timer;
    std::vector<Timer<std::chrono::nanoseconds::period>> timers(
        benchmark->_devices.size());

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      for (auto device_id : device_ids) {
        // A timestamp event re-signalled without a reset reports a stale
        // value, so it has to be cleared on every iteration.
        if (use_event_timer) {
          SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        }
        benchmark->commandQueueExecuteCommandList(queue[device_id], 1,
                                                  &list[device_id]);
      }

      for (auto device_id : device_ids) {
        timers[device_id].start();
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        benchmark->commandQueueSynchronize(queue[device_id]);

        timers[device_id].end();
        if (use_event_timer) {
          device_times_nsec[device_id] +=
              event_time_nsec(event[device_id], device_id);
        } else {
          device_times_nsec[device_id] +=
              timers[device_id].period_minus_overhead();
        }
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();

    for (auto device_id : device_ids) {
      benchmark->commandListReset(list[device_id]);
    }
  } else {
    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            list[device_id], destination_buffer[device_id],
            source_buffer[device_id], buffer_size, event[device_id], 1,
            &wait_event));
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event[device_id], UINT64_MAX));
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    Timer<std::chrono::nanoseconds::period> timer;
    std::vector<Timer<std::chrono::nanoseconds::period>> timers(
        benchmark->_devices.size());

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            list[device_id], destination_buffer[device_id],
            source_buffer[device_id], buffer_size, event[device_id], 1,
            &wait_event));
      }

      for (auto device_id : device_ids) {
        timers[device_id].start();
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event[device_id], UINT64_MAX));

        timers[device_id].end();
        if (use_event_timer) {
          device_times_nsec[device_id] +=
              event_time_nsec(event[device_id], device_id);
        } else {
          device_times_nsec[device_id] +=
              timers[device_id].period_minus_overhead();
        }
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();
  }

  if (verify) {
    uint32_t number_of_errors = 0;
    benchmark->memoryAllocHost(buffer_size, &host_buffer_verify);

    for (auto device_id : device_ids) {
      run_verify_copy(device_id, host_buffer_verify,
                      destination_buffer[device_id], buffer_size);

      char *host_buffer_verify_char = static_cast<char *>(host_buffer_verify);
      char *host_buffer_verify_char1 = static_cast<char *>(host_buffer_verify1);
      for (uint32_t i = 0; i < buffer_size; i++) {
        if (host_buffer_verify_char[i] != host_buffer_verify_char1[i]) {
          number_of_errors++;
        }
      }
    }

    benchmark->memoryFree(host_buffer_verify1);
    benchmark->memoryFree(host_buffer_verify);

    if (number_of_errors > 0) {
      throw std::runtime_error("Host memory verification failed ");
    } else {
      info() << "Verification successful\n";
    }
  }
}

//---------------------------------------------------------------------
// Samples that survived the wrap check, reporting any that did not. Deriving
// a figure from a short set of samples against the full iteration count would
// raise the reported bandwidth for every sample that was thrown away.
//---------------------------------------------------------------------
uint32_t ZeBandwidth::report_dropped_samples(const BidirTimings &timings,
                                             uint32_t device_id) {
  const uint32_t kept = timings.samples[device_id];
  if (kept < number_iterations) {
    std::cerr << "\tNOTE: [Device " << device_id << "] "
              << (number_iterations - kept) << " of " << number_iterations
              << " samples were dropped on a wrapped timestamp\n";
  }
  return kept;
}

//---------------------------------------------------------------------
// A tick expressed relative to a reference tick from the same device. The
// counter is kernelTimestampValidBits wide and wraps; a single sample lasts
// milliseconds against a wrap period of minutes, so every true difference
// within one sample is far below half the counter range. Reading the modular
// difference as signed therefore recovers the correct order and magnitude
// even when the counter rolled over between the two operations, which a raw
// comparison of two individually unwrapped intervals cannot do.
//---------------------------------------------------------------------
int64_t ZeBandwidth::tick_offset(uint64_t tick, uint64_t reference,
                                 uint32_t device_id) {
  const uint32_t valid_bits =
      device_properties[device_id].kernelTimestampValidBits;
  if (valid_bits == 0 || valid_bits >= 64) {
    return static_cast<int64_t>(tick - reference);
  }

  const uint64_t range = 1ULL << valid_bits;
  const uint64_t difference = (tick - reference) & (range - 1);
  return (difference > (range >> 1)) ? -static_cast<int64_t>(range - difference)
                                     : static_cast<int64_t>(difference);
}

void ZeBandwidth::accumulate_bidir_sample(
    uint32_t device_id, Timer<std::chrono::nanoseconds::period> &timer0,
    Timer<std::chrono::nanoseconds::period> &timer1, BidirTimings &timings) {
  if (use_event_timer) {
    uint64_t start0 = 0;
    uint64_t end0 = 0;
    uint64_t start1 = 0;
    uint64_t end1 = 0;
    event_timestamps(event[device_id], start0, end0);
    event_timestamps(event1[device_id], start1, end1);

    // Everything is measured from direction 0's start, so a rollover anywhere
    // inside the sample cancels instead of producing a span close to the
    // width of the counter.
    const int64_t end0_offset = tick_offset(end0, start0, device_id);
    const int64_t start1_offset = tick_offset(start1, start0, device_id);
    const int64_t end1_offset = tick_offset(end1, start0, device_id);

    // Neither direction can end before it started. Anything else is a
    // timestamp pair the device could not have produced, so the whole sample
    // is dropped rather than guessed.
    if (end0_offset < 0 || end1_offset < start1_offset) {
      return;
    }

    const int64_t zero = 0;
    const int64_t span =
        std::max(end0_offset, end1_offset) - std::min(zero, start1_offset);
    const int64_t overlap = std::max(zero, std::min(end0_offset, end1_offset) -
                                               std::max(zero, start1_offset));

    timings.direction0_nsec[device_id] +=
        ticks_to_nsec(static_cast<uint64_t>(end0_offset), device_id);
    timings.direction1_nsec[device_id] += ticks_to_nsec(
        static_cast<uint64_t>(end1_offset - start1_offset), device_id);
    timings.overlap_nsec[device_id] +=
        ticks_to_nsec(static_cast<uint64_t>(overlap), device_id);
    timings.span_nsec[device_id] +=
        ticks_to_nsec(static_cast<uint64_t>(span), device_id);
    timings.samples[device_id]++;
  } else {
    const long double time0 = timer0.period_minus_overhead();
    const long double time1 = timer1.period_minus_overhead();

    timings.direction0_nsec[device_id] += time0;
    timings.direction1_nsec[device_id] += time1;
    timings.span_nsec[device_id] += std::max(time0, time1);
    timings.samples[device_id]++;
  }
}

void ZeBandwidth::transfer_bidir_size_test(
    size_t size, size_t size1, std::vector<void *> &destination_buffer,
    std::vector<void *> &source_buffer,
    std::vector<void *> &destination_buffer1,
    std::vector<void *> &source_buffer1, BidirTimings &timings,
    long double &total_time_nsec, bool use_h2d_slot) {
  size_t element_size = sizeof(uint8_t);
  size_t buffer_size = element_size * size;
  size_t buffer_size1 = element_size * size1;

  // Direction 0 is the measured one, so it takes the slot named for its
  // direction and the interfering copy takes the other. Events stay bound to
  // the direction rather than to the slot: accumulate_bidir_sample reads
  // event/event1 as direction 0/1.
  auto &queue = use_h2d_slot ? command_queue : command_queue1;
  auto &list = use_h2d_slot ? command_list : command_list1;
  auto &queue_interfering = use_h2d_slot ? command_queue1 : command_queue;
  auto &list_interfering = use_h2d_slot ? command_list1 : command_list;

  // Direction 1 carries the interfering traffic and is never shorter than
  // direction 0, so draining direction 0 first keeps its timer honest.
  if (use_immediate_command_list == false) {
    for (auto device_id : device_ids) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          list[device_id], destination_buffer[device_id],
          source_buffer[device_id], buffer_size,
          use_event_timer ? event[device_id] : nullptr, 1, &wait_event));
    }

    for (auto device_id : device_ids) {
      benchmark->commandListClose(list[device_id]);
    }

    for (auto device_id : device_ids) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          list_interfering[device_id], destination_buffer1[device_id],
          source_buffer1[device_id], buffer_size1,
          use_event_timer ? event1[device_id] : nullptr, 1, &wait_event));
    }

    for (auto device_id : device_ids) {
      benchmark->commandListClose(list_interfering[device_id]);
    }

    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      for (auto device_id : device_ids) {
        if (use_event_timer) {
          SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
          SUCCESS_OR_TERMINATE(zeEventHostReset(event1[device_id]));
        }
        benchmark->commandQueueExecuteCommandList(queue[device_id], 1,
                                                  &list[device_id]);
        benchmark->commandQueueExecuteCommandList(
            queue_interfering[device_id], 1, &list_interfering[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        benchmark->commandQueueSynchronize(queue[device_id]);
        benchmark->commandQueueSynchronize(queue_interfering[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    Timer<std::chrono::nanoseconds::period> timer;
    std::vector<Timer<std::chrono::nanoseconds::period>> timers(
        benchmark->_devices.size());
    std::vector<Timer<std::chrono::nanoseconds::period>> timers1(
        benchmark->_devices.size());

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      for (auto device_id : device_ids) {
        if (use_event_timer) {
          SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
          SUCCESS_OR_TERMINATE(zeEventHostReset(event1[device_id]));
        }
        benchmark->commandQueueExecuteCommandList(queue[device_id], 1,
                                                  &list[device_id]);
        benchmark->commandQueueExecuteCommandList(
            queue_interfering[device_id], 1, &list_interfering[device_id]);
      }

      for (auto device_id : device_ids) {
        timers[device_id].start();
        timers1[device_id].start();
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        benchmark->commandQueueSynchronize(queue[device_id]);
        timers[device_id].end();
        benchmark->commandQueueSynchronize(queue_interfering[device_id]);
        timers1[device_id].end();

        accumulate_bidir_sample(device_id, timers[device_id],
                                timers1[device_id], timings);
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();

    for (auto device_id : device_ids) {
      benchmark->commandListReset(list[device_id]);
      benchmark->commandListReset(list_interfering[device_id]);
    }
  } else {
    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        SUCCESS_OR_TERMINATE(zeEventHostReset(event1[device_id]));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            list[device_id], destination_buffer[device_id],
            source_buffer[device_id], buffer_size, event[device_id], 1,
            &wait_event));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            list_interfering[device_id], destination_buffer1[device_id],
            source_buffer1[device_id], buffer_size1, event1[device_id], 1,
            &wait_event));
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event[device_id], UINT64_MAX));
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event1[device_id], UINT64_MAX));
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    Timer<std::chrono::nanoseconds::period> timer;
    std::vector<Timer<std::chrono::nanoseconds::period>> timers(
        benchmark->_devices.size());
    std::vector<Timer<std::chrono::nanoseconds::period>> timers1(
        benchmark->_devices.size());

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        SUCCESS_OR_TERMINATE(zeEventHostReset(event1[device_id]));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            list[device_id], destination_buffer[device_id],
            source_buffer[device_id], buffer_size, event[device_id], 1,
            &wait_event));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            list_interfering[device_id], destination_buffer1[device_id],
            source_buffer1[device_id], buffer_size1, event1[device_id], 1,
            &wait_event));
      }

      for (auto device_id : device_ids) {
        timers[device_id].start();
        timers1[device_id].start();
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event[device_id], UINT64_MAX));
        timers[device_id].end();
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event1[device_id], UINT64_MAX));
        timers1[device_id].end();

        accumulate_bidir_sample(device_id, timers[device_id],
                                timers1[device_id], timings);
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();
  }
}

void ZeBandwidth::test_host2device(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  info() << std::endl;
  info() << "HOST-TO-DEVICE BANDWIDTH AND LATENCY" << std::endl;
  if (csv_output) {
    print_csv_header(nullptr);
  }

  for (auto size : transfer_size) {
    long double total_time_nsec = 0;
    std::vector<long double> device_times_nsec(benchmark->_devices.size());
    for (size_t i = 0U; i < device_times_nsec.size(); i++) {
      device_times_nsec[i] = 0;
    }

    for (auto device_id : device_ids) {
      benchmark->memoryAlloc(device_id, size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);
    }

    transfer_size_test(size, device_buffers, host_buffers, device_times_nsec,
                       total_time_nsec, true);

    info() << "-----------------------------------------------------"
              "---------------------------\n";
    info() << "Host->Device\n";
    for (auto device_id : device_ids) {
      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);

      calculate_metrics(device_times_nsec[device_id],
                        static_cast<long double>(size * number_iterations),
                        total_bandwidth, total_latency);
      print_results(size, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ");
    }

    calculate_metrics(
        total_time_nsec,
        static_cast<long double>(device_ids.size() * size * number_iterations),
        total_bandwidth, total_latency);
    print_results(size * device_ids.size(), total_bandwidth, total_latency,
                  "[Total    ");
    info() << "-----------------------------------------------------"
              "---------------------------\n";
  }
}

void ZeBandwidth::test_device2host(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  info() << std::endl;
  info() << "DEVICE-TO-HOST BANDWIDTH AND LATENCY" << std::endl;
  if (csv_output) {
    print_csv_header(nullptr);
  }

  for (auto size : transfer_size) {
    long double total_time_nsec = 0;
    std::vector<long double> device_times_nsec(benchmark->_devices.size());
    for (size_t i = 0U; i < device_times_nsec.size(); i++) {
      device_times_nsec[i] = 0;
    }

    for (auto device_id : device_ids) {
      benchmark->memoryAlloc(device_id, size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);
    }

    transfer_size_test(size, host_buffers, device_buffers, device_times_nsec,
                       total_time_nsec, false);

    info() << "-----------------------------------------------------"
              "---------------------------\n";
    info() << "Device->Host\n";
    for (auto device_id : device_ids) {
      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);

      calculate_metrics(device_times_nsec[device_id],
                        static_cast<long double>(size * number_iterations),
                        total_bandwidth, total_latency);
      print_results(size, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ");
    }

    calculate_metrics(
        total_time_nsec,
        static_cast<long double>(device_ids.size() * size * number_iterations),
        total_bandwidth, total_latency);
    print_results(size * device_ids.size(), total_bandwidth, total_latency,
                  "[Total    ");
    info() << "-----------------------------------------------------"
              "---------------------------\n";
  }
}

//---------------------------------------------------------------------
// Whether a per-device allocation fits, leaving room for the driver's own
// allocations rather than filling the device. A size that does not fit is
// skipped with the budget printed, rather than failing in the allocator part
// way through a size ladder.
//---------------------------------------------------------------------
bool ZeBandwidth::device_size_fits(size_t largest_allocation,
                                   size_t device_bytes, const char *what) {
  // Total memory is not the only limit: a driver caps a single allocation
  // well below it, and that cap is what a doubled interference buffer hits
  // first. Checking only the total lets the run reach the allocator and abort.
  for (auto device_id : device_ids) {
    const uint64_t cap = device_properties[device_id].maxMemAllocSize;
    if (static_cast<uint64_t>(largest_allocation) > cap) {
      std::cerr << "\t[skipped " << what << ": one allocation of "
                << (largest_allocation >> 20) << " MB exceeds the "
                << (cap >> 20) << " MB single allocation limit of device "
                << device_id << "]\n";
      return false;
    }
  }

  uint64_t smallest = UINT64_MAX;
  for (auto device_id : device_ids) {
    smallest = std::min(smallest, device_memory_size[device_id]);
  }

  const uint64_t budget = (smallest / 100) * device_memory_headroom_percent;
  if (static_cast<uint64_t>(device_bytes) <= budget) {
    return true;
  }

  std::cerr << "\t[skipped " << what << ": needs " << (device_bytes >> 20)
            << " MB on a device, budget is " << (budget >> 20) << " MB of "
            << (smallest >> 20) << " MB]\n";
  return false;
}

//---------------------------------------------------------------------
// Whether the summed host allocation fits in what the host has left. The
// multi device tests pin a host buffer per device, so the host side grows
// with the device count and can run out long before device memory does.
// MemAvailable is the kernel's own estimate of what can be handed out
// without swapping, which is the closest thing to the right budget here.
//---------------------------------------------------------------------
bool ZeBandwidth::host_size_fits(size_t host_bytes, const char *what) {
#ifdef __linux__
  std::ifstream meminfo("/proc/meminfo");
  std::string key;
  uint64_t available_kb = 0;

  while (meminfo >> key) {
    if (key == "MemAvailable:") {
      if (!(meminfo >> available_kb)) {
        available_kb = 0;
      }
      break;
    }
  }

  if (available_kb == 0) {
    return true;
  }

  const uint64_t budget =
      ((available_kb * 1024) / 100) * device_memory_headroom_percent;
  if (static_cast<uint64_t>(host_bytes) <= budget) {
    return true;
  }

  std::cerr << "\t[skipped " << what << ": needs " << (host_bytes >> 20)
            << " MB of host memory, budget is " << (budget >> 20) << " MB]\n";
  return false;
#else
  (void)host_bytes;
  (void)what;
  return true;
#endif
}

void ZeBandwidth::test_bidir(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  info() << std::endl;
  info() << "BIDIRECTIONAL HOST-TO-DEVICE/DEVICE-TO-HOST BANDWIDTH AND LATENCY"
         << std::endl;
  if (csv_output) {
    print_csv_header(use_event_timer ? "Overlap" : nullptr);
  }

  for (auto size : transfer_size) {
    long double total_time_nsec = 0;
    if (!device_size_fits(size, 2 * size, std::to_string(size).c_str())) {
      continue;
    }
    BidirTimings timings;
    timings.reset(benchmark->_devices.size());

    for (auto device_id : device_ids) {
      benchmark->memoryAlloc(device_id, size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);
      benchmark->memoryAlloc(device_id, size, &device_buffers_bidir[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers_bidir[device_id]);
    }

    transfer_bidir_size_test(size, size, device_buffers, host_buffers,
                             host_buffers_bidir, device_buffers_bidir, timings,
                             total_time_nsec, true);

    info() << "-----------------------------------------------------"
              "---------------------------\n";
    info() << "Host<->Device\n";
    for (auto device_id : device_ids) {
      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);
      benchmark->memoryFree(device_buffers_bidir[device_id]);
      benchmark->memoryFree(host_buffers_bidir[device_id]);

      const uint32_t kept = report_dropped_samples(timings, device_id);
      if (kept == 0) {
        std::cerr << "\t[Device " << device_id
                  << " has no result: every sample was dropped]\n";
        continue;
      }
      calculate_metrics(timings.span_nsec[device_id],
                        static_cast<long double>(2 * size) * kept,
                        total_bandwidth, total_latency, kept);
      long double overlap = 0.0L;
      if (timings.span_nsec[device_id] > 0.0L) {
        overlap =
            timings.overlap_nsec[device_id] / timings.span_nsec[device_id];
      }
      print_results(size, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ",
                    use_event_timer ? "overlap" : nullptr, overlap);
      if (use_event_timer && overlap < 0.90L) {
        std::cerr << "\tWARNING: the two directions overlapped for only "
                  << std::setprecision(3) << overlap
                  << " of the run, the aggregate figure is not full duplex\n";
      }
    }

    calculate_metrics(total_time_nsec,
                      static_cast<long double>(device_ids.size() * 2 * size *
                                               number_iterations),
                      total_bandwidth, total_latency);
    print_results(size * device_ids.size(), total_bandwidth, total_latency,
                  "[Total    ");
    info() << "-----------------------------------------------------"
              "---------------------------\n";
  }
}

//---------------------------------------------------------------------
// Bidirectional run where only one direction is reported. The interfering
// copy is twice the size so that it outlives the measured copy, otherwise
// the tail of the measured copy would run unopposed.
//---------------------------------------------------------------------
void ZeBandwidth::test_bidir_measured(bool measure_h2d) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;
  const char *measured_name = measure_h2d ? "HOST-TO-DEVICE" : "DEVICE-TO-HOST";
  const char *interfering_name =
      measure_h2d ? "DEVICE-TO-HOST" : "HOST-TO-DEVICE";

  info() << std::endl;
  info() << "BIDIRECTIONAL " << measured_name
         << " BANDWIDTH AND LATENCY (measured while " << interfering_name
         << " runs at twice the size)" << std::endl;
  if (csv_output) {
    print_csv_header(use_event_timer ? "Coverage" : nullptr);
  }

  for (auto size : transfer_size) {
    long double total_time_nsec = 0;
    const size_t size_interference = 2 * size;
    if (!device_size_fits(size_interference, size + size_interference,
                          std::to_string(size).c_str())) {
      continue;
    }
    BidirTimings timings;
    timings.reset(benchmark->_devices.size());

    for (auto device_id : device_ids) {
      benchmark->memoryAlloc(device_id, size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);
      benchmark->memoryAlloc(device_id, size_interference,
                             &device_buffers_bidir[device_id]);
      benchmark->memoryAllocHost(size_interference,
                                 &host_buffers_bidir[device_id]);
    }

    if (measure_h2d) {
      transfer_bidir_size_test(size, size_interference, device_buffers,
                               host_buffers, host_buffers_bidir,
                               device_buffers_bidir, timings, total_time_nsec,
                               true);
    } else {
      transfer_bidir_size_test(size, size_interference, host_buffers,
                               device_buffers, device_buffers_bidir,
                               host_buffers_bidir, timings, total_time_nsec,
                               false);
    }

    info() << "-----------------------------------------------------"
              "---------------------------\n";
    info() << (measure_h2d ? "Host->Device" : "Device->Host")
           << " (bidirectional)\n";
    for (auto device_id : device_ids) {
      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);
      benchmark->memoryFree(device_buffers_bidir[device_id]);
      benchmark->memoryFree(host_buffers_bidir[device_id]);

      const uint32_t kept = report_dropped_samples(timings, device_id);
      if (kept == 0) {
        std::cerr << "\t[Device " << device_id
                  << " has no result: every sample was dropped]\n";
        continue;
      }
      calculate_metrics(timings.direction0_nsec[device_id],
                        static_cast<long double>(size) * kept, total_bandwidth,
                        total_latency, kept);

      // The interfering copy is deliberately longer, so the useful figure is
      // how much of the measured copy was contended, not the union overlap.
      long double coverage = 0.0L;
      if (timings.direction0_nsec[device_id] > 0.0L) {
        coverage = timings.overlap_nsec[device_id] /
                   timings.direction0_nsec[device_id];
      }
      print_results(size, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ",
                    use_event_timer ? "coverage" : nullptr, coverage);
      if (use_event_timer && coverage < 0.95L) {
        std::cerr << "\tWARNING: the measured copy was contended for only "
                  << std::setprecision(3) << coverage << " of its duration\n";
      }
    }
    info() << "-----------------------------------------------------"
              "---------------------------\n";
  }
}

//---------------------------------------------------------------------
// Engine validation. Out of range selections are rejected rather than
// clamped: silently falling back to the first engine would report a single
// engine used twice as if it were two engines running in parallel.
//---------------------------------------------------------------------
bool ZeBandwidth::resolve_engine_names(const std::vector<EngineId> &engines) {
  bool resolved = true;

  if (!h2d_engine_name.empty() &&
      !find_engine(h2d_engine_name, engines, command_queue_group_ordinal,
                   command_queue_index)) {
    std::cerr << "Unknown engine '" << h2d_engine_name << "' for --h2dEngine\n";
    resolved = false;
  }

  if (!d2h_engine_name.empty() &&
      !find_engine(d2h_engine_name, engines, command_queue_group_ordinal1,
                   command_queue_index1)) {
    std::cerr << "Unknown engine '" << d2h_engine_name << "' for --d2hEngine\n";
    resolved = false;
  }

  // A test that drives one engine takes it from whichever name was given, so
  // that naming only the direction it measures is enough. A test that drives
  // two must not, or naming one direction would move the other onto the same
  // engine and serialise a run that reports itself as bidirectional.
  if (!uses_two_engines()) {
    if (h2d_engine_name.empty() && !d2h_engine_name.empty()) {
      command_queue_group_ordinal = command_queue_group_ordinal1;
      command_queue_index = command_queue_index1;
    } else if (d2h_engine_name.empty() && !h2d_engine_name.empty()) {
      command_queue_group_ordinal1 = command_queue_group_ordinal;
      command_queue_index1 = command_queue_index;
    }
  }

  return resolved;
}

bool ZeBandwidth::uses_two_engines(void) const {
  return run_bidirectional || run_bidir_h2d || run_bidir_d2h ||
         (run_all_host && all_host_bidirectional && !all_host_use_kernel);
}

//---------------------------------------------------------------------
// Two directions on one engine serialise, so the result is not full duplex.
// The run is still allowed, because it is the control case that shows the
// overlap metric collapsing to zero, but it has to say so unprompted: the
// overlap and coverage columns only appear under --useEvents, and without
// them nothing else distinguishes this from a real bidirectional figure.
//---------------------------------------------------------------------
void ZeBandwidth::warn_if_directions_share_engine(
    const std::string &engine_label) {
  if (!uses_two_engines() ||
      command_queue_group_ordinal != command_queue_group_ordinal1 ||
      command_queue_index != command_queue_index1) {
    return;
  }

  std::cerr << "WARNING: both directions were placed on " << engine_label
            << ", so they serialise and the reported figure is not full "
               "duplex.\n         Name a second engine to measure both "
               "directions at once, for example --h2dEngine=bcs0 "
               "--d2hEngine=ccs0.\n";
}

void ZeBandwidth::validate_numeric_engines(uint32_t num_queue_groups) {
  bool valid = true;

  if (command_queue_group_ordinal >= num_queue_groups ||
      command_queue_group_ordinal1 >= num_queue_groups) {
    std::cerr << "Command queue group out of range, the device exposes "
              << num_queue_groups << " groups\n";
    valid = false;
  } else {
    if (command_queue_index >=
        queueProperties[command_queue_group_ordinal].numQueues) {
      std::cerr << "Command queue index " << command_queue_index
                << " is not valid for group " << command_queue_group_ordinal
                << ", which has "
                << queueProperties[command_queue_group_ordinal].numQueues
                << " queues\n";
      valid = false;
    }
    if (command_queue_index1 >=
        queueProperties[command_queue_group_ordinal1].numQueues) {
      std::cerr << "Command queue index " << command_queue_index1
                << " is not valid for group " << command_queue_group_ordinal1
                << ", which has "
                << queueProperties[command_queue_group_ordinal1].numQueues
                << " queues\n";
      valid = false;
    }
  }

  if (!valid) {
    exit(-1);
  }
}

//---------------------------------------------------------------------
// The kernel driven tests append a launch, which a copy only queue group
// cannot execute.
//---------------------------------------------------------------------
bool ZeBandwidth::h2d_slot_needs_compute(void) const {
  return run_host2dev_kernel || run_bidirectional_kernel ||
         (run_all_host && all_host_use_kernel &&
          all_host_measures_on_h2d_slot());
}

bool ZeBandwidth::d2h_slot_needs_compute(void) const {
  return run_dev2host_kernel || (run_all_host && all_host_use_kernel &&
                                 !all_host_measures_on_h2d_slot());
}

void ZeBandwidth::validate_compute_engines(
    const std::vector<EngineId> &engines) {
  bool valid = true;

  if (h2d_slot_needs_compute() &&
      !queue_is_ccs(queueProperties[command_queue_group_ordinal].flags)) {
    std::cerr << "Test requires a compute engine but "
              << engine_label(engines, command_queue_group_ordinal,
                              command_queue_index)
              << " was selected for the Host-to-Device direction\n";
    valid = false;
  }

  if (d2h_slot_needs_compute() &&
      !queue_is_ccs(queueProperties[command_queue_group_ordinal1].flags)) {
    std::cerr << "Test requires a compute engine but "
              << engine_label(engines, command_queue_group_ordinal1,
                              command_queue_index1)
              << " was selected for the Device-to-Host direction\n";
    valid = false;
  }

  // The split direction kernel is one dispatch on one queue, so a second
  // engine cannot be honoured. Accepting it silently would report a single
  // engine result as though two engines had been used.
  const bool single_engine_kernel =
      run_bidirectional_kernel ||
      (run_all_host && all_host_use_kernel && all_host_bidirectional);

  if (single_engine_kernel &&
      (command_queue_group_ordinal != command_queue_group_ordinal1 ||
       command_queue_index != command_queue_index1)) {
    std::cerr << "This test issues a single kernel that carries both "
                 "directions, so it runs on one engine, but "
              << engine_label(engines, command_queue_group_ordinal,
                              command_queue_index)
              << " and "
              << engine_label(engines, command_queue_group_ordinal1,
                              command_queue_index1)
              << " were selected. Name one engine, or use a copy engine test "
                 "for two engines.\n";
    valid = false;
  }

  if (!valid) {
    print_engine_list(engines);
    exit(-1);
  }
}

//---------------------------------------------------------------------
// Engines are resolved once, from the first selected device. Every other
// selected device is then checked against that choice, because a host may
// mix devices with different queue group layouts and the multi device tests
// take every enumerated device by default. Without this the mismatch would
// surface as a bare abort inside zeCommandQueueCreate, naming no device.
//---------------------------------------------------------------------
void ZeBandwidth::validate_additional_devices(void) {
  bool valid = true;

  for (size_t i = 1; i < device_ids.size(); i++) {
    const uint32_t device_id = device_ids[i];

    uint32_t num_queue_groups = 0;
    benchmark->deviceGetCommandQueueGroupProperties(device_id,
                                                    &num_queue_groups, nullptr);
    std::vector<ze_command_queue_group_properties_t> properties(
        num_queue_groups,
        {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES, nullptr});
    benchmark->deviceGetCommandQueueGroupProperties(
        device_id, &num_queue_groups, properties.data());

    auto check_slot = [&](uint32_t ordinal, uint32_t index, bool needs_compute,
                          const char *direction) {
      if (ordinal >= num_queue_groups) {
        std::cerr << "[Device " << device_id << "] group " << ordinal
                  << " selected for the " << direction
                  << " direction does not exist, the device exposes "
                  << num_queue_groups << " groups\n";
        valid = false;
        return;
      }
      if (index >= properties[ordinal].numQueues) {
        std::cerr << "[Device " << device_id << "] queue index " << index
                  << " selected for the " << direction
                  << " direction is not valid for group " << ordinal
                  << ", which has " << properties[ordinal].numQueues
                  << " queues\n";
        valid = false;
        return;
      }
      if (needs_compute && !queue_is_ccs(properties[ordinal].flags)) {
        std::cerr << "[Device " << device_id << "] group " << ordinal
                  << " selected for the " << direction
                  << " direction is not a compute group, which the kernel "
                     "driven tests require\n";
        valid = false;
      }
    };

    check_slot(command_queue_group_ordinal, command_queue_index,
               h2d_slot_needs_compute(), "Host-to-Device");
    check_slot(command_queue_group_ordinal1, command_queue_index1,
               d2h_slot_needs_compute(), "Device-to-Host");
  }

  if (!valid) {
    std::cerr << "Engines are selected once and used on every device, so pass "
                 "-d to restrict the run to devices that share a layout.\n";
    exit(-1);
  }
}

//---------------------------------------------------------------------
// Utility function to query queue group properties
//---------------------------------------------------------------------
void ZeBandwidth::ze_bandwidth_query_engines() {
  // Engine naming and validation are driven by the first selected device.
  // They must not be repeated per device, or a value rejected for one device
  // would already have been overwritten by the time the next is checked.
  uint32_t numQueueGroups = 0;
  benchmark->deviceGetCommandQueueGroupProperties(device_ids[0],
                                                  &numQueueGroups, nullptr);

  if (numQueueGroups == 0) {
    info() << " No queue groups found\n" << std::endl;
    exit(0);
  }

  queueProperties.assign(
      numQueueGroups,
      {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES, nullptr});
  benchmark->deviceGetCommandQueueGroupProperties(
      device_ids[0], &numQueueGroups, queueProperties.data());

  const std::vector<EngineId> engines = enumerate_engines(queueProperties);

  for (uint32_t i = 0; i < numQueueGroups; i++) {
    const bool is_ccs = queue_is_ccs(queueProperties[i].flags);
    const bool is_bcs = queue_is_bcs(queueProperties[i].flags);
    if (!is_ccs && !is_bcs) {
      continue;
    }

    info() << " Group " << i << (is_ccs ? " (compute): " : " (copy): ")
           << queueProperties[i].numQueues << " queues ->";
    for (const auto &engine : engines) {
      if (engine.ordinal == i) {
        info() << " " << engine.name;
      }
    }
    info() << "\n";
  }

  if (enable_engine_names) {
    if (!resolve_engine_names(engines)) {
      print_engine_list(engines);
      exit(-1);
    }
  } else {
    validate_numeric_engines(numQueueGroups);
  }

  if (any_kernel_test()) {
    validate_compute_engines(engines);
  }

  validate_additional_devices();

  const std::string h2d_label =
      engine_label(engines, command_queue_group_ordinal, command_queue_index);
  const std::string d2h_label =
      engine_label(engines, command_queue_group_ordinal1, command_queue_index1);

  warn_if_directions_share_engine(h2d_label);

  device_properties.resize(benchmark->_devices.size());
  device_memory_size.resize(benchmark->_devices.size());

  for (auto device_id : device_ids) {
    const DeviceLocation location =
        device_location(benchmark->_devices[device_id]);
    if (!location.pci_address.empty()) {
      info() << "[Device " << device_id << "] PCI " << location.pci_address
             << ", NUMA node ";
      if (location.numa_node >= 0) {
        info() << location.numa_node;
      } else {
        info() << "unknown";
      }
      info() << "\n";
    }

    if (run_host2dev || run_host2dev_kernel) {
      info() << "[Device " << device_id << "] Running H2D with " << h2d_label
             << " (" << command_queue_group_ordinal << ","
             << command_queue_index << ")\n";
    }
    if (run_dev2host || run_dev2host_kernel) {
      info() << "[Device " << device_id << "] Running D2H with " << d2h_label
             << " (" << command_queue_group_ordinal1 << ","
             << command_queue_index1 << ")\n";
    }
    if (run_bidirectional || run_bidir_h2d || run_bidir_d2h) {
      info() << "[Device " << device_id << "] Running H2D with " << h2d_label
             << " (" << command_queue_group_ordinal << ","
             << command_queue_index << ") and D2H with " << d2h_label << " ("
             << command_queue_group_ordinal1 << "," << command_queue_index1
             << ")\n";
    }
    if (run_bidirectional_kernel) {
      info() << "[Device " << device_id
             << "] Running bidirectional split kernel with " << h2d_label
             << " (" << command_queue_group_ordinal << ","
             << command_queue_index << ")\n";
    }

    device_properties[device_id] = {};
    device_properties[device_id].stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    SUCCESS_OR_TERMINATE(zeDeviceGetProperties(benchmark->_devices[device_id],
                                               &device_properties[device_id]));

    uint32_t memory_count = 0;
    SUCCESS_OR_TERMINATE(zeDeviceGetMemoryProperties(
        benchmark->_devices[device_id], &memory_count, nullptr));
    std::vector<ze_device_memory_properties_t> memory_properties(
        memory_count, {ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES, nullptr});
    SUCCESS_OR_TERMINATE(
        zeDeviceGetMemoryProperties(benchmark->_devices[device_id],
                                    &memory_count, memory_properties.data()));

    device_memory_size[device_id] = 0;
    for (const auto &memory : memory_properties) {
      device_memory_size[device_id] += memory.totalSize;
    }

    if (use_immediate_command_list == false) {
      benchmark->commandQueueCreate(device_id, command_queue_group_ordinal,
                                    command_queue_index,
                                    &command_queue[device_id]);
      benchmark->commandListCreate(device_id, command_queue_group_ordinal,
                                   &command_list[device_id]);

      benchmark->commandQueueCreate(device_id, command_queue_group_ordinal1,
                                    command_queue_index1,
                                    &command_queue1[device_id]);
      benchmark->commandListCreate(device_id, command_queue_group_ordinal1,
                                   &command_list1[device_id]);
    } else {
      ze_command_queue_desc_t command_queue_description{};
      command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
      command_queue_description.pNext = nullptr;
      command_queue_description.ordinal = command_queue_group_ordinal;
      command_queue_description.index = command_queue_index;
      command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

      SUCCESS_OR_TERMINATE(zeCommandListCreateImmediate(
          benchmark->context, benchmark->_devices[device_id],
          &command_queue_description, &command_list[device_id]));

      command_queue_description.ordinal = command_queue_group_ordinal1;
      command_queue_description.index = command_queue_index1;
      SUCCESS_OR_TERMINATE(zeCommandListCreateImmediate(
          benchmark->context, benchmark->_devices[device_id],
          &command_queue_description, &command_list1[device_id]));
    }
  }

  if (verify) {
    command_queue_verify.resize(benchmark->_devices.size());
    command_list_verify.resize(benchmark->_devices.size());
    for (auto device_id : device_ids) {
      benchmark->commandQueueCreate(device_id, 0, 0,
                                    &command_queue_verify[device_id]);
      benchmark->commandListCreate(device_id, 0,
                                   &command_list_verify[device_id]);
    }
  }

  // The measurement events live in their own pool because requesting kernel
  // timestamps leaves an event duration undefined, while wait_event is only
  // ever used as a host controlled release gate.
  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
  event_pool_desc.count = static_cast<uint32_t>(2 * device_ids.size());
  event_pool_desc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  if (use_event_timer) {
    event_pool_desc.flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
  }
  SUCCESS_OR_TERMINATE(
      zeEventPoolCreate(benchmark->context, &event_pool_desc,
                        static_cast<uint32_t>(benchmark->_devices.size()),
                        benchmark->_devices.data(), &event_pool));

  ze_event_pool_desc_t wait_event_pool_desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
  wait_event_pool_desc.count = 1;
  wait_event_pool_desc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  SUCCESS_OR_TERMINATE(
      zeEventPoolCreate(benchmark->context, &wait_event_pool_desc,
                        static_cast<uint32_t>(benchmark->_devices.size()),
                        benchmark->_devices.data(), &wait_event_pool));

  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

  uint32_t event_index = 0;
  for (auto device_id : device_ids) {
    event_desc.index = event_index++;
    SUCCESS_OR_TERMINATE(
        zeEventCreate(event_pool, &event_desc, &event[device_id]));
    event_desc.index = event_index++;
    SUCCESS_OR_TERMINATE(
        zeEventCreate(event_pool, &event_desc, &event1[device_id]));
  }

  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  SUCCESS_OR_TERMINATE(
      zeEventCreate(wait_event_pool, &event_desc, &wait_event));
}

int main(int argc, char **argv) {
  ZeBandwidth bw;
  size_t default_size;
  srand(1);

  bw.parse_arguments(argc, argv);

  bw.command_queue.resize(bw.benchmark->_devices.size());
  bw.command_queue1.resize(bw.benchmark->_devices.size());
  bw.command_list.resize(bw.benchmark->_devices.size());
  bw.command_list1.resize(bw.benchmark->_devices.size());
  bw.event.resize(bw.benchmark->_devices.size());
  bw.event1.resize(bw.benchmark->_devices.size());

  // The multi device tests are only meaningful across every device, so they
  // opt in to all of them unless -d says otherwise.
  if (bw.device_ids.empty()) {
    if (bw.run_all_host && !bw.device_ids_explicit) {
      for (uint32_t i = 0; i < bw.benchmark->_devices.size(); i++) {
        bw.device_ids.push_back(i);
      }
    } else {
      bw.device_ids.push_back(0);
    }
  }

  bw.ze_bandwidth_query_engines();

  if (!bw.query_engines) {
    default_size = bw.transfer_lower_limit;
    while (default_size < bw.transfer_upper_limit) {
      bw.transfer_size.push_back(default_size);
      default_size <<= 1;
    }
    bw.transfer_size.push_back(bw.transfer_upper_limit);

    if (bw.verify) {
      bw.number_iterations = 1;
      bw.warmup_iterations = 0;
    }

    bw.info() << std::endl
              << "Iterations per transfer size = " << bw.number_iterations
              << std::endl;

    bw.device_buffers.resize(bw.benchmark->_devices.size());
    bw.device_buffers_bidir.resize(bw.benchmark->_devices.size());
    bw.host_buffers.resize(bw.benchmark->_devices.size());
    bw.host_buffers_bidir.resize(bw.benchmark->_devices.size());

    if (bw.any_kernel_test()) {
      bw.kernel_init();
    }

    if (bw.run_host2dev) {
      bw.test_host2device();
    }

    if (bw.run_dev2host) {
      bw.test_device2host();
    }

    if (bw.run_bidirectional) {
      bw.test_bidir();
    }

    if (bw.run_bidir_h2d) {
      bw.test_bidir_measured(true);
    }

    if (bw.run_bidir_d2h) {
      bw.test_bidir_measured(false);
    }

    if (bw.run_host2dev_kernel) {
      bw.test_host2device_kernel();
    }

    if (bw.run_dev2host_kernel) {
      bw.test_device2host_kernel();
    }

    if (bw.run_bidirectional_kernel) {
      bw.test_bidir_kernel();
    }

    if (bw.run_all_host) {
      bw.test_all_host();
    }

    if (bw.any_kernel_test()) {
      bw.kernel_cleanup();
    }

    bw.info() << std::endl;

    std::cout << std::flush;
  }
  return 0;
}
