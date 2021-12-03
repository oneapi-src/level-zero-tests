# oneAPI Level Zero Performance Tests Build Guide

### Building Level Zero Performance Tests

```
mkdir build
cd build
cmake ../ -DBUILD_ZE_PERF_TESTS_ONLY=1
cd perf_tests
make -j`nproc`
make install
```

#### CMAKE Groups

```
mkdir build
cd build
cmake
  -D GROUP=/perf_tests
  -D CMAKE_INSTALL_PREFIX=$PWD/../out/perf_tests
  ..
cmake --build . --config Release --target install
```

Executables are installed to your CMAKE build/out directory.
