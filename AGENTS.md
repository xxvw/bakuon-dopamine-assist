# AGENTS.md

このファイルは、`bakuon-dopamine-assist` リポジトリで今後Codexが作業する際の恒久ルールである。

## プロジェクト概要

プロジェクト名は `bakuon-dopamine-assist`。

このプロジェクトは、音割れRemix向けのマスターチャンネル用オーディオプラグインを開発するプロジェクトである。

基本思想:

> Keep the damage. Protect the output.

意図的な音割れ、ハードクリップ、サチュレーションなどの音色を不必要に修復せず、最終出力のSample Peak、Inter-sample Peak、True Peakを安全に制御することを目的とする。

Git初期設定タスクではプラグイン本体を実装しない。JUCE/CMakeのセットアップ、DSP、GUI、テスト、CI/CDにも着手しない。

## 要件定義書

プロジェクトルートには、要件定義書 v0.1 の `.docx` ファイルがローカルに存在することを前提とする。

今後実装を行う際は、必ず次の順序を守る。

1. 最初にプロジェクトルートのDOCXを探す。
2. DOCXを読む。
3. DOCXを仕様の一次情報（source of truth）として扱う。
4. DOCXを読まずにDSP実装を開始しない。

`.docx` は `.gitignore` 対象であり、Gitにはcommitしない。DOCXをignoreすることと、仕様として参照することは別である。

仕様の優先順位:

1. ローカルの要件定義書 v0.1 DOCX
2. ユーザーから与えられた最新の指示
3. `AGENTS.md`
4. JUCE、CMake、OSなどの技術的制約
5. 一般的なオーディオDSPの慣例

ユーザーから明示的な仕様変更があった場合は、その最新指示を適切に反映する。

## 開発環境

主開発環境はmacOS。

最終的な対応予定:

- macOS
  - Apple Silicon
  - Intel
  - VST3
  - AU
- Windows
  - x86-64
  - VST3

今後プラグイン実装を行う際は、JUCE + CMakeを基本構成とする。ただしGit初期設定タスクではJUCE/CMakeを導入しない。

## Git branchルール

基本開発ブランチは `main` とする。ユーザーから別途指示がない限り、基本的に `main` 上で作業し、不必要にfeature branchを作成しない。

作業開始時には必ず現在branchを確認する。

```bash
git branch --show-current
```

## Git remote

正しいorigin:

```text
git@github.com:xxvw/bakuon-dopamine-assist.git
```

作業開始時または必要に応じて確認する。

```bash
git remote -v
```

## コミットメッセージ

Codexが作成するコミットメッセージは原則として日本語にし、変更内容が具体的に分かるものにする。

良い例:

- `プロジェクトのGit初期設定を追加`
- `JUCEとCMakeの初期構成を追加`
- `True Peak検出処理を実装`
- `Lookaheadリミッターを追加`
- `Ceiling制御の単体テストを追加`
- `macOS向けビルド設定を修正`
- `Windows向けCIを追加`
- `READMEにビルド手順を追加`

`update`、`fix`、`changes`、`misc`、`WIP`、`final`、`test` のような、変更内容が不明なメッセージは避ける。

## コミット単位

「1つの目的を持つ変更 = 1commit」を基本とする。意味のない細かすぎるcommitを避け、巨大な変更を最後に1commitへまとめることも避ける。

## Build / Test / Commit / Push

今後実装が始まった後は、原則として次のサイクルで作業する。

```text
変更
↓
build
↓
test
↓
問題があれば修正
↓
再build
↓
再test
↓
git status
↓
git add
↓
日本語commit
↓
mainへpush
```

ビルドやテストが存在する変更は、可能な限り検証してからcommitする。明らかに壊れている状態を通常の完成commitとしてpushしない。

## Pushルール

意味のある変更をcommitしたら、その都度 `main` へpushする。

通常:

```bash
git push origin main
```

初回のみ必要であれば:

```bash
git push -u origin main
```

複数の意味のあるcommitを長時間ローカルに溜め込まず、変更単位でpushする。

## Force push禁止

明確な必要性とユーザーからの指示がない限り、次を実行しない。

```bash
git push --force
git push -f
```

remoteに既存履歴が存在する場合は、安全に状態を確認し、勝手にremote履歴を上書きしない。

## destructive Git操作

明確な必要性がない限り、次のような破壊的Git操作を使用しない。

```bash
git reset --hard
git clean -fd
git checkout -- .
git restore .
```

ユーザーの未コミット作業や既存ファイルを勝手に破棄しない。

## 既存変更の保護

作業開始時に既存の未コミット変更がある場合、それをユーザーの作業として扱う。Codex自身が作成したと確認できない既存変更を勝手に削除、reset、上書きしない。

## Secrets

次の情報をGitへcommitしない。

- API key
- password
- GitHub token
- Apple Developer credential
- signing password
- certificate private key
- `.env` 内の秘密情報
- その他の認証情報

秘密情報をソースコードへ直接記述しない。

## DOCX

次のルールを必ず `.gitignore` に含め、DOCXをGit管理対象外にする。

```gitignore
*.docx
```

今後の実装開始時にはローカルDOCXを必ず確認する。

## DSP実装時の基本ルール

今後DSP実装を開始する場合は、次を守る。

- 音割れを修復するプラグインにしない。
- intentional distortionを可能な限り維持する。
- Sample PeakだけでなくTrue Peakを考慮する。
- Inter-sample Peakを考慮する。
- ±1.0を超えたfloating-point inputを無条件にhard clipしない。
- True Peak検出を単純なsample clampで代用しない。
- DSPとGUIを分離する。
- DSPを単体テスト可能な構造にする。
- audio threadでdynamic allocationしない。
- audio threadでmutexを使用しない。
- audio threadでfilesystem/network accessをしない。
- parameter smoothingを行う。
- NaN、Inf、denormalを考慮する。

具体的な数値仕様は必ず要件定義書DOCXを優先する。

## ドキュメント

重要な設計判断が必要になった場合は、必要に応じて `docs/DECISIONS.md` に次を記録する。

- 決定事項
- 理由
- 検討した選択肢
- 将来的な変更可能性

曖昧な点が存在しても、重大な情報不足でなければ合理的なデフォルトを選び、作業を進める。

## 作業終了時

作業終了前に最低限、次を確認する。

```bash
git status
git branch --show-current
git remote -v
```

変更がある場合は、適切な検証、commit、pushが完了していることを確認する。

pushできなかった場合は、次をユーザーへ報告する。

- commit hash
- pushできなかった理由
- 未pushのcommit
