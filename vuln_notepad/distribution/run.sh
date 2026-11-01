#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$ROOT/ld-linux-x86-64.so.2" --library-path "$ROOT" "$ROOT/vuln_notepad" "$@"
