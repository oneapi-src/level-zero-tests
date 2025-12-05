/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void test_device_memory1_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory2_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory3_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory4_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory5_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory6_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory7_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory8_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory9_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory10_unit_size1(__global char *src, __global char *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory1_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory2_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory3_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory4_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory5_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory6_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory7_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory8_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory9_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
kernel void test_device_memory10_unit_size4(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}

struct buffer {
  uint *data;
};

kernel void test_device_memory1_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory2_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory3_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory4_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory5_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory6_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory7_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory8_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory9_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

kernel void test_device_memory10_indirect(__global struct buffer *src_ptrs, __global struct buffer *dst_ptrs, uint dispatch_id) {
	uint *src = src_ptrs[dispatch_id].data;
    uint *dst = dst_ptrs[dispatch_id].data;
    size_t tid = get_global_id(0);
    dst[tid] = src[tid];
}

