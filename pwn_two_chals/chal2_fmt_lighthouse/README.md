# chal2_fmt_lighthouse

## Theme

format string vulnerability / libc leak / ASLR bypass / `%n`-family write.

This is intentionally scoped. It is not a ROP chain challenge. The key task is to convert a format-string primitive into:

1. libc leak
2. libc base calculation
3. overwrite of a writable function pointer
4. `system("/bin/sh")`

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
- ASLR: intended to be enabled

## Build

```bash
make clean all
```

## Run

```bash
./chall
```

## Format string argument layout

The vulnerable call is intentionally supplied with useful arguments so the problem stays focused on the format-string primitive:

```c
printf(fmt,
       puts,                         // %1$p leaks puts@libc
       (char *)&command_hook,         // %2$hn
       (char *)&command_hook + 2,     // %3$hn
       (char *)&command_hook + 4,     // %4$hn
       (char *)&command_hook + 6,     // %5$hn
       printf,
       system,
       0, 0, 0, 0);                  // padding args for width printing
```

Expected strategy:

1. Send `%1$p` to leak `puts`.
2. Compute `libc_base = leaked_puts - puts_offset`.
3. Compute `system = libc_base + system_offset`.
4. Use `%hn` writes through `%2$hn`..`%5$hn` to overwrite `command_hook`.
5. Use menu option `2` with `/bin/sh`.

## Reference solve

```bash
python3 solve_ref.py
```
