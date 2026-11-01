# SecureDB — 運営・開発用

## クリーンな clone から配布物を作る

必要なもの: `gcc`、`curl`、`dpkg-deb`、`python3`。Nix を使う場合は、この
ディレクトリで `nix-shell` に入ってから実行します。

```sh
make dist
```

`make dist` は次を行います。

1. `setup_glibc231.sh` で Ubuntu 20.04 の libc / libc-dev を `.glibc231/` に展開
2. `build_securedb_glibc231.sh` で glibc 2.31 向けの `securedb` を再ビルド
3. `package_dist.py` の allowlist に従って `dist/securedb.tar.gz` を作成

生成物:

- `dist/securedb.tar.gz`: 参加者向け添付ファイル
- `dist/SHA256SUMS`: アーカイブの SHA-256

配布するのはこの2ファイルです。アーカイブには、`securedb`、
`libc.so.6`、`ld-2.31.so` と、`distribution/` の説明・起動スクリプト・
Dockerfile・Compose・`.dockerignore`・ダミーフラグが入ります。
参加者向け説明は `distribution/README.md` で編集できます。
入力ファイルが同一なら、アーカイブも同一になります。

`securedb`、`libc.so.6`、`ld-2.31.so`、`.glibc231/`、`dist/` は Git 管理外です。
ビルド済みのファイルがある場合は、再ビルドせずにアーカイブだけを作れます。

```sh
make package
```

接続先は開催環境に合わせて別途案内してください。
展開後に `docker compose up --build -d` を実行すると、
`nc localhost 9999` でローカルサービスに接続できます。
Docker のビルドコンテキストは、元の開発ディレクトリではなく展開先を使ってください。
本番運用時は `/flag` の内容を本物のフラグに差し替え、公開ポートの設定を調整します。
配布用の `distribution/flag.txt` はダミーのままにしてください。

## 開発用ファイル

- `securedb.c`、`Makefile`、`*_glibc231.sh`、Nix ファイル: ビルド環境
- `exploit_securedb.py`、`securedb_solution/`: 解答・検証用
- `hints.md`、`README_Makefile.txt`: ヒント・内部資料

開発用ファイルは配布アーカイブに入りません。
ディレクトリ全体をそのまま配布しないでください。
`package_dist.py` は再ビルドを行わず、現在の実行ファイルをそのまま収録します。
