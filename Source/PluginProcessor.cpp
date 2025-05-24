#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    // Срез низов на входе (~720 Гц)
    *inputHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 62.0f, 2.0f);

    // Срез верхов после искажения (~1 кГц)
    *postLPF.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1000.0f, 0.f);
    // Срез середины после обработки (~700 Гц)
    *midCut.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), 700.0f, 2.0f, juce::Decibels::decibelsToGain(-3.0f));
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

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto* bypassParam = apvts.getRawParameterValue("BYPASS");
    if (*bypassParam > 0.5f)
        return; // Если включён bypass, просто пропускаем обработку
    juce::ScopedNoDenormals noDenormals;

    // Получаем параметры
    float inputGain = *apvts.getRawParameterValue("INPUT_GAIN");
    float gain      = *apvts.getRawParameterValue("GAIN");
    float tone      = *apvts.getRawParameterValue("TONE");
    float volume    = *apvts.getRawParameterValue("VOLUME");
    
    // Оборачиваем буфер в DSP-блок
    juce::dsp::AudioBlock<float> block(buffer);

    // Входной High-pass фильтр (~720 Гц)
    inputHPF.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Получаем указатель на единственный канал (моно)
    auto* data = buffer.getWritePointer(0);
    int numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float clean = data[sample] * inputGain * 3.0f;    // Входной усилитель
        float clipped     = std::tanh(clean* (gain + 5.0f));            // Клиппинг (мягкий)
        float compensated = clipped / (gain * 0.35f + 5.0f);  //Компенсация прироста громкости
        data[sample]      = compensated * volume * 3.0f;            // Применение выходной громкости
    }

    // Выходной Low-pass фильтр (~1 кГц)
    postLPF.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Регулировка Tone
    if (tone != lastToneValue)
    {
        lastToneValue = tone;

        // Настройка high-shelf фильтра на основе ручки tone
        float toneFreq = 1000.0f;  // Частота фильтра high-shelf
        float toneGainDb = juce::jmap(tone, -3.0f, 12.0f);  // амплитуда фильтра

        *toneHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(getSampleRate(), toneFreq, 0.707f, juce::Decibels::decibelsToGain(toneGainDb));
    }
    toneHighShelf.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Вырез середины
    midCut.process(juce::dsp::ProcessContextReplacing<float>(block));

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

    params.push_back(std::make_unique<juce::AudioParameterFloat>("INPUT_GAIN", "Input Gain", 1.0f, 10.0f, 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", 0.0f, 20.0f, 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TONE", "Tone", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("VOLUME", "Volume", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}