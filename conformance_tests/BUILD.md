# oneAPI Level Zero Conformance Tests Build Guide

### Building Level Zero Conformance Tests

```
mkdir build
cd build
cmake ..
cd conformance_tests
make -j`nproc`
make install
```

#### CMAKE Groups

```
mkdir build
cd build
cmake
  -D GROUP=/conformance_tests
  -D CMAKE_INSTALL_PREFIX=$PWD/../out/conformance_tests
  ..
cmake --build . --config Release --target install
```

Executables are installed to your CMAKE build directory.
