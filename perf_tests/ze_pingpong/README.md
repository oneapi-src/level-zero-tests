# Description
ze_pingpong is a performance benchmark suite ported from clpingpong which measures round-trip delay times for a light-weight kernel execution of 32-bit addition on a 32-bit integer followed by 32-bit subtraction of 32-bit integer on host CPU.  Transfers or mappings of the kernel 32-bit integer to the host CPU add to the delay.

It is based off the clpingpong here licensed with the MIT License:  https://github.com/ekondis/clpingpong

ze_pingpong measures the following:
* Kernel execution time for integer argument in Device Memory
* Kernel execution time for integer argument in Host Memory
* Kernel execution time for integer argument in Shared Memory
* Round-trip (ping-pong) time for kernel integer argument in Device Memory and transfer to Host for decrement
* Round-trip time for kernel integer argument in Host Memory and decrement in Host
* Round-trip time for kernel integer argument in Shared Memory and memcpy to Host for decrement (Note:  this is intended to resemeble the OpenCL mapping operation)
* Host overhead for transfer/mapping operations

# How to Build it
See Build instructions in [BUILD](../BUILD.md) file.

# How to Run it
To run all benchmarks, use the following command. 
```
    ./ze_pingpong
```

