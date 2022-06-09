/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#pragma OPENCL EXTENSION __cl_clang_function_pointers : enable
typedef int (*fptr)(int);
int add_int(int a) {
  return a + 1;
}

__kernel void module_call_fptr(long fptr_mem, __global int *values) {
  fptr my_fptr = (fptr)(fptr_mem);
  int val = 1;
  values[0] = my_fptr(val);
}
