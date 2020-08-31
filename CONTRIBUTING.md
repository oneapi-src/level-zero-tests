# oneAPI Level Zero Tests Contribution Guidelines

## C++ Coding standards

* C++11 maximum support
* Avoid C Arrays, replace with `std::array<>` / `std::vector<>`
* Avoid "magic numbers"
* Avoid C-style memory allocations in favor of C++
* Use `nullptr` instead of `NULL`
* Don’t add `void` to empty argument lists
* Use `std::unique_ptr` in place of `std::auto_ptr`

In addition, these naming conventions should be followed:

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
* GTEST Test cases will follow the [Given/When/Then naming convention][given_when_then]

[given_when_then]: https://martinfowler.com/bliki/GivenWhenThen.html

## Code Formatting

Follow the [LLVM code formatting guidelines][llvm_code_formatting].

A [`.clang-format`](./.clang-format) file is included in this repository and
build targets are provided in the cmake for convenience. `clang-format` will
check and fix any formatting issues with your code, and `clang-format-check`
will check for issues and print a diff of the corrections required.

Examples:

```
cmake --build . --target clang-format-check
cmake --build . --target clang-format
```

[llvm_code_formatting]: https://llvm.org/docs/CodingStandards.html#source-code-formatting

## Kernels (SPV files)

OpenCL C kernel source code (`.cl`) and binaries (`.spv`) should be placed in a
`kernels/` subdirectory of your test. The
`add_lzt_test` and `add_lzt_test_executable` CMake functions will search for
kernels in that directory.

Whenever you add or update any kernels, you must re-generate the SPIR-V binaries
and commit them with your changes. A specialized build of clang is required to
do this.

### Setup Clang to build spv from .cl
* git clone -b khronos/spirv-3.6.1 https://github.com/KhronosGroup/SPIRV-LLVM.git llvm
* export LLVM_SRC_ROOT=<path_to_llvm>

* cd $LLVM_SRC_ROOT/tools
* git clone -b spirv-1.1 https://github.com/KhronosGroup/SPIR clang
* cd $LLVM_SRC_ROOT
* mkdir build
* cd build
* cmake -G "Unix Makefiles" ../
* make -j 4 (NOTE:careful with the number of threads you select, llvm compile can consume all the threads and resources on the system)
* sudo make install
* git clone https://github.com/KhronosGroup/libclcxx.git desired_path_to_opencl_cxx_headers
### Build SPV file from .cl
* (OpenCL C) `clang -cc1 -emit-spirv -triple spir64-unknown-unknown -cl-std=CL2.0 -include opencl.h -x cl -o test.spv test.cl`
* (OpenCL C++) `clang -cc1 -emit-spirv -triple spir64-unknown-unknown -cl-std=c++ -I <libclcxx dir> -x cl -o test.spv test.cl`
(Note: use "-triple spir-unknown-unknown" to generate 32-bit spv)
