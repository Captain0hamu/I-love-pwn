# pwn_two_chals

Two focused x86_64 pwn training challenges.

## Environment

- Distro: Debian GNU/Linux 13 `trixie`
- Architecture: `x86_64`
- glibc: 2.41
- libc and dynamic loader are generated into each challenge directory
- binaries are linked with `--dynamic-linker=./ld-linux-x86-64.so.2` and rpath `$ORIGIN`

## Build

The runtime files (`chall`, `libc.so.6`, `ld-linux-x86-64.so.2`) are not committed.
Build them in a Debian 13 container with:

```sh
./build.sh
```

Then run a challenge from its directory:

```sh
cd chal1_heap_cartographer
./chall
```

You can also use the development containers:

```sh
cd chal1_heap_cartographer
docker compose run --rm heap-cartographer
make clean all
./chall
```

## Challenges

1. `chal1_heap_cartographer`
   - Topic: glibc heap basics, tcache, UAF, safe-linking, tcache poisoning
   - Goal: overwrite `target_hook` with `win_hook`

2. `chal2_fmt_lighthouse`
   - Topic: format string vulnerability, libc leak, libc base calculation, `%n` writes
   - Goal: leak libc, overwrite `command_hook` with `system`, then call it with `/bin/sh`

These are local CTF-style exercises. They intentionally contain memory-corruption bugs and should be run in a disposable directory or container.

## Packaging note

`build.sh` produces only local runtime files. If these challenges need to be
distributed, create a separate `distribution/` template per challenge following
the same allowlist pattern as `securedb/` and `vuln_notepad/`.
