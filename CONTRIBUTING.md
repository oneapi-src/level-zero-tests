# oneAPI Level Zero Tests Contribution Guidelines

We encourage anyone who wants to contribute to submit
[Issues](https://github.com/oneapi-src/level-zero-tests/issues) and
[Pull Requests](https://github.com/oneapi-src/level-zero-tests/pulls).

## C++ Coding Standards

* C++14 maximum support
* Avoid C Arrays, replace with `std::array<>` / `std::vector<>`
* Avoid "magic numbers"
* Avoid C-style memory allocations in favor of C++
* Use `nullptr` instead of `NULL`
* Don’t add `void` to empty argument lists
* Use `std::unique_ptr` in place of `std::auto_ptr`

## Naming Conventions

In addition, use the following naming conventions:

* Class - UpperCamelCase - `class MyClass`
* Class data member - snake_case_with_suffix - `MyClass::my_class_data_member_`
* Struct - UpperCamelCase - `struct MyStruct`
* Struct data member - snake_case - `MyStruct::my_struct_data_member`
* Function - snake_case - `void my_function()`
* Variable - snake_case - `int my_variable`
* Constant - snake_case - `const int my_constant`
* Enum - snake_case - `enum class my_enum`
* Enum member - snake_case - `my_enum::my_enum_member`
* Namespace - snake_case - `namespace my_namespace`
* Macro - CAPITALIZED_WITH_UNDERSCORES - `#define MY_MACRO`
* Module - snake_case - `my_module`
* GTEST Test cases:
  * Test names SHALL follow the
    [Given/When/Then naming convention][given_when_then].
  * A test requiring **exactly one sub-device** SHALL include one of
    `SingleSubDevice` or `OneSubDevice` in its name.
  * A test requiring **at least one sub-device** SHALL include `SubDevice` in
    its name.
  * A test requiring **more than one sub-device** SHALL include one of
    `MultipleSubDevice`, `DifferentSubDevice` or `TwoSubDevice` in its name.
  * A test requiring **exactly zero sub-devices** SHALL include `NoSubDevice` in
    its name.
  * A test requiring **exactly one root device** SHALL include one of
    `SingleRootDevice`, `SingleDevice`, `OneRootDevice` or `OneDevice` in its
    name.
  * A test requiring **at least one root device** MAY include one of
    `RootDevice` or `Device` in its name.
  * A test requiring **more than one root device** SHALL include one of
    `MultipleRootDevice`, `MultipleDevice`, `DifferentRootDevice`,
    `DifferentDevice`, `TwoRootDevice` or `TwoDevice` in its name.
  * A test requiring **exactly zero root devices** SHALL include one of
    `NoRootDevice` or `NoDevice` in its name.
  * A test referring to both a sub-device and a root device SHALL refer to the
    root device as a `RootDevice` instead of just a `Device`.
  * A test requiring **image support** SHALL include one of `Image` or `Sampler`
    in its name.

[given_when_then]: https://martinfowler.com/bliki/GivenWhenThen.html

## Code Formatting

Follow the [LLVM code formatting guidelines][llvm_code_formatting].

A [.clang-format](./.clang-format) file is included in this repository and
build targets are defined in [clang_tools.cmake](./cmake/clang_tools.cmake) in the [cmake](./cmake) directory for convenience. `clang-format` will
check and fix any formatting issues with your code, and `clang-format-check`
will check for issues and print a diff of the corrections required.

Examples:

```
cmake --build . --target clang-format-check
cmake --build . --target clang-format
```

[llvm_code_formatting]: https://llvm.org/docs/CodingStandards.html#source-code-formatting

## Test Type Guidelines

The tests written for oneAPI Level Zero should follow these guidelines depending on the test type written.

All tests are meant to be written as "Vendor & Platform agnostic" to ensure usage on any Level Zero Driver and supported device.

* Functional Tests
  * All tests verifying functionality are written in Google Test Format, see here for a guide: https://google.github.io/googletest/
    * Conformance
      * Tests which verify the correct usage of oneAPI Level Zero APIs per the specification: https://spec.oneapi.io/level-zero/latest/index.html
    * Negative
      * Tests which verify the failure conditions described in the oneAPI Level Zero specification: https://spec.oneapi.io/level-zero/latest/index.html
      * Is is strongly recommended to enable the environment variables ZE_ENABLE_VALIDATION_LAYER=1 and ZE_ENABLE_PARAMETER_VALIDATION=1 to enable the Validation Layer and verify the Parameters for these tests.
      * NOTE: The test may or may not verify the error handling thru support in the Validation Layer, the error handling is expected to be present between the Level Zero Driver and the Validation Layer.
    * Layer
      * Tests which verify features specifically in the layer(s) supported by the oneAPI Level Zero Loader. See here for layers currently supported: https://github.com/oneapi-src/level-zero/tree/master/source/layers
      * Test are expected to enable the tested layers during init. For example, ZE_ENABLE_TRACING_LAYER=1 is required for testing the Tracing Layer provided by the oneAPI Level Zero Loader.
    * Stress
      * Tests which stress the Level Zero Driver through resource limit testing and longevity.
      * These test verify the correct usage of oneAPI Level Zero APIs per the specification: https://spec.oneapi.io/level-zero/latest/index.html.
* Performance Tests
  * Performance tests are not required to follow the Google Test Format as the Functional tests since the goal is not checking for binary pass/fail results.
  * These tests are expected to demonstrate valid usage of oneAPI Level Zero APIs per the specification: https://spec.oneapi.io/level-zero/latest/index.html.
  * Performance tests are meant to provide usecases to analyze the quality of a given Level Zero Driver or Device under test.

## Code Review

Quality Code Review of the oneAPI Level Zero Tests is important for all Maintainers and Contributors to ensure that valid test cases are added that verify the oneAPI Level Zero specification.

### Review Checklist

When performing a code review please refer to this checklist to guide your comments:

* Does the code follow C++ Coding Standards? [C++ Coding Standards](#c-coding-standards)
* Does the code follow the naming conventions? [Naming Conventions](#naming-conventions)
* Are the tests using Level Zero Specification APIs per the spec? See here for the latest spec: https://spec.oneapi.io/level-zero/latest/index.html
* Are the tests using Level Zero APIs "Vendor & Platform agnostic"?
* Is the Code Modular or can the code be Modular? ie Can the code expand the test_harness to simplify the next written test (where applicable).
* Can the code handle Multiple Driver or Device environments?

## Kernels (SPV Files)

OpenCL C kernel source code (`.cl`) and binaries (`.spv`) should be placed in a
`kernels/` subdirectory of your test. The
`add_lzt_test` and `add_lzt_test_executable` CMake functions will search for
kernels in that directory.

Whenever you add or update any kernels, you must re-generate the SPIR-V binaries
and commit them with your changes. A specialized build of clang is required to
do this.

### Setup Clang to Build SPV From .cl

```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project/llvm/projects/
git clone https://github.com/KhronosGroup/SPIRV-LLVM-Translator.git
cd ../..
mkdir build
cd build
cmake -DLLVM_ENABLE_PROJECTS=clang -G "Unix Makefiles" ../llvm
cmake ../llvm

make llvm-spirv -j 8
make clang -j 8

sudo make -j 8 clang llvm-spirv install
```

### Build SPV File From .cl

```bash
clang -c -cl-std=CL2.0 -emit-llvm -target spir64 -o <filename>.bc <filename>.cl
llvm-spirv <filename>.bc -o <filename>.spv
```

## Test Feature Assignment

When adding a new test one needs to ensure that the new test case has features and tags assigned for use in the Level Zero Test Report Generator.

Each test case has features and tags assigned based on criteria in the following file:

[level_zero_report_utils.py](scripts/level_zero_report_utils.py)

These features are used in the test run script:

[run_test_report.py](scripts/run_test_report.py)

See here for details on the L0 Test Report Tool: [README.md](scripts/README.md)

The features assigned are determined either manually by the test creator by updates to the python scripts or are associated to a given feature by naming convention.

Three tags are assigned per test:

* test_feature_tag = Overall Category of the Feature (discrete, advanced, basic, tools, negative, stress)
  * discrete = Tests for features specific to discrete devices
  * advanced = Tests using features that are considered not base functionality ie non-smoke tests
  * basic = Very basic functionality of L0, tests used to determine if the L0 stack is functional.
  * tools = Tests for Tools features.
  * negative = Tests verifying error functionality.
  * stress = Tests verifying longevity and high resource usage.
* test_feature = General Feature being tested.
  * oneAPI Level Zero Core Features
    * Driver Handles
    * Device Handling
    * Barriers
    * Command Lists
    * Images
    * Command Queues
    * Fences
    * Device Memory
    * Host Memory
    * Shared Memory
    * Events
    * Kernels
    * Image Samplers
    * Sub-Devices
    * Allocation Residency
    * Inter-Process Communication
    * Peer-To-Peer
    * Unified Shared Memory
    * Thread Safety Support
  * oneAPI Level Zero Tools Features
    * Program Instrumentation
    * API Tracing
    * SysMan Frequency
    * SysMan Device Properties
    * SysMan PCIe
    * SysMan Power
    * SysMan Standby
    * SysMan LEDs
    * SysMan Memory
    * SysMan Engines
    * SysMan Temperature
    * SysMan Power Supplies
    * SysMan Fans
    * SysMan Reliability & Stability
    * SysMan Fabric
    * SysMan Diagnostics
    * SysMan Device Reset
    * SysMan Device Properties
    * SysMan Events
    * SysMan Frequency
    * SysMan Scheduler
    * SysMan Firmware
    * SysMan Perf Profiles
    * Metrics
    * Program Debug
* test_section = grouping of the test associated with functionality being tested for overall test plans.
  * tools = Tests spanning all oneAPI Level Zero Tools
  * negative = Tests exercising support for handling negative/invalid user input, expected to be run with a validation layer ie ZE_ENABLE_VALIDATION_LAYER=1
  * stress = Tests tests which exercise the maximum limits of a given oneAPI Level Zero Driver
  * core = Tests spanning all oneAPI Level Zero Core Features, this is the oneAPI Level Zero Certification for Production Stability of an L0 Stack

If a test does not get a feature assigned, then the run_test_report.py with fail with: `ERROR: test case " + test_name + " has no assigned feature`

## Sign Your Work

Please use the sign-off line at the end of your patch. Your signature certifies
that you wrote the patch or otherwise have the right to pass it on as an
open-source patch. To do so, if you can certify the below
(from [developercertificate.org](http://developercertificate.org/)):

```
Developer Certificate of Origin
Version 1.1

Copyright (C) 2004, 2006 The Linux Foundation and its contributors.
660 York Street, Suite 102,
San Francisco, CA 94110 USA

Everyone is permitted to copy and distribute verbatim copies of this
license document, but changing it is not allowed.

Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

Then add a line to every git commit message:

    Signed-off-by: Kris Smith <kris.smith@email.com>

Use your real name (sorry, no pseudonyms or anonymous contributions).

If you set your `user.name` and `user.email` git configs, you can sign your
commit automatically with `git commit -s`.
