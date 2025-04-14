global int global_data[16];

kernel void test_global_data(global int *out)
{
    int gid = get_global_id(0);
    out[gid] = global_data[gid];
}
