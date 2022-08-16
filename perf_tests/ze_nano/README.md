# Description
ze_nano is a performance benchmark suite for individual function calls. Some of the measurements are latency, instruction count, cycle count, function calls per second. In addition, it's integrated with gtest to allow easy test filtering.

# Prerequisites
* libpapi library on Linux systems is required. Metrics that use hardware counters such as cycle count and instruction count are only supported on Linux systems as the libpapi library is used. If libpapi is not installed in the system, ze_nano will omit hardware counter metrics.
* For ze_nano to access hardware counters, they have to be enabled via a sysfs variable on Linux systems by:
```
    sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'
```

# How to Build it
See Build instructions in [BUILD](../BUILD.md) file.

# How to Run it
To run all tests, use the following command. Options for filtering tests are below.
```
    cd bin
    ./ze_nano
```

# Additional Options
* To view tests available:
```
    $ ./ze_nano --h
    Select tests to be run. If none specified, all test cases will be run.

    Allowed options:
    --help                                produce help message
    --zeKernelSetArgumentValue_Buffer     enable this test case
    --zeKernelSetArgumentValue_Immediate  enable this test case
    --zeKernelSetArgumentValue_Image      enable this test case
    --zeCommandListAppendLaunchKernel     enable this test case
    --zeCommandQueueExecuteCommandLists   enable this test case
    --zeDeviceGroupGetMemIpcHandle        enable this test case
```

* To select tests available:
```
      $ ./ze_nano --zeKernelSetArgumentValue_Buffer --zeCommandQueueExecuteCommandLists
```
