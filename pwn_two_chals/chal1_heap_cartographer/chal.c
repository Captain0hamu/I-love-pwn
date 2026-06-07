// chal1_heap_cartographer
// Build target: x86_64, glibc 2.41, no PIE, NX enabled, partial RELRO.
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define MAX 8

typedef void (*hook_t)(const char *);

struct entry {
    void *ptr;
    size_t size;
};

static struct entry entries[MAX];

static void normal_hook(const char *s) {
    puts(s);
}

static void win_hook(const char *s) {
    (void)s;
    puts("[+] target_hook overwritten");
    system("/bin/sh");
}

struct target_obj {
    hook_t fp;
    char pad[0x38];
};

static struct target_obj *target;

static long read_long(void) {
    char buf[32];
    if (!fgets(buf, sizeof(buf), stdin)) exit(0);
    return strtol(buf, NULL, 0);
}

static void read_exactish(void *dst, size_t n) {
    ssize_t r = read(STDIN_FILENO, dst, n);
    if (r <= 0) exit(0);
}

static void menu(void) {
    puts("\n== heap cartographer ==");
    puts("1. alloc");
    puts("2. free");
    puts("3. edit");
    puts("4. show");
    puts("5. inspect");
    puts("6. call target_hook");
    puts("7. addresses");
    puts("8. exit");
    printf("> ");
}

static void alloc_note(void) {
    printf("idx: ");
    int idx = (int)read_long();
    if (idx < 0 || idx >= MAX) return;
    printf("size: ");
    size_t sz = (size_t)read_long();
    if (sz < 0x20 || sz > 0x80) {
        puts("size must be 0x20..0x80");
        return;
    }
    entries[idx].ptr = malloc(sz);
    entries[idx].size = sz;
    printf("data: ");
    read_exactish(entries[idx].ptr, sz);
    puts("ok");
}

static void free_note(void) {
    printf("idx: ");
    int idx = (int)read_long();
    if (idx < 0 || idx >= MAX || !entries[idx].ptr) return;
    free(entries[idx].ptr);
    // Bug: pointer is intentionally kept. edit/show/inspect remain possible.
    puts("freed, but the stale pointer survived.");
}

static void edit_note(void) {
    printf("idx: ");
    int idx = (int)read_long();
    if (idx < 0 || idx >= MAX || !entries[idx].ptr) return;
    printf("data: ");
    read_exactish(entries[idx].ptr, entries[idx].size);
    puts("edited");
}

static void show_note(void) {
    printf("idx: ");
    int idx = (int)read_long();
    if (idx < 0 || idx >= MAX || !entries[idx].ptr) return;
    write(STDOUT_FILENO, entries[idx].ptr, entries[idx].size);
    puts("");
}

static void inspect_note(void) {
    printf("idx: ");
    int idx = (int)read_long();
    if (idx < 0 || idx >= MAX || !entries[idx].ptr) return;
    uintptr_t *q = (uintptr_t *)entries[idx].ptr;
    printf("ptr      = %p\n", entries[idx].ptr);
    printf("size     = %#zx\n", entries[idx].size);
    printf("qword[0] = %#018lx\n", (unsigned long)q[0]);
    printf("qword[1] = %#018lx\n", (unsigned long)q[1]);
}

static void call_hook(void) {
    char msg[64];
    printf("message: ");
    if (!fgets(msg, sizeof(msg), stdin)) exit(0);
    target->fp(msg);
}

static void addresses(void) {
    printf("target_hook @ %p\n", (void *)&target->fp);
    printf("normal_hook @ %p\n", (void *)normal_hook);
    printf("win_hook    @ %p\n", (void *)win_hook);
    puts("note: heap chunk addresses are visible through inspect().");
}

int main(void) {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    target = malloc(sizeof(*target));
    target->fp = normal_hook;
    memset(target->pad, 0, sizeof(target->pad));
    puts("glibc heap basics: tcache, UAF, safe-linking.");
    for (;;) {
        menu();
        long c = read_long();
        switch (c) {
            case 1: alloc_note(); break;
            case 2: free_note(); break;
            case 3: edit_note(); break;
            case 4: show_note(); break;
            case 5: inspect_note(); break;
            case 6: call_hook(); break;
            case 7: addresses(); break;
            case 8: return 0;
            default: puts("?"); break;
        }
    }
}
