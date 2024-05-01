# syntax=docker/dockerfile:1.4

ARG VMAJ
ARG VMIN
FROM ghcr.io/oneapi-src/level-zero-linux-compute/sles:${VMAJ}.${VMIN}

SHELL ["/bin/bash", "-e", "-c"]

# Static libraries for boost are not part of the SLES distribution
RUN <<EOF
git clone --recurse-submodules --branch boost-1.70.0 https://github.com/boostorg/boost.git
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
  --with-timer
cd ..
rm -rf boost
EOF
