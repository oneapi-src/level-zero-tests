/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void cooperative_reduction(__global const int* input, __global int* output,
                                    __global int* shared_buffer, const int elements_per_group) {
  int gid = get_global_id(0);
  int group_id = get_group_id(0);
  int lid = get_local_id(0);

  // Each workgroup computes a partial sum
  int partial_sum = 0;
  int start = group_id * elements_per_group;
  int end = start + elements_per_group;
  for (int i = start + lid; i < end; i += get_local_size(0)) {
    partial_sum += input[i];
  }

  // Reduce within workgroup
  __local float local_sums[256]; // adjust size as needed
  local_sums[lid] = partial_sum;
  barrier(CLK_LOCAL_MEM_FENCE);

  // Final reduction in workgroup
  if (lid == 0) {
    float group_sum = 0.0f;
    for (int i = 0; i < get_local_size(0); ++i) {
      group_sum += local_sums[i];
    }
    shared_buffer[group_id] = group_sum;
  }

  // Device-wide barrier: synchronize all workgroups
  work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_device);

  // One workgroup performs final reduction
  if (group_id == 0 && lid == 0) {
    float total = 0.0f;
    int num_groups = get_num_groups(0);
    for (int i = 0; i < num_groups; ++i) {
      total += shared_buffer[i];
    }
    output[0] = total;
  }
}

__kernel void cooperative_reduction_atomic(__global const int* input, __global int* output,
                                    __global int* shared_buffer, const int elements_per_group) {
  int gid = get_global_id(0);
  int group_id = get_group_id(0);
  int lid = get_local_id(0);

  // Each workgroup computes a partial sum
  int partial_sum = 0;
  int start = group_id * elements_per_group;
  int end = start + elements_per_group;
  for (int i = start + lid; i < end; i += get_local_size(0)) {
    partial_sum += input[i];
  }

  // Reduce within workgroup
  __local float local_sums[256]; // adjust size as needed
  local_sums[lid] = partial_sum;
  barrier(CLK_LOCAL_MEM_FENCE);

  // Final reduction in workgroup
  if (lid == 0) {
    float group_sum = 0.0f;
    for (int i = 0; i < get_local_size(0); ++i) {
      group_sum += local_sums[i];
    }
    shared_buffer[group_id] = group_sum;
  }

  // Device-wide barrier: synchronize all workgroups
  work_group_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_device);

  // One workgroup performs final reduction
  if (group_id == 0 && lid == 0) {
    float total = 0.0f;
    int num_groups = get_num_groups(0);
    for (int i = 0; i < num_groups; ++i) {
      total += shared_buffer[i];
    }
    output[0] = total;
  }
}
