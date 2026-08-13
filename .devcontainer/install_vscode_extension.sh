#!/usr/bin/env bash
set -eu

VSIX="/workspaces/vscode-extension/kernel-tools.vsix"
EXT_ID="local.kernel-tools"
EXT_VERSION="0.1.0"

# VS Code Server typically keeps remote extensions here.
EXT_ROOT="$HOME/.vscode-server/extensions"
DEST="$EXT_ROOT/${EXT_ID}-${EXT_VERSION}"

echo "[vscode] Installing $EXT_ID..."

mkdir -p "$EXT_ROOT"

# Remove previous versions of our private extension.
rm -rf "$EXT_ROOT/${EXT_ID}-"*

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

unzip -q "$VSIX" -d "$TMP"

if [ ! -d "$TMP/extension" ]; then
    echo "[vscode] Invalid VSIX: extension/ directory not found"
    exit 1
fi

mv "$TMP/extension" "$DEST"

echo "[vscode] Installed to $DEST"
