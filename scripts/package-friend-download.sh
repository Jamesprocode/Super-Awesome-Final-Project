#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_CONFIG="${1:-Debug}"
BUILD_DIR="${SAFC_BUILD_DIR:-$ROOT_DIR/cmake-build-debug}"
ARTEFACT_DIR="$BUILD_DIR/SAFProject_artefacts/$BUILD_CONFIG"
DIST_DIR="$ROOT_DIR/dist"
PACKAGE_STAMP="${SAFC_PACKAGE_STAMP:-$(date +%Y%m%d-%H%M%S)}"
PACKAGE_NAME="Super-Awesome-Vocal-Chain-Friend-Download-$PACKAGE_STAMP"
PACKAGE_DIR="$DIST_DIR/$PACKAGE_NAME"
PAYLOAD_DIR="$PACKAGE_DIR/Payload"
ZIP_PATH="$DIST_DIR/$PACKAGE_NAME-mac.zip"
INCLUDE_STANDALONE="${SAFC_INCLUDE_STANDALONE:-0}"

PLUGIN_NAME="Super Awesome Vocal Chain"
AU_SOURCE="$ARTEFACT_DIR/AU/$PLUGIN_NAME.component"
VST3_SOURCE="$ARTEFACT_DIR/VST3/$PLUGIN_NAME.vst3"
APP_SOURCE="$ARTEFACT_DIR/Standalone/$PLUGIN_NAME.app"

require_bundle() {
  local path="$1"
  local label="$2"

  if [[ ! -d "$path" ]]; then
    echo "Missing $label at: $path" >&2
    echo "Build the plugin first, then run this script again." >&2
    exit 1
  fi
}

require_bundle "$AU_SOURCE" "AU component"
require_bundle "$VST3_SOURCE" "VST3 plugin"

mkdir -p "$PAYLOAD_DIR" "$DIST_DIR"

sign_and_verify_bundle() {
  local path="$1"
  local label="$2"

  if command -v codesign >/dev/null 2>&1; then
    codesign --force --deep --sign - "$path" >/dev/null
    codesign --verify --deep --strict "$path" >/dev/null
    echo "Prepared $label"
  fi
}

cp -R "$AU_SOURCE" "$PAYLOAD_DIR/"
sign_and_verify_bundle "$PAYLOAD_DIR/$PLUGIN_NAME.component" "AU component"

cp -R "$VST3_SOURCE" "$PAYLOAD_DIR/"
sign_and_verify_bundle "$PAYLOAD_DIR/$PLUGIN_NAME.vst3" "VST3 plugin"

if [[ "$INCLUDE_STANDALONE" == "1" && -d "$APP_SOURCE" ]]; then
  cp -R "$APP_SOURCE" "$PAYLOAD_DIR/"
  sign_and_verify_bundle "$PAYLOAD_DIR/$PLUGIN_NAME.app" "standalone app"
fi

cat > "$PACKAGE_DIR/Install.command" <<'INSTALLER'
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PAYLOAD_DIR="$SCRIPT_DIR/Payload"

PLUGIN_NAME="Super Awesome Vocal Chain"
AU_SOURCE="$PAYLOAD_DIR/$PLUGIN_NAME.component"
VST3_SOURCE="$PAYLOAD_DIR/$PLUGIN_NAME.vst3"
APP_SOURCE="$PAYLOAD_DIR/$PLUGIN_NAME.app"

AU_TARGET="$HOME/Library/Audio/Plug-Ins/Components"
VST3_TARGET="$HOME/Library/Audio/Plug-Ins/VST3"
APP_TARGET="$HOME/Applications"

mkdir -p "$AU_TARGET" "$VST3_TARGET"

echo "Installing $PLUGIN_NAME for $(whoami)..."
echo

if [[ -d "$AU_SOURCE" ]]; then
  rm -rf "$AU_TARGET/$PLUGIN_NAME.component"
  cp -R "$AU_SOURCE" "$AU_TARGET/"
  echo "Installed AU:"
  echo "  $AU_TARGET/$PLUGIN_NAME.component"
fi

if [[ -d "$VST3_SOURCE" ]]; then
  rm -rf "$VST3_TARGET/$PLUGIN_NAME.vst3"
  cp -R "$VST3_SOURCE" "$VST3_TARGET/"
  echo "Installed VST3:"
  echo "  $VST3_TARGET/$PLUGIN_NAME.vst3"
fi

if [[ -d "$APP_SOURCE" ]]; then
  mkdir -p "$APP_TARGET"
  rm -rf "$APP_TARGET/$PLUGIN_NAME.app"
  cp -R "$APP_SOURCE" "$APP_TARGET/"
  echo "Installed standalone app:"
  echo "  $APP_TARGET/$PLUGIN_NAME.app"
fi

echo
echo "Done. Restart your DAW and rescan plugins if needed."
echo "Logic/GarageBand use the AU. Ableton/Cubase/Reaper can use the VST3."
echo
read -r -p "Press Return to close this window..."
INSTALLER

cat > "$PACKAGE_DIR/Uninstall.command" <<'UNINSTALLER'
#!/usr/bin/env bash
set -euo pipefail

PLUGIN_NAME="Super Awesome Vocal Chain"

rm -rf "$HOME/Library/Audio/Plug-Ins/Components/$PLUGIN_NAME.component"
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/$PLUGIN_NAME.vst3"
rm -rf "$HOME/Applications/$PLUGIN_NAME.app"

echo "Removed $PLUGIN_NAME from this user account."
echo
read -r -p "Press Return to close this window..."
UNINSTALLER

cat > "$PACKAGE_DIR/README.txt" <<'README'
Super Awesome Vocal Chain - Friend Download

INSTALL
1. Unzip this folder.
2. Right-click Install.command and choose Open.
3. If macOS warns that it cannot verify the developer, choose Open again.
4. Restart your DAW and rescan plugins if needed.

WHAT GETS INSTALLED
- AU:
  ~/Library/Audio/Plug-Ins/Components/Super Awesome Vocal Chain.component

- VST3:
  ~/Library/Audio/Plug-Ins/VST3/Super Awesome Vocal Chain.vst3

- Standalone app, only when included:
  ~/Applications/Super Awesome Vocal Chain.app

WHICH ONE TO USE
- Logic and GarageBand: use the AU.
- Ableton, Cubase, Reaper, Studio One: use the VST3.

UNINSTALL
Right-click Uninstall.command and choose Open.

This friend-download build is not Apple Developer ID signed or notarized.
That is okay for private sharing, but macOS may show a security warning the first time.

This installer copies only the included plugin files into this user's home folder.
It does not ask for an admin password, does not install background services, and does
not remove macOS quarantine attributes.
README

chmod +x "$PACKAGE_DIR/Install.command" "$PACKAGE_DIR/Uninstall.command"

(
  cd "$PACKAGE_DIR"
  shasum -a 256 \
    "Install.command" \
    "Uninstall.command" \
    "Payload/$PLUGIN_NAME.component/Contents/MacOS/$PLUGIN_NAME" \
    "Payload/$PLUGIN_NAME.vst3/Contents/MacOS/$PLUGIN_NAME" \
    > CHECKSUMS.txt

  if [[ -f "Payload/$PLUGIN_NAME.app/Contents/MacOS/$PLUGIN_NAME" ]]; then
    shasum -a 256 "Payload/$PLUGIN_NAME.app/Contents/MacOS/$PLUGIN_NAME" >> CHECKSUMS.txt
  fi
)

(
  cd "$DIST_DIR"
  zip -r -X "$ZIP_PATH" "$PACKAGE_NAME" >/dev/null
)

echo "Created:"
echo "  $ZIP_PATH"
