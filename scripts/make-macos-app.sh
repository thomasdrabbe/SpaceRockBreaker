#!/usr/bin/env bash
# Maakt Space Rock Breaker.app van build-macos/SpaceRockBreaker
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-macos"
BIN="$BUILD/SpaceRockBreaker"
APP="$BUILD/Space Rock Breaker.app"
VERSION="$(tr -d '\r\n' < "$ROOT/version.txt" 2>/dev/null || echo "1.0.0")"

if [[ ! -x "$BIN" ]]; then
  echo "Binary ontbreekt. Bouw eerst: ./scripts/build-macos.sh"
  exit 1
fi

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"

cp "$BIN" "$APP/Contents/MacOS/SpaceRockBreaker"
chmod +x "$APP/Contents/MacOS/SpaceRockBreaker"

# Zelfde layout als platte build: assets naast executable (applicationDirectory)
cp -R "$BUILD/assets" "$APP/Contents/MacOS/assets"
# Altijd repo-version.txt (build-map kan achterlopen na cmake-bump zonder rebuild).
cp "$ROOT/version.txt" "$APP/Contents/MacOS/version.txt"

# Ook in Resources (handig voor Finder / toekomstige bundle-paden)
cp -R "$BUILD/assets" "$APP/Contents/Resources/assets"
cp "$ROOT/version.txt" "$APP/Contents/Resources/version.txt"

cat > "$APP/Contents/Info.plist" << PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>nl</string>
	<key>CFBundleExecutable</key>
	<string>SpaceRockBreaker</string>
	<key>CFBundleIdentifier</key>
	<string>com.thomasdrabbe.spacerockbreaker</string>
	<key>CFBundleName</key>
	<string>Space Rock Breaker</string>
	<key>CFBundleDisplayName</key>
	<string>Space Rock Breaker</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>${VERSION}</string>
	<key>CFBundleVersion</key>
	<string>${VERSION}</string>
	<key>LSMinimumSystemVersion</key>
	<string>13.0</string>
	<key>NSHighResolutionCapable</key>
	<true/>
</dict>
</plist>
PLIST

echo "App-bundle: $APP"
echo "Open: open \"$APP\""
