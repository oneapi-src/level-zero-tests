#!/usr/bin/env python3
# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

"""
Python setup for zesysman interface
"""

from distutils.core import setup, Extension

# Specify these Extension option on the command line via -I/-L/-l:
#
# include_dirs=['$ZE_PREFIX/include/level_zero'],
# library_dirs=['$ZE_PREFIX/lib'],
# libraries=['ze_loader'],

zesysman_module = Extension('_zesysman', sources=['zesysman.i'])

setup(name='zesysman',
      version='0.1',
      description="""Interface to L0 sysman""",
      ext_modules=[zesysman_module],
      py_modules=["zesysman"],
      scripts=["zesysman"])
