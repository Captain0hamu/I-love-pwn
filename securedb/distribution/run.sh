#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$ROOT/ld-2.31.so" --library-path "$ROOT" "$ROOT/securedb" "$@"
