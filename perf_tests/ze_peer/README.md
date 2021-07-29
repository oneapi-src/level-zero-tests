# Description
ze_peer is a performance benchmark suite for measuing peer-to-peer bandwidth
and latency.

# How to Build it

### Building Level Zero Performance Tests

```
mkdir build
cd build
cmake
  -D CMAKE_INSTALL_PREFIX=$PWD/out/perf_tests
  ..
cd perf_tests
make -j`nproc`
make install
```

ze_peer test executable is installed in your CMAKE build/out/perf_tests directory.

# How to Run it
To run, use the following command options.
```
 ./ze_peer -h

 ze_peer [OPTIONS]

 OPTIONS:
  -t, string                  selectively run a particular test
      ipc                     selectively run IPC tests
      ipc_bw                  selectively run IPC bandwidth test
      ipc_latency             selectively run IPC latency test
      transfer_bw             selectively run transfer bandwidth test
      latency                 selectively run latency test
  -a                          run all above tests [default]
  -q                          query for number of engines available
  -g, number                  select engine group (default: 0)
  -i, number                  select engine index (default: 0)
  -h, --help                  display help message
```
