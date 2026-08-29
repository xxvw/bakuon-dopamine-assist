#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace
{
const auto backgroundTop = juce::Colour(0xff080912);
const auto backgroundBottom = juce::Colour(0xff11142a);
const auto surface = juce::Colour(0xff171a30);
const auto surfaceRaised = juce::Colour(0xff222743);
const auto cyan = juce::Colour(0xff39f5ff);
const auto magenta = juce::Colour(0xffff3ac8);
const auto yellow = juce::Colour(0xffffd447);
const auto green = juce::Colour(0xff58ff9e);
const auto danger = juce::Colour(0xffff4e6a);
const auto primaryText = juce::Colour(0xfff5f7ff);
const auto secondaryText = juce::Colour(0xffaeb7d5);
const auto quietText = juce::Colour(0xff7882a7);

juce::String operator""_utf8(const char* text, std::size_t length)
{
    return juce::String::fromUTF8(text, static_cast<int>(length));
}

juce::Font japaneseFont(float height, int style = juce::Font::plain)
{
   #if JUCE_MAC
    constexpr auto family = "Hiragino Sans";
   #elif JUCE_WINDOWS
    constexpr auto family = "Yu Gothic UI";
   #else
    constexpr auto family = "Noto Sans CJK JP";
   #endif

    return juce::Font(juce::FontOptions(family, height, style));
}

struct UiLayout
{
    juce::Rectangle<int> outer;
    juce::Rectangle<int> header;
    juce::Rectangle<int> controls;
    juce::Rectangle<int> meters;
    juce::Rectangle<int> status;
};

UiLayout makeLayout(juce::Rectangle<int> editorBounds)
{
    UiLayout layout;
    layout.outer = editorBounds.reduced(18);
    auto content = layout.outer;
    layout.header = content.removeFromTop(96);
    content.removeFromTop(12);
    layout.status = content.removeFromBottom(44);
    content.removeFromBottom(10);
    const auto meterHeight = std::clamp(content.getHeight() / 2, 128, 158);
    layout.meters = content.removeFromBottom(meterHeight);
    content.removeFromBottom(12);
    layout.controls = content;
    return layout;
}

void fillCard(juce::Graphics& graphics,
              juce::Rectangle<float> bounds,
              juce::Colour edge,
              float corner = 16.0f)
{
    juce::ColourGradient cardGradient(surfaceRaised.withAlpha(0.92f),
                                      bounds.getX(), bounds.getY(),
                                      surface.withAlpha(0.96f),
                                      bounds.getRight(), bounds.getBottom(),
                                      false);
    graphics.setGradientFill(cardGradient);
    graphics.fillRoundedRectangle(bounds, corner);
    graphics.setColour(edge.withAlpha(0.24f));
    graphics.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.0f);
}

void drawGlow(juce::Graphics& graphics,
              juce::Point<float> centre,
              float radius,
              juce::Colour colour)
{
    juce::ColourGradient glow(colour.withAlpha(0.20f),
                              centre.x, centre.y,
                              juce::Colours::transparentBlack,
                              centre.x + radius, centre.y,
                              true);
    graphics.setGradientFill(glow);
    graphics.fillEllipse(centre.x - radius,
                         centre.y - radius,
                         radius * 2.0f,
                         radius * 2.0f);
}
}

class DopamineLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DopamineLookAndFeel()
    {
        setColour(juce::ComboBox::backgroundColourId, surfaceRaised);
        setColour(juce::ComboBox::textColourId, primaryText);
        setColour(juce::ComboBox::outlineColourId, cyan.withAlpha(0.35f));
        setColour(juce::ComboBox::arrowColourId, cyan);
        setColour(juce::PopupMenu::backgroundColourId, surfaceRaised);
        setColour(juce::PopupMenu::textColourId, primaryText);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, magenta.withAlpha(0.55f));
        setColour(juce::PopupMenu::highlightedTextColourId, primaryText);
        setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff252a48));
        setColour(juce::TooltipWindow::textColourId, primaryText);
        setColour(juce::TooltipWindow::outlineColourId, cyan.withAlpha(0.6f));
    }

    void drawRotarySlider(juce::Graphics& graphics,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPosition,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        const auto area = juce::Rectangle<float>(static_cast<float>(x),
                                                 static_cast<float>(y),
                                                 static_cast<float>(width),
                                                 static_cast<float>(height));
        const auto centre = area.getCentre();
        const auto radius = std::max(18.0f,
                                     std::min(area.getWidth(), area.getHeight()) * 0.5f - 11.0f);
        const auto angle = rotaryStartAngle
                         + sliderPosition * (rotaryEndAngle - rotaryStartAngle);
        const auto accentColour = slider.findColour(juce::Slider::rotarySliderFillColourId);

        drawGlow(graphics, centre, radius * 1.25f, accentColour);
        graphics.setColour(juce::Colours::black.withAlpha(0.38f));
        graphics.fillEllipse(centre.x - radius,
                             centre.y - radius,
                             radius * 2.0f,
                             radius * 2.0f);
        graphics.setColour(surfaceRaised.brighter(0.12f));
        graphics.drawEllipse(centre.x - radius,
                             centre.y - radius,
                             radius * 2.0f,
                             radius * 2.0f,
                             1.5f);

        for (int tick = 0; tick <= 12; ++tick)
        {
            const auto tickPosition = static_cast<float>(tick) / 12.0f;
            const auto tickAngle = rotaryStartAngle
                                 + tickPosition * (rotaryEndAngle - rotaryStartAngle);
            const auto outer = centre + juce::Point<float>(std::sin(tickAngle),
                                                           -std::cos(tickAngle)) * (radius + 5.0f);
            const auto inner = centre + juce::Point<float>(std::sin(tickAngle),
                                                           -std::cos(tickAngle)) * (radius + 1.0f);
            graphics.setColour(tickPosition <= sliderPosition
                                   ? accentColour.withAlpha(0.85f)
                                   : quietText.withAlpha(0.34f));
            graphics.drawLine(juce::Line<float>(inner, outer), tick % 3 == 0 ? 1.8f : 1.0f);
        }

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centre.x, centre.y,
                                    radius + 1.0f, radius + 1.0f,
                                    0.0f, rotaryStartAngle, rotaryEndAngle, true);
        graphics.setColour(quietText.withAlpha(0.18f));
        graphics.strokePath(backgroundArc,
                            juce::PathStrokeType(5.0f,
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        if (sliderPosition > 0.001f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y,
                                   radius + 1.0f, radius + 1.0f,
                                   0.0f, rotaryStartAngle, angle, true);
            graphics.setColour(accentColour.withAlpha(0.18f));
            graphics.strokePath(valueArc,
                                juce::PathStrokeType(10.0f,
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            graphics.setColour(accentColour);
            graphics.strokePath(valueArc,
                                juce::PathStrokeType(4.0f,
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        juce::Path pointer;
        pointer.addRoundedRectangle(-2.2f, -radius + 13.0f,
                                    4.4f, radius * 0.43f, 2.2f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle)
                                   .translated(centre.x, centre.y));
        graphics.setColour(primaryText);
        graphics.fillPath(pointer);
        graphics.setColour(accentColour);
        graphics.fillEllipse(centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f);
    }

    void drawToggleButton(juce::Graphics& graphics,
                          juce::ToggleButton& button,
                          bool highlighted,
                          bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.5f);
        const auto isOn = button.getToggleState();
        const auto onColour = button.findColour(juce::ToggleButton::tickColourId);
        const auto base = isOn ? onColour : surface;

        graphics.setColour(juce::Colours::black.withAlpha(0.34f));
        graphics.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 12.0f);
        graphics.setColour(base.withMultipliedBrightness(down ? 0.78f
                                                               : (highlighted ? 1.12f : 1.0f)));
        graphics.fillRoundedRectangle(bounds, 12.0f);
        graphics.setColour((isOn ? onColour.brighter(0.55f) : quietText).withAlpha(0.72f));
        graphics.drawRoundedRectangle(bounds, 12.0f, 1.2f);

        const auto indicator = bounds.removeFromLeft(34.0f).reduced(9.0f);
        graphics.setColour(isOn ? primaryText : quietText);
        graphics.fillEllipse(indicator);
        if (isOn)
        {
            graphics.setColour(onColour.withAlpha(0.32f));
            graphics.drawEllipse(indicator.expanded(4.0f), 4.0f);
        }

        graphics.setColour(isOn ? primaryText : secondaryText);
        graphics.setFont(japaneseFont(13.0f, juce::Font::bold));
        graphics.drawFittedText(button.getButtonText(),
                                bounds.toNearestInt().reduced(4, 0),
                                juce::Justification::centredLeft,
                                1);
    }

    void drawComboBox(juce::Graphics& graphics,
                      int width,
                      int height,
                      bool isButtonDown,
                      int,
                      int,
                      int,
                      int,
                      juce::ComboBox&) override
    {
        auto bounds = juce::Rectangle<float>(0.0f, 0.0f,
                                             static_cast<float>(width),
                                             static_cast<float>(height)).reduced(1.0f);
        graphics.setColour(surface.withMultipliedBrightness(isButtonDown ? 0.82f : 1.0f));
        graphics.fillRoundedRectangle(bounds, 10.0f);
        graphics.setColour(cyan.withAlpha(0.55f));
        graphics.drawRoundedRectangle(bounds, 10.0f, 1.2f);

        const auto arrowArea = bounds.removeFromRight(32.0f).reduced(9.0f, 12.0f);
        juce::Path arrow;
        arrow.startNewSubPath(arrowArea.getX(), arrowArea.getY());
        arrow.lineTo(arrowArea.getCentreX(), arrowArea.getBottom());
        arrow.lineTo(arrowArea.getRight(), arrowArea.getY());
        graphics.setColour(cyan);
        graphics.strokePath(arrow,
                            juce::PathStrokeType(2.0f,
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(12, 1, box.getWidth() - 44, box.getHeight() - 2);
        label.setFont(japaneseFont(13.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centredLeft);
    }
};

RemixSafeMasterAudioProcessorEditor::RemixSafeMasterAudioProcessorEditor(
    RemixSafeMasterAudioProcessor& audioProcessor)
    : AudioProcessorEditor(audioProcessor),
      processorReference(audioProcessor),
      dopamineLookAndFeel(std::make_unique<DopamineLookAndFeel>()),
      tooltipWindow(this, 650)
{
    setLookAndFeel(dopamineLookAndFeel.get());

    productBadge.setText("REMIX SAFE MASTER  /  TRUE PEAK GUARD",
                         juce::dontSendNotification);
    productBadge.setJustificationType(juce::Justification::centredLeft);
    productBadge.setFont(japaneseFont(10.5f, juce::Font::bold));
    productBadge.setColour(juce::Label::textColourId, cyan);

    title.setText("爆音ドーパミン・セーフティ"_utf8, juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centredLeft);
    title.setFont(japaneseFont(27.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, primaryText);

    subtitle.setText("壊れた質感はそのまま。出口だけ、確実に守る。"_utf8,
                     juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centredLeft);
    subtitle.setFont(japaneseFont(13.0f));
    subtitle.setColour(juce::Label::textColourId, secondaryText);

    liveBadge.setText("●  LIVE PROTECTION"_utf8, juce::dontSendNotification);
    liveBadge.setJustificationType(juce::Justification::centred);
    liveBadge.setFont(japaneseFont(11.0f, juce::Font::bold));
    liveBadge.setColour(juce::Label::textColourId, green);

    configureRotary(inputTrim, " dB", magenta,
                    "入力ドライブ"_utf8,
                    "安全処理へ入る前の音量。音色を保ったまま -24〜+6 dBで調整します。"_utf8);
    configureRotary(ceiling, " dBTP", cyan,
                    "出力上限"_utf8,
                    "最終出力のTrue Peak上限。配信や書き出しには -1.0 dBTPが安全な出発点です。"_utf8);
    configureRotary(release, " ms", yellow,
                    "戻り時間"_utf8,
                    "ゲイン抑制が元へ戻る速さ。おまかせ復帰を切ったときに有効です。"_utf8);

    configureControlLabel(inputTrimLabel, "入力ドライブ"_utf8, "01  音を入れる"_utf8);
    configureControlLabel(ceilingLabel, "出力上限"_utf8, "02  出口を守る"_utf8);
    configureControlLabel(releaseLabel, "戻り時間"_utf8, "03  揺れを整える"_utf8);
    configureControlLabel(qualityLabel, "検出精度"_utf8, "04  見張りを選ぶ"_utf8);

    quality.addItem("標準・4倍 〔軽快〕"_utf8, 1);
    quality.addItem("高精度・8倍 〔精密〕"_utf8, 2);
    quality.setJustificationType(juce::Justification::centredLeft);
    quality.setName("True Peak検出精度"_utf8);
    quality.setTooltip("標準は低負荷、高精度は8倍補間でピークを細かく監視します。"_utf8);

    qualityHint.setText("通常は標準、最終書き出しは高精度"_utf8,
                        juce::dontSendNotification);
    qualityHint.setJustificationType(juce::Justification::centredLeft);
    qualityHint.setFont(japaneseFont(10.5f));
    qualityHint.setColour(juce::Label::textColourId, secondaryText);

    autoRelease.setColour(juce::ToggleButton::tickColourId, yellow);
    autoRelease.setButtonText("おまかせ復帰"_utf8);
    autoRelease.setName("おまかせ復帰"_utf8);
    autoRelease.setTooltip("素材に合わせて戻り時間を自動調整し、ポンピングを抑えます。"_utf8);
    bypass.setColour(juce::ToggleButton::tickColourId, danger);
    bypass.setButtonText("保護を停止"_utf8);
    bypass.setName("保護を停止"_utf8);
    bypass.setTooltip("安全処理を滑らかに停止します。比較確認以外ではオフを推奨します。"_utf8);

    meterSectionTitle.setText("リアルタイム安全モニター"_utf8, juce::dontSendNotification);
    meterSectionTitle.setJustificationType(juce::Justification::centredLeft);
    meterSectionTitle.setFont(japaneseFont(12.0f, juce::Font::bold));
    meterSectionTitle.setColour(juce::Label::textColourId, primaryText);

    for (auto* meter : { &inputSampleMeter, &inputTruePeakMeter,
                         &outputTruePeakMeter, &gainReductionMeter })
        configureMeterValueLabel(*meter);

    inputSampleMeter.setText("-∞ dBFS"_utf8, juce::dontSendNotification);
    inputTruePeakMeter.setText("-∞ dBTP"_utf8, juce::dontSendNotification);
    outputTruePeakMeter.setText("-∞ dBTP"_utf8, juce::dontSendNotification);
    gainReductionMeter.setText("0.0 dB / 最大 0.0 dB"_utf8, juce::dontSendNotification);

    characterWarning.setJustificationType(juce::Justification::centredLeft);
    characterWarning.setFont(japaneseFont(13.0f, juce::Font::bold));
    characterWarning.setColour(juce::Label::textColourId, green);
    characterWarning.setText("✓ 保護中 — 壊れた質感はそのまま、出口だけ安全"_utf8,
                             juce::dontSendNotification);

    footerHint.setText("TIP  出力上限を決める → 入力ドライブで攻める → 緑なら完成"_utf8,
                       juce::dontSendNotification);
    footerHint.setJustificationType(juce::Justification::centredRight);
    footerHint.setFont(japaneseFont(10.5f));
    footerHint.setColour(juce::Label::textColourId, quietText);

    const std::array<juce::Component*, 22> components {
        &productBadge, &title, &subtitle, &liveBadge,
        &inputTrim, &ceiling, &release,
        &inputTrimLabel, &ceilingLabel, &releaseLabel,
        &quality, &qualityLabel, &qualityHint, &autoRelease, &bypass,
        &meterSectionTitle, &inputSampleMeter, &inputTruePeakMeter,
        &outputTruePeakMeter, &gainReductionMeter, &characterWarning, &footerHint
    };
    for (auto* component : components)
        addAndMakeVisible(component);

    auto& state = processorReference.parameters;
    inputTrimAttachment = std::make_unique<SliderAttachment>(
        state, bakuon::parameters::ids::inputTrim, inputTrim);
    ceilingAttachment = std::make_unique<SliderAttachment>(
        state, bakuon::parameters::ids::ceiling, ceiling);
    releaseAttachment = std::make_unique<SliderAttachment>(
        state, bakuon::parameters::ids::release, release);
    qualityAttachment = std::make_unique<ComboBoxAttachment>(
        state, bakuon::parameters::ids::quality, quality);
    autoReleaseAttachment = std::make_unique<ButtonAttachment>(
        state, bakuon::parameters::ids::autoRelease, autoRelease);
    bypassAttachment = std::make_unique<ButtonAttachment>(
        state, bakuon::parameters::ids::bypass, bypass);

    setResizable(true, true);
    setResizeLimits(820, 600, 1200, 820);
    setSize(900, 620);
    startTimerHz(30);
}

RemixSafeMasterAudioProcessorEditor::~RemixSafeMasterAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void RemixSafeMasterAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    const auto layout = makeLayout(getLocalBounds());
    const auto editorBounds = getLocalBounds().toFloat();

    juce::ColourGradient backgroundGradient(backgroundTop,
                                            editorBounds.getTopLeft(),
                                            backgroundBottom,
                                            editorBounds.getBottomRight(),
                                            false);
    backgroundGradient.addColour(0.52, juce::Colour(0xff121127));
    graphics.setGradientFill(backgroundGradient);
    graphics.fillAll();

    drawGlow(graphics, { editorBounds.getWidth() * 0.16f, 85.0f }, 190.0f, magenta);
    drawGlow(graphics, { editorBounds.getWidth() * 0.86f, 120.0f }, 220.0f, cyan);

    graphics.setColour(primaryText.withAlpha(0.025f));
    for (int x = 0; x < getWidth(); x += 32)
        graphics.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
    for (int y = 0; y < getHeight(); y += 32)
        graphics.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));

    const auto headerBounds = layout.header.toFloat();
    juce::ColourGradient headerGradient(magenta.withAlpha(0.22f),
                                        headerBounds.getTopLeft(),
                                        cyan.withAlpha(0.12f),
                                        headerBounds.getBottomRight(),
                                        false);
    graphics.setGradientFill(headerGradient);
    graphics.fillRoundedRectangle(headerBounds, 18.0f);
    graphics.setColour(cyan.withAlpha(0.25f));
    graphics.drawRoundedRectangle(headerBounds.reduced(0.5f), 18.0f, 1.0f);

    const auto scanPosition = static_cast<float>((std::sin(animationPhase) + 1.0) * 0.5)
                            * headerBounds.getWidth();
    juce::ColourGradient scan(juce::Colours::transparentBlack,
                              headerBounds.getX() + scanPosition - 70.0f, 0.0f,
                              cyan.withAlpha(0.18f),
                              headerBounds.getX() + scanPosition, 0.0f,
                              false);
    scan.addColour(1.0, juce::Colours::transparentBlack);
    graphics.setGradientFill(scan);
    graphics.fillRoundedRectangle(headerBounds, 18.0f);

    const auto controlCards = layout.controls.toFloat();
    constexpr auto gap = 10.0f;
    const auto cardWidth = (controlCards.getWidth() - gap * 3.0f) / 4.0f;
    const std::array<juce::Colour, 4> cardColours { magenta, cyan, yellow, cyan };
    for (int index = 0; index < 4; ++index)
    {
        const auto card = juce::Rectangle<float>(
            controlCards.getX() + static_cast<float>(index) * (cardWidth + gap),
            controlCards.getY(), cardWidth, controlCards.getHeight());
        fillCard(graphics, card, cardColours[static_cast<std::size_t>(index)]);
        graphics.setColour(cardColours[static_cast<std::size_t>(index)].withAlpha(0.75f));
        graphics.fillRoundedRectangle(card.withHeight(3.0f), 2.0f);
    }

    fillCard(graphics, layout.meters.toFloat(), cyan, 14.0f);
    const std::array<juce::String, 4> names {
        "入力ピーク"_utf8, "入力 TRUE PEAK"_utf8,
        "出力 TRUE PEAK"_utf8, "安全ゲイン"_utf8
    };
    const std::array<float, 4> values {
        displayInputSample, displayInputTruePeak,
        displayOutputTruePeak, displayGainReduction
    };
    const std::array<juce::Colour, 4> colours { magenta, magenta, cyan, yellow };
    for (std::size_t index = 0; index < meterPaintBounds.size(); ++index)
        drawMeter(graphics,
                  meterPaintBounds[index],
                  names[index],
                  values[index],
                  index == 3 ? 0.0f : -60.0f,
                  index == 3 ? 12.0f : 6.0f,
                  colours[index],
                  index == 3);

    const auto statusBounds = layout.status.toFloat();
    const auto bypassed = bypass.getToggleState();
    const auto statusColour = bypassed ? danger : (heldGainReduction > 3.0f ? yellow : green);
    const auto pulse = static_cast<float>(std::sin(animationPhase) * 0.5 + 0.5);
    graphics.setColour(statusColour.withAlpha(0.10f + 0.04f * pulse));
    graphics.fillRoundedRectangle(statusBounds, 13.0f);
    graphics.setColour(statusColour.withAlpha(0.48f));
    graphics.drawRoundedRectangle(statusBounds.reduced(0.5f), 13.0f, 1.2f);

    graphics.setColour(cyan.withAlpha(0.32f));
    graphics.drawRoundedRectangle(layout.outer.toFloat().reduced(0.5f), 20.0f, 1.0f);
}

void RemixSafeMasterAudioProcessorEditor::resized()
{
    const auto layout = makeLayout(getLocalBounds());

    auto header = layout.header.reduced(20, 10);
    auto liveArea = header.removeFromRight(std::clamp(header.getWidth() / 4, 150, 210));
    liveBadge.setBounds(liveArea.reduced(6, 22));
    productBadge.setBounds(header.removeFromTop(18));
    title.setBounds(header.removeFromTop(38));
    subtitle.setBounds(header);

    const auto controls = layout.controls;
    constexpr auto gap = 10;
    const auto columnWidth = (controls.getWidth() - gap * 3) / 4;
    std::array<juce::Rectangle<int>, 4> columns;
    for (int index = 0; index < 4; ++index)
        columns[static_cast<std::size_t>(index)] = {
            controls.getX() + index * (columnWidth + gap),
            controls.getY(), columnWidth, controls.getHeight()
        };

    const auto setRotaryColumn = [](juce::Rectangle<int> area,
                                    juce::Label& label,
                                    juce::Slider& slider)
    {
        area.reduce(10, 8);
        label.setBounds(area.removeFromTop(38));
        slider.setBounds(area.reduced(2, 0));
    };
    setRotaryColumn(columns[0], inputTrimLabel, inputTrim);
    setRotaryColumn(columns[1], ceilingLabel, ceiling);
    setRotaryColumn(columns[2], releaseLabel, release);

    auto qualityArea = columns[3].reduced(13, 9);
    qualityLabel.setBounds(qualityArea.removeFromTop(38));
    qualityArea.removeFromTop(8);
    quality.setBounds(qualityArea.removeFromTop(38));
    qualityArea.removeFromTop(4);
    qualityHint.setBounds(qualityArea.removeFromTop(30));
    qualityArea.removeFromTop(std::max(2, qualityArea.getHeight() / 8));
    autoRelease.setBounds(qualityArea.removeFromTop(38));
    qualityArea.removeFromTop(8);
    bypass.setBounds(qualityArea.removeFromTop(38));

    auto meters = layout.meters.reduced(15, 8);
    meterSectionTitle.setBounds(meters.removeFromTop(22));
    const auto rowHeight = std::max(20, meters.getHeight() / 4);
    const std::array<juce::Label*, 4> meterLabels {
        &inputSampleMeter, &inputTruePeakMeter, &outputTruePeakMeter, &gainReductionMeter
    };
    for (std::size_t index = 0; index < meterPaintBounds.size(); ++index)
    {
        auto row = meters.removeFromTop(rowHeight);
        meterPaintBounds[index] = row.toFloat();
        meterLabels[index]->setBounds(row.removeFromRight(128));
    }

    auto status = layout.status.reduced(15, 0);
    characterWarning.setBounds(status.removeFromLeft(status.getWidth() * 58 / 100));
    footerHint.setBounds(status);
}

void RemixSafeMasterAudioProcessorEditor::timerCallback()
{
    const auto meters = processorReference.getMeterValues();
    displayInputSample = updatePeakDisplay(displayInputSample,
                                           meters.inputSamplePeakDb, 1.15f);
    displayInputTruePeak = updatePeakDisplay(displayInputTruePeak,
                                             meters.inputTruePeakDb, 1.0f);
    displayOutputTruePeak = updatePeakDisplay(displayOutputTruePeak,
                                              meters.outputTruePeakDb, 0.85f);
    displayGainReduction = meters.currentGainReductionDb > displayGainReduction
        ? meters.currentGainReductionDb
        : std::max(meters.currentGainReductionDb, displayGainReduction - 0.18f);

    inputSampleMeter.setText(formatDecibels(displayInputSample, " dBFS"),
                             juce::dontSendNotification);
    inputTruePeakMeter.setText(formatDecibels(displayInputTruePeak, " dBTP"),
                               juce::dontSendNotification);
    outputTruePeakMeter.setText(formatDecibels(displayOutputTruePeak, " dBTP"),
                                juce::dontSendNotification);

    if (meters.peakGainReductionDb >= heldGainReduction)
    {
        heldGainReduction = meters.peakGainReductionDb;
        peakHoldTicks = 30;
    }
    else if (peakHoldTicks > 0)
    {
        --peakHoldTicks;
    }
    else
    {
        heldGainReduction = std::max(displayGainReduction,
                                     heldGainReduction - 0.15f);
    }

        gainReductionMeter.setText(juce::String(displayGainReduction, 1)
                               + " dB  /  最大 "_utf8 + juce::String(heldGainReduction, 1) + " dB",
                               juce::dontSendNotification);

    if (bypass.getToggleState())
    {
        characterWarning.setText("●  保護を停止中 — 出力を必ず確認してください"_utf8,
                                 juce::dontSendNotification);
        characterWarning.setColour(juce::Label::textColourId, danger);
        liveBadge.setText("●  PROTECTION OFF"_utf8, juce::dontSendNotification);
        liveBadge.setColour(juce::Label::textColourId, danger);
    }
    else if (heldGainReduction > 3.0f)
    {
        characterWarning.setText("▲  変化注意 — 抑制が3 dBを超えています"_utf8,
                                 juce::dontSendNotification);
        characterWarning.setColour(juce::Label::textColourId, yellow);
        liveBadge.setText("●  HEAVY GUARD"_utf8, juce::dontSendNotification);
        liveBadge.setColour(juce::Label::textColourId, yellow);
    }
    else
    {
        characterWarning.setText("✓  保護中 — 壊れた質感はそのまま、出口だけ安全"_utf8,
                                 juce::dontSendNotification);
        characterWarning.setColour(juce::Label::textColourId, green);
        liveBadge.setText("●  LIVE PROTECTION"_utf8, juce::dontSendNotification);
        liveBadge.setColour(juce::Label::textColourId, green);
    }

    release.setEnabled(! autoRelease.getToggleState());
    release.setAlpha(release.isEnabled() ? 1.0f : 0.42f);
    animationPhase = std::fmod(animationPhase + 0.055f,
                              static_cast<float>(2.0 * std::numbers::pi));
    repaint();
}

void RemixSafeMasterAudioProcessorEditor::drawMeter(juce::Graphics& graphics,
                                                      juce::Rectangle<float> bounds,
                                                      const juce::String& name,
                                                      float value,
                                                      float minimum,
                                                      float maximum,
                                                      juce::Colour colour,
                                                      bool gainReduction) const
{
    const auto labelArea = bounds.removeFromLeft(132.0f);
    bounds.removeFromRight(132.0f);
    const auto bar = bounds.reduced(2.0f, 7.0f);

    graphics.setColour(secondaryText);
    graphics.setFont(japaneseFont(10.8f, juce::Font::bold));
    graphics.drawFittedText(name, labelArea.toNearestInt(),
                            juce::Justification::centredLeft, 1);

    graphics.setColour(backgroundTop.withAlpha(0.72f));
    graphics.fillRoundedRectangle(bar, bar.getHeight() * 0.5f);

    const auto normalised = gainReduction
        ? juce::jlimit(0.0f, 1.0f, value / maximum)
        : juce::jlimit(0.0f, 1.0f, (value - minimum) / (maximum - minimum));
    const auto filled = bar.withWidth(std::max(2.0f, bar.getWidth() * normalised));
    juce::ColourGradient meterGradient(colour.darker(0.45f),
                                       bar.getX(), 0.0f,
                                       colour, bar.getRight(), 0.0f,
                                       false);
    graphics.setGradientFill(meterGradient);
    graphics.fillRoundedRectangle(filled, bar.getHeight() * 0.5f);
    graphics.setColour(colour.withAlpha(0.20f));
    graphics.fillRoundedRectangle(filled.expanded(3.0f, 2.0f),
                                  bar.getHeight() * 0.5f + 2.0f);

    for (int tick = 1; tick < 6; ++tick)
    {
        const auto x = bar.getX() + bar.getWidth() * static_cast<float>(tick) / 6.0f;
        graphics.setColour(primaryText.withAlpha(0.10f));
        graphics.drawVerticalLine(static_cast<int>(x), bar.getY(), bar.getBottom());
    }

    if (! gainReduction)
    {
        const auto zeroPosition = juce::jlimit(0.0f, 1.0f,
                                               (0.0f - minimum) / (maximum - minimum));
        const auto zeroX = bar.getX() + bar.getWidth() * zeroPosition;
        graphics.setColour(danger.withAlpha(0.72f));
        graphics.drawVerticalLine(static_cast<int>(zeroX),
                                  bar.getY() - 2.0f,
                                  bar.getBottom() + 2.0f);
    }
}

void RemixSafeMasterAudioProcessorEditor::configureRotary(
    juce::Slider& slider,
    const juce::String& suffix,
    juce::Colour colour,
    const juce::String& accessibleName,
    const juce::String& tooltip)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f,
                               juce::MathConstants<float>::pi * 2.78f,
                               true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 104, 27);
    slider.setTextValueSuffix(suffix);
    slider.setName(accessibleName);
    slider.setTooltip(tooltip);
    slider.setColour(juce::Slider::rotarySliderFillColourId, colour);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, quietText);
    slider.setColour(juce::Slider::textBoxTextColourId, primaryText);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, backgroundTop.withAlpha(0.82f));
    slider.setColour(juce::Slider::textBoxOutlineColourId, colour.withAlpha(0.34f));
    slider.setColour(juce::Slider::textBoxHighlightColourId, colour.withAlpha(0.45f));
}

void RemixSafeMasterAudioProcessorEditor::configureControlLabel(
    juce::Label& label,
    const juce::String& titleText,
    const juce::String& step)
{
    label.setText(step + "\n" + titleText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, primaryText);
    label.setFont(japaneseFont(12.0f, juce::Font::bold));
}

void RemixSafeMasterAudioProcessorEditor::configureMeterValueLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centredRight);
    label.setFont(japaneseFont(12.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, primaryText);
}

juce::String RemixSafeMasterAudioProcessorEditor::formatDecibels(
    float value,
    const juce::String& suffix)
{
    return value <= -200.0f ? "-∞"_utf8 + suffix : juce::String(value, 1) + suffix;
}

float RemixSafeMasterAudioProcessorEditor::updatePeakDisplay(float current,
                                                              float target,
                                                              float decayPerTick) noexcept
{
    const auto safeTarget = std::isfinite(target) ? target : -72.0f;
    return safeTarget >= current ? safeTarget : std::max(safeTarget, current - decayPerTick);
}
