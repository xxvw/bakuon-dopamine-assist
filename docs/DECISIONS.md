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

決定: NormalはITU-R BS.1770-5 Annex 2記載の4相48次FIR係数（4 phase × 12 taps）を使用し、Highは64-tap Blackman-windowed sincの8相補間を使用する。Input/Output meterは選択Qualityへ追従し、Ceiling保証用predictorは別系統とする。

理由: Normalで規格記載方式との直接対応とCPU目標を両立し、Highでは8xの時間分解能を追加するため。いずれも入力段でsampleを±1へclampしない。

検討した選択肢: Sample Peakのみ、線形補間、JUCE Oversamplingを検証なしでmeterとして流用する。

将来の変更可能性: BS.2217 test materialや外部golden referenceとの比較結果に応じて、windowまたは検証済みSRC実装へ更新する。

## D-005 Ceiling conformance predictor

決定: ユーザー選択のNormal 4x / High 8x detectorとは別に、gain計算専用の16相・64-tap conformance predictorを使用する。内部targetはconfigured Ceilingから0.05 dB guardと、初回conformanceで確定した0.02 dB calibration marginを差し引く。

理由: 4x/8xの離散時点だけではhard-clipped/square-like信号のbandlimited reconstruction peakを16x独立参照に対して最大約0.36 dB under-readしたため。大きな固定marginで全素材を過剰に減衰する代わりに、時間分解能を安全予測だけに追加する。

検討した選択肢: 0.4 dB以上の固定guard、output hard clipper、test許容値の緩和。

将来の変更可能性: 外部規格meterおよびBS.2217 vectorで同等以上の精度を証明できる低CPU方式へ置換可能。16相predictorはユーザー向けquality modeや非線形oversampling処理を意味しない。

## D-006 Normal modeのCPU最適化

決定: Input/Output meterは選択中のNormal 4xまたはHigh 8xで処理し、NormalではAnnex 2の12-tap係数だけを使用する。Ceiling用16相predictorは独立して維持する。

理由: 48 kHz、stereo、512 samplesのRelease benchmarkで、Normalの初回実測6.206%が5%目標を超えたため。重複していた64-tap meter処理を除くことで3.796%へ低減し、独立16x referenceによるCeiling +0.05 dBTP試験は引き続き合格した。

検討した選択肢: Ceiling predictorのphase/tap削減、Output True Peak meterの廃止、5%目標の緩和。

将来の変更可能性: SIMD化または検証済みpolyphase実装により、High modeを含む追加最適化を行える。性能閾値はCI worker差を考慮し、正式なbenchmark runnerを導入した時点で再調整する。
