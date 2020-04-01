/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include <chrono>
#include <cstdint>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <numeric>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>

class Timer {
  public:
    std::chrono::high_resolution_clock::time_point tick, tock;

    void start();

    // Stop and return time in micro-seconds
    long double stopAndTime();
};

uint64_t roundToMultipleOf(uint64_t number, uint64_t base, uint64_t maxValue);

#endif /* COMMON_H */
