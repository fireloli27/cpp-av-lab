#!/usr/bin/env bash
# Point this clone at scripts/githooks (checked-in pre-commit → check-learning.sh).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"
git config core.hooksPath scripts/githooks
chmod +x scripts/githooks/pre-commit scripts/check-learning.sh
echo "core.hooksPath=scripts/githooks (pre-commit runs ./scripts/check-learning.sh)"
