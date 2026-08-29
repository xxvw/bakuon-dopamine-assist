# Remix Safe Master

`bakuon-dopamine-assist` は、音割れRemix向けのマスターチャンネル用True Peak Safety Pluginです。

> Keep the damage. Protect the output.

意図的なhard clipping、saturation、distortionを復元するのではなく、その質感を可能な限り保ちながら、マスター出口で新たに発生するSample Peak、Inter-sample Peak、True Peak超過を抑えます。ラウドネス最大化を主目的とするLimiterではありません。

## MVP機能

- macOS VST3 / AU / QA用Standalone
- Windows VST3向けCMake構成
- mono / stereo、float32 / float64 processing
- Input Trim: -24.0〜+6.0 dB、0.1 dB刻み
- Ceiling: -3.0〜-0.1 dBTP、初期値-1.0 dBTP
- Normal 4x / High 8x True Peak detector
- 1.5 ms lookahead、100% stereo-linked Safety Limiter
- Auto Releaseおよび20〜500 ms固定Release
- latency-aligned smooth bypass
- Input Sample Peak、Input/Output True Peak、Gain Reduction meter
- versioned parameter state
- DSP unit / conformance tests

## 必要なツール

- macOS 12.0以上をtargetにできるXcode / Apple Clang
- Xcode Command Line Tools
- CMake 3.22以上
- Git
- Windows buildの場合はWindows 10/11 x64、MSVC、Visual Studio 2022

JUCE 9.0.0は `third_party/JUCE` のGit submoduleとしてcommit固定されています。

## macOS setup

```bash
git clone --recurse-submodules git@github.com:xxvw/bakuon-dopamine-assist.git
cd bakuon-dopamine-assist
```

既にclone済みでsubmoduleが空の場合:

```bash
git submodule update --init --recursive
```

## CMake configure

日常のApple Silicon Debug環境:

```bash
cmake --preset mac-debug-arm64
```

Apple Silicon Release:

```bash
cmake --preset mac-release-arm64
```

Universal Binary Release候補:

```bash
cmake --preset mac-release-universal
```

## Debug build

```bash
cmake --build --preset mac-debug-arm64 --parallel
```

## Release build

```bash
cmake --build --preset mac-release-arm64 --parallel
```

Universal Binary:

```bash
cmake --build --preset mac-release-universal --parallel
```

## Tests

```bash
ctest --preset mac-debug-arm64 --output-on-failure
ctest --preset mac-release-arm64 --output-on-failure
```

DSP testは、silence、透明性、Inter-sample Peak、4x/8x、lookahead、stereo link、+18 dBFS floating input、NaN/Inf、bypass、reset、対応sample rates、irregular block-size準備、独立16x referenceによるCeiling精度を対象にします。

## 検証結果

2026-08-29のローカル検証:

- macOS arm64 Debug: VST3 / AU / Standalone build、CTest合格
- macOS arm64 Release: VST3 / AU / Standalone build、CTest合格
- macOS Universal Release: VST3 / AU / Standalone build、CTest合格
- Universal binary: VST3 / AU / Standaloneで `x86_64 arm64` を確認
- Apple `auval -strict`: `AU VALIDATION SUCCEEDED`
- 48 kHz / stereo / 512 samples / Normal: 10秒相当のRelease処理で3.796% CPU（この開発環境でのwall-clock比）
- VST3: JUCE/Steinberg manifest helperによるfactory loadとJSON5 `moduleinfo.json` 生成を確認

VST3 SDK validatorと実DAWでのscan/save-reloadは、配布前のhost compatibility matrixで追加実施します。

## VST3 / AU build

個別target:

```bash
cmake --build --preset mac-debug-arm64 --target RemixSafeMaster_VST3
cmake --build --preset mac-debug-arm64 --target RemixSafeMaster_AU
```

## Build artifact location

Debug:

```text
build/mac-debug-arm64/RemixSafeMaster_artefacts/Debug/VST3/Remix Safe Master.vst3
build/mac-debug-arm64/RemixSafeMaster_artefacts/Debug/AU/Remix Safe Master.component
build/mac-debug-arm64/RemixSafeMaster_artefacts/Debug/Standalone/Remix Safe Master.app
```

Releaseは対応するpreset下の `RemixSafeMaster_artefacts/Release/` に生成されます。

配布用Universal Release:

```text
build/mac-release-universal/RemixSafeMaster_artefacts/Release/VST3/Remix Safe Master.vst3
build/mac-release-universal/RemixSafeMaster_artefacts/Release/AU/Remix Safe Master.component
```

## Windows build概要

Windows x86-64 / VST3用presetを用意しています。

```powershell
cmake --preset win-release-x64
cmake --build --preset win-release-x64 --parallel
ctest --preset win-release-x64 --output-on-failure
```

Windows releaseはWindows上のMSVC toolchainで生成します。Macからのcross compileは正式経路にしません。

現時点の状態:

```text
Windows build configuration prepared but not locally validated
```

## 仕様

ローカルの要件定義書 v0.1 DOCXが仕様の一次情報です。DOCXは参照しますがGitにはcommitしません。設計詳細は [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)、DOCXにない判断は [docs/DECISIONS.md](docs/DECISIONS.md) を参照してください。
