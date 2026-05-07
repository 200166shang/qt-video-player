#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="Debug"
if [[ $# -gt 0 && "${1}" != -* ]]; then
  BUILD_TYPE="${1}"
  shift
fi

mkdir -p "${ROOT_DIR}/bin"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "$@"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j

echo "Build done. Executable should be in: ${ROOT_DIR}/bin"
