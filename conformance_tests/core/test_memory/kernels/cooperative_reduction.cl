/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void cooperative_reduction(__global const int* input, __global int* output,
                                    __local int* local_sums) {
  int gid = get_global_id(0);
  int group_id = get_group_id(0);
  int lid = get_local_id(0);
  int local_size = get_local_size(0);

  local_sums[lid] = input[gid];
  barrier(CLK_LOCAL_MEM_FENCE);

  if (lid == 0) {
    for (uint i = 1; i < local_size; i++) {
      local_sums[0] += local_sums[i];
    }
    output[group_id] = local_sums[0];
  }

  // Device-wide barrier: synchronize all workgroups
  global_barrier();

  if (group_id == 0 && lid == 0) {
    for (uint i = 1; i < get_num_groups(0); i++) {
      output[0] += output[i];
    }
  }
}

__kernel void cooperative_reduction_atomic(__global const int* input, __global int* output,
                                           __local int* local_sums) {
  int gid = get_global_id(0);
  int group_id = get_group_id(0);
  int lid = get_local_id(0);
  int local_size = get_local_size(0);

  local_sums[lid] = input[gid];
  barrier(CLK_LOCAL_MEM_FENCE);

  if (lid == 0) {
    for (uint i = 1; i < local_size; i++) {
      atomic_add(&local_sums[0], local_sums[i]);
    }
    output[group_id] = local_sums[0];
  }

  // Device-wide barrier: synchronize all workgroups
  global_barrier();

  if (group_id == 0 && lid == 0) {
    for (uint i = 1; i < get_num_groups(0); i++) {
      atomic_add(&output[0], output[i]);
    }
  }
}
