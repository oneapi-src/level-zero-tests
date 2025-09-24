/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _API_STATIC_PROBE_HPP_
#define _API_STATIC_PROBE_HPP_

#include "common.hpp"
#include "hardware_counter.hpp"
#include <level_zero/ze_api.h>

#include <assert.h>
#include <iomanip>
#include <locale>
#include <string>

const std::string PREFIX_LATENCY = "[ PERF LATENCY nS ]\t";
const std::string PREFIX_FUNCTION_CALL_RATE = "[ PERF FUNC_CALL_RATE ]\t";
const std::string PREFIX_CYCLES = "[ PERF CYCLES ]\t\t";
const std::string PREFIX_INSTRUCTION = "[ PERF INSTRUCTIONS ]\t";
const std::string PREFIX_IPC = "[ PERF IPC ]\t\t";

const std::string UNIT_LATENCY = "nanoseconds";
const std::string UNIT_FUNCTION_CALL_RATE = "function calls/sec";
const std::string UNIT_CYCLES = "cycles";
const std::string UNIT_INSTRUCTION = "instructions";
const std::string UNIT_IPC = UNIT_INSTRUCTION + "/" + UNIT_CYCLES;

extern HardwareCounter *hardware_counters;
void api_static_probe_init();
void api_static_probe_cleanup();
bool api_static_probe_is_init();

typedef struct _probe_cofig {
  int warm_up_iteration;
  int measure_iteration;
} probe_config_t;

template <typename T> inline constexpr double to_f64(T val) {
  return static_cast<double>(val);
}

template <typename T>
inline void
print_probe_output(const std::string prefix, const std::string filename,
                   const int line_number, const std::string function_name,
                   T output_value, const std::string suffix) {
  std::cout.imbue(std::locale(""));
  std::cout << prefix
            << (verbose ? filename + ":" + std::to_string(line_number) + "\t"
                        : "")
            << (verbose ? function_name + "\t" : "") << std::setw(15)
            << std::setprecision(5) << output_value << "\t" + suffix
            << std::endl;
}

#define PROBE_MEASURE_LATENCY_ITERATION(prefix, probe_setting, function_name,  \
                                        ...)                                   \
  _function_call_iter_measure_latency(__FILE__, __LINE__, #function_name,      \
                                      prefix, probe_setting, function_name,    \
                                      __VA_ARGS__)
template <typename... Params, typename... Args>
long double _function_call_iter_measure_latency(
    const std::string filename, const int line_number,
    const std::string function_name, const std::string prefix,
    const probe_config_t &probe_setting,
    ze_result_t (*api_function)(Params... params), Args... args) {
  int num_iters = probe_setting.measure_iteration;
  Timer<> timer;
  long double nsec;

  timer.start();
  for (int i = 0; i < num_iters; i++) {
    api_function(args...);
  }
  timer.end();

  nsec = timer.period_minus_overhead();

  print_probe_output(PREFIX_LATENCY + prefix, filename, line_number,
                     function_name, nsec / static_cast<long double>(num_iters),
                     UNIT_LATENCY);

  return nsec;
}

#define PROBE_MEASURE_HARDWARE_COUNTERS(prefix, probe_setting, function_name,  \
                                        ...)                                   \
  _function_call_iter_hardware_counters(__FILE__, __LINE__, #function_name,    \
                                        prefix, probe_setting, function_name,  \
                                        __VA_ARGS__)
template <typename... Params, typename... Args>
void _function_call_iter_hardware_counters(
    const std::string filename, const int line_number,
    const std::string function_name, const std::string prefix,
    const probe_config_t &probe_setting,
    ze_result_t (*api_function)(Params... params), Args... args) {
  int num_iters = probe_setting.measure_iteration;

  assert(api_static_probe_is_init());

  if (hardware_counters->is_supported() == false) {
    std::string warning = HardwareCounter::support_warning();
    print_probe_output(UNIT_CYCLES + prefix, filename, line_number,
                       function_name, warning, "");
    /*
     * Even though no hardware counters are retrieved, call the api
     * function once in case other parts of the test case expect the
     * api function to update some state.
     */
    api_function(args...);
    return;
  }

  hardware_counters->start();
  for (int i = 0; i < num_iters; i++) {
    api_function(args...);
  }
  hardware_counters->end();

  auto total_instr_cnt = to_f64(hardware_counters->counter_instructions());
  auto total_cycle_cnt = to_f64(hardware_counters->counter_cycles());

  auto avg_instr_cnt = total_instr_cnt / to_f64(num_iters);
  auto avg_cycle_cnt = total_cycle_cnt / to_f64(num_iters);
  auto instruction_per_cycle = avg_instr_cnt / avg_cycle_cnt;

  print_probe_output(PREFIX_INSTRUCTION + prefix, filename, line_number,
                     function_name, avg_instr_cnt, UNIT_INSTRUCTION);
  print_probe_output(PREFIX_CYCLES + prefix, filename, line_number,
                     function_name, avg_cycle_cnt, UNIT_CYCLES);
  print_probe_output(PREFIX_IPC + prefix, filename, line_number, function_name,
                     instruction_per_cycle, UNIT_IPC);
}

#define PROBE_MEASURE_FUNCTION_CALL_RATE(prefix, probe_setting, function_name, \
                                         ...)                                  \
  _function_call_rate_iter(__FILE__, __LINE__, #function_name, prefix,         \
                           function_name, __VA_ARGS__)
template <typename... Params, typename... Args>
void _function_call_rate_iter(const std::string filename, const int line_number,
                              const std::string function_name,
                              const std::string prefix,
                              ze_result_t (*api_function)(Params... params),
                              Args... args) {
  Timer<> timer;
  long double nsec;
  const long double one_second_in_nano = 1000000000.0;
  /*
   * 500 ms will be used to count function calls. This is determined by
   * dividing 1 second in nanoseconds by division_factor.
   */
  const long double division_factor = 2.0;
  const long double period = one_second_in_nano / division_factor;
  int function_call_counter = 0;

  /* Determine number of function calls per 500 milliseconds */
  while (timer.has_it_been(static_cast<long long>(period)) == false) {
    api_function(args...);
    function_call_counter++;
  }

  timer.start();
  for (int i = 0; i < function_call_counter; i++) {
    api_function(args...);
  }
  timer.end();

  nsec = timer.period_minus_overhead();
  function_call_counter = static_cast<int>(division_factor * (period / nsec) *
                                           function_call_counter);
  if (verbose) {
    /* It helps verify that function calls are one second */
    timer.start();
    for (int i = 0; i < function_call_counter; i++) {
      api_function(args...);
    }
    timer.end();

    nsec = timer.period_minus_overhead();
    std::cout << "Period " << nsec << " number function calls "
              << function_call_counter << std::endl;
  }
  print_probe_output(PREFIX_FUNCTION_CALL_RATE + prefix, filename, line_number,
                     function_name, function_call_counter,
                     UNIT_FUNCTION_CALL_RATE);
}
#endif /* _API_STATIC_PROBE_HPP_ */
