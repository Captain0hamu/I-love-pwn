# tools

チャレンジ本体とは独立した小物です。

- `justfile` — ELF のヘッダ・セグメント・シンボル・逆アセンブルを表示するレシピ
  - 例: `just -f tools/justfile all ./vuln_notepad/build/vuln_notepad`
- `ansi_template.py` — ターミナル出力用の ANSI カラーテンプレート
- `banner.py` — pwn の成功時などに使う表示用バナー
