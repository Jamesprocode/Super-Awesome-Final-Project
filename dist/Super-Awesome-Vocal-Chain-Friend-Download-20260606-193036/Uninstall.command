#!/usr/bin/env bash
set -euo pipefail

PLUGIN_NAME="Super Awesome Vocal Chain"

rm -rf "$HOME/Library/Audio/Plug-Ins/Components/$PLUGIN_NAME.component"
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/$PLUGIN_NAME.vst3"
rm -rf "$HOME/Applications/$PLUGIN_NAME.app"

echo "Removed $PLUGIN_NAME from this user account."
echo
read -r -p "Press Return to close this window..."
