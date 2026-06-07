# chal1_heap_cartographer

## Theme

glibc heap behavior / tcache / UAF / safe-linking.

This is not a full-chain heap nightmare. The intended path is a narrow primitive-building exercise:

1. Understand that `free()` leaves a stale pointer in the program's table.
2. Use the UAF edit primitive to corrupt a freed tcache entry.
3. Account for glibc safe-linking:

```c
stored_next = target ^ (chunk_user_pointer >> 12)
```

4. Make `malloc()` return a controlled target object.
5. Overwrite the function pointer with `win_hook`.

## Environment

- Distro: Debian GNU/Linux 13 `trixie`
- Arch: `x86_64`
- glibc: `2.41-12+deb13u2`
- Dynamic loader: `./ld-linux-x86-64.so.2`
- libc: `./libc.so.6`

## Mitigations

Expected properties:

- NX: enabled
- PIE: disabled
- Stack canary: disabled
- RELRO: partial
- glibc safe-linking: enabled

## Build

```bash
make clean all
```

## Run

```bash
./chall
```

## Intended vulnerability

`free_note()` frees the allocation but does not clear `entries[idx].ptr`. The program still allows `edit`, `show`, and `inspect` on the stale pointer.

The challenge provides an `inspect` option because the point is not to brute-force heap addresses. The point is to recognize what value must be written into the tcache `next` field under safe-linking.

## Win condition

Trigger menu option `6` after overwriting `target_hook` with `win_hook`.

## Reference solve

```bash
python3 solve_ref.py
```
