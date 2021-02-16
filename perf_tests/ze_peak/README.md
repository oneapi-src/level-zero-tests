#Description
ze_peak is a performance benchmark suite ported from clpeak which profiles Ze devices to find their peak capacities for supported functionality.

This Benchmark is based off the clpeak here licensed with the "unlicense": https://github.com/krrishnarraj/clpeak.

`NOTE: Due to differences in API semantics between oneAPI Level Zero and OpenCL, this benchmark does not provide a 1:1 comparison to clpeak.`
`In particular, Level Zero uses shared memory allocations in order to provide flexibility and ease of use in moving memory allocations between`
`a host and device. Therefore, the transfer bandwidth measurements for shared memory taken below may not be directly comparable as a driver`
`might decide to migrate an allocation between host/device based on its own heuristics such as when accessed by either entity.`

ze_peak measures the following:
* Global Memory Bandwidth in GigaBytes Per Second
* Half Precision Compute in GigaFlops
* Single Precision Compute in GigaFlops
* Double Precision Compute in GigaFlops
* Integer Compute in GigaInteger Flops
* Memory Transfer Bandwidth in GigaBytes Per Second
  * GPU Copy Host <-> Shared Memory
  * System Memory Copy Host <-> Shared Memory
* Kernel Launch Latency in micro seconds
* Kernel Duration in micro seconds

#How to Build it
See Build instructions in [BUILD](../BUILD.md) file.

#How to Run it
To run all benchmarks, use the following command. Additional Options and filtering benchmarks are described in the next section.
```
    cd bin
    ./ze_peak
```

#Additional Options
* To look up options available:
```
      $ ./ze_peak -h
        -r, --driver num            choose driver (num starts with 0)
        -d, --device num            choose device   (num starts with 0)
        -e                          time using ze events instead of std chrono timer
                                    hide driver latencies [default: No]
        -t, string                  selectively run a particular test
            global_bw               selectively run global bandwidth test
            hp_compute              selectively run half precision compute test
            sp_compute              selectively run single precision compute test
            dp_compute              selectively run double precision compute test
            int_compute             selectively run integer compute test
            transfer_bw             selectively run transfer bandwidth test
            kernel_lat              selectively run kernel latency test
        -a                          run all above tests [default]
        -v                          enable verbose prints
        -i                          set number of iterations to run[default: 50]
        -w                          set number of warmup iterations to run[default: 10]
        -h, --help                  display help message

```

* Example: Run only the global_bw benchmark:
```
      $ ./ze_peak -t global_bw
```
