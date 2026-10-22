/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <iostream>

class HardwareCounter {
public:
  HardwareCounter();
  ~HardwareCounter();
  void start(void);
  void end(void);
  long long counter_instructions(void);
  long long counter_cycles(void);
  bool is_supported(void);
  static std::string support_warning(void);

private:
  static const unsigned int number_events = 2;
  inline void counter_asserts(void);

#ifdef WITH_PAPI
  bool _counter_enabled;
  int event_set;

  /*
   * It is used to check that at least on measurement was taken
   * by calling start() and end()
   */
  bool measurement_taken;

  /*
   * It is used to determine if user finished the measurement
   * by calling end() after start().
   */
  bool active_period;

  long long values[number_events];
#endif
};
