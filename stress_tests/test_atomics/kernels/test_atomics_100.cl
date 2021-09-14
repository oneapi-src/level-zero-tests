/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void test_atomics_add_overlap(__global unsigned int *src) {
  __local volatile atomic_uint working_thread_memory[1];
  const size_t gid = get_global_id(0);
  const size_t lid = get_local_id(0);
  const size_t lws = get_local_size(0);

  if (lid == 0)
    for (uint idx = 0; idx < lws; idx++)
      atomic_store_explicit(working_thread_memory + idx, src[gid],
                            memory_order_relaxed, memory_scope_work_group);

  barrier(CLK_LOCAL_MEM_FENCE);
  for (int i = 0; i < 100; i++)
    atomic_fetch_add_explicit(&working_thread_memory[0], 1u,
                              memory_order_relaxed, memory_scope_work_group);

  barrier(CLK_LOCAL_MEM_FENCE);
  src[gid] = atomic_load_explicit(working_thread_memory, memory_order_relaxed,
                                  memory_scope_work_group);
}

kernel void test_atomics_add_non_overlap(__global unsigned int *src) {
  __local volatile atomic_uint working_thread_memory[1024];
  const size_t gid = get_global_id(0);
  const size_t lid = get_local_id(0);
  const size_t lws = get_local_size(0);
  src = src + lws * get_group_id(0);
  if (lid == 0)
    for (uint idx = 0; idx < lws; idx++)
      atomic_store_explicit(working_thread_memory + idx, src[idx],
                            memory_order_relaxed, memory_scope_work_group);

  barrier(CLK_LOCAL_MEM_FENCE);
  for (int i = 0; i < 100; i++)
    atomic_fetch_add_explicit(&working_thread_memory[lid], (uint)lid + 1u,
                              memory_order_relaxed, memory_scope_work_group);

  barrier(CLK_LOCAL_MEM_FENCE);

  if (get_local_id(0) == 0) // first thread in workgroup
    for (uint idx = 0; idx < lws; idx++)
      src[idx] =
          atomic_load_explicit(working_thread_memory + idx,
                               memory_order_relaxed, memory_scope_work_group);
}

kernel void test_atomics_multi_op_overlap(__global unsigned int *src) {
  __local volatile atomic_uint working_thread_memory[1];
  const size_t gid = get_global_id(0);
  const size_t lid = get_local_id(0);
  const size_t lws = get_local_size(0);
  if (lid == 0)
    for (uint idx = 0; idx < lws; idx++)
      atomic_store_explicit(working_thread_memory + idx, src[gid],
                            memory_order_relaxed, memory_scope_work_group);

  barrier(CLK_LOCAL_MEM_FENCE);
  for (int i = 0; i < 100; i++) {
    atomic_fetch_add_explicit(&working_thread_memory[0], 10u,
                              memory_order_relaxed, memory_scope_work_group);
    atomic_fetch_sub_explicit(&working_thread_memory[0], 5u,
                              memory_order_relaxed, memory_scope_work_group);
    atomic_fetch_max_explicit(&working_thread_memory[0], 7u,
                              memory_order_relaxed, memory_scope_work_group);
    atomic_fetch_min_explicit(&working_thread_memory[0], 12u,
                              memory_order_relaxed, memory_scope_work_group);
  }

  barrier(CLK_LOCAL_MEM_FENCE);
  src[gid] = atomic_load_explicit(working_thread_memory, memory_order_relaxed,
                                  memory_scope_work_group);
}

kernel void test_atomics_multi_op_non_overlap(__global unsigned int *src) {
  __local volatile atomic_uint working_thread_memory[1024];
  const size_t gid = get_global_id(0);
  const size_t lid = get_local_id(0);
  const size_t lws = get_local_size(0);
  src = src + lws * get_group_id(0);
  if (lid == 0)
    for (uint idx = 0; idx < lws; idx++)
      atomic_store_explicit(working_thread_memory + idx, src[idx],
                            memory_order_relaxed, memory_scope_work_group);

  barrier(CLK_LOCAL_MEM_FENCE);
  for (int i = 0; i < 100; i++) {
    atomic_fetch_min_explicit(&working_thread_memory[lid], (uint)lid + 4u,
                              memory_order_relaxed, memory_scope_work_group);
    atomic_fetch_add_explicit(&working_thread_memory[lid], (uint)lid + 10u,
                              memory_order_relaxed, memory_scope_work_group);
    atomic_fetch_max_explicit(&working_thread_memory[lid], (uint)lid + 7u,
                              memory_order_relaxed, memory_scope_work_group);
    atomic_fetch_sub_explicit(&working_thread_memory[lid], (uint)lid + 5u,
                              memory_order_relaxed, memory_scope_work_group);
  }
  barrier(CLK_LOCAL_MEM_FENCE);

  if (get_local_id(0) == 0) // first thread in workgroup
    for (uint idx = 0; idx < lws; idx++)
      src[idx] =
          atomic_load_explicit(working_thread_memory + idx,
                               memory_order_relaxed, memory_scope_work_group);
}
