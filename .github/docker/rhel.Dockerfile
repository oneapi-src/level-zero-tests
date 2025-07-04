# syntax=docker/dockerfile:1.4

ARG VMAJ
ARG VMIN
FROM ghcr.io/oneapi-src/level-zero-linux-compute/rhel:${VMAJ}.${VMIN}

RUN <<EOF
git clone --recurse-submodules --branch boost-1.76.0 https://github.com/boostorg/boost.git
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
  --with-timer \
  --with-date_time
cd ..
rm -rf boost
EOF