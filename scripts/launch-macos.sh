#!/usr/bin/env bash
# Start Space Rock Breaker (.app) na build.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="$ROOT/build-macos/Space Rock Breaker.app"

if [[ ! -d "$APP" ]]; then
  echo "App ontbreekt. Bouw eerst: ./scripts/build-macos.sh"
  exit 1
fi

exec open "$APP"
