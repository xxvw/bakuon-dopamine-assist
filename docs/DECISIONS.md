# 設計判断

## D-001 JUCEの固定バージョン

決定: JUCE 9.0.0 (`f8f8864172464b9adf9eba6101e1f784838d1597`) をGit submoduleとして固定する。

理由: 要件定義書の「tag/commitを固定して再現可能にする」を満たし、実装開始時点の公式stable releaseを使用するため。

検討した選択肢: FetchContent、JUCE 8系、develop branch。

将来の変更可能性: JUCE更新はbreaking changes、全DSP test、VST3/AU build、host validationを確認した独立commitで行う。

## D-002 暫定plugin metadata

決定: Product nameは `Remix Safe Master`、company metadataには法人名ではなくproject識別子 `bakuon-dopamine-assist`、manufacturer codeは `Baku`、plugin codeは `RsMp`、bundle identifierは `audio.bakuon.remix-safe-master` とする。

理由: 要件定義書には製品仮称のみがあり、ビルドに安定した識別子が必要なため。企業名や個人情報は設定しない。

検討した選択肢: repository名をそのままproduct nameに使う、架空のcompany nameを追加する。

将来の変更可能性: 正式な製品名・manufacturer・bundle IDが決まった時点で、state/host互換性への影響を確認して更新する。

## D-003 macOS deployment target

決定: CMake presetの初期deployment targetをmacOS 12.0とする。

理由: 要件定義書の初期候補に一致するため。

検討した選択肢: 現在の開発Macに合わせてより新しいtargetにする。

将来の変更可能性: 対象DAWとIntel Macの互換性試験後に確定する。

## D-004 True Peak補間方式

決定: NormalはITU-R BS.1770-5 Annex 2記載の4相48次FIR係数（4 phase × 12 taps）を使用し、Highは32-tap Blackman-windowed sincの8相補間を使用する。

理由: Normalで規格記載方式との直接対応を保ち、Highでは8xの時間分解能を追加するため。いずれも入力段でsampleを±1へclampしない。

検討した選択肢: Sample Peakのみ、線形補間、JUCE Oversamplingを検証なしでmeterとして流用する。

将来の変更可能性: BS.2217 test materialや外部golden referenceとの比較結果に応じて、Highのtap数・windowまたは検証済みSRC実装へ更新する。
