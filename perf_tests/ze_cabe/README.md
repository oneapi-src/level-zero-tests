# Description
ze_cabe is a GeekBench/Basemark/CompuBench-style benchmark to compare the performance of level-zero and opencl. It is a part of compute-api-bench, which also tests vulkan and dx11.

As for the scenarios, it currently has Sobel as an image processing scenario, Mandelbrot as fractal generation scenario and BlackScholes (fp32 and fp64) as a finance workload scenario (+SimpleAdd as a HelloWorld type of application).

As these APIs are quite different, API calls needed to run the workloads are divided into 4 groups: device creation, kernel compilation, buffer & command list creation and work execution. The benchmark reports the time taken to execute each group. The table below shows how this is currently done for OpenCL and Level-Zero.

```
                        | OpenCL                        | Level-Zero
------------------------|-------------------------------|-----------------------------------
Device Creation         | clGetPlatformIDs              | zeInit
                        | clGetDeviceIDs                | zeDriverGet
                        | clCreateContext               | zeDeviceGet
------------------------|-------------------------------|-----------------------------------
Kernel Compilation      | clCreateProgramWithSource     | zeModuleCreate
                        | clBuildProgram                | zeKernelCreate
------------------------|-------------------------------|-----------------------------------
Buffer&CmdList Creation | clCreateBuffer                | zeDriverAllocDeviceMem
                        | clCreateCommandQueue          | zeKernelSetGroupSize
                        | clCreateKernel                | zeKernelSetArgumentValue
                        | clSetKernelArg                | zeCommandListCreate
                        |                               | zeCommandListAppendMemoryCopy
                        |                               | zeCommandListAppendBarrier
                        |                               | zeCommandListAppendLaunchKernel
                        |                               | zeCommandListAppendBarrier
                        |                               | zeCommandListAppendMemoryCopy
                        |                               | zeCommandListClose
                        |                               | zeCommandQueueCreate
------------------------|-------------------------------|-----------------------------------
Work execution          | clEnqueueWriteBuffer          | zeCommandQueueExecuteCommandLists
                        | clEnqueueTask                 | zeCommandQueueSynchronize
                        | clEnqueueReadBuffer           |
```

The results are presented in the following way. 

![cabe-result][img:cabe-result]

[img:cabe-result]: ../../doc/cabe-result.png

With each time measurement, standard deviation (SD) is reported to show result variation.

# Scenarios
Currently, there are five scenarios implemented for each API: simpleadd, mandelbrot, sobel, blackscholesfp32 and blackscholesfp64. 
- simpleadd - a naïve implementation of adding 1 to all elements of buffer a and storing the result in buffer b; GWS=LWS=1.
- mandelbrot - generating Mandelbrot fractal of a given size (in our case it is 1024x1024); GWS=1024x1024, LWS=16x16.
- sobel - finding edges in images (512x512 image of Lena in this case); GWS=512x512, LWS=16x16.
- blackscholes fp32 - calculating Call and Put values for 1 mln options using Black–Scholes formula; GWS=1024x1024, LWS=256x1x1.
- blackscholes fp64 - the same as above in double precission.

# Prerequisite
Requires L0 and OpenCL UMD 
  
# How to Build it
Built as performance test for level_zero_test library

# How to Run it
To run all benchmarks, use the following command. 
```
    ./ze_cabe
```

Optional parameters:
```
 -api <api> - Valid values: opencl, level-zero, all. The default is all. 
 -scenario <scenario> - Valid values: simpleadd, mandelbrot, sobel, blackscholesfp32,
                        blackscholesfp64, all. The default is all.
 -iterations <X> - X is a value between 1..200. The default is 30.
 -median - uses median instead of default mean for reporting detailed results.
 -csv <filename> - saves results to a filename file in csv format. 
 -color - presents SDs in color (does not work in Windows cmd).
```
