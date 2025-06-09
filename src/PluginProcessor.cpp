#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Constants.h"

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::mono(), true)
                                        .withOutput("Output", juce::AudioChannelSet::mono(), true)),
       apvts (*this, nullptr, "Parameters", createParameters())
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

const juce::String AudioPluginAudioProcessor::getName() const { return JucePlugin_Name; }
bool AudioPluginAudioProcessor::acceptsMidi() const { return false; }
bool AudioPluginAudioProcessor::producesMidi() const { return false; }
bool AudioPluginAudioProcessor::isMidiEffect() const { return false; }
double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioPluginAudioProcessor::getNumPrograms() { return 1; }
int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }
void AudioPluginAudioProcessor::setCurrentProgram (int) {}
const juce::String AudioPluginAudioProcessor::getProgramName (int) { return {}; }
void AudioPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<uint32_t>(samplesPerBlock), static_cast<uint32_t>(getTotalNumInputChannels()) };

    inputHPF.prepare(spec);
    postLPF.prepare(spec);
    toneHighShelf.prepare(spec);
    midCut.prepare(spec);
    cabIR.prepare(spec);
    lowShelf.prepare(spec);


    *inputHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, DSPConstants::inputHPFFrequency, DSPConstants::defaultQ);

    *postLPF.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, DSPConstants::postLPFFrequency, DSPConstants::defaultQ);

    *midCut.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), DSPConstants::midCutFrequency, DSPConstants::midCutQ, juce::Decibels::decibelsToGain(DSPConstants::midCutGain));

    interstageHPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, DSPConstants::interstageHPFFreq, DSPConstants::defaultQ);
    interstageMid.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, DSPConstants::interstageMidFreq, DSPConstants::interstageMidQ, juce::Decibels::decibelsToGain(DSPConstants::interstageMidGain));
    interstageLPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, DSPConstants::interstageLPFFreq, DSPConstants::defaultQ);

    *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(getSampleRate(), DSPConstants::LowShelfFreq, DSPConstants::defaultQ, juce::Decibels::decibelsToGain(DSPConstants::LowShelfGain));
    
    bypassIRToggle.setButtonText("Bypass IR");
    loadIRButton.setButtonText("Load IR");
    irLoaded = false;
    irBypassed = false;
    lastToneValue = -1.0f;

}

void AudioPluginAudioProcessor::releaseResources() {}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void AudioPluginAudioProcessor::loadImpulseResponse(const juce::File& irFile)
{
    if (irFile.existsAsFile())
    {
        cabIR.loadImpulseResponse(irFile,
                                  juce::dsp::Convolution::Stereo::no,
                                  juce::dsp::Convolution::Trim::no,
                                  0);
        irLoaded = true;
        irBypassed = false;
    }
}

void AudioPluginAudioProcessor::setIRBypass(bool shouldBypass)
{
    irBypassed = shouldBypass;
}

bool AudioPluginAudioProcessor::isIRLoaded() const { return irLoaded; }
bool AudioPluginAudioProcessor::isIRBypassed() const { return irBypassed; }

namespace
{
    inline float tubeClip(float x)
    {
        float pos = std::tanh(x * 2.5f);
        float neg = std::tanh(x);
        return x >= 0.0f ? pos : neg;
    }
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto* bypassParam = apvts.getRawParameterValue("BYPASS");
    if (*bypassParam > 0.5f)
        return;

    juce::ScopedNoDenormals noDenormals;

    float inputGain = *apvts.getRawParameterValue("INPUT_GAIN");
    float gain      = *apvts.getRawParameterValue("GAIN");
    float tone      = *apvts.getRawParameterValue("TONE");
    float volume    = *apvts.getRawParameterValue("VOLUME");
    float prevSample = 0.0f;
    
    juce::dsp::AudioBlock<float> block(buffer);

    inputHPF.process(juce::dsp::ProcessContextReplacing<float>(block));

    postLPF.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Получаем указатель на единственный канал (моно)
    auto* data = buffer.getWritePointer(0);
    int numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float x = data[sample] * inputGain;

        //Transient boost
        float transient = 0.7f * (x - prevSample);
        x += transient;
        prevSample = x;

        // Первый каскад
        float stage1 = tubeClip(x * (gain * 3.0f + 1.0f));

        stage1 = interstageLPF.processSample(stage1);
        stage1 = interstageHPF.processSample(stage1);
        stage1 = interstageMid.processSample(stage1);

        // Второй каскад
        float stage2 = std::tanh(stage1 * 1.5f);

        // Удаление "гейта" на затухании
        float threshold = 0.005f;
        if (std::abs(stage2) < threshold)
            stage2 *= juce::jmap(std::abs(stage2), 0.0f, threshold, 0.5f, 1.0f);

        // Громкость  логарифмическая
        float out = stage2 * std::pow(volume, 0.5f);
        data[sample] = out;
    }


    // Регулировка Tone
    if (tone != lastToneValue)
    {
        lastToneValue = tone;

        float toneGainDb = juce::jmap(tone, 0.0f, 1.0f, -6.0f, 6.0f);  // амплитуда фильтра

        *toneHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(getSampleRate(), DSPConstants::toneFreq, DSPConstants::defaultQ, juce::Decibels::decibelsToGain(toneGainDb));
    }
    toneHighShelf.process(juce::dsp::ProcessContextReplacing<float>(block));

    midCut.process(juce::dsp::ProcessContextReplacing<float>(block));

    lowShelf.process(juce::dsp::ProcessContextReplacing<float>(block));

   if (irLoaded && !irBypassed)
       cabIR.process(juce::dsp::ProcessContextReplacing<float>(block));

}

bool AudioPluginAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);
    apvts.state.writeToStream(stream);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    if (tree.isValid())
        apvts.replaceState(tree);
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN", "Input Gain", DSPConstants::inputMin, DSPConstants::inputMax, DSPConstants::inputDefault));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", DSPConstants::gainMin, DSPConstants::gainMax, DSPConstants::gainDefault));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TONE", "Tone", DSPConstants::toneMin, DSPConstants::toneMax, DSPConstants::toneDefault));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME", "Volume", DSPConstants::volumeMin, DSPConstants::volumeMax, DSPConstants::volumeDefault));
    params.push_back(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}