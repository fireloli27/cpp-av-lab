#!/usr/bin/env bash
# Guardrail for Learning Project: configure, build, run CTest.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/experiments/build"

cmake -S "${ROOT}/experiments" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure
