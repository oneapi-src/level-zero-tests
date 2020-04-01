# oneAPI Level Zero Negative Tests Build Guide

### Building Level Zero Negative Tests

```
mkdir build
cd build
cmake ..
cd negative_tests
make -j`nproc`
make install
```

#### CMAKE Groups

```
mkdir build
cd build
cmake
  -D GROUP=/negative_tests
  -D CMAKE_INSTALL_PREFIX=$PWD/../out/negative_tests
  ..
cmake --build . --config Release --target install
```

Executables are installed to your CMAKE build/out directory.
