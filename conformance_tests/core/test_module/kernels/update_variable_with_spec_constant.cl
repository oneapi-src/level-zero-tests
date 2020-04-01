/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <opencl_spec_constant>
#include <opencl_work_item>

cl::spec_constant<uint64_t, 0> constant0{10};
cl::spec_constant<uint64_t, 1> constant1{30};
cl::spec_constant<uint64_t, 2> constant2{50};
cl::spec_constant<uint64_t, 3> constant3{70};

kernel void test(__global uint64_t *output) {
    *output = cl::get(constant0) + cl::get(constant1) + cl::get(constant2) + cl::get(constant3);
}
