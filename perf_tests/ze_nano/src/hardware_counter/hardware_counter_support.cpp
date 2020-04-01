/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "hardware_counter.hpp"

#include <assert.h>
#include <papi.h>

static void print_enable_events(std::string counter_name) {
  std::cout << counter_name
            << " Counter is not enabled. Try enabling it with:\n"
            << "  sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'"
            << std::endl
            << std::endl;
}

static void error_handler_counter(std::string function_name,
                                  std::string counter_name, int ret) {
  std::cout << "ERROR: " << function_name << " failed with " << ret
            << std::endl;
  print_enable_events(counter_name);
}

HardwareCounter::HardwareCounter() {
  int ret;
  int number_active_events;
  measurement_taken = false;
  active_period = false;
  event_set = PAPI_NULL;

  ret = PAPI_library_init(PAPI_VER_CURRENT);
  if (ret != PAPI_VER_CURRENT) {
    ERROR_RETURN(ret);
  }

  SUCCESS_OR_TERMINATE(PAPI_create_eventset(&event_set));

  /* Add Total Instructions Executed to the event_set */
  ret = PAPI_add_event(event_set, PAPI_TOT_INS);
  if (ret != PAPI_OK) {
    error_handler_counter("PAPI_add_event", "PAPI_TOT_INS", ret);
  }

  /* Add Total Cycles event to the event_set */
  ret = PAPI_add_event(event_set, PAPI_TOT_CYC);
  if (ret != PAPI_OK) {
    error_handler_counter("PAPI_add_event", "PAPI_TOT_CYC", ret);
  }

  number_active_events = 0;
  SUCCESS_OR_TERMINATE(
      PAPI_list_events(event_set, NULL, &number_active_events));

  /*
   * false if PAPI library is available and the events/counters are
   * disabled; Otherwise, true.
   */
  _counter_enabled = (number_active_events == number_events);
}

HardwareCounter::~HardwareCounter() {
  SUCCESS_OR_TERMINATE(PAPI_cleanup_eventset(event_set));
  SUCCESS_OR_TERMINATE(PAPI_destroy_eventset(&event_set));
  PAPI_shutdown();
}

void HardwareCounter::start(void) {
  measurement_taken = true;
  active_period = true;
  SUCCESS_OR_TERMINATE(PAPI_start(event_set));
}

void HardwareCounter::end(void) {
  active_period = false;
  SUCCESS_OR_TERMINATE(PAPI_stop(event_set, values));
}

void HardwareCounter::counter_asserts(void) {
  /* No period was measured. start() and end() need to be called first */
  assert(measurement_taken == true);

  /*
   * Period was not measured properly.
   * start() was called without end().
   */
  assert(active_period == false);
}

long long HardwareCounter::counter_instructions(void) {
  counter_asserts();
  return values[0];
}

long long HardwareCounter::counter_cycles(void) {
  counter_asserts();
  return values[1];
}

bool HardwareCounter::is_supported(void) { return _counter_enabled; }

std::string HardwareCounter::support_warning(void) {
  return "PAPI counters are disabled. Decrease perf_event_paranoid level.";
}
