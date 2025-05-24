#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    gainSlider.setSliderStyle(juce::Slider::Rotary);
    toneSlider.setSliderStyle(juce::Slider::Rotary);
    volumeSlider.setSliderStyle(juce::Slider::Rotary);
    inputGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);

    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    toneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    bypassButton.setButtonText("Bypass");

    addAndMakeVisible(gainSlider);
    addAndMakeVisible(toneSlider);
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(inputGainSlider);
    addAndMakeVisible(bypassButton);
    addAndMakeVisible(loadIRButton);
    addAndMakeVisible(bypassIRToggle);

    loadIRButton.onClick = [this]()
    {
        juce::FileChooser chooser("Select an IR file", {}, "*.wav");
        if (chooser.browseForFileToOpen())
            processor.loadImpulseResponse(chooser.getResult());
    };

    bypassIRToggle.onClick = [this]()
    {
        processor.setIRBypass(bypassIRToggle.getToggleState());
    };

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "GAIN", gainSlider);
    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "TONE", toneSlider);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "VOLUME", volumeSlider);
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "INPUT_GAIN", inputGainSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processorRef.apvts, "BYPASS", bypassButton);

    setSize (500, 500);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawFittedText ("Ibanez TS9 Emulation", getLocalBounds(), juce::Justification::centredTop, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(40);
    auto row = area.removeFromTop(200);

    inputGainSlider.setBounds(row.removeFromLeft(100));
    gainSlider.setBounds(row.removeFromLeft(100));
    toneSlider.setBounds(row.removeFromLeft(100));
    volumeSlider.setBounds(row.removeFromLeft(100));
    bypassButton.setBounds(200, 300, 30, 100);
    loadIRButton.setBounds(200, 350, 80, 30);
    bypassIRToggle.setBounds(200, 450, 100, 30);
}