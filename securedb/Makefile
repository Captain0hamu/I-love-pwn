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
