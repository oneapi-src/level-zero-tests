# oneAPI Level Zero Conformance Tests

Conformance test content for validating all features of a oneAPI Level Zero driver.

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