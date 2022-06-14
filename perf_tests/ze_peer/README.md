# Description
ze_peer is a performance benchmark suite for measuing peer-to-peer bandwidth
and latency.

# How to Build it

See top-level [BUILD.md](../../BUILD.md) file.

# How to Run it
To run, use the following command options.
```
ze_peer: Level Zero microbenchmark to analyze the P2P performance
of a multi-GPU system.

To execute: ze_peer [OPTIONS]

By default, unidirectional transfer bandwidth and latency tests
are executed, for sizes between 8 B and 256 MB, for write and read
operations, between all devices detected in the system, using the
first compute engine in the device.


 OPTIONS:
  -b                          run bidirectional mode. Default: Not set.
  -c                          run continuously until hitting CTRL+C. Default: Not set.
  -i                          number of iterations to run. Default: 50.
  -z                          size to run in bytes. Default: 8192(8MB) to 268435456(256MB).
  -v                          validate data (only 1 iteration is executed). Default: Not set.
  -t                          type of transfer to measure
      transfer_bw             run transfer bandwidth test
      latency                 run latency test
  -o                          operation to perform
      read                    read from remote
      write                   write to remote

 Engine selection:

  -q                          query for number of engines available
  -u                          engine to use (use -q option to see available values).
                              Accepts a comma-separated list of engines when running
                              parallel tests with multiple targets.
 Device selection:

  -d                          comma separated list of destination devices
  -s                          comma separated list of source devices

 Tests:
  --parallel_single_target    Divide the buffer into the number of engines passed
                              with option -u, and perform parallel copies using those
                              engines from the source passed with option -s to a single
                              target specified with option -d.
                              By default, copy is made from device 0 to device 1 using
                              all available engines with compute capability.
                              Extra options: -x

  --parallel_multiple_targets  Perform parallel copies from the source passed with option -s
                              to the targets specified with option -d, each one using a
                              separate engine specified with option -u.
                              By default, copy is made from device 0 to all other devices,
                              using all available engines with compute capability.
                              Extra options: -x
  -x                          for unidirectional parallel tests, select where to place the queue
      src                     use queue in source
      dst                     use queue in source

  --ipc                       perform a copy between two devices, specified by options -s and -d,
                              with each device being managed by a separate process.

  --version                   display version
  -h, --help                  display help message
  ```

# Sample commands

Query for engines available in the devices. Numbers under
the `-u` column can be used to select a set of engines. In
sample below, two devides were discovered, each one having
5 engines: 4 with compute and copy capabilities, and 1 with
only copy capabilities.

```
 ./ze_peer -q
====================================================
 Device 0
        1400 MHz
----------------------------------------------------
| Group  | Number of Queues | Compute | Copy |  -u |
====================================================
|   0    |         1        |    X    |   X  |     |
|        |                  |         |      |   0 |
|        |                  |         |      |   1 |
|        |                  |         |      |   2 |
|        |                  |         |      |   3 |
----------------------------------------------------
|   1    |         1        |         |   X  |     |
|        |                  |         |      |   4 |
====================================================
====================================================
 Device 1
        1400 MHz
----------------------------------------------------
| Group  | Number of Queues | Compute | Copy |  -u |
====================================================
|   0    |         4        |    X    |   X  |     |
|        |                  |         |      |   0 |
|        |                  |         |      |   1 |
|        |                  |         |      |   2 |
|        |                  |         |      |   3 |
----------------------------------------------------
|   1    |         1        |         |   X  |     |
|        |                  |         |      |   4 |
====================================================
```

Run default, with BW and latency results, between all available devices, using first engine present.
```
./ze_peer
```

Run BW test, for only 256 MB
```
./ze_peer -t transfer_bw -z 268435456
```

Run latency tests, for 64B, between devices 1 and 3 (assuming the system has such devices).
```
./ze_peer -t latency -z 64 -s 1 -d 3
```

Run parallel_single_target test, between devices 0 and 1, using engines 2 and 3, for a size of
256 MB. In this case, the buffer is divided into the number of engines (2), for a resulting
chunk size of 128MB. Then, two copies are submitted in parallel, per chunk, and per engine.
```
./ze_peer --parallel_single_target -t transfer_bw -z 268435456 -s 0 -d 1 -u 2,3
```

Run parallel_multiple_targets test, with source device as 1, and destination devices 2 and 3,
using engines 0 and 1, for a 256MB buffer. In this case, two allocations of such size are created.
Then, two copies are submitted in parallel, one /between devices 1 and 2, and other between
devices 1 and 3, using engines 0 and 1, respectively.
```
 ./ze_peer --parallel_multiple_targets -t transfer_bw -z 268435456 -s 1 -d 2,3 -u 0,1
```

Same as above, but bidirectional. In this case, two additional copies are submitted from
devices 2 and 3, in addition to those in device 1. All copies are done in parallel.
```
./ze_peer --parallel_multiple_targets -t transfer_bw -z 268435456 -s 1 -d 2,3 -u 0,1 -b
```
