# SecureDB — 運営・開発用

## 配布物の作成

このディレクトリで次を実行します（Python 3.8 以降）。

```sh
python3 package_dist.py
```

生成物:

- `dist/securedb.tar.gz`: 参加者向け添付ファイル
- `dist/SHA256SUMS`: アーカイブの SHA-256

配布するのはこの2ファイルです。アーカイブには、既存の `securedb`、
`libc.so.6`、`ld-2.31.so` と、`distribution/` の説明・起動スクリプト・
Dockerfile・Compose・`.dockerignore`・ダミーフラグが入ります。
参加者向け説明は `distribution/README.md` で編集できます。
入力ファイルが同一なら、アーカイブも同一になります。

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
パッケージ作成では再ビルドを行わず、現在の実行ファイルをそのまま収録します。
