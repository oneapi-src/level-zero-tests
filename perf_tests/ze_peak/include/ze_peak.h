/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef ZE_PEAK_H
#define ZE_PEAK_H

#include "../include/common.h"

/* ze includes */
#include <level_zero/ze_api.h>

#define MIN(X, Y) (X < Y) ? X : Y

#undef FETCH_2
#undef FETCH_8

#define FETCH_2(sum, id, A, jumpBy)                                            \
  sum += A[id];                                                                \
  id += jumpBy;                                                                \
  sum += A[id];                                                                \
  id += jumpBy;
#define FETCH_4(sum, id, A, jumpBy)                                            \
  FETCH_2(sum, id, A, jumpBy);                                                 \
  FETCH_2(sum, id, A, jumpBy);
#define FETCH_8(sum, id, A, jumpBy)                                            \
  FETCH_4(sum, id, A, jumpBy);                                                 \
  FETCH_4(sum, id, A, jumpBy);

#define FETCH_PER_WI 16

enum class TimingMeasurement {
  BANDWIDTH = 0,
  BANDWIDTH_EVENT_TIMING,
  KERNEL_LAUNCH_LATENCY,
  KERNEL_COMPLETE_RUNTIME
};

struct L0Context {
  ze_command_queue_handle_t command_queue = nullptr;
  std::vector<ze_command_queue_handle_t> cmd_queue;
  ze_command_queue_handle_t copy_command_queue = nullptr;
  ze_command_list_handle_t command_list = nullptr;
  std::vector<ze_command_list_handle_t> cmd_list;
  ze_command_list_handle_t copy_command_list = nullptr;
  ze_module_handle_t module = nullptr;
  std::vector<ze_module_handle_t> subdevice_module;
  ze_context_handle_t context = nullptr;
  ze_driver_handle_t driver = nullptr;
  ze_device_handle_t device = nullptr;
  uint32_t device_count = 0;
  uint32_t sub_device_count = 0;
  std::vector<ze_device_handle_t> sub_devices;
  const uint32_t default_device = 0;
  const uint32_t command_queue_id = 0;
  ze_device_properties_t device_property;
  ze_device_compute_properties_t device_compute_property;
  bool verbose = false;
  std::vector<ze_command_queue_group_properties_t> queueProperties;

  void init_xe(uint32_t specified_driver, uint32_t specified_device,
               bool query_engines, bool enable_explicit_scaling,
               bool &enable_fixed_ordinal_index,
               uint32_t command_queue_group_ordinal,
               uint32_t command_queue_index);
  void clean_xe();
  void print_ze_device_properties(const ze_device_properties_t &props);
  void reset_commandlist(ze_command_list_handle_t cmd_list);
  void execute_commandlist_and_sync(bool use_copy_only_queue = false);
  std::vector<uint8_t> load_binary_file(const std::string &file_path);
  void create_module(std::vector<uint8_t> binary_file);
  void ze_peak_query_engines();
};

struct ZeWorkGroups {
  ze_group_count_t thread_group_dimensions;
  uint32_t group_size_x;
  uint32_t group_size_y;
  uint32_t group_size_z;
};

class ZePeak {
public:
  bool use_event_timer = false;
  bool verbose = false;
  bool run_global_bw = true;
  bool run_hp_compute = true;
  bool run_sp_compute = true;
  bool run_dp_compute = true;
  bool run_int_compute = true;
  bool run_transfer_bw = true;
  bool run_kernel_lat = true;
  bool enable_explicit_scaling = false;
  bool query_engines = false;
  bool enable_fixed_ordinal_index = false;
  uint32_t specified_driver = 0;
  uint32_t specified_device = 0;
  uint32_t global_bw_max_size = 1 << 29;
  uint32_t transfer_bw_max_size = 1 << 29;
  uint32_t iters = 50;
  uint32_t warmup_iterations = 10;
  uint32_t current_sub_device_id = 0;
  uint32_t command_queue_group_ordinal = 0;
  uint32_t command_queue_index = 0;

  int parse_arguments(int argc, char **argv);

  /* Helper Functions */
  long double run_kernel(L0Context context, ze_kernel_handle_t &function,
                         struct ZeWorkGroups &workgroup_info,
                         TimingMeasurement type,
                         bool reset_command_list = true);
  uint64_t set_workgroups(L0Context &context,
                          const uint64_t total_work_items_requested,
                          struct ZeWorkGroups *workgroup_info);
  void setup_function(L0Context &context, ze_kernel_handle_t &function,
                      const char *name, void *input, void *output,
                      size_t outputSize = 0u);
  uint64_t get_max_work_items(L0Context &context);
  void print_test_complete();
  void run_command_queue(L0Context &context);
  void synchronize_command_queue(L0Context &context);
  /* Benchmark Functions*/
  void ze_peak_global_bw(L0Context &context);
  void ze_peak_kernel_latency(L0Context &context);
  void ze_peak_hp_compute(L0Context &context);
  void ze_peak_sp_compute(L0Context &context);
  void ze_peak_dp_compute(L0Context &context);
  void ze_peak_int_compute(L0Context &context);
  void ze_peak_transfer_bw(L0Context &context);

private:
  long double _transfer_bw_gpu_copy(L0Context &context,
                                    void *destination_buffer,
                                    void *source_buffer, size_t buffer_size);
  long double _transfer_bw_host_copy(L0Context &context,
                                     void *destination_buffer,
                                     void *source_buffer, size_t buffer_size,
                                     bool shared_is_dest);
  void _transfer_bw_shared_memory(L0Context &context,
                                    size_t local_memory_size,
                                    void *local_memory);
  TimingMeasurement is_bandwidth_with_event_timer(void);
  long double calculate_gbps(long double period, long double buffer_size);
  long double context_time_in_us(L0Context &context, ze_event_handle_t &event);
};

TimingMeasurement is_bandwidth_with_event_timer(void);

#endif /* ZE_PEAK_H */
