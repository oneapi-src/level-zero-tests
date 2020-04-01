/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "hardware_counter.hpp"

#include <assert.h>

HardwareCounter::HardwareCounter() {}

HardwareCounter::~HardwareCounter() {}

void HardwareCounter::start(void) { assert(0); }

void HardwareCounter::end(void) { assert(0); }

void HardwareCounter::counter_asserts(void) { assert(0); }

long long HardwareCounter::counter_instructions(void) {
  assert(0);
  return -1;
}

long long HardwareCounter::counter_cycles(void) {
  assert(0);
  return -1;
}

bool HardwareCounter::is_supported(void) { return false; }

std::string HardwareCounter::support_warning(void) {
  return "Hardware counters are not supported. Compile benchmark with the PAPI "
         "library on Unix system";
}
