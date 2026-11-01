# I-love-pwn

CTF / 学習用の pwn チャレンジ集です。各チャレンジは「ソース・ビルド手順・配布用テンプレート」をリポジトリに置き、生成物（実行ファイル・libc・配布アーカイブ）はコミットしません。

## 収録チャレンジ

| ディレクトリ | 内容 | 想定環境 | 配布物 |
|---|---|---|---|
| [`securedb/`](securedb/) | tcache / heap exploitation | Ubuntu 20.04, glibc 2.31 | あり |
| [`vuln_notepad/`](vuln_notepad/) | format string + buffer overflow + ROP | Debian 13, glibc 2.41 | あり |
| [`pwn_two_chals/`](pwn_two_chals/) | heap と format string の演習 2 問 | Debian 13, glibc 2.41 | 開発用 |
| [`docs/`](docs/) | Phoenix などの write-up | - | - |

各チャレンジの詳しいビルド方法・配布物の作り方は、それぞれの `README.md` を見てください。

## リポジトリの方針

- **外部リポジトリを同梱しない**: 以前 `how2pwn/` に置いていた clone や `PwnPad/` のような丸ごとコピーは削除しました。外部資料は下のリンクと取得スクリプトで最新版を取ります。
- **生成物を Git に入れない**: 実行ファイル、libc、loader、`make` の出力、`dist/` のアーカイブは `.gitignore` で除外します。
- **配布物は allowlist で作る**: `securedb/` と `vuln_notepad/` は `package_dist.py` が明示したファイルだけをアーカイブします。開発用ファイルや解答は入りません。
- **クリーンな clone から再現できる**: 各チャレンジにビルドスクリプトを用意しています。

## ディレクトリ構成

```text
.
├── securedb/          # 配布パッケージ + 解答
├── vuln_notepad/      # 配布パッケージ + exploit 雛形
├── pwn_two_chals/     # 2 問の演習セット
├── docs/              # write-up
├── scripts/           # 外部リポジトリ取得
└── tools/             # ELF 解析・表示用の小物
```

## 配布物のビルド

```sh
# SecureDB (glibc 2.31)
cd securedb
nix-shell --run 'make dist'

# vuln_notepad (Docker / Debian 13)
cd vuln_notepad
make dist
```

生成された `dist/*.tar.gz` と `dist/SHA256SUMS` を配布します。展開先で `docker compose up --build -d` を実行すると、ローカルで `nc localhost 9999` から接続できます。

## 学習リソース

外部リポジトリは同梱せず、必要なものを `external/` に取得する形にしています。`external/` は Git 管理外です。

```sh
./scripts/fetch-resources.sh how2heap
./scripts/fetch-resources.sh --all
```

| おすすめ | リソース | 内容 |
|---|---|---|
| ★★★★★ | [shellphish/how2heap](https://github.com/shellphish/how2heap) | glibc heap exploitation の定番。以前同梱していた `how2pwn` の中身はこれです。まずはここから。 |
| ★★★★☆ | [pwn.college](https://pwn.college/) | 基礎から学べる演習プラットフォーム。 |
| ★★★★☆ | [ROP Emporium](https://ropemporium.com/) | ROP を段階的に練習できる。 |
| ★★★☆☆ | [Nightmare](https://github.com/guyinatuxedo/nightmare) | CTF pwn の幅広い入門資料。 |
| ★★★☆☆ | [exploit.education](https://exploit.education/) | Phoenix / Fusion などの演習 VM。`docs/` の write-up と相性が良い。 |
| ★★★☆☆ | [libc.rip](https://libc.rip/) | libc の特定・オフセット調査。 |
| ★★☆☆☆ | [pwntools](https://github.com/Gallopsled/pwntools) | exploit 開発用ライブラリ。 |
| ★★☆☆☆ | [pwndbg](https://github.com/pwndbg/pwndbg) / [GEF](https://github.com/hugsy/gef) | gdb の pwn 向け拡張。 |
| ★★☆☆☆ | [glibc-all-in-one](https://github.com/matrix1001/glibc-all-in-one) | 各種 glibc の取得。 |

> 外部リポジトリをこのリポジトリへコピーしないでください。更新は `scripts/fetch-resources.sh` で行います。
