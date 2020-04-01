#! /usr/bin/env bash

set -eu
set -o pipefail

TMP_DIR=$(mktemp -d)
TMP_SRC=$TMP_DIR/src
cp -r $1 $TMP_SRC

git -C $TMP_SRC add -u >/dev/null
git -C $TMP_SRC -c "user.name=null" -c "user.email=null" commit -m "base" >/dev/null || true

FILES=$( \
    find \
    $TMP_SRC/conformance_tests \
    $TMP_SRC/negative_tests \
    $TMP_SRC/perf_tests \
    $TMP_SRC/utils \
    -name '*.cpp' -or -name '*.hpp' \
)
clang-format-7 -style=file -i $FILES >/dev/null

git -C $TMP_SRC diff

rm -rf $TMP_DIR
