#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "background_data.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    gainSlider.setSliderStyle(juce::Slider::Rotary);
    toneSlider.setSliderStyle(juce::Slider::Rotary);
    volumeSlider.setSliderStyle(juce::Slider::Rotary);
    inputGainSlider.setSliderStyle(juce::Slider::Rotary);

    gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    toneSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    inputGainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    bypassButton.setButtonText("Bypass Amp");
    bypassIRToggle.setButtonText("Bypass IR");

    addAndMakeVisible(gainSlider);
    addAndMakeVisible(toneSlider);
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(inputGainSlider);
    addAndMakeVisible(bypassButton);
    addAndMakeVisible(loadIRButton);
    addAndMakeVisible(bypassIRToggle);

    loadIRButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>("Select an IR file", juce::File{}, "*.wav");

        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    if (auto* proc = dynamic_cast<AudioPluginAudioProcessor*>(&processor))
                        proc->loadImpulseResponse(file);
                }
            });
    };

    bypassIRToggle.onClick = [this]
    {
         if (auto* proc = dynamic_cast<AudioPluginAudioProcessor*>(&processor))
        proc->setIRBypass(!proc->irBypassed); // переключение состояния
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
    auto image = juce::ImageCache::getFromMemory(background_png, background_png_len);
    if (image.isValid())
        g.drawImage(image, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    else
        g.fillAll(juce::Colours::black);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(40);
    auto row = area.removeFromTop(160);

    inputGainSlider.setBounds(row.removeFromLeft(105));
    gainSlider.setBounds(row.removeFromLeft(105));
    toneSlider.setBounds(row.removeFromLeft(105));
    volumeSlider.setBounds(row.removeFromLeft(105));
    bypassButton.setBounds(40, 153, 100, 30);
    loadIRButton.setBounds(210, 305, 80, 30);
    bypassIRToggle.setBounds(40, 177, 100, 30);
}