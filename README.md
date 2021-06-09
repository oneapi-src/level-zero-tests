# oneAPI Level Zero Tests

oneAPI Level Zero test source repository contains:
 * Conformance test content for validating all features of a oneAPI Level Zero driver.
 * Performance Benchmarks implemented using the oneAPI Level Zero Specification.
 * Tests for validation Level Zero Loader Layers

## Getting Started

**Prerequisites:**
 * oneAPI Level Zero Loader v1.3.6 
 * Compiler with C++11 support
 * GCC 5.4 or newer
 * Clang 3.8 or newer
 * CMake 3.8 or newer

## Build

See the [BUILD](BUILD.md) file for build instructions.

## Environment Variables

* `LZT_DEFAULT_DEVICE_IDX` = [`INTEGER`] Identifying the index of the default device to load when calling get_default_device test_harness function.
* `LZT_DEFAULT_DRIVER_IDX` = [`INTEGER`] Identifying the index of the default driver to load when calling get_default_driver test_harness function.
* `LZT_DEFAULT_DEVICE_NAME` = [`STRING`] Identifying the name of the default device to load when calling get_default_device test_harness function.

*NOTE: `LZT_DEFAULT_DEVICE_NAME` will be used if set, otherwise `LZT_DEFAULT_DEVICE_IDX` will be used.*

## Contribute

See [CONTRIBUTING](CONTRIBUTING.md) for more information.

## License

Distributed under the MIT license. See [LICENSE](LICENSE) for more information.

## Security

See Intel's [Security Center](https://www.intel.com/content/www/us/en/security-center/default.html) for information on how to report a potential security issue or vulnerability.

See also [SECURITY](SECURITY.md).
