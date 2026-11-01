#!/usr/bin/env bash
# Build both challenges in a Debian 13 userspace and copy the runtime files.
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
IMAGE_PREFIX="pwn-two-chals-build"
PLATFORM=${PLATFORM:-linux/amd64}
CHALLENGES=(
  chal1_heap_cartographer
  chal2_fmt_lighthouse
)

for challenge in "${CHALLENGES[@]}"; do
  image="$IMAGE_PREFIX/$challenge:local"
  echo "==> building $challenge"
  docker build \
    --platform "$PLATFORM" \
    --build-arg "CHALLENGE=$challenge" \
    -f "$ROOT/packaging/Dockerfile" \
    -t "$image" \
    "$ROOT"

  container=$(docker create --platform "$PLATFORM" "$image")
  trap 'docker rm -f "$container" >/dev/null 2>&1 || true' EXIT
  docker cp "$container:/out/." "$ROOT/$challenge/"
  docker rm -f "$container" >/dev/null
  trap - EXIT
done

echo "built:"
for challenge in "${CHALLENGES[@]}"; do
  ls -l "$ROOT/$challenge/chall" \
        "$ROOT/$challenge/libc.so.6" \
        "$ROOT/$challenge/ld-linux-x86-64.so.2"
done
