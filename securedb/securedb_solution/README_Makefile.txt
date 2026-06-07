# ── Makefile ──────────────────────────────────────────────────────
# SecureDB CTF Challenge
# Target: Ubuntu 20.04  glibc 2.31

CC      = gcc
CFLAGS  = -O0 -Wall -Wextra
HARDENED= -fstack-protector-all -pie -fpie -z relro -z now -z noexecstack
EASY    = -fno-stack-protector -no-pie -z execstack

.PHONY: all hardened easy clean run patchelf

# Full protections (intended challenge binary)
all: hardened

hardened: securedb.c
	$(CC) $(CFLAGS) $(HARDENED) -o securedb securedb.c
	@echo "[*] built with: CANARY  PIE  RELRO=FULL  NX  (no safe-linking = glibc 2.31)"

# Strip progressively for debugging / testing easier versions
easy: securedb.c
	$(CC) $(CFLAGS) $(EASY) -o securedb_easy securedb.c
	@echo "[*] built without protections (for initial exploration)"

# Patch the binary to use the provided libc / ld
patchelf: securedb
	patchelf --set-interpreter ./ld-2.31.so \
	         --replace-needed libc.so.6 ./libc.so.6 \
	         securedb
	@echo "[*] patched: securedb now uses ./libc.so.6"

run: securedb
	./securedb

clean:
	rm -f securedb securedb_easy


# ──────────────────────────────────────────────────────────────────
# README
# ──────────────────────────────────────────────────────────────────
#
# SecureDB  CTF Pwn – Heap Grooming / Tcache Poisoning
# =====================================================
#
# Difficulty:  ★★★★☆  (hard)
# Category:    pwn / heap exploitation
# Target OS:   Ubuntu 20.04 (glibc 2.31)
#
# ── Challenge Description ─────────────────────────────────────────
#
#   "Our developer built SecureDB – a blazing-fast in-memory record
#    store.  They assured us the memory management is 'safe enough'.
#    The binary is running on the server at nc challenge.example.com 9999.
#    Prove them wrong."
#
#   Flag: /flag  (standard CTF format)
#
# ── Provided Files ────────────────────────────────────────────────
#
#   securedb      – challenge binary (hardened build)
#   libc.so.6     – Ubuntu 20.04  glibc 2.31-0ubuntu9.16
#   ld-2.31.so    – matching dynamic linker
#   hints.md      – three progressive hints  (optional reading)
#
#   Download the correct libc:
#     https://libc.rip  →  search by function offsets
#     OR:  docker run --rm ubuntu:20.04 cat /lib/x86_64-linux-gnu/libc.so.6 > libc.so.6
#
# ── Protections (checksec) ────────────────────────────────────────
#
#   Arch:     amd64-64-little
#   RELRO:    Full
#   Stack:    Canary found
#   NX:       enabled
#   PIE:      enabled
#   ASLR:     enabled (kernel)
#
#   glibc 2.31 specifics:
#     tcache:          enabled  (up to chunk size 0x410)
#     safe-linking:    DISABLED (added in 2.32) ← key exploit detail
#     tcache key:      enabled  (double-free detection, not fd protection)
#
# ── Vulnerability Summary ─────────────────────────────────────────
#
#   Root cause: cmd_del() frees both heap allocations but does NOT
#               clear db[id].  All subsequent operations (show, write,
#               read) use validate_id() which only checks db[id] != NULL.
#               A deleted record remains "valid" from the program's
#               perspective → classic Use-After-Free.
#
#   Three UAF primitives:
#     1. UAF read   (cmd_read)  – dump freed heap → info leak
#     2. UAF write  (cmd_write) – write to freed heap → tcache corruption
#     3. UAF call   (cmd_show)  – call freed function pointer → PC control
#
# ── Full Exploit Chain ────────────────────────────────────────────
#
#   Phase 1 │ Libc leak
#            │  Alloc large buffer (0x500), add heap fence.
#            │  Free large buffer → unsorted bin (> tcache max 0x410).
#            │  UAF read → first 8 bytes = main_arena fd → libc base.
#
#   Phase 2 │ Heap leak
#            │  Alloc two 0x28-size records.  Free in reverse order.
#            │  tcache[0x30]: data_h1 (fd → data_h2).
#            │  UAF read on data_h1 → first 8 bytes = heap address.
#
#   Phase 3 │ Heap Feng Shui (grooming)
#            │  Carefully track tcache[0x30] and tcache[0x40] state.
#            │  new() always consumes from BOTH bins simultaneously.
#            │  Plan allocation sequence so that at time of poisoning,
#            │  tcache[0x30] = data_h1 → data_h2 → NULL  (count=2).
#
#   Phase 4 │ Tcache Poisoning
#            │  UAF write on data_h1: overwrite fd → __free_hook - 8.
#            │  new() × 2: first pops data_h1 (safe);
#            │             second pops __free_hook - 8 → ARBITRARY ALLOC.
#
#   Phase 5 │ Hook overwrite + shell
#            │  write(alias, "\x00"*8 + p64(system)) → __free_hook = system.
#            │  new(data="/bin/sh\0"), delete → system("/bin/sh").
#
# ── Environment Setup ─────────────────────────────────────────────
#
#   # Install pwntools
#   pip3 install pwntools
#
#   # Build challenge binary
#   make hardened
#
#   # Patch binary to use provided glibc
#   make patchelf
#
#   # Run locally
#   python3 exploit.py
#
#   # Run against remote
#   python3 exploit.py REMOTE
#
# ── Server Deployment ─────────────────────────────────────────────
#
#   Recommended: xinetd or socat with a 60-second timeout.
#   Run as a low-privilege user in a Docker container.
#
#   Dockerfile snippet:
#     FROM ubuntu:20.04
#     COPY securedb /challenge/
#     COPY flag     /flag
#     RUN chmod 444 /flag
#     CMD socat TCP-LISTEN:9999,fork,reuseaddr EXEC:/challenge/securedb
