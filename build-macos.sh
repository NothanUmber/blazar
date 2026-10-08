#!/bin/bash
set -euo pipefail

if [[ "$(uname -s)" != Darwin ]]; then
    echo "This script requires macOS." >&2
    exit 1
fi

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$project_dir/build-macos"

# Universal plugins support both Apple Silicon and Intel hosts. Extra CMake
# arguments can override these defaults, e.g. -DCMAKE_OSX_ARCHITECTURES=arm64.
cmake -S "$project_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" \
    "$@"
cmake --build "$build_dir" --config Release --parallel 2
ctest --test-dir "$build_dir" -C Release --output-on-failure
