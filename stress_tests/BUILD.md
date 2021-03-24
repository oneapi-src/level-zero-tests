# oneAPI Level Zero Stress Tests Build Guide

### Building Level Zero Stress Tests

```
mkdir build
cd build
cmake ..
cd stress_tests
make -j`nproc`
make install
```

#### CMAKE Groups

```
mkdir build
cd build
cmake
  -D GROUP=/stress_tests
  -D CMAKE_INSTALL_PREFIX=$PWD/../out/stress_tests
  ..
cmake --build . --config Release --target install
```

Executables are installed to your CMAKE build directory.
