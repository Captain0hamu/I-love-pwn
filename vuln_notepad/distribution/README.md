# Secure Notepad

Category: pwn

Secure Notepad は、ノートの追加・表示・編集・削除ができる小さなメモアプリです。
メモの管理処理には、意図的にいくつかの脆弱性が含まれています。

## 配布ファイル

- `vuln_notepad`: 問題の実行ファイル
- `libc.so.6`: 実行用ライブラリ
- `ld-linux-x86-64.so.2`: 動的リンカー
- `run.sh`: ローカル起動スクリプト
- `Dockerfile`、`compose.yaml`、`.dockerignore`: ローカルサービス用の実行環境
- `flag.txt`: ローカル動作確認用のダミーフラグ

## Docker で起動

Docker と Docker Compose が利用できる環境で、展開したディレクトリ内から実行します。

```sh
docker compose up --build -d
nc localhost 9999
```

接続ごとに独立したプロセスが起動し、1 接続の実行時間は最大 120 秒です。
コンテナ内では `flag.txt` が `/flag` に配置されます。
配布されるフラグはローカル確認用で、本番のフラグとは異なります。
ポートはローカルホストのみに公開されます。

終了・削除:

```sh
docker compose down
```

実行対象は x86-64 です。ARM 環境では Docker 側の amd64 エミュレーションが必要です。

## Docker を使わずに起動

Linux x86-64 環境で、展開したディレクトリ内の以下を実行してください。
ビルドや追加ライブラリのインストールは不要です。

```sh
./run.sh
```

終了するには、メニューで `5` を入力してください。

## リモート接続

接続先は開催時に案内されるホスト名とポート番号を使用してください。

```sh
nc <host> <port>
```
