#!/usr/bin/env bash
set -eu

PTY_REAL="/tmp/ttyCOM"
PTY_LINK="/dev/ttyCOM"

rm -f "$PTY_REAL"

sudo rm -f "$PTY_LINK"
sudo ln -s "$PTY_REAL" "$PTY_LINK"

echo "[serial] Starting /dev/ttyCOM bridge"

exec socat \
    PTY,link="$PTY_REAL",raw,echo=0,mode=666 \
    TCP:host.docker.internal:3333,nodelay,forever,interval=1
