/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

global int global_variable = 123;

kernel void test() { global_variable = 0; }