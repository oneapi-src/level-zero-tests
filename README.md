# oneAPI Level Zero Tests

oneAPI Level Zero test source repository containing:
 * Conformance test content for validating all features of a oneAPI Level Zero driver.
 * Performance Benchmarks implemented using the oneAPI Level Zero Specification.

## Getting Started

**Prerequisites:**
 * oneAPI Level Zero
 * Compiler with C++11 support
 * GCC 5.4 or newer
 * Clang 3.8 or newer
 * CMake 3.8 or newer

## Build

Build instructions in [BUILD](BUILD.md) file.

## Environment Variables
* `LZT_DEFAULT_DEVICE_IDX` = [`INTEGER`] Identifying the index of the default device to load when calling get_default_device test_harness function.
* `LZT_DEFAULT_DEVICE_NAME` = [`STRING`] Identifying the name of the default device to load when calling get_default_device test_harness function.

*NOTE: `LZT_DEFAULT_DEVICE_NAME` will be used if set, otherwise `LZT_DEFAULT_DEVICE_IDX` will be used.*
