/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void cooperative_kernel(__global int* input, __global int* output, const int N) {
  const int gid = get_global_id(0);

  barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
  output[gid] = input[gid] + N;
  barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
}

__kernel void cooperative_kernel_atomic(__global int* input, __global int* output, const int N) {
  const int gid = get_global_id(0);

  barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
  output[gid] = 0;
  atomic_add(&output[gid], input[gid] + N);
  barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
}
