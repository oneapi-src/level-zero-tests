# oneAPI Level Zero Tests Build Guide

## Dependencies

### Linux Dependencies
Requires the `level-zero`, `level-zero-devel`, `opencl-headers`, 
`libpng-dev`, `ocl-icd-opencl-dev`, `libboost-all-dev`, & `libva-dev` packages
to be installed.

### Dependencies
For building against level-zero, you can either build against the version you
have installed on your system (automatic, Linux only), or specify an install
prefix with the `L0_ROOT` cmake flag during configuration.
This `L0_ROOT` must point to the top-level install directory where level-zero was installed.
for example: `-DL0_ROOT=/home/username/level-zero/build/output/`.

Some tests depend on level-zero's OpenCL interop functionality in order to work.
If OpenCL is available on the system (specify a non-standard path with
`OPENCL_ROOT`), then the interop support will be enabled automatically. To
require the interop support, set the `REQUIRE_LEVELZERO_OPENCL_INTEROP` cmake
flag to `YES`.

Some benchmarks written against OpenCL are included to enable easy performance
comparisons. These benchmarks will be built automatically if OpenCL is available
on the system. To require that these benchmarks also be built, set the
`REQUIRE_OPENCL_BENCHMARKS` cmake flag to `YES`.

[Google Test](https://github.com/google/googletest) is used by all tests in this
repo for handling test case generation and result analysis.

Google Test is included as a submodule at `third_party/googletest`. You can
provide a path to your own version with the `GTEST_ROOT` cmake flag.

## Building

This project uses cmake to configure the build. By default, all the tests are
built.

The `install` target will by default create an `out` directory in your cmake
build directory containing the built test executables and their data files.
Nothing will get installed to any system paths. You can override the default
install location by setting `CMAKE_INSTALL_PREFIX`.

```
mkdir build
cd build
cmake -D CMAKE_INSTALL_PREFIX=$PWD/../out ..
cmake --build . --config Release --target install
```

### Building a subset of the test executables

Test executables are divided into a group hierarchy, and it is possible to
select a specific grouping of test executables for build using the `GROUP`
cmake flag. The following group specifiers are available:

  - `/`: All tests.
  - `/perf_tests`: All the performance tests.
  - `/negative_tests`: All the negative tests.
  - `/conformance_tests`: All the conformance tests.
  - `/conformance_tests/core`: All of the conformance tests for the core API.
  - `/conformance_tests/tools`: All of the conformance tests for the tools API.
  - `/conformance_tests/tools/tracing`: All of the tools API conformance tests
    related to tracing.
  - `/conformance_tests/tools/sysman`: ALl of the tools API conformance tests
    relating to system management.
  - `/conformance_tests/tools/pin`: ALl of the tools API conformance tests
    relating to Instrumenting of L0 applications.

```
cmake
  -D GROUP=/perf_tests
  -D CMAKE_INSTALL_PREFIX=$PWD/../out/perf_tests
  ..
cmake --build . --config Release --target install
```

The group that any particular test executable belongs to can be seen by looking
in its `CMakeLists.txt` file at the `add_lzt_test()` call.
