/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef unsigned long uint64_t;
typedef unsigned uint32_t;
typedef unsigned short uint16_t;

kernel void fill_device_memory(__global uint64_t *pattern_memory,
                               const size_t pattern_memory_count,
                               const uint16_t pattern_base) {
  uint32_t i;

  /* fill memory with pattern */
  for (i = 0; i < pattern_memory_count; i++)
    pattern_memory[i] = (i << (sizeof(uint16_t) * 8)) + pattern_base;
}

/*
 * Verify pattern buffer against expected pattern.
 * In case of unexpected differences, use output buffers to record
 * some of those differences.
 */
kernel void test_device_memory(__global uint64_t *pattern_memory,
                               const size_t pattern_memory_count,
                               const uint16_t pattern_base,
                               __global uint64_t *expected_output,
                               __global uint64_t *found_output,
                               const size_t output_count) {
  uint32_t i, j;

  j = 0;
  for (i = 0; i < pattern_memory_count; i++) {
    if (pattern_memory[i] != (i << (sizeof(uint16_t) * 8)) + pattern_base) {
      if (j++ < output_count) {
        expected_output[j] = (i << (sizeof(uint16_t) * 8)) + pattern_base;
        found_output[j] = pattern_memory[i];
      }
    }
  }
}
