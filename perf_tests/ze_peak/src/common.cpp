/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/common.h"

using namespace std;

void Timer::start() { tick = chrono::high_resolution_clock::now(); }

long double Timer::stopAndTime() {
  tock = chrono::high_resolution_clock::now();
  return std::chrono::duration<long double, std::chrono::microseconds::period>(
             tock - tick)
      .count();
}

uint64_t roundToMultipleOf(uint64_t number, uint64_t base, uint64_t maxValue) {
  uint64_t n = (number > maxValue) ? maxValue : number;
  return (n / base) * base;
}
