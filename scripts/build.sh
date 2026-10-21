#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
repo_root=$(pwd)
base_revision=1153ac2b39e937a2c2f6bd1e956643518a2117e3
build_tree="$repo_root/.build/qmk"
mkdir -p .build
if [ ! -d "$build_tree/.git" ]; then
    git clone --branch em-2.0.0 --depth 1 https://github.com/emolitor/qmk_firmware.git "$build_tree"
fi
actual_revision=$(git -C "$build_tree" rev-parse HEAD)
if [ "$actual_revision" != "$base_revision" ]; then
    echo "Build checkout has an unexpected revision: $actual_revision" >&2
    exit 1
fi
git -C "$build_tree" submodule update --init --recursive
cp -R "$repo_root/keyboards/." "$build_tree/keyboards/"
cd "$build_tree"
if [ -n "${ARM_PREFIX:-}" ]; then
    qmk compile -kb shortcut/bridge75/ansi -km battery -e "TOOLCHAIN=$ARM_PREFIX"
else
    qmk compile -kb shortcut/bridge75/ansi -km battery
fi
