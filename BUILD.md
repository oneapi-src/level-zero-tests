# oneAPI Level Zero Tests Build Guide

## Dependencies

### Windows Dependencies
Requirements: MSVC 2022

```powershell
Invoke-WebRequest -Uri 'https://zlib.net/zlib131.zip' -OutFile 'C:\TEMP\zlib.zip';
Expand-Archive C:\TEMP\zlib.zip -DestinationPath C:\TEMP;
Move-Item C:\TEMP\zlib-1.3.1 C:\TEMP\zlib;
Remove-Item C:\TEMP\zlib.zip;

Invoke-WebRequest -UserAgent 'Wget' -Uri 'https://downloads.sourceforge.net/project/libpng/libpng16/1.6.47/lpng1647.zip' -OutFile 'C:\TEMP\libpng.zip';
Expand-Archive C:\TEMP\libpng.zip -DestinationPath C:\TEMP;
Move-Item C:\TEMP\lpng1647 C:\TEMP\libpng;
Remove-Item C:\TEMP\libpng.zip

Invoke-WebRequest -UserAgent "Wget" -Uri 'https://sourceforge.net/projects/boost/files/boost/1.79.0/boost_1_79_0.zip' -OutFile 'C:\TEMP\Boost.zip' -UseBasicParsing;
Add-Type -AssemblyName System.IO.Compression.FileSystem;
[System.IO.Compression.ZipFile]::ExtractToDirectory('C:\TEMP\Boost.zip', 'C:\TEMP');
Move-Item C:\TEMP\boost_1_79_0 C:\TEMP\Boost;
Remove-Item C:\TEMP\Boost.zip;

Invoke-WebRequest -Uri 'https://github.com/oneapi-src/level-zero/archive/refs/tags/v1.28.0.zip' -OutFile 'C:\TEMP\level-zero.zip'; `
Expand-Archive C:\TEMP\level-zero.zip -DestinationPath C:\TEMP; `
Move-Item C:\TEMP\level-zero-1.28.0 C:\TEMP\level-zero; `
Remove-Item C:\TEMP\level-zero.zip;

$WORKSPACE = "C:\LZT_Workspace"
mkdir $WORKSPACE

cmake -B C:\TEMP\build\zlib\ -S C:\TEMP\zlib -A x64 -D BUILD_SHARED_LIBS=YES -DCMAKE_INSTALL_PREFIX:PATH=$WORKSPACE\zlib
cmake --build C:\TEMP\build\zlib\ --config Release --target install
cmake -B C:\TEMP\build\libpng\ -S C:\TEMP\libpng -A x64 -DCMAKE_INSTALL_PREFIX:PATH=$WORKSPACE\libpng -DCMAKE_PREFIX_PATH:PATH=$WORKSPACE\zlib
cmake --build C:\TEMP\build\libpng\ --config Release --target install
cmake -B C:\TEMP\build\level-zero\ -S C:\TEMP\level-zero -A x64 -DCMAKE_INSTALL_PREFIX:PATH=$WORKSPACE\level-zero
cmake --build C:\TEMP\build\level-zero\ --config Release --target install

cd C:\TEMP\Boost
.\bootstrap.bat vc143
.\b2.exe install `
  --prefix=$WORKSPACE\Boost `
  -j 16 `
  address-model=64 `
  --with-chrono `
  --with-log `
  --with-program_options `
  --with-serialization `
  --with-system `
  --with-date_time `
  --with-timer

cd $WORKSPACE\level-zero-tests
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="$WORKSPACE\zlib;$WORKSPACE\libpng;$WORKSPACE\level-zero;$WORKSPACE\Boost"
cmake --build . --config Release --parallel

```

### Linux Dependencies
Requires the `level-zero`, `level-zero-devel`, `libpng-dev`, `libboost-all-dev`, `libva-dev` packages
to be installed.

### SLES Dependencies
On SLES distributions only:

Requires the `level-zero`, `level-zero-devel`, `libpng16-devel`, `libva-devel`
packages to be installed.

In addition to the above, the Boost C++ Library needs to be installed. Example below with Boost 1.79 (i.e. https://www.boost.org/users/history/version_1_79_0.html)

```bash
git clone --recurse-submodules --branch boost-1.79.0 https://github.com/boostorg/boost.git
cd boost
./bootstrap.sh
./b2 install \
  -j 4 \
  address-model=64 \
  --with-chrono \
  --with-log \
  --with-program_options \
  --with-serialization \
  --with-system \
  --with-date_time \
  --with-timer
```

### Dependencies

The HEAD of this repo is compatible with the latest Level Zero Loader release. Please report incompatibilities at https://github.com/oneapi-src/level-zero-tests/issues.

To build against loader releases prior to [v1.15.7](https://github.com/oneapi-src/level-zero/releases/tag/v1.15.7), add the cmake flag `-DENABLE_RUNTIME_TRACING=OFF`.

You can either build against the version you have installed on your system (automatic, Linux only), 
or specify an install prefix with the `CMAKE_PREFIX_PATH` cmake flag during configuration.
This `CMAKE_PREFIX_PATH` must point to the top-level install directory where level-zero was installed.
for example: `-DCMAKE_PREFIX_PATH=/home/username/level-zero/build/output/`.


Some benchmarks written against OpenCL are included to enable easy performance
comparisons. These benchmarks will be built automatically if OpenCL is available
on the system. To require that these benchmarks also be built, set the
`REQUIRE_OPENCL_BENCHMARKS` cmake flag to `YES`.

[Google Test](https://github.com/google/googletest) is used by all tests in this
repo for handling test case generation and result analysis. Google Test is
included as a submodule at `third_party/googletest`.

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

### Building a Subset of the Test Executables

Test executables are divided into a group hierarchy, and it is possible to
select a specific grouping of test executables for build using the `GROUP`
cmake flag. The following group specifiers are available:

  - `/`: All tests.
  - `/conformance_tests`: All of the conformance tests.
  - `/conformance_tests/core`: All of the conformance tests for the core API.
  - `/conformance_tests/tools`: All of the conformance tests for the tools API.
  - `/conformance_tests/tools/debug`: All the conformance tests for debugger.
  - `/conformance_tests/tools/metrics`: All the conformance tests for debugger.
  - `/conformance_tests/tools/pin`: All of the tools API conformance tests relating to instrumentation of L0 applications.
  - `/conformance_tests/tools/sysman`: All of the tools API conformance tests relating to system management.
  - `/conformance_tests/tools/tracing`: All of the tools API conformance tests related to tracing.
  - `/layer_tests/ltracing`: Tests for the tracing layer.
  - `/negative_tests`: All the negative tests.
  - `/perf_tests`: All the performance tests.
  - `/stress_tests`: All the stress tests.

```
cmake
  -D GROUP=/perf_tests
  -D CMAKE_INSTALL_PREFIX=$PWD/../out/perf_tests
  ..
cmake --build . --config Release --target install
```

The group that any particular test executable belongs to can be seen by looking
in its `CMakeLists.txt` file at the `add_lzt_test()` call.

### Building performance tests

Level Zero Performance tests can be built alone with `BUILD_ZE_PERF_TESTS_ONLY` cmake
flag.

### Building Zesysman

`zesysman` is an optional component that can be built by setting the `ENABLE_ZESYSMAN` 
cmake flag to `yes`. Building zesysman requires the following additional dependencies
  - `cmake` >= 3.12
  - `swig`
  - `python3`
  - `python3-devel` <-- On SLES only
