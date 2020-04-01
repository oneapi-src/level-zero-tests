# CMake - Cross Platform Makefile Generator
# Copyright 2000-2019 Kitware, Inc. and Contributors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of Contributors
#   may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Visual Studio 2019 (v16) includes an additional VC runtime DLL that the
# InstallRequiredSystemLibraries module included with Visual Studio cmake does
# not find, so we have to append it to CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS.

# Based on code from CMake/Modules/InstallRequiredSystemLibraries.cmake

if(MSVC)
    if(MSVC_TOOLSET_VERSION EQUAL 142)  # VS 2019
        set(MSVC_REDIST_NAME VC${MSVC_TOOLSET_VERSION})
        cmake_host_system_information(RESULT _vs_dir QUERY VS_16_DIR) # undocumented query
        if(IS_DIRECTORY "${_vs_dir}")
            file(GLOB _vs_redist_paths "${_vs_dir}/VC/Redist/MSVC/*")
        endif()
        unset(_vs_dir)
        if(CMAKE_CL_64)
            set(CMAKE_MSVC_ARCH x64)
        else()
            set(CMAKE_MSVC_ARCH x86)
        endif()
        find_path(MSVC_REDIST_DIR NAMES ${CMAKE_MSVC_ARCH}/Microsoft.${MSVC_REDIST_NAME}.CRT PATHS ${_vs_redist_paths})
        unset(_vs_redist_paths)
        set(MSVC_CRT_DIR "${MSVC_REDIST_DIR}/${CMAKE_MSVC_ARCH}/Microsoft.${MSVC_REDIST_NAME}.CRT")
        list(APPEND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "${MSVC_CRT_DIR}/vcruntime140_1.dll")
    endif()
endif()
