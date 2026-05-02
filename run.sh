#!/usr/bin/env bash
#
# Super Awesome Vocal Chain — one-shot local UI build + native plugin build + Standalone launch.
#
# ── How to run this script ─────────────────────────────────────────────────────────
#
# • macOS / Linux natively — use Terminal:
#       chmod +x run.sh && ./run.sh
#
# • Windows — use Git Bash (“Git for Windows”) or bash inside WSL, from the repo root:
#       chmod +x run.sh && ./run.sh
#   (Plain cmd.exe / PowerShell will not execute this file; install Git for Windows, or mirror
#   the steps manually: plugin UI npm commands, then cmake from a “Developer” shell.)
#
# ── Prerequisites (install before first run; first CMake configure needs Internet) ─
#
#   All platforms:  CMake ≥ 3.24, Node.js + npm (https://nodejs.org/)
#
#   macOS:          Xcode Command Line Tools ( xcode-select --install )
#
#   Windows (MSVC): “Desktop development with C++” workload (Visual Studio 2022 Build Tools OK),
#                   plus the WebView2 runtime for the embedded JUCE UI
#                   (CMake usually picks “Visual Studio 17 2022” as generator — multi‑config.)
#
#   Linux:          distro build tools (Debian/Ubuntu example):
#                     sudo apt install build-essential cmake ninja-build pkg-config \
#                                      libgtk-3-dev libwebkit2gtk-4.1-dev
#                   Exact WebKitGTK package names differ by distro—match your JUCE / OS version.
#
# Optional overrides:
#   BUILD_CONFIG=Release ./run.sh     # Release instead of Debug
#   SKIP_LAUNCH=1 ./run.sh           # Build only; don’t auto-start Standalone (CI / scripting)
#

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

CFG="${BUILD_CONFIG:-Debug}"

# -----------------------------------------------------------------------------
# Identify host OS (used for CMake flags + how we launch Standalone).
# -----------------------------------------------------------------------------
case "$(uname -s)" in
  Darwin*) OS_KIND=darwin ;;
  CYGWIN*|MSYS*|MINGW*) OS_KIND=windows ;;
  Linux*) OS_KIND=linux ;;
  *) OS_KIND=other ;;
esac

# -----------------------------------------------------------------------------
# STEP 1 — Install JS deps & bundle UI into plugin/ui/public
#
#   Runs: npm install   → JS dependencies into plugin/ui/node_modules/
#   Runs: npm run build → Vite emits plugin/ui/public/ (bundled JS/CSS/HTML)
#
# The JUCE editor serves files from plugin/ui/public — see PluginEditor.cpp:getUiPublicFolder().
# -----------------------------------------------------------------------------
echo ""
echo "== [1/3] Building embedded Web UI (plugin/ui) =="
echo "   (host detected as: ${OS_KIND} / $(uname -s))"
echo ""
(
  cd "$ROOT/plugin/ui"
  npm install
  npm run build
)

# -----------------------------------------------------------------------------
# STEP 2 & 3 — Configure + compile the native plugin
#
#   cmake -S . -B build  [+ CMAKE_BUILD_TYPE on single-config generators]
#       Writes build files under ./build/; first run downloads CPM + clones JUCE.
#
#   cmake --build build --parallel --config <Debug|Release>
#       Invokes Xcode, Visual Studio, Ninja, Makefile, … depending on generator.
#
# If you switched OS or generator after a failed attempt, rm -rf build and re-run this script.
# -----------------------------------------------------------------------------
PRODUCT="Super Awesome Vocal Chain"
BUILD_DIR="$ROOT/build"

CONFIGURE_ARGS=( -S "$ROOT" -B "$BUILD_DIR" )
case "$OS_KIND" in
  darwin)
    # Xcode is usually multi‑config — don’t bake CMAKE_BUILD_TYPE into configure step.
    ;;
  windows)
    # Typical MSVC generator is multi‑config; skip CMAKE_BUILD_TYPE to avoid clashes.
    ;;
  *)
    # Ninja / Makefile on Linux/WSL/other: CMAKE_BUILD_TYPE must match how you intend to build.
    CONFIGURE_ARGS+=( -DCMAKE_BUILD_TYPE="$CFG" )
    ;;
esac

echo ""
echo "== [2/3] CMake configure (${BUILD_DIR}) =="
echo ""
cmake "${CONFIGURE_ARGS[@]}"

echo ""
echo "== [3/3] CMake build (configuration: ${CFG}) =="
echo ""
cmake --build "$BUILD_DIR" --parallel --config "$CFG"

# -----------------------------------------------------------------------------
# STEP 4 — Find Standalone output (layout differs slightly by generator / OS).
#
#   macOS bundle: …/Standalone/<PRODUCT>.app
#   Windows:      …/Standalone/<PRODUCT>.exe
#   Linux:        …/Standalone/<PRODUCT>           (executable, often no suffix)
# -----------------------------------------------------------------------------

candidate_paths() {
  local base="$ROOT/build/SAFProject_artefacts"
  local conf

  # JUCE artefacts: SAFProject_artefacts/<Debug|Release>/Standalone/<executable or .app>
  for conf in "$CFG" "Debug" "Release"; do
    printf '%s/%s/Standalone/%s.app\n' "$base" "$conf" "$PRODUCT"
    printf '%s/%s/Standalone/%s.exe\n' "$base" "$conf" "$PRODUCT"
    printf '%s/%s/Standalone/%s\n' "$base" "$conf" "$PRODUCT"
  done
  # Single-config CMake sometimes omits Debug/Release in the path hierarchy.
  printf '%s/Standalone/%s.app\n' "$base" "$PRODUCT"
  printf '%s/Standalone/%s.exe\n' "$base" "$PRODUCT"
  printf '%s/Standalone/%s\n' "$base" "$PRODUCT"
}

pick_standalone() {
  local p
  while IFS= read -r p && [[ -n "$p" ]]; do
    [[ -e "$p" ]] || continue
    # macOS application bundle:
    [[ -d "$p" ]] && [[ "$p" == *.app ]] && { echo "$p"; return 0; }
    # Windows Standalone executable (case-insensitive suffix; compatible with Bash 3.2 on macOS):
    [[ -f "$p" ]] && case "$p" in
      *.exe|*.EXE) echo "$p"; return 0 ;;
    esac
    # Typical Linux ELF / Standalone binary (usually no suffix, exact PRODUCT name):
    if [[ "$OS_KIND" == linux || "$OS_KIND" == other ]] && [[ -f "$p" ]] && [[ "$(basename "$p")" == "$PRODUCT" ]]; then
      echo "$p"; return 0
    fi
  done < <(candidate_paths | awk '!seen[$0]++')
  return 1
}

launch_standalone() {
  local target="$1"
  echo ""
  case "$OS_KIND" in
    darwin)
      echo "== Launching Standalone (macOS open) … =="
      open "$target"
      ;;
    linux | other)
      echo "== Launching Standalone … =="
      chmod +x "$target" 2>/dev/null || true
      "$target" &
      ;;
    windows)
      echo "== Launching Standalone (Windows) … =="
      if command -v cygpath >/dev/null 2>&1; then
        local win=""
        win="$(cygpath -w "$target" 2>/dev/null || true)"
        if [[ -n "$win" ]]; then
          MSYS2_ARG_CONV_EXCL=\* cmd.exe /c start "" "$win"
          return
        fi
      fi
      if command -v powershell.exe >/dev/null 2>&1; then
        powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -LiteralPath \"$target\""
        return
      fi
      echo "Could not locate cmd.exe/cygpath/powershell.exe to auto‑launch."
      echo "Run the Standalone binary manually:"
      echo "  $target"
      ;;
  esac
}

echo ""

if [[ "${SKIP_LAUNCH:-0}" == "1" ]]; then
  echo "SKIP_LAUNCH=1 — build finished; skipping Standalone launch."
  exit 0
fi

if APP_PATH="$(pick_standalone)"; then
  echo "Standalone found: $APP_PATH"
  launch_standalone "$APP_PATH"
else
  echo "Build ran, but this script could not find Standalone output under:"
  echo "  ${ROOT}/build/SAFProject_artefacts/"
  echo "Search for \"${PRODUCT}\" (.app / .exe / no suffix) inside build/"
  exit 1
fi
