#!/usr/bin/env python3
# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

import re
import sys
import traceback

#
# Output support
#

class Logger:
    def __init__(self):
        self.outputFile = sys.stdout
        self.debugFile = sys.stderr
        self.teeOutput = False

    # Print to standard error
    def err(self, *args, **kwargs):
        kwargs["file"] = sys.stderr
        print(*args, **kwargs)

    # Print to selected debug stream (suppressed when self.debugFile=None)
    def dbg(self, *args, **kwargs):
        if self.debugFile:
            kwargs["file"] = self.debugFile
            print(*args, **kwargs)

    # Print to current output file, copy to stdout if tee requested:
    def __call__(self, *args, **kwargs):
        kwargs["file"] = self.outputFile
        print(*args, **kwargs)
        if self.teeOutput:
            kwargs["file"] = sys.stdout
            print(*args, **kwargs)

    def fail(self, *args, **kwargs):
        self.err(*args, **kwargs)
        sys.exit(1)

pr = Logger()

def reportZeException():
    eType, eValue, eTrace = sys.exc_info()
    tb = traceback.format_tb(eTrace, limit=1)[0]
    match = re.search(r".*zeCall\((\w+)\s*,", tb)
    if match:
        pr.err("WARNING:", match.group(1), "returned", eValue)
    else:
        pr.err("WARNING:", eValue, "returned from\n", tb.rstrip())
