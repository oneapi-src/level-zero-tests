# oneAPI Level Zero Tests Build Guide

## Dependencies

### Windows Dependencies
Requirements: MSVC 2022 version latest (tested on 17.14.30)

```powershell

$LZT_TEMP = if ($env:LZT_TEMP_DIR) { $env:LZT_TEMP_DIR } else { 'C:\TEMP' }
New-Item -ItemType Directory -Path $LZT_TEMP -Force | Out-Null

$zlibVersion = "v1.3.2"
$zlibUrl = "https://github.com/madler/zlib/archive/refs/tags/$zlibVersion.zip"
Invoke-WebRequest -Uri $zlibUrl -OutFile (Join-Path $LZT_TEMP 'zlib.zip') -UseBasicParsing;
Expand-Archive (Join-Path $LZT_TEMP 'zlib.zip') -DestinationPath $LZT_TEMP;
Move-Item (Join-Path $LZT_TEMP "zlib-$($zlibVersion -replace 'v','')") (Join-Path $LZT_TEMP 'zlib');
Remove-Item (Join-Path $LZT_TEMP 'zlib.zip');

Invoke-WebRequest -UserAgent 'Wget' -Uri 'https://downloads.sourceforge.net/project/libpng/libpng16/1.6.47/lpng1647.zip' -OutFile (Join-Path $LZT_TEMP 'libpng.zip') -UseBasicParsing;
Expand-Archive (Join-Path $LZT_TEMP 'libpng.zip') -DestinationPath $LZT_TEMP;
Move-Item (Join-Path $LZT_TEMP 'lpng1647') (Join-Path $LZT_TEMP 'libpng');
Remove-Item (Join-Path $LZT_TEMP 'libpng.zip')

Invoke-WebRequest -UserAgent "Wget" -Uri 'https://sourceforge.net/projects/boost/files/boost/1.79.0/boost_1_79_0.zip' -OutFile (Join-Path $LZT_TEMP 'Boost.zip') -UseBasicParsing;
Add-Type -AssemblyName System.IO.Compression.FileSystem;
[System.IO.Compression.ZipFile]::ExtractToDirectory((Join-Path $LZT_TEMP 'Boost.zip'), $LZT_TEMP);
Move-Item (Join-Path $LZT_TEMP 'boost_1_79_0') (Join-Path $LZT_TEMP 'Boost');

$msvcJamPath = Join-Path $LZT_TEMP 'Boost\tools\build\src\tools\msvc.jam'
$reader = New-Object System.IO.StreamReader($msvcJamPath, [System.Text.UTF8Encoding]::new($false), $true)
try {
  $msvcJamContent = $reader.ReadToEnd()
  $encoding = $reader.CurrentEncoding
} finally {
  $reader.Close()
}
$old1 = 'if [ MATCH "(14.3)"'
$new1 = 'if [ MATCH "(14.[3-9])"'
$old2 = 'if [ MATCH "(MSVC\\\\14.3)" : $(command) ]'
$new2 = 'if [ MATCH "(MSVC\\\\14.[3-9])" : $(command) ]'
if (-not $msvcJamContent.Contains($old1)) { throw "Pattern not found: $old1" }
if (-not $msvcJamContent.Contains($old2)) { throw "Pattern not found: $old2" }
$updatedMsvcJamContent = $msvcJamContent.Replace($old1, $new1).Replace($old2, $new2)
[System.IO.File]::WriteAllText($msvcJamPath, $updatedMsvcJamContent, $encoding)
Remove-Item (Join-Path $LZT_TEMP 'Boost.zip');

$LZT_WORKSPACE = if ($env:LZT_WORKSPACE) { $env:LZT_WORKSPACE } else { 'C:\LZT_Workspace' }
New-Item -ItemType Directory -Path $LZT_WORKSPACE -Force | Out-Null

$levelZeroTag = (Invoke-RestMethod -Headers @{ 'User-Agent' = 'PowerShell' } -Uri 'https://api.github.com/repos/oneapi-src/level-zero/releases/latest').tag_name
$levelZeroVersion = $levelZeroTag.TrimStart('v')
$levelZeroSdkZip = Join-Path $LZT_TEMP 'level-zero-win-sdk.zip'
$levelZeroSdkUrl = "https://github.com/oneapi-src/level-zero/releases/download/$levelZeroTag/level-zero-win-sdk-$levelZeroVersion.zip"
Invoke-WebRequest -Uri $levelZeroSdkUrl -OutFile $levelZeroSdkZip -UseBasicParsing
Remove-Item (Join-Path $LZT_WORKSPACE 'level-zero') -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path (Join-Path $LZT_WORKSPACE 'level-zero') -Force | Out-Null
Expand-Archive -Path $levelZeroSdkZip -DestinationPath (Join-Path $LZT_WORKSPACE 'level-zero') -Force
Remove-Item $levelZeroSdkZip

cmake -B (Join-Path $LZT_TEMP 'build\zlib') -S (Join-Path $LZT_TEMP 'zlib') -A x64 -D BUILD_SHARED_LIBS=YES -DCMAKE_INSTALL_PREFIX:PATH=$LZT_WORKSPACE\zlib
cmake --build (Join-Path $LZT_TEMP 'build\zlib') --config Release --target install
cmake -B (Join-Path $LZT_TEMP 'build\libpng') -S (Join-Path $LZT_TEMP 'libpng') -A x64 -DCMAKE_INSTALL_PREFIX:PATH=$LZT_WORKSPACE\libpng -DCMAKE_PREFIX_PATH:PATH=$LZT_WORKSPACE\zlib
cmake --build (Join-Path $LZT_TEMP 'build\libpng') --config Release --target install

cd (Join-Path $LZT_TEMP 'Boost')
.\bootstrap.bat vc143
.\b2.exe install `
  define=BOOST_USE_WINAPI_VERSION=0x0601 `
  --prefix=$LZT_WORKSPACE\Boost `
  -j 16 `
  address-model=64 `
  --with-chrono `
  --with-log `
  --with-program_options `
  --with-serialization `
  --with-system `
  --with-date_time `
  --with-timer

if (-not (Test-Path (Join-Path $LZT_WORKSPACE 'level-zero-tests'))) {
  git clone https://github.com/oneapi-src/level-zero-tests.git (Join-Path $LZT_WORKSPACE 'level-zero-tests')
}

cd $LZT_WORKSPACE\level-zero-tests
mkdir build
cd build
cmake .. -DLevelZero_ROOT="$LZT_WORKSPACE\level-zero" -DBOOST_ROOT="$LZT_WORKSPACE\Boost" -DPNG_ROOT="$LZT_WORKSPACE\libpng" -DZLIB_ROOT="$LZT_WORKSPACE\zlib"
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
