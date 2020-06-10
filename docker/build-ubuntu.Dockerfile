# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

ARG IMAGE_VERSION=bionic-20200807
FROM ubuntu:$IMAGE_VERSION

RUN apt-get update && apt-get install -y \
        build-essential \
        ccache \
        clang-format-7 \
        clang-tidy \
        cmake \
        curl \
        git \
        libboost-all-dev \
        libpapi-dev \
        libpng-dev \
        libva-dev \
        ninja-build \
        ocl-icd-opencl-dev \
        opencl-headers \
    && rm -rf /var/lib/apt/lists/*
