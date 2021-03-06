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
* GTEST Test cases will follow the [Given/When/Then naming convention][given_when_then]

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

## Kernels (SPV Files)

OpenCL C kernel source code (`.cl`) and binaries (`.spv`) should be placed in a
`kernels/` subdirectory of your test. The
`add_lzt_test` and `add_lzt_test_executable` CMake functions will search for
kernels in that directory.

Whenever you add or update any kernels, you must re-generate the SPIR-V binaries
and commit them with your changes. A specialized build of clang is required to
do this.

### Setup Clang to Build SPV From .cl
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
### Build SPV File From .cl
* (OpenCL C) `clang -cc1 -emit-spirv -triple spir64-unknown-unknown -cl-std=CL2.0 -include opencl.h -x cl -o test.spv test.cl`
* (OpenCL C++) `clang -cc1 -emit-spirv -triple spir64-unknown-unknown -cl-std=c++ -I <libclcxx dir> -x cl -o test.spv test.cl`
(Note: use "-triple spir-unknown-unknown" to generate 32-bit spv)

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
