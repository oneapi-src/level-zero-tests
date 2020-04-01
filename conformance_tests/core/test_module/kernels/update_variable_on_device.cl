/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

global int global_variable = 1;

kernel void test() { global_variable = 2; }
