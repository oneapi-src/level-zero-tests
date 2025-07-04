# syntax=docker/dockerfile:1.4

ARG VMAJ
ARG VMIN
FROM ghcr.io/oneapi-src/level-zero-linux-compute/sles:${VMAJ}.${VMIN}

SHELL ["/bin/bash", "-e", "-c"]

ARG VMAJ
ARG VMIN

RUN <<EOF
if [ "$VMIN" = "4" ]; then
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 120 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-11
  update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-11 120
  update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-11 120
elif [ "$VMIN" = "6" ]; then
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 120 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-13
  update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-13 120
  update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-13 120
fi
update-alternatives --config gcc
update-alternatives --config c++
update-alternatives --config cc
EOF

# Static libraries for boost are not part of the SLES distribution
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
  --with-date_time \
  --with-timer
cd ..
rm -rf boost
EOF