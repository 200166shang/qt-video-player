#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="Debug"
if [[ $# -gt 0 && "${1}" != -* ]]; then
  BUILD_TYPE="${1}"
  shift
fi

if command -v conan >/dev/null 2>&1; then
  conan install "${ROOT_DIR}" -of "${BUILD_DIR}" -s build_type="${BUILD_TYPE}" --build=missing
fi

mkdir -p "${ROOT_DIR}/bin"

TOOLCHAIN_FILE="${BUILD_DIR}/build/${BUILD_TYPE}/generators/conan_toolchain.cmake"
CMAKE_ARGS=()
if [[ -f "${TOOLCHAIN_FILE}" ]]; then
  CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "${CMAKE_ARGS[@]}" "$@"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j

echo "Build done. Executable should be in: ${ROOT_DIR}/bin"
