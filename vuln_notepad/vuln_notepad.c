/*
 * vuln_notepad.c — Pwn Challenge: "Secure Notepad"
 *
 * 脆弱性:
 *   1. [Stage 1] Format String Vulnerability  → Canary / PIE base / libc base リーク
 *   2. [Stage 2] Stack Buffer Overflow        → リターンアドレス上書き
 *   3. [Stage 3] ROP Chain                    → NX bypass → system("/bin/sh")
 *
 * コンパイル (全保護有効):
 *   gcc -o vuln_notepad vuln_notepad.c \
 *       -fstack-protector-all \
 *       -pie -fpie \
 *       -z noexecstack \
 *       -z now \
 *       -z relro \
 *       -no-strip \
 *       -O0 -g
 *
 * セキュリティ状態:
 *   CANARY   : enabled  (要リーク)
 *   NX       : enabled  (ROPが必要)
 *   PIE      : enabled  (要ベースリーク)
 *   RELRO    : Full     (GOT書き換え不可)
 *   ASLR     : kernel設定次第 (libc要リーク)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_NOTES   4
#define NOTE_SIZE   64
#define TITLE_SIZE  32

typedef struct {
    char  title[TITLE_SIZE];
    char  body[NOTE_SIZE];
    int   pinned;
} Note;

static Note notes[MAX_NOTES];
static int  note_count = 0;

/* ------------------------------------------------------------------ */
/*  バナーとメニュー                                                    */
/* ------------------------------------------------------------------ */

void print_banner(void) {
    puts("============================================");
    puts("      ** Secure Notepad v0.1 **            ");
    puts("    Your notes. Safe. Always. Probably.    ");
    puts("============================================");
    puts("");
}

void print_menu(void) {
    puts("  [1] Add note");
    puts("  [2] View note");
    puts("  [3] Edit note title");
    puts("  [4] Delete note");
    puts("  [5] Exit");
    printf("  > ");
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/*  入力ユーティリティ                                                  */
/* ------------------------------------------------------------------ */

/* ★ 脆弱性なし: 安全なサイズ制限付き読み込み */
int read_int(void) {
    char buf[16];
    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    return atoi(buf);
}

/* ★★ 脆弱性あり [Stage 2 Buffer Overflow]
 *
 *    body は NOTE_SIZE(64) バイトだが、
 *    gets() / fgets() の代わりに read() を直接使っている。
 *    ここに 200 バイト以上流し込むと:
 *      [saved RBP] [canary] [return addr] を上書きできる。
 *
 *    スタックレイアウト (概算):
 *      [   body[64]   ]  <- buf 開始
 *      [  padding[8]  ]  <- アライメント
 *      [  canary[8]   ]  <- ★ 上書き前にリーク必須
 *      [ saved rbp[8] ]
 *      [ ret addr[8]  ]  <- ★ ROP chain の先頭アドレスを書く
 */
void read_body(char *buf, size_t limit) {
    (void)limit;                        /* limit を意図的に無視 */
    read(STDIN_FILENO, buf, 256);       /* ← BOF: 256バイト読む */
}

/* ------------------------------------------------------------------ */
/*  ノート操作                                                          */
/* ------------------------------------------------------------------ */

void add_note(void) {
    if (note_count >= MAX_NOTES) {
        puts("  [!] Note limit reached.");
        return;
    }
    Note *n = &notes[note_count];

    printf("  Title (max %d chars): ", TITLE_SIZE - 1);
    fflush(stdout);
    if (!fgets(n->title, TITLE_SIZE, stdin)) return;
    n->title[strcspn(n->title, "\n")] = '\0';

    printf("  Body  (max %d chars): ", NOTE_SIZE - 1);
    fflush(stdout);
    read_body(n->body, NOTE_SIZE);      /* ← BOF ここで発生 */
    n->body[NOTE_SIZE - 1] = '\0';

    n->pinned = 0;
    note_count++;
    puts("  [+] Note saved.");
}

/* ★★★ 脆弱性あり [Stage 1 Format String]
 *
 *    title をそのまま printf の第一引数として渡している。
 *    例: title = "%p.%p.%p.%p.%p.%p.%p.%p"
 *        → スタック上の値がそのまま表示される
 *
 *    重要なオフセット (64bit Ubuntu 22.04 / libc 2.35 での例):
 *      %7$p  → stack canary      (末尾が \x00 のポインタ値)
 *      %9$p  → PIE base への相対値 (バイナリ内のアドレス)
 *      %15$p → libc 内アドレス   (puts/printf 等のフレーム)
 *
 *    実際のオフセットは gdb/pwndbg で確認すること:
 *      (gdb) b printf
 *      (gdb) x/32gx $rsp
 *
 *    チャレンジのヒント:
 *      "AAAA.%1$p.%2$p.%3$p..." と順に送ってスタックを読む
 */
void view_note(void) {
    if (note_count == 0) {
        puts("  [!] No notes yet.");
        return;
    }

    printf("  Note index (0-%d): ", note_count - 1);
    fflush(stdout);
    int idx = read_int();

    if (idx < 0 || idx >= note_count) {
        puts("  [!] Invalid index.");
        return;
    }

    Note *n = &notes[idx];
    puts("  ---- Note ----");

    /* ★★★ FSB: printf(n->title) ← フォーマット文字列そのもの */
    printf("  Title : ");
    printf(n->title);                   /* ← Format String Bug */
    putchar('\n');

    printf("  Body  : %s\n", n->body); /* body は安全 */
    printf("  Pinned: %s\n", n->pinned ? "yes" : "no");
    puts("  --------------");
}

void edit_title(void) {
    if (note_count == 0) {
        puts("  [!] No notes yet.");
        return;
    }

    printf("  Note index (0-%d): ", note_count - 1);
    fflush(stdout);
    int idx = read_int();

    if (idx < 0 || idx >= note_count) {
        puts("  [!] Invalid index.");
        return;
    }

    printf("  New title: ");
    fflush(stdout);
    /* fgets で TITLE_SIZE に制限 — タイトル編集自体は安全 */
    if (!fgets(notes[idx].title, TITLE_SIZE, stdin)) return;
    notes[idx].title[strcspn(notes[idx].title, "\n")] = '\0';
    puts("  [+] Title updated.");
}

void delete_note(void) {
    if (note_count == 0) {
        puts("  [!] No notes yet.");
        return;
    }

    printf("  Note index (0-%d): ", note_count - 1);
    fflush(stdout);
    int idx = read_int();

    if (idx < 0 || idx >= note_count) {
        puts("  [!] Invalid index.");
        return;
    }

    /* 後ろ詰めして削除 */
    for (int i = idx; i < note_count - 1; i++) {
        notes[i] = notes[i + 1];
    }
    memset(&notes[note_count - 1], 0, sizeof(Note));
    note_count--;
    puts("  [+] Note deleted.");
}

/* ------------------------------------------------------------------ */
/*  メインループ                                                        */
/* ------------------------------------------------------------------ */

int main(void) {
    /* バッファリング無効化 (CTF定番) */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    print_banner();

    while (1) {
        print_menu();
        int choice = read_int();

        switch (choice) {
            case 1: add_note();    break;
            case 2: view_note();   break;
            case 3: edit_title();  break;
            case 4: delete_note(); break;
            case 5:
                puts("  Bye!");
                return 0;
            default:
                puts("  [!] Unknown option.");
        }
        puts("");
    }
}
