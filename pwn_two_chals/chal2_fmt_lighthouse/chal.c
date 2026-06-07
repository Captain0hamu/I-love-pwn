// chal2_fmt_lighthouse
// Build target: x86_64, glibc 2.41, no PIE, NX enabled, partial RELRO.
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

typedef void (*handler_t)(char *);

static void echo_handler(char *s) {
    printf("echo: %s\n", s);
}

__attribute__((aligned(16)))
static handler_t command_hook = echo_handler;

static void banner(void) {
    puts("== fmt lighthouse ==");
    puts("Leak libc, compute libc base, then overwrite command_hook.");
}

static ssize_t read_line(char *buf, size_t n) {
    ssize_t r = read(STDIN_FILENO, buf, n - 1);
    if (r <= 0) exit(0);
    buf[r] = '\0';
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';
    return r;
}

int main(void) {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    char fmt[512];
    char cmd[128];
    banner();

    for (;;) {
        puts("\n1. format");
        puts("2. run command_hook");
        puts("3. info");
        puts("4. exit");
        printf("> ");
        char choice[8];
        read_line(choice, sizeof(choice));

        if (choice[0] == '1') {
            printf("format: ");
            read_line(fmt, sizeof(fmt));
            // Bug: attacker controls the format string.
            // Useful arguments are intentionally reachable to keep the challenge focused.
            printf(fmt,
                   puts,
                   (char *)&command_hook,
                   (char *)&command_hook + 2,
                   (char *)&command_hook + 4,
                   (char *)&command_hook + 6,
                   printf,
                   system,
                   0, 0, 0, 0);
            puts("");
        } else if (choice[0] == '2') {
            printf("arg: ");
            read_line(cmd, sizeof(cmd));
            command_hook(cmd);
        } else if (choice[0] == '3') {
            printf("command_hook @ %p\n", (void *)&command_hook);
            printf("printf@libc    %p\n", (void *)printf);
            puts("arg1=puts, arg2..arg5=&command_hook+{0,2,4,6}, arg7=system, arg8..arg11=zero padding args");
        } else if (choice[0] == '4') {
            return 0;
        } else {
            puts("?");
        }
    }
}
