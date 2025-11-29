#!/bin/bash
set -e  # při chybě okamžitě skonči

ROOT_DIR=$(dirname "$(realpath "$0")")

echo "=== [1/3] Building kernel (initial pass) ==="
(cd "$ROOT_DIR/" && ./build_stdlib.sh)

echo "=== [2/3] Building userspace ==="
(cd "$ROOT_DIR/userspace" && COMPLETE_BUILD=ON ./build.sh)

echo "=== [3/3] Rebuilding kernel with userspace symbols ==="
(cd "$ROOT_DIR/" && ./build.sh)

echo "=== Build complete ==="
