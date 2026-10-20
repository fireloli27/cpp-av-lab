#!/usr/bin/env bash
# Guardrail for Learning Project: configure, build, run CTest.
# Reuses experiments/build/ — do not delete it between agent iterations.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/experiments/build"

# Skip remote FetchContent update checks on repeat configures; first clone still works.
cmake -S "${ROOT}/experiments" -B "${BUILD_DIR}" \
  -DFETCHCONTENT_UPDATES_DISCONNECTED=ON \
  -DFETCHCONTENT_BASE_DIR="${ROOT}/experiments/.fetchcontent"

cmake --build "${BUILD_DIR}" --parallel
ctest --test-dir "${BUILD_DIR}" --output-on-failure
