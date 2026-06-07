#!/usr/bin/env bash
set -euo pipefail

libc_pkg="libc6_2.31-0ubuntu9.16_amd64.deb"
dev_pkg="libc6-dev_2.31-0ubuntu9.16_amd64.deb"
libc_url="https://launchpad.net/ubuntu/+archive/primary/+files/${libc_pkg}"
dev_url="https://launchpad.net/ubuntu/+archive/primary/+files/${dev_pkg}"
workdir=".glibc231"

mkdir -p "$workdir"

if [ ! -f "$workdir/$libc_pkg" ]; then
  curl -L --fail --output "$workdir/$libc_pkg" "$libc_url"
fi

if [ ! -f "$workdir/$dev_pkg" ]; then
  curl -L --fail --output "$workdir/$dev_pkg" "$dev_url"
fi

rm -rf "$workdir/root"
mkdir -p "$workdir/root"
dpkg-deb -x "$workdir/$libc_pkg" "$workdir/root"
dpkg-deb -x "$workdir/$dev_pkg" "$workdir/root"

cp "$workdir/root/lib/x86_64-linux-gnu/libc-2.31.so" ./libc.so.6
cp "$workdir/root/lib/x86_64-linux-gnu/ld-2.31.so" ./ld-2.31.so
chmod +x ./ld-2.31.so

echo "installed ./libc.so.6 and ./ld-2.31.so from $libc_pkg"
echo "installed glibc 2.31 development files under $workdir/root"
