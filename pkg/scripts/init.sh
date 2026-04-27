#!/bin/sh
set -eu

cd /app

if [ -f "/scripts/setup.sh" ]; then
  echo "[init] running /scripts/setup.sh"
  exec sh /scripts/setup.sh "$@"
fi

if [ ! -x "./server" ]; then
  echo "[init] /app/server not found or not executable." >&2
  ls -al /app >&2 || true
  exit 1
fi

echo "[init] starting /app/server"
exec ./server "$@"
