#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEB_DIR="$(dirname "$SCRIPT_DIR")"
CORE_DIR="$(dirname "$WEB_DIR")/core"
WASM_OUT="$WEB_DIR/public/wasm"

if ! command -v emcc &> /dev/null; then
  echo "Error: emcc not found. Please install and activate the Emscripten SDK."
  echo "  https://emscripten.org/docs/getting_started/downloads.html"
  exit 1
fi

echo "Building WASM in $CORE_DIR..."
cd "$CORE_DIR"
make -f emscripten.mk clean
make -f emscripten.mk wasm

echo "Copying output to $WASM_OUT..."
mkdir -p "$WASM_OUT"
cp "$CORE_DIR/WebCEmu.js" "$WASM_OUT/"
cp "$CORE_DIR/WebCEmu.wasm" "$WASM_OUT/"

echo "Done! WASM files are in $WASM_OUT/"
