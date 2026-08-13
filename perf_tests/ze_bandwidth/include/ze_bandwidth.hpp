/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <string>
#include <level_zero/ze_api.h>
#include "../../common/include/common.hpp"
#include "ze_app.hpp"

// Level Zero has no name for a queue group, so one is derived from the group
// flags and from the queue index within the group.
struct EngineId {
  std::string name;
  uint32_t ordinal;
  uint32_t index;
};

// Per-device timings collected by a bidirectional run. Direction 0 is the
// measured direction, direction 1 carries the interfering traffic.
struct BidirTimings {
  std::vector<long double> direction0_nsec;
  std::vector<long double> direction1_nsec;
  std::vector<long double> overlap_nsec;
  std::vector<long double> span_nsec;
  // Samples that contributed to overlap_nsec and span_nsec. A wrapped
  // timestamp drops a sample, and the data volume has to shrink with it or
  // the reported bandwidth rises for every sample that was thrown away.
  std::vector<uint32_t> samples;

  void reset(size_t count) {
    direction0_nsec.assign(count, 0.0L);
    direction1_nsec.assign(count, 0.0L);
    overlap_nsec.assign(count, 0.0L);
    span_nsec.assign(count, 0.0L);
    samples.assign(count, 0);
  }
};

class ZeBandwidth {
public:
  ZeBandwidth();
  ~ZeBandwidth();
  int parse_arguments(int argc, char **argv);
  void test_host2device(void);
  void test_device2host(void);
  void test_bidir(void);
  void test_bidir_measured(bool measure_h2d);
  void test_host2device_kernel(void);
  void test_device2host_kernel(void);
  void test_bidir_kernel(void);
  void test_all_host(void);
  void kernel_init(void);
  void kernel_cleanup(void);
  void ze_bandwidth_query_engines();
  bool any_kernel_test(void) const;
  // Commentary stream: stdout normally, stderr under --csv, so that stdout
  // carries nothing but the header and the data rows.
  std::ostream &info(void);

  std::vector<size_t> transfer_size;
  std::vector<uint32_t> device_ids{};
  size_t transfer_lower_limit = 1;
  size_t transfer_upper_limit = (1 << 28);
  bool verify = false;
  bool run_host2dev = true;
  bool run_dev2host = true;
  bool run_bidirectional = false;
  bool run_bidir_h2d = false;
  bool run_bidir_d2h = false;
  bool run_host2dev_kernel = false;
  bool run_dev2host_kernel = false;
  bool run_bidirectional_kernel = false;
  bool run_all_host = false;
  bool all_host_source_is_host = false;
  bool all_host_bidirectional = false;
  bool all_host_use_kernel = false;
  bool device_ids_explicit = false;
  bool use_immediate_command_list = false;
  bool use_event_timer = false;
  uint32_t number_iterations = 500;
  uint32_t warmup_iterations = 100;
  bool query_engines = false;
  bool enable_fixed_ordinal_index = false;
  bool enable_engine_names = false;
  std::string h2d_engine_name;
  std::string d2h_engine_name;
  uint32_t command_queue_group_ordinal = 0;
  uint32_t command_queue_index = 0;
  uint32_t command_queue_group_ordinal1 = 0;
  uint32_t command_queue_index1 = 0;
  bool csv_output = false;
  ze_event_pool_handle_t event_pool = {};
  ze_event_pool_handle_t wait_event_pool = {};
  ze_event_handle_t wait_event = {};

  std::vector<ze_event_handle_t> event{};
  std::vector<ze_event_handle_t> event1{};

  std::vector<void *> device_buffers;
  std::vector<void *> device_buffers_bidir;
  std::vector<void *> host_buffers;
  std::vector<void *> host_buffers_bidir;

  std::vector<ze_command_queue_handle_t> command_queue{};
  std::vector<ze_command_queue_handle_t> command_queue1{};
  std::vector<ze_command_list_handle_t> command_list{};
  std::vector<ze_command_list_handle_t> command_list1{};

  std::vector<ze_module_handle_t> module_handle{};
  std::vector<ze_kernel_handle_t> kernel_striding{};
  std::vector<ze_kernel_handle_t> kernel_split{};
  std::vector<ze_device_properties_t> device_properties{};

  // The measured direction decides which engine slot carries it, so that
  // --h2dEngine and --d2hEngine always name the engine they say they do. The
  // split direction kernel is a single dispatch and both slots are required to
  // resolve to the same engine, so it can use either.
  bool all_host_measures_on_h2d_slot(void) const {
    return all_host_source_is_host ||
           (all_host_bidirectional && all_host_use_kernel);
  }

  std::vector<uint64_t> device_memory_size{};

  ZeApp *benchmark;

private:
  void transfer_size_test(size_t size, std::vector<void *> &destination_buffer,
                          std::vector<void *> &source_buffer,
                          std::vector<long double> &device_times_nsec,
                          long double &total_time_nsec, bool use_h2d_slot);
  void transfer_bidir_size_test(size_t size, size_t size1,
                                std::vector<void *> &destination_buffer,
                                std::vector<void *> &source_buffer,
                                std::vector<void *> &destination_buffer1,
                                std::vector<void *> &source_buffer1,
                                BidirTimings &timings,
                                long double &total_time_nsec,
                                bool use_h2d_slot);
  void kernel_transfer_test(size_t size, bool split_direction, void *buffer_a,
                            void *buffer_b, uint32_t device_id,
                            bool use_h2d_slot, size_t &bytes_moved,
                            long double &device_time_nsec);
  bool kernel_launch_config(uint32_t device_id, bool split_direction,
                            size_t size, ze_kernel_handle_t &kernel,
                            ze_group_count_t &group_count,
                            uint32_t &total_threads, uint32_t &chunk_elements,
                            size_t &bytes_moved);
  size_t append_kernel_copy(ze_command_list_handle_t list, uint32_t device_id,
                            bool split_direction, void *buffer_a,
                            void *buffer_b, size_t size,
                            ze_event_handle_t signal_event);
  void allocate_all_host_buffers(size_t size, uint32_t target,
                                 bool second_pair);
  void free_all_host_buffers(bool second_pair);
  bool all_host_size_fits(size_t size, bool second_pair);
  void all_host_verify_target(uint32_t target, size_t bytes_moved,
                              bool split_kernel, bool source_is_host);
  void kernel_verify_split(size_t bytes_moved, void *host_buffer,
                           void *device_buffer, uint32_t device_id);
  void kernel_fill_pattern(uint32_t device_id, void *buffer, size_t bytes);
  void kernel_fill_split_patterns(uint32_t device_id, void *host_buffer,
                                  void *device_buffer, size_t bytes);
  void kernel_check_pattern(uint32_t device_id, void *buffer, size_t bytes);
  void print_results(size_t buffer_size, long double total_bandwidth,
                     long double total_latency, std::string direction_string,
                     const char *extra_label = nullptr,
                     long double extra_value = 0.0L);
  void print_csv_header(const char *extra_column);
  void calculate_metrics(long double total_time_nsec, /* Units in nanoseconds */
                         long double total_data_transfer, /* Units in bytes */
                         long double &total_bandwidth,
                         long double &total_latency, uint32_t iterations = 0);
  uint32_t report_dropped_samples(const BidirTimings &timings,
                                  uint32_t device_id);
  bool device_size_fits(size_t largest_allocation, size_t device_bytes,
                        const char *what);
  bool host_size_fits(size_t host_bytes, const char *what);

  void accumulate_bidir_sample(uint32_t device_id,
                               Timer<std::chrono::nanoseconds::period> &timer0,
                               Timer<std::chrono::nanoseconds::period> &timer1,
                               BidirTimings &timings);
  long double ticks_to_nsec(uint64_t ticks, uint32_t device_id);
  int64_t tick_offset(uint64_t tick, uint64_t reference, uint32_t device_id);
  long double timestamp_delta_nsec(uint64_t start, uint64_t end,
                                   uint32_t device_id);
  void event_timestamps(ze_event_handle_t event_handle, uint64_t &start,
                        uint64_t &end);
  long double event_time_nsec(ze_event_handle_t event_handle,
                              uint32_t device_id);
  long double event_overlap_nsec(ze_event_handle_t first,
                                 ze_event_handle_t second, uint32_t device_id);

  bool resolve_engine_names(const std::vector<EngineId> &engines);
  void validate_numeric_engines(uint32_t num_queue_groups);
  void validate_compute_engines(const std::vector<EngineId> &engines);
  void validate_additional_devices(void);
  void warn_if_directions_share_engine(const std::string &engine_label);
  bool uses_two_engines(void) const;
  bool h2d_slot_needs_compute(void) const;
  bool d2h_slot_needs_compute(void) const;

  bool csv_extra_column = false;

  // One bundle per device: a copy is issued on the queue of the device that
  // owns the memory, so verification works for any device in -d, not only 0.
  std::vector<ze_command_queue_handle_t> command_queue_verify{};
  std::vector<ze_command_list_handle_t> command_list_verify{};
  void run_verify_copy(uint32_t device_id, void *destination, void *source,
                       size_t bytes);
  std::vector<ze_command_queue_group_properties_t> queueProperties;

  void *host_buffer_verify;
  void *host_buffer_verify1;
};
