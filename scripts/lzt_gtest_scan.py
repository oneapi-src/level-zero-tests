#!/usr/bin/env python3
# Copyright (C) 2025 Intel Corporation
# SPDX-License-Identifier: MIT
import argparse
import logging
import re
import sys
from pathlib import Path

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
handler = logging.StreamHandler(sys.stdout)
formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
handler.setFormatter(formatter)
logger.addHandler(handler)


def main(args=None):
    if not args:
        args = process_command_line()
    logger.debug("Arguments: %s", args)

    try:
        return scan_codebase(args.workspace_dir)
    except:
        logger.exception("Fatal error")
        return -1


def process_command_line():
    parser = argparse.ArgumentParser()
    setup_parser(parser)
    return parser.parse_args()


def setup_parser(root_parser):
    root_parser.add_argument(
        "--workspace_dir",
        type=Path,
        default=Path(".."),
        help="path to workspace directory",
    )


def scan_codebase(workspace: Path) -> int:
    extensions = {".h", ".hpp", ".cpp", ".c"}
    invalid = False
    for path in workspace.rglob("*"):
        if not path.is_file() or path.suffix not in extensions:
            continue
        if "third_party" in str(path):
            logger.debug("Skipping third-party file: %s", path)
            continue
        if "utils_gtest_helper.hpp" in str(path):
            logger.debug("Skipping utils_gtest_helper.hpp file: %s", path)
            continue
        logger.debug("Found source file: %s", path)

        with open(path, "r") as file:
            content = file.read()
            match = re.search(r"EXPECT_EQ\((?:(?!;).|\n)*?ZE_RESULT_SUCCESS(?:(?!;).|\n)*?\);", content)
            if match and '"' not in match.group(0):
                logger.info("Found deprecated macro usage in file: %s\n Line: %s", path, match.group(0))
                invalid = True
            match = re.search(r"ASSERT_EQ\((?:(?!;).|\n)*?ZE_RESULT_SUCCESS(?:(?!;).|\n)*?\);", content)
            if match and '"' not in match.group(0):
                logger.info("Found deprecated macro usage in file: %s\n Line: %s", path, match.group(0))
                invalid = True
    if invalid:
        logger.error(
            "Deprecated macros found in the codebase. Please transition to EXPECT_ZE_RESULT_SUCCESS or ASSERT_ZE_RESULT_SUCCESS."
        )
        return -1
    return 0


if __name__ == "__main__":
    sys.exit(main())
