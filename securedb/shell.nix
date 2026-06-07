{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  packages = with pkgs; [
    binutils
    curl
    dpkg
    file
    gdb
    patchelf
    python3Packages.pwntools
  ];

  shellHook = ''
    echo "pwn env: pwntools, patchelf, gdb, dpkg-deb"
    echo "target glibc files expected: ./libc.so.6 and ./ld-2.31.so"
  '';
}
