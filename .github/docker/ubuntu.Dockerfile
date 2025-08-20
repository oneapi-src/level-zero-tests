# syntax=docker/dockerfile:1.4

ARG VMAJ
ARG VMIN
FROM ghcr.io/oneapi-src/level-zero-linux-compute/ubuntu:${VMAJ}.${VMIN}

ENV BOOST_BUILD_PATH=/boost_1_76_0
ADD https://archives.boost.io/release/1.76.0/source/boost_1_76_0.tar.gz /boost_1_76_0.tar.gz
RUN <<EOF
tar xf /boost_1_76_0.tar.gz
rm /boost_1_76_0.tar.gz
cd /boost_1_76_0
./bootstrap.sh
./b2 install \
  -j $(nproc) \
  link=static \
  address-model=64 \
  --with-chrono \
  --with-log \
  --with-program_options \
  --with-regex \
  --with-serialization \
  --with-system \
  --with-timer
rm -rf /boost_1_76_0
EOF