#!/usr/bin/env python3
# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

import threading

ZESYSMAN_PROG_VERSION = "version 0.3"
maxIterations = 1
pollInterval = 1.0
indentStr = "    "
condensedList = False
addAnsi256ColorBlock = False
hideTimestamp = False
indexAttribute = "Index"
earlyExit = threading.Event()
