#!/usr/bin/env bash
# Bouwt SpaceRockBreaker op macOS (Apple Silicon preset).
# Vereist: Xcode CLI tools, vcpkg in $VCPKG_ROOT (standaard ~/vcpkg).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLS="$ROOT/.tools"
PREFIX="$TOOLS/prefix"

export VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"

# Optionele lokale CMake/Ninja (zie readme als Homebrew ontbreekt)
if [[ -d "$TOOLS/cmake-3.31.6-macos-universal/CMake.app/Contents/bin" ]]; then
  export PATH="$TOOLS/cmake-3.31.6-macos-universal/CMake.app/Contents/bin:$PATH"
fi
if [[ -x "$TOOLS/ninja" ]]; then
  export PATH="$TOOLS:$PATH"
fi
if [[ -x "$PREFIX/bin/pkg-config" ]]; then
  export PATH="$PREFIX/bin:$PATH"
  export PKG_CONFIG="$PREFIX/bin/pkg-config"
fi

if [[ ! -x "$VCPKG_ROOT/vcpkg" ]]; then
  echo "vcpkg niet gevonden. Installeer met:"
  echo "  git clone https://github.com/microsoft/vcpkg.git \"\$VCPKG_ROOT\""
  echo "  \"\$VCPKG_ROOT/bootstrap-vcpkg.sh\""
  exit 1
fi

if ! command -v cmake >/dev/null; then
  echo "cmake niet gevonden. Installeer met: brew install cmake ninja"
  exit 1
fi

ARCH="$(uname -m)"
PRESET="macos-arm64"
if [[ "$ARCH" == "x86_64" ]]; then
  PRESET="macos-x64"
fi

cd "$ROOT"
cmake --preset "$PRESET"
cmake --build --preset macos-release -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

cp "$ROOT/version.txt" "$ROOT/build-macos/version.txt"
bash "$ROOT/scripts/make-macos-app.sh"

echo ""
echo "Klaar:"
echo "  Binary: $ROOT/build-macos/SpaceRockBreaker"
echo "  App:    $ROOT/build-macos/Space Rock Breaker.app"
echo "Start:  open \"$ROOT/build-macos/Space Rock Breaker.app\""
