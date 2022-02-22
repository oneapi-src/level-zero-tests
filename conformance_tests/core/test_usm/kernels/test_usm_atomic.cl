/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void atomic_copy_kernel(global atomic_int *destination,
                               global atomic_int *source) {
  const size_t tid = get_global_id(0);
  atomic_store(&destination[tid], atomic_load(&source[tid]));
}
