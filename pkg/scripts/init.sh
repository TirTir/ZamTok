#!/bin/sh
set -eu

cd /app

if [ ! -x "./server" ]; then
  echo "[init] /app/server 실행 파일이 없거나 권한이 없습니다." >&2
  ls -al /app >&2 || true
  exit 1
fi

echo "[init] starting /app/server"
exec ./server "$@"
