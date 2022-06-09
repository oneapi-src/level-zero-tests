/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#pragma OPENCL EXTENSION __cl_clang_function_pointers : enable
typedef void (*fptr)(__global int*);
__kernel void add_int(__global int *a) {
  *a += 2;
}

__kernel void module_call_fptr(long fptr_mem, __global int *values) {
  fptr my_fptr = (fptr)(fptr_mem);
  values[0] = 1;
  my_fptr(values);
}
