# syntax=docker/dockerfile:1.4

ARG VMAJ
ARG VMIN
FROM ghcr.io/oneapi-src/level-zero-linux-compute/ubuntu:${VMAJ}.${VMIN}

ARG VMAJ
ARG VMIN

SHELL ["/bin/bash", "-e", "-c"]

ENV DEBIAN_FRONTEND=noninteractive
# /etc/apt/apt.conf.d/docker-clean doesn't work on older versions of docker for U2204 containers
RUN --mount=type=cache,target=/var/cache/apt <<EOF
rm /etc/apt/apt.conf.d/docker-clean
apt-get update
apt-get install -o Dpkg::Options::="--force-overwrite" -y \
  build-essential \
  ccache \
  clang-tidy \
  cmake \
  curl \
  file \
  git \
  ninja-build \
  libboost-all-dev \
  libpapi-dev \
  libpng-dev \
  libva-dev \
  ocl-icd-opencl-dev \
  opencl-headers \
  python3 \
  python3-pip
rm -rf /var/lib/apt/lists/*
EOF
