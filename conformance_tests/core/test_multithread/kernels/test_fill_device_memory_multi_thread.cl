/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef ulong uint64_t;
typedef uint uint32_t;
typedef ushort uint16_t;

kernel void fill_device_memory(__global uint64_t *pattern_memory,
                               const uint16_t pattern_base) {
  /* Fill memory with pattern */
  uint32_t i = get_global_id(0);
  pattern_memory[i] = (i << (sizeof(uint16_t) * 8)) + pattern_base;
}

/*
 * Verify pattern buffer against expected pattern.
 * In case of unexpected differences, use found_output buffers to record
 * some of those differences and expected_output buffer to record golden data
 */
kernel void test_device_memory(__global uint64_t *pattern_memory,
                               const uint16_t pattern_base,
                               __global uint64_t *expected_output,
                               __global uint64_t *found_output,
                               const uint32_t output_count) {
  uint32_t i = get_global_id(0);
  if (pattern_memory[i] != (i << (sizeof(uint16_t) * 8)) + pattern_base) {
    uint32_t j = i % output_count;
    expected_output[j] = (i << (sizeof(uint16_t) * 8)) + pattern_base;
    found_output[j] = pattern_memory[i];
  }
}
