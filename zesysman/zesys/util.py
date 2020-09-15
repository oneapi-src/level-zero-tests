#!/usr/bin/env python3
# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

import os

# specify upper/lower bounds
def saturate(value, minLimit, maxLimit):
    return min(max(value, minLimit), maxLimit)

# determine if running in a developer mode
def developer_mode():
    return os.environ.get("ZESYSMAN_DEVELOPER_MODE","").upper()
