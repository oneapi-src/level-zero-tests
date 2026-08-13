/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

// Both constants are supplied by the host through build flags so that there is a
// single source of truth; the defaults below only apply to a manual compile.

// Outstanding loads issued per thread before the first store.
#ifndef PIPE_DEPTH
#define PIPE_DEPTH 12u
#endif

// Direction stripe width in uint4 elements: 32 * 16 B = 512 B of contiguous
// traffic per direction before the next stripe reverses it. A fixed count is
// used rather than the sub-group size so that the access pattern does not
// change with whatever SIMD width the compiler picks for the kernel.
#ifndef STRIPE_ELEMENTS
#define STRIPE_ELEMENTS 32u
#endif

__kernel void bandwidth_striding_copy(__global uint4 *dst,
                                      __global const uint4 *src,
                                      const uint total_threads,
                                      const uint chunk_elements) {
  const uint from = (uint)get_global_id(0);
  __global uint4 *cd = dst + from;
  __global const uint4 *cs = src + from;
  const uint big_chunks = chunk_elements / PIPE_DEPTH;

  for (uint c = 0; c < big_chunks; c++) {
    uint4 pipe[PIPE_DEPTH];

#pragma unroll
    for (uint k = 0; k < PIPE_DEPTH; k++) {
      pipe[k] = cs[k * total_threads];
    }

#pragma unroll
    for (uint k = 0; k < PIPE_DEPTH; k++) {
      cd[k * total_threads] = pipe[k];
    }

    cs += PIPE_DEPTH * total_threads;
    cd += PIPE_DEPTH * total_threads;
  }

  for (uint c = big_chunks * PIPE_DEPTH; c < chunk_elements; c++) {
    *cd = *cs;
    cs += total_threads;
    cd += total_threads;
  }
}

// Alternates copy direction across fixed-width stripes so that a single kernel
// drives host-to-device and device-to-host traffic concurrently. The arguments
// are not named dst/src because the roles swap between stripes.
__kernel void bandwidth_split_copy(__global uint4 *a, __global uint4 *b) {
  const uint idx = (uint)get_global_id(0);
  const uint stripe = idx / STRIPE_ELEMENTS;

  if (stripe & 1u) {
    a[idx] = b[idx];
  } else {
    b[idx] = a[idx];
  }
}
