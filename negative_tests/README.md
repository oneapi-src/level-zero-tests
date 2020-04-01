# oneAPI Level Zero Negative Tests

Negative test content for validating all features of the oneAPI Level Zero validation layer which checks and handles all invalid inputs to the oneAPI Level Zero APIs.

Validation Layer must be enabled via setting the `ZE_ENABLE_VALIDATION_LAYER` environment variable to `1` to get valid results.

Parameter Validation is enabled via the setting the `ZE_ENABLE_PARAMETER_VALIDATION` environment variable to `1` which will enable checking the individual arguments to an L0 API. This requires that ZE_ENABLE_VALIDATION_LAYER was already enabled.

## Getting Started

**Prerequisites:**
 * oneAPI Level Zero
 * Compiler with C++11 support
 * GCC 5.4 or newer
 * Clang 3.8 or newer
 * CMake 3.8 or newer

## Build

Build instructions in [BUILD](BUILD.md) file.

## Running

**Executing the conformance tests on Linux**
 * Execute each test individually
    * (Optional) Set LD_LIBRARY_PATH= "path to libze_loader.so.*"
    * ./test_<filename>