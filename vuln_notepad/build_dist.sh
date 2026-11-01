#!/usr/bin/env bash
# Build the player-facing binary in a pinned Debian 13 userspace.
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$ROOT/build"
IMAGE="vuln-notepad-build:local"
PLATFORM=${PLATFORM:-linux/amd64}

docker build \
  --platform "$PLATFORM" \
  -f "$ROOT/packaging/Dockerfile" \
  -t "$IMAGE" \
  "$ROOT"

container=$(docker create --platform "$PLATFORM" "$IMAGE")
trap 'docker rm -f "$container" >/dev/null 2>&1 || true' EXIT

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
docker cp "$container:/out/." "$BUILD_DIR/"

echo "built artifacts:"
ls -l "$BUILD_DIR"
