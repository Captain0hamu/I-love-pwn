# pwn_two_chals

Two focused x86_64 pwn training challenges.

Environment used to build the included binaries:

- Distro: Debian GNU/Linux 13 `trixie`
- Architecture: `x86_64`
- glibc: `2.41-12+deb13u2`
- libc shipped per challenge: `./libc.so.6`
- dynamic loader shipped per challenge: `./ld-linux-x86-64.so.2`
- binaries are linked with `--dynamic-linker=./ld-linux-x86-64.so.2` and rpath `$ORIGIN`

Challenges:

1. `chal1_heap_cartographer`
   - Topic: glibc heap basics, tcache, UAF, safe-linking, tcache poisoning
   - Goal: overwrite `target_hook` with `win_hook`

2. `chal2_fmt_lighthouse`
   - Topic: format string vulnerability, libc leak, libc base calculation, `%n` writes
   - Goal: leak libc, overwrite `command_hook` with `system`, then call it with `/bin/sh`

These are local CTF-style exercises. They intentionally contain memory-corruption bugs and should be run in a disposable directory or container.
