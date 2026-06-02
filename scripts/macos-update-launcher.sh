#!/usr/bin/env bash
# macOS launcher: vergelijk lokale version.txt met GitHub main en herbouw bij nieuwere tag.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP_DIR="${SRB_APP_DIR:-$ROOT/build-macos}"
LOCAL_VER_FILE="$APP_DIR/version.txt"
REMOTE_VER_URL="https://raw.githubusercontent.com/thomasdrabbe/SpaceRockBreaker/main/version.txt"

read_local_ver() {
  if [[ -f "$LOCAL_VER_FILE" ]]; then
    tr -d '\r\n ' < "$LOCAL_VER_FILE"
  elif [[ -f "$ROOT/version.txt" ]]; then
    tr -d '\r\n ' < "$ROOT/version.txt"
  else
    echo "0.0.0"
  fi
}

fetch_remote_ver() {
  curl -fsSL "$REMOTE_VER_URL" 2>/dev/null | tr -d '\r\n ' || echo ""
}

ver_gt() {
  # Returns 0 if $1 > $2 (semver-ish)
  local a="$1" b="$2"
  if [[ "$a" == "$b" ]]; then return 1; fi
  local winner
  winner="$(printf '%s\n%s\n' "$a" "$b" | sort -V | tail -1)"
  [[ "$winner" == "$a" ]]
}

LOCAL="$(read_local_ver)"
REMOTE="$(fetch_remote_ver)"

echo "Space Rock Breaker (macOS)"
echo "  Lokaal:  $LOCAL"
echo "  Remote:  ${REMOTE:-<onbereikbaar>}"

if [[ -n "$REMOTE" ]] && ver_gt "$REMOTE" "$LOCAL"; then
  echo "  -> Nieuwere versie op GitHub. Bouwen..."
  bash "$ROOT/scripts/build-macos.sh"
  LOCAL="$(read_local_ver)"
  echo "  -> Klaar: $LOCAL"
else
  if [[ ! -d "$APP_DIR/Space Rock Breaker.app" ]]; then
    echo "  -> Geen .app gevonden. Eerste build..."
    bash "$ROOT/scripts/build-macos.sh"
  else
    echo "  -> Up-to-date."
  fi
fi

APP="$APP_DIR/Space Rock Breaker.app"
if [[ -d "$APP" ]]; then
  exec open "$APP"
else
  echo "App niet gevonden: $APP" >&2
  exit 1
fi
