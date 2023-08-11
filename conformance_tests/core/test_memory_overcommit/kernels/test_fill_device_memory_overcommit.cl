/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef ulong uint64_t;
typedef uint uint32_t;
typedef ushort uint16_t;

kernel void fill_device_memory(__global uint64_t *pattern_memory,
                               const uint64_t pattern_memory_count,
                               const uint16_t pattern_base) {
  uint32_t i;

  /* Fill memory with pattern */
  for (i = 0; i < pattern_memory_count; i++)
    pattern_memory[i] = (i << (sizeof(uint16_t) * 8)) + pattern_base;
}

/*
 * Verify pattern buffer against expected pattern.
 * In case of unexpected differences, use found_output buffers to record
 * some of those differences and expected_output buffer to record golden data
 */
kernel void test_device_memory(__global uint64_t *pattern_memory,
                               const uint64_t pattern_memory_count,
                               const uint16_t pattern_base,
                               __global uint64_t *expected_output,
                               __global uint64_t *found_output,
                               const uint32_t output_count) {
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
