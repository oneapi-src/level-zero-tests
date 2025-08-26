/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void memory_sum(__global const int* input, __global int* partial_sums, __local int* local_data, const int N) {
    const int gid = get_global_id(0);
    const int lid = get_local_id(0);

    int sum = 0;
    for (int i = gid; i < N; i += get_global_size(0)) {
        sum += input[gid];
    }
    local_data[lid] = sum;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int stride = get_local_size(0) / 2; stride > 0; stride /= 2) {
        if (lid < stride) {
            local_data[lid] += local_data[lid + stride];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (lid == 0) {
        partial_sums[get_group_id(0)] = local_data[0];
    }
}

__kernel void memory_sum_atomic(__global const int* input, __global int* partial_sums, __local int* local_data, const int N, __global int* result) {
    const int gid = get_global_id(0);
    const int lid = get_local_id(0);

    int sum = 0;
    for (int i = gid; i < N; i += get_global_size(0)) {
        sum += input[gid];
    }
    local_data[lid] = sum;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int stride = get_local_size(0) / 2; stride > 0; stride /= 2) {
        if (lid < stride) {
            local_data[lid] += local_data[lid + stride];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (lid == 0) {
        partial_sums[get_group_id(0)] = local_data[0];
    }

    if (lid == 0) {
        atomic_inc(&partial_sums[N]); // Use partial_sums[N] as a counter
    }
    barrier(CLK_GLOBAL_MEM_FENCE);

    if (gid == 0 && lid == 0) {
        while (partial_sums[N] < get_num_groups(0)) {
            // Wait until all workgroups have finished
        }

        int total = 0;
        for (int i = 0; i < get_num_groups(0); ++i) {
            total += partial_sums[i];
        }
        *result = total;
    }
}
