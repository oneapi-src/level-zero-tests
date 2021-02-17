typedef long TYPE;
kernel void scratch_kernel(global int *resIdx, global TYPE *src, global TYPE *dst) {
    const int lid = get_local_id(0);
    const int gid = get_global_id(0);

    TYPE res1 = src[gid * 3];
    TYPE res2 = src[gid * 3 + 1];
    TYPE res3 = src[gid * 3 + 2];

    local TYPE locMem[32];
    locMem[lid] = res1;
    barrier(CLK_LOCAL_MEM_FENCE);
    barrier(CLK_GLOBAL_MEM_FENCE);
    TYPE res = (locMem[resIdx[gid]] * res3) * res2 + res1;
    dst[gid] = res;
}
