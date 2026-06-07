#!/usr/bin/env bash
set -euo pipefail

root=".glibc231/root"
sysroot="$PWD/$root"
libdir="$root/usr/lib/x86_64-linux-gnu"

if [ ! -f "$root/lib/x86_64-linux-gnu/libc.so.6" ] || [ ! -f "$libdir/Scrt1.o" ]; then
  echo "missing glibc 2.31 files; run ./setup_glibc231.sh inside nix-shell first" >&2
  exit 1
fi

gcc --sysroot="$sysroot" \
  -std=gnu11 -O0 -Wall -Wextra \
  -fstack-protector-all -pie -fpie \
  -Wl,-z,relro,-z,now,-z,noexecstack \
  -nostartfiles \
  "$libdir/Scrt1.o" "$libdir/crti.o" \
  -Wl,--dynamic-linker=./ld-2.31.so \
  -Wl,-rpath,'$ORIGIN' \
  -L"$root/lib/x86_64-linux-gnu" \
  -L"$libdir" \
  -o securedb \
  securedb.c \
  -lc \
  "$libdir/crtn.o"

echo "rebuilt ./securedb for local glibc 2.31 loader/libc"
