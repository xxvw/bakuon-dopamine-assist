# アーキテクチャ

## 目的

Remix Safe Masterは、入力に既に含まれるintentional distortionを修復せず、マスター出口の再構成ピークを設定Ceiling内へ制御する安全処理である。

## Signal flow

```text
floating-point input（±1.0を超えてよい）
  ↓
finite sanitization / denormal handling
  ├──────── latency-aligned dry bypass path ────────┐
  ↓                                                 │
smoothed Input Trim                                 │
  ↓                                                 │
4x Normal / 8x High True Peak meter                 │
  + 16-phase conformance safety predictor           │
  ↓                                                 │
stereo-linked required gain                         │
  ↓                                                 │
1.5 ms lookahead / gain attack-hold-release          │
  ↓                                                 │
delayed signal × gain envelope                       │
  ↓                                                 │
smooth bypass crossfade ◀────────────────────────────┘
  ↓
8x output True Peak verification / lock-free metering
  ↓
output
```

## DSP分離

- `DspUtilities`: dB/linear変換、finite/denormal sanitization
- `OversamplingStage`: polyphase interpolationと履歴state
- `TruePeakDetector`: channel peakと100% linked peakの検出
- `GainComputer`: required gain、lookahead hold、attack/release envelope
- `SafetyLimiter`: delay、parameter smoothing、bypass、DSP全体の調停
- `Metering`: audio threadからGUIへのatomic snapshot
- `Parameters`: APVTS parameter layoutとraw atomic pointer
- `PluginProcessor`: host buffer、latency、state、DSPの接続
- `PluginEditor`: parameter attachmentとmeter表示のみ。DSPロジックを持たない

## Oversampling / True Peak detection

NormalはITU-R BS.1770-5 Annex 2の4 phase × 12 taps係数を使用する。Highは64-tap・8 phaseのwindowed-sincで補間する。Input/Output True Peak meterはいずれも選択中のQualityを使用する。

4x/8xだけでhard-clipped素材の再構成ピークをunder-readしないよう、gain計算には64-tap・16 phaseのconformance predictorを追加する。これはユーザー向け16x quality modeや非線形oversampling処理ではなく、Ceiling保証専用の安全予測である。

test側は実装クラスを共有せず、別コードの16x・64-tap reference meterで出力を測定する。

## Lookahead / latency

Lookaheadは固定sample数ではなく次式で求める。

```text
round(sampleRate × 0.0015)
```

報告latencyはLookaheadと64-tap predictorのgroup delay 31 samplesの合計である。48 kHzでは72 + 31 = 103 samples、約2.15 msとなる。delay memoryは `prepareToPlay` で確保し、hostへ `setLatencySamples` で報告する。

## Gain computer / limiter

基本式:

```text
requiredGain = min(1, effectiveCeilingLinear / (predictedPeak + epsilon))
```

`effectiveCeiling` はconfigured Ceilingから0.05 dB guardと0.02 dB conformance calibrationを差し引く。検出したgainをlookahead期間保持することで、対象ピークがdelay出力へ到達するまでreleaseを開始しない。

Attackはピーク到達前に完了する短い指数補間、Releaseは1.0へ滑らかに復帰する。Auto Releaseはgain reductionの深さと継続時間に応じて約50〜500 ms内で適応し、固定modeは20〜500 msを使用する。

通常動作はoutput hard clipperに依存しない。入力が±1.0を超えることを理由にclampせず、gainのみで安全化する。

## Parameter smoothing / bypass

Input Trim、Ceiling、Bypassは10 msで補間する。Quality切替は両modeの固定長stateを事前確保しており、audio threadでreallocationしない。

Bypassはraw input用のlatency-aligned delayを常時進め、processed pathとのcrossfadeで切り替える。host bypass callbackも同じpathを使用する。

## Stereo

MVPは100% Stereo Link固定。各oversampled時点でL/Rの絶対値最大から1つのgain envelopeを計算し、両channelへ同じgainを適用する。片channelだけがhotでもstereo imageを移動させない。

## Metering

block内では通常の数値としてInput Sample Peak、Input True Peak、Output Sample Peak、Output True Peak、current/peak Gain Reductionを集計する。block終了時にatomicへ1回publishし、GUIは30 Hz Timerから読む。audio threadとGUIの間にmutexはない。

## Realtime safety

Audio callbackおよびそこから呼ぶDSPでは以下を行わない。

- dynamic allocation / vector resize
- mutex / blocking synchronization
- filesystem / network / console I/O
- sleep

delay bufferとfilter stateは `prepareToPlay` で確保する。parameterはAPVTSのraw atomic pointerからblockごとに読み、sample loop内でhost/API呼び出しを行わない。

## Numeric safety

- NaN / +Inf / -Infは0へ置換し、後段へ伝播させない
- denormal相当は0へ置換し、processorでも `ScopedNoDenormals` を使用
- 少なくとも|x|=32（約+30.10 dBFS）まで入力clampなしで処理
- 演算結果が非finiteになった場合は安全値0へ置換
- silence、reset、sample-rate再初期化時はfilter/delay/gain stateを明示初期化

## Test architecture

JUCE UnitTest runnerをconsole targetとして構成する。True Peak/Ceiling testは実装と同じdetectorを合否判定に使わず、独立16x referenceを使用する。主要test signalはsine、phase-shifted high-frequency sine、impulse、clipped transient、square-like、+18 dBFS、NaN/Inf、mono/stereoを含む。

Release testには、48 kHz、stereo、512 samples、Normalで10秒相当を連続処理するCPU benchmarkを含める。wall-clock処理時間を音声時間で割り、5%未満を受入条件とする。初回実測6.206%に対し、Normal meterをAnnex 2へ限定しOutput meterを選択Qualityへ追従させた後は3.796%となった。Ceiling判定用16相predictorは維持し、最適化前後で独立16x conformance testを通す。
