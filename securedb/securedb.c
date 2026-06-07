/*
 * securedb.c  –  SecureDB v2.3
 *
 * Lightweight in-memory record store.
 * Manages named binary records with pluggable display callbacks.
 *
 * Build (hardened):
 *   gcc -o securedb securedb.c -O0 \
 *       -fstack-protector-all -pie -fpie \
 *       -z relro -z now -z noexecstack
 *
 * Target: Ubuntu 20.04 (glibc 2.31)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

/* ── tunables ─────────────────────────────────────────────────────────── */
#define DB_CAPACITY   16
#define NAME_MAX_LEN  24
#define DATA_MAX_SIZE 0x500

/* ── record structure ─────────────────────────────────────────────────────
 *
 *  On-heap layout  (sizeof(Record) == 0x38  →  glibc chunk 0x40)
 *
 *  offset  field        size   notes
 *  +0x00   name          24    human-readable identifier
 *  +0x18   is_live        4    1 = active; 0 = deleted
 *  +0x1c   _reserved      4    reserved for future use
 *  +0x20   data_len       8    byte size of the data buffer
 *  +0x28   data           8    pointer to heap-allocated payload
 *  +0x30   display        8    function pointer for formatted output
 *
 * ─────────────────────────────────────────────────────────────────────── */
typedef struct record {
    char     name[NAME_MAX_LEN];           /* +0x00  24 bytes */
    uint32_t is_live;                      /* +0x18   4 bytes */
    uint32_t _reserved;                    /* +0x1c   4 bytes */
    size_t   data_len;                     /* +0x20   8 bytes */
    char    *data;                         /* +0x28   8 bytes */
    void   (*display)(struct record *);    /* +0x30   8 bytes */
} Record;   /* total: 0x38 bytes */

/* ── globals ──────────────────────────────────────────────────────────── */
static Record *db[DB_CAPACITY];
static int     db_next = 0;          /* monotone slot counter */

/* ── helpers ──────────────────────────────────────────────────────────── */

static void default_display(Record *r)
{
    printf("  name     : %.24s\n", r->name);
    printf("  data_len : %zu\n",   r->data_len);
    printf("  payload  : ");
    if (r->data && r->data_len)
        fwrite(r->data, 1, r->data_len, stdout);
    putchar('\n');
}

/* read a decimal integer from stdin */
static int read_int(void)
{
    char buf[32];
    if (!fgets(buf, (int)sizeof buf, stdin)) return -1;
    return (int)strtol(buf, NULL, 10);
}

/* read a line of text; strips trailing newline */
static ssize_t read_str(char *dst, size_t max)
{
    if (!fgets(dst, (int)max, stdin)) return -1;
    size_t n = strlen(dst);
    if (n && dst[n - 1] == '\n') dst[--n] = '\0';
    return (ssize_t)n;
}

/* read exactly |len| raw bytes using read(2) – allows NUL / newline */
static ssize_t read_raw(char *dst, size_t len)
{
    size_t got = 0;
    while (got < len) {
        ssize_t r = read(STDIN_FILENO, dst + got, len - got);
        if (r <= 0) break;
        got += (size_t)r;
    }
    return (ssize_t)got;
}

static int validate_id(int id)
{
    return (id >= 0) && (id < DB_CAPACITY) && (db[id] != NULL);
}

/* ── commands ─────────────────────────────────────────────────────────── */

/*
 * cmd_new – allocate a new record
 *
 * Performs two separate malloc calls:
 *   (1)  malloc(sizeof(Record))   → chunk 0x40
 *   (2)  malloc(size)             → chunk determined by user-supplied size
 */
static void cmd_new(void)
{
    if (db_next >= DB_CAPACITY) {
        puts("[-] database is full");
        return;
    }

    char name[NAME_MAX_LEN] = {0};
    printf("  name > ");
    if (read_str(name, sizeof name) <= 0) return;

    printf("  size > ");
    int size = read_int();
    if (size <= 0 || size > DATA_MAX_SIZE) {
        puts("[-] invalid size (1–0x500)");
        return;
    }

    Record *r = malloc(sizeof(Record));
    if (!r) { perror("malloc"); return; }
    memset(r, 0, sizeof(Record));

    r->data = malloc((size_t)size);
    if (!r->data) { free(r); perror("malloc"); return; }
    memset(r->data, 0, (size_t)size);

    strncpy(r->name, name, NAME_MAX_LEN - 1);
    r->data_len = (size_t)size;
    r->is_live  = 1;
    r->display  = default_display;

    db[db_next] = r;
    printf("[+] record created  id=%d\n", db_next);
    db_next++;
}

/*
 * cmd_del – remove a record and release its memory
 *
 * ╔══════════════════════════════════════════════╗
 * ║  BUG: db[id] is NOT set to NULL after free.  ║
 * ║  The stale pointer remains in db[].          ║
 * ╚══════════════════════════════════════════════╝
 */
static void cmd_del(void)
{
    printf("  id > ");
    int id = read_int();
    if (!validate_id(id)) { puts("[-] invalid id"); return; }

    Record *r = db[id];
    r->is_live = 0;       /* mark logically deleted … */
    free(r->data);         /* … release the payload  … */
    free(r);               /* … release the struct   … */
    /* INTENDED FIX (not applied): db[id] = NULL; */
    puts("[+] record deleted");
}

/*
 * cmd_show – invoke the record's display callback
 *
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  BUG: is_live is NOT re-checked.                             ║
 * ║  After cmd_del, db[id] is stale. Calling display on a freed  ║
 * ║  record can invoke an attacker-controlled function pointer.  ║
 * ╚══════════════════════════════════════════════════════════════╝
 */
static void cmd_show(void)
{
    printf("  id > ");
    int id = read_int();
    if (!validate_id(id)) { puts("[-] invalid id"); return; }

    /* invoke callback – no liveness check */
    db[id]->display(db[id]);
}

/*
 * cmd_write – overwrite a record's payload
 *
 * ╔═══════════════════════════════════════════════════════════════╗
 * ║  BUG: is_live is NOT checked.                                 ║
 * ║  After cmd_del, db[id] still holds the stale Record pointer.  ║
 * ║  The stale r->data still contains the freed buffer address.   ║
 * ║  Writing through r->data overwrites freed heap memory,        ║
 * ║  allowing corruption of tcache / bin fd pointers.             ║
 * ╚═══════════════════════════════════════════════════════════════╝
 */
static void cmd_write(void)
{
    printf("  id > ");
    int id = read_int();
    if (!validate_id(id)) { puts("[-] invalid id"); return; }

    Record *r = db[id];
    printf("  bytes to write (max %zu) > ", r->data_len);
    int len = read_int();
    if (len <= 0 || (size_t)len > r->data_len) {
        puts("[-] invalid length");
        return;
    }

    printf("  data > ");
    ssize_t got = read_raw(r->data, (size_t)len);
    if (got < 0) puts("[-] read error");
    else          printf("[+] wrote %zd bytes\n", got);
}

/*
 * cmd_read – dump a record's payload as hex
 *
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  BUG: is_live is NOT checked.                                ║
 * ║  Reading through a stale r->data leaks allocator metadata    ║
 * ║  (tcache fd, unsorted-bin fd/bk → heap/libc addresses).      ║
 * ╚══════════════════════════════════════════════════════════════╝
 */
static void cmd_read(void)
{
    printf("  id > ");
    int id = read_int();
    if (!validate_id(id)) { puts("[-] invalid id"); return; }

    Record *r   = db[id];
    size_t  n   = r->data_len;
    char   *p   = r->data;

    printf("[*] %zu bytes @%p:\n", n, (void *)p);
    for (size_t i = 0; i < n; i++) {
        printf("%02x", (unsigned char)p[i]);
        if ((i & 15) == 15 || i == n - 1) putchar('\n');
        else                               putchar(' ');
    }
}

/* ── menu / main ──────────────────────────────────────────────────────── */

static void banner(void)
{
    puts("╔══════════════════════════════════╗");
    puts("║  SecureDB v2.3                   ║");
    puts("║  in-memory record store service  ║");
    puts("╚══════════════════════════════════╝");
}

static void print_menu(void)
{
    puts("\n  1) new record");
    puts("  2) delete record");
    puts("  3) show record");
    puts("  4) write to record");
    puts("  5) read from record");
    puts("  0) quit");
    printf("\nchoice > ");
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    banner();

    for (;;) {
        print_menu();
        switch (read_int()) {
        case 1:  cmd_new();   break;
        case 2:  cmd_del();   break;
        case 3:  cmd_show();  break;
        case 4:  cmd_write(); break;
        case 5:  cmd_read();  break;
        case 0:  return 0;
        default: puts("[-] unknown command");
        }
    }
}
