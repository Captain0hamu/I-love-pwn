#!/usr/bin/env bash
# Fetch recommended external repositories without vendoring them.
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEST="$ROOT/external"

declare -A REPOS=(
  [how2heap]="https://github.com/shellphish/how2heap.git"
  [nightmare]="https://github.com/guyinatuxedo/nightmare.git"
  [pwntools]="https://github.com/Gallopsled/pwntools.git"
  [pwndbg]="https://github.com/pwndbg/pwndbg.git"
  [gef]="https://github.com/hugsy/gef.git"
  [libc-database]="https://github.com/niklasb/libc-database.git"
  [glibc-all-in-one]="https://github.com/matrix1001/glibc-all-in-one.git"
)

usage() {
  echo "usage: $0 <name>... | --all"
  echo
  echo "available:"
  printf '  %s\n' "${!REPOS[@]}" | sort
  echo
  echo "repositories are cloned into: $DEST"
}

if [ "$#" -eq 0 ]; then
  usage
  exit 1
fi

if [ "$1" = "--all" ]; then
  names=("${!REPOS[@]}")
else
  names=("$@")
fi

mkdir -p "$DEST"

for name in "${names[@]}"; do
  url=${REPOS[$name]:-}
  if [ -z "$url" ]; then
    echo "unknown repository: $name" >&2
    usage >&2
    exit 1
  fi

  target="$DEST/$name"
  if [ -d "$target/.git" ]; then
    echo "==> updating $name"
    git -C "$target" pull --ff-only
  elif [ -e "$target" ]; then
    echo "not a git repository: $target" >&2
    exit 1
  else
    echo "==> cloning $name"
    git clone --depth 1 "$url" "$target"
  fi
done
