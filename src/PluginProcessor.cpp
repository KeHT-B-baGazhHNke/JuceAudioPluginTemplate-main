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
    lowShelf.prepare(spec);

    // Срез низов на входе (~62 Гц)
    *inputHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, DSPConstants::inputHPFFrequency, DSPConstants::defaultQ);

    // Срез верхов после искажения (~1 кГц)
    *postLPF.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 6000.0f, DSPConstants::defaultQ);
    // Срез середины после обработки (~700 Гц)
    *midCut.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), 700.0f, 2.0f, juce::Decibels::decibelsToGain(-1.5f));
    // Каскадные фильтры между сатурациями (эмуляция реактивных цепей усилителя)
    interstageHPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 120.0f, DSPConstants::defaultQ);     // Удаление гула и низов
    interstageMid.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 600.0f, 1.5f, juce::Decibels::decibelsToGain(-2.5f)); // Вырез середины
    interstageLPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 6000.0f, DSPConstants::defaultQ);     // Срез высоких перед финальной сатурацией
    *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(getSampleRate(), 300.0f, DSPConstants::defaultQ, juce::Decibels::decibelsToGain(6.0f));
    
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
    // Асимметричный клиппинг
    inline float tubeClip(float x)
    {
        float pos = std::tanh(x * 2.5f);  // сильнее клип вверх
        float neg = std::tanh(x * 1.0f);  // слабее клип вниз
        return x >= 0.0f ? pos : neg;
    }

    inline  float triodeCurve(float x)
    {
        // Простейшая модель триода на основе Koren's model approximation
        const float gain = 3.0f;
        const float asym = 0.2f;

        float nonlinear = std::tanh(gain * (x + asym)) - std::tanh(gain * (x - asym));
        return 0.5f * nonlinear;
    }
}

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
    float prevSample = 0.0f;
    
    // Оборачиваем буфер в DSP-блок
    juce::dsp::AudioBlock<float> block(buffer);

    // Входной High-pass фильтр (~100 Гц)
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

        // Первый каскад сатурации (мягкий)
        float stage1 = triodeCurve(x * 2.5f);

        // Второй каскад сатурации (более агрессивный, с гейном)
        float stage2 = tubeClip(x * (gain * 3.0f + 1.0f));

        stage2 = interstageLPF.processSample(stage2);
        stage2 = interstageHPF.processSample(stage2);
        stage2 = interstageMid.processSample(stage2);

        // Дополнительная сатурация (необязательная)
        float stage3 = std::tanh(stage2 * 1.5f); // третья стадия добавляет плотность

        // Удаление "гейта" на затухании
        float threshold = 0.005f;
        if (std::abs(stage3) < threshold)
            stage3 *= juce::jmap(std::abs(stage3), 0.0f, threshold, 0.5f, 1.0f);

        // Громкость с логарифмом (отзывчивая)
        float out = stage3 * std::pow(volume, 0.5f);
        data[sample] = out;
    }


    // Регулировка Tone
    if (tone != lastToneValue)
    {
        lastToneValue = tone;

        // Настройка high-shelf фильтра на основе ручки tone
        float toneFreq = 4000.0f;  // Частота фильтра high-shelf
        float toneGainDb = juce::jmap(tone, 0.0f, 1.0f, -6.0f, 6.0f);  // амплитуда фильтра

        *toneHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(getSampleRate(), toneFreq, 0.707f, juce::Decibels::decibelsToGain(toneGainDb));
    }
    toneHighShelf.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Вырез середины
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