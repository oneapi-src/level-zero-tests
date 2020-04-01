/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */


kernel void many_buffers(global int *buf1, global int *buf2, global int *buf3, global int *buf4, global int *buf5) {
    
    const int id = get_global_id(0);
    if(id>0)
        return;
    *buf1=1;
    *buf2=2;
    *buf3=3;
    *buf4=4;
    *buf5=5;
}

  kernel void many_2d_images(read_write image2d_t image1, read_write image2d_t image2, read_write image2d_t image3, 
                             read_write image2d_t image4, read_write image2d_t image5) {
  const int id = get_global_id(0);
  if(id>0)
    return;
  int2 read_coord;
  int2 write_coord;
  uint4 elem;
  //read pixel at location ([imagenum],[imagenum]) and write that pixel to ([imagenum+10],[imagenum+10])
  read_coord = (int2)(1,1);
  write_coord = (int2)(11,11);
  elem = read_imageui(image1, read_coord);
  write_imageui(image1, write_coord, elem);

  read_coord = (int2)(2,2);
  write_coord = (int2)(12,12);
  elem = read_imageui(image2, read_coord);
  write_imageui(image2, write_coord, elem);


  read_coord = (int2)(3,3);
  write_coord = (int2)(13,13);
  elem = read_imageui(image3, read_coord);
  write_imageui(image3, write_coord, elem);

  read_coord = (int2)(4,4);
  write_coord = (int2)(14,14);
  elem = read_imageui(image4, read_coord);
  write_imageui(image4, write_coord, elem);

  read_coord = (int2)(5,5);
  write_coord = (int2)(15,15);
  elem = read_imageui(image5, read_coord);
  write_imageui(image5, write_coord, elem);

}

kernel void many_samplers(sampler_t samp1, sampler_t samp2, sampler_t samp3, sampler_t samp4, sampler_t samp5){
    const int id = get_global_id(0);
    if(id>0)
        return;

}

kernel void many_locals(__local char* local1, __local char* local2, __local char* local3, __local char* local4, __global char* global1){
  const int id = get_global_id(0);
  if(id>0)
      return;
  const int size = 256;
  for(int i=0; i< size; i++){
    local1[i] = 0x55;
    local2[i] = local1[i];
    local3[i] = local2[i];
    local4[i] = local3[i];
    global1[i] = local4[i];
  }
}

kernel void many_args_all_types(global int *buf1, global int *buf2, local int *local_buf, 
                                global int *buf3, global int *buf4, read_write image2d_t image1,
                                read_write image2d_t image2, read_write image2d_t image3,
                                read_write image2d_t image4, read_write image2d_t image5, 
                                sampler_t samp1, sampler_t samp2){

  // Kernel will perform the following operations:
  // buffers[0] copied to buffers[2] using local mem as staging area
  // buffers[1] copied to buffers[3] using local mem as staging area
  // For each image, pixel value at coord ([imagenum],[imagenum])
  //   will be written to coord ([imagenum+10],[imagenum+10])
  const int id = get_global_id(0);
  if(id>0)
      return;

  //do buffer copies
  *local_buf = *buf1;
  *buf3 = *local_buf;
  *local_buf = *buf2;
  *buf4 = *local_buf;
  
  //now do image operations
  int2 read_coord;
  int2 write_coord;
  uint4 elem;
  //read pixel at location ([imagenum],[imagenum]) and write that pixel to ([imagenum+10],[imagenum+10])
  read_coord = (int2)(1,1);
  write_coord = (int2)(11,11);
  elem = read_imageui(image1, read_coord);
  write_imageui(image1, write_coord, elem);
  read_coord = (int2)(2,2);
  write_coord = (int2)(12,12);
  elem = read_imageui(image2, read_coord);
  write_imageui(image2, write_coord, elem);
  read_coord = (int2)(3,3);
  write_coord = (int2)(13,13);
  elem = read_imageui(image3, read_coord);
  write_imageui(image3, write_coord, elem);
  read_coord = (int2)(4,4);
  write_coord = (int2)(14,14);
  elem = read_imageui(image4, read_coord);
  write_imageui(image4, write_coord, elem);
  read_coord = (int2)(5,5);
  write_coord = (int2)(15,15);
  elem = read_imageui(image5, read_coord);
  write_imageui(image5, write_coord, elem);
}

