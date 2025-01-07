# syntax=docker/dockerfile:1.4

ARG VMAJ
ARG VMIN
FROM ghcr.io/oneapi-src/level-zero-linux-compute/ubuntu:${VMAJ}.${VMIN}

ARG VMAJ
ARG VMIN

SHELL ["/bin/bash", "-e", "-c"]

RUN <<EOF
source /etc/lsb-release
if ((VMAJ < 24)); then
    sed -i 's/^deb/deb [arch=amd64]/' /etc/apt/sources.list
    cat >> /etc/apt/sources.list <<EOF2
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ ${DISTRIB_CODENAME} main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ ${DISTRIB_CODENAME}-updates main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ ${DISTRIB_CODENAME}-security main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ ${DISTRIB_CODENAME}-backports main restricted universe multiverse
EOF2
else
    sed -i '/^Components:/a Architectures: amd64' /etc/apt/sources.list.d/ubuntu.sources
    cat >> /etc/apt/sources.list.d/ubuntu.sources <<EOF2

types: deb
URIs: http://ports.ubuntu.com/ubuntu-ports/
Suites: ${DISTRIB_CODENAME} ${DISTRIB_CODENAME}-updates ${DISTRIB_CODENAME}-security ${DISTRIB_CODENAME}-backports
Components: main universe restricted multiverse
Architectures: arm64
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF2
fi
dpkg --add-architecture arm64
EOF

ENV DEBIAN_FRONTEND=noninteractive
# /etc/apt/apt.conf.d/docker-clean doesn't work on older versions of docker for U2204 containers
RUN --mount=type=cache,target=/var/cache/apt <<EOF
rm /etc/apt/apt.conf.d/docker-clean
apt-get update
apt-get install -o Dpkg::Options::="--force-overwrite" -y \
  build-essential \
  ccache \
  $(((VMAJ == 20)) && echo \
    clang-format-7) \
  clang-tidy \
  cmake \
  curl \
  file \
  git \
  ninja-build \
  gcc-aarch64-linux-gnu \
  g++-aarch64-linux-gnu \
  libc6:arm64 \
  libstdc++6:arm64 \
  libpapi-dev \
  libpapi-dev:arm64 \
  libpng-dev \
  libpng-dev:arm64 \
  libva-dev \
  libva-dev:arm64 \
  ocl-icd-opencl-dev \
  ocl-icd-opencl-dev:arm64 \
  opencl-headers \
  python3 \
  python3-pip
rm -rf /var/lib/apt/lists/*
EOF

# Make newest version of aarch64 toolchain the default and enable switching.
RUN <<EOF
shopt -s extglob
for tool in $(ls /usr/bin/aarch64-linux-gnu-*([a-z\-+])); do
    for v in $(ls /usr/bin/aarch64-linux-gnu-* | grep -o [0-9]*$ | sort | uniq); do
        if [[ -f ${tool}-${v} ]]; then
            update-alternatives --install ${tool} $(basename ${tool}) ${tool}-${v} ${v}
        fi
    done
done
EOF

# Static libraries for boost cannot be installed from the apt sources
# without conflicts
# Required for https://github.com/boostorg/thread/issues/364 on Ubuntu 22+
ENV BOOST_BUILD_PATH=/boost_1_73_0
ADD https://archives.boost.io/release/1.73.0/source/boost_1_73_0.tar.gz /boost_1_73_0.tar.gz
RUN <<EOF
tar xf /boost_1_73_0.tar.gz
rm /boost_1_73_0.tar.gz
cd /boost_1_73_0
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
EOF
COPY <<EOF /boost_1_73_0/config.jam
using gcc : arm : aarch64-linux-gnu-g++ ;
EOF
RUN <<EOF
cd /boost_1_73_0
rm ./b2
./bootstrap.sh --libdir=/usr/local/lib/aarch64-linux-gnu
./b2 install \
  --config=/boost_1_73_0/config.jam \
  -j $(nproc) \
  link=static \
  address-model=64 \
  --libdir=/usr/local/lib/aarch64-linux-gnu \
  toolset=gcc-arm \
  --with-chrono \
  --with-log \
  --with-program_options \
  --with-regex \
  --with-serialization \
  --with-system \
  --with-timer
cd ..
rm -rf /boost_1_73_0
EOF
