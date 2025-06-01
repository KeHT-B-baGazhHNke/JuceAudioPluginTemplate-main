#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "background_data.h"
#include "Constants.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    gainSlider.setSliderStyle(juce::Slider::Rotary);
    toneSlider.setSliderStyle(juce::Slider::Rotary);
    volumeSlider.setSliderStyle(juce::Slider::Rotary);
    inputGainSlider.setSliderStyle(juce::Slider::Rotary);

    gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, UIConstants::textboxWidth, UIConstants::textboxHeight);
    toneSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, UIConstants::textboxWidth, UIConstants::textboxHeight);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, UIConstants::textboxWidth, UIConstants::textboxHeight);
    inputGainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, UIConstants::textboxWidth, UIConstants::textboxHeight);
    bypassButton.setButtonText("Bypass Amp");
    bypassIRToggle.setButtonText("Bypass IR");
    loadIRButton.setButtonText("Load IR");

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

    setSize (UIConstants::windowWidth, UIConstants::windowHeight);
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
    auto area = getLocalBounds().reduced(UIConstants::sliderTopOffset);
    auto row = area.removeFromTop(UIConstants::sliderRowHeight);

    inputGainSlider.setBounds(row.removeFromLeft(UIConstants::sliderLeftOffset));
    gainSlider.setBounds(row.removeFromLeft(UIConstants::sliderLeftOffset));
    toneSlider.setBounds(row.removeFromLeft(UIConstants::sliderLeftOffset));
    volumeSlider.setBounds(row.removeFromLeft(UIConstants::sliderLeftOffset));
    bypassButton.setBounds(UIConstants::buttonX, UIConstants::buttonY1, UIConstants::buttonWidth, UIConstants::buttonHeight);
    loadIRButton.setBounds(UIConstants::IrX, UIConstants::IrY, UIConstants::IrWidth, UIConstants::IrHeight);
    bypassIRToggle.setBounds(UIConstants::buttonX, UIConstants::buttonY2, UIConstants::buttonWidth, UIConstants::buttonHeight);
}