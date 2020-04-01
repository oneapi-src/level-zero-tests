/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "api_static_probe.hpp"

HardwareCounter *hardware_counters = NULL;
static bool static_probe_init = false;

void api_static_probe_init() {
  assert(static_probe_init == false); /* Initialize it only once */
  static_probe_init = true;
  hardware_counters = new HardwareCounter;
}

void api_static_probe_cleanup() {
  /* api static probe needs to be initialized first*/
  assert(static_probe_init == true);
  static_probe_init = false;
  delete hardware_counters;
}

bool api_static_probe_is_init() { return static_probe_init; }
