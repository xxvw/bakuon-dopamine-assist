#include "PluginEditor.h"
#include "PluginProcessor.h"

RemixSafeMasterAudioProcessorEditor::RemixSafeMasterAudioProcessorEditor(
    RemixSafeMasterAudioProcessor& audioProcessor)
    : AudioProcessorEditor(audioProcessor)
{
    title.setText("Remix Safe Master", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    addAndMakeVisible(title);
    setSize(540, 240);
}

void RemixSafeMasterAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(20, 24, 30));
    graphics.setColour(juce::Colour::fromRGB(96, 190, 220));
    graphics.drawText("Keep the damage. Protect the output.",
                      getLocalBounds().reduced(20).withTrimmedTop(80),
                      juce::Justification::centredTop);
}

void RemixSafeMasterAudioProcessorEditor::resized()
{
    title.setBounds(getLocalBounds().removeFromTop(72).reduced(16));
}
