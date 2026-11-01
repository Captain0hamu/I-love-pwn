# vuln_notepad — 運営・開発用

## 概要

「Secure Notepad」は、format string、stack buffer overflow、ROP を組み合わせる
x86-64 pwn チャレンジです。

- 想定環境: Debian 13 (trixie) / glibc 2.41 / x86-64
- 保護: canary, PIE, NX, Full RELRO
- 配布物: `vuln_notepad`、`libc.so.6`、`ld-linux-x86-64.so.2`、起動スクリプト、
  Dockerfile、Compose、ダミーフラグ

## 配布物の作成

Docker と Docker Compose が使える環境で、このディレクトリから実行します。

```sh
make dist
```

`make dist` は次を行います。

1. `packaging/Dockerfile` で Debian 13 のビルド環境を作る
2. `build/` に実行ファイル・libc・loader を出力する
3. `package_dist.py` の allowlist に従って `dist/vuln_notepad.tar.gz` を作る
4. `dist/SHA256SUMS` を更新する

生成物:

- `dist/vuln_notepad.tar.gz`: 参加者向け添付ファイル
- `dist/SHA256SUMS`: アーカイブの SHA-256

配布するのはこの 2 ファイルです。`build/`、`dist/`、開発用ファイルは Git 管理外です。
アーカイブには `distribution/` の説明・起動スクリプト・Dockerfile・Compose・
`.dockerignore`・ダミーフラグが入ります。参加者向け説明は
`distribution/README.md` で編集できます。

ローカルでビルド済みの `build/` がある場合は、Docker を使わずに
`python3 package_dist.py` だけでもアーカイブを作れます。

```sh
make package
```

## 開発用ビルド

ホスト環境でビルドする場合:

```sh
make
./vuln_notepad
```

`make` の出力 `vuln_notepad` は Git 管理外です。

## ファイル構成

```text
vuln_notepad/
├── vuln_notepad.c          # 問題本体
├── Makefile                # 開発用ビルド / 配布物作成
├── package_dist.py         # allowlist ベースのアーカイブ作成
├── build_dist.sh           # Docker で build/ を生成
├── packaging/Dockerfile    # 配布用バイナリのビルド環境
├── distribution/           # 参加者向けの説明・実行環境
├── exploit.py              # exploit 雛形（開発用）
└── build/, dist/           # 生成物（Git 管理外）
```

## 注意

- ディレクトリ全体をそのまま配布しないでください。
- `distribution/flag.txt` はローカル確認用のダミーです。本番では `/flag` を
  本物のフラグに差し替えてください。
- 接続先は開催環境に合わせて別途案内してください。
