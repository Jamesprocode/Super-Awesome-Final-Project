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
