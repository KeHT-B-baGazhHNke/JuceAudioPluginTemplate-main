#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class PluginUnitTests : public juce::UnitTest
{
public:
    PluginUnitTests() : juce::UnitTest("Plugin Unit Tests") {}

    void runTest() override
    {
        testParameterHandling();
        testFilterInitialization();
        testIRBypassLogic();
        testProcessBlock();
    }

private:
    void testParameterHandling()
    {
        beginTest("AudioProcessorValueTreeState параметры");

        AudioPluginAudioProcessor processor;

        // Установка гейна
        auto* gainParam = processor.apvts.getParameter("GAIN");
        gainParam->setValueNotifyingHost(0.75f);

        auto gainVal = processor.apvts.getRawParameterValue("GAIN")->load();
        expectWithinAbsoluteError(gainVal, 0.75f, 0.0001f);

        // Проверка TONE
        auto* toneParam = processor.apvts.getParameter("TONE");
        toneParam->setValueNotifyingHost(0.5f);
        auto toneVal = processor.apvts.getRawParameterValue("TONE")->load();
        expectWithinAbsoluteError(toneVal, 0.5f, 0.0001f);
    }

    void testFilterInitialization()
    {
        beginTest("Инициализация фильтров");

        AudioPluginAudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);

        expect(processor.inputHPF.state != nullptr, "inputHPF не инициализирован");
        expect(processor.inputLPF.state != nullptr, "inputLPF не инициализирован");
        expect(processor.lowShelf.state != nullptr, "lowShelf не инициализирован");
        expect(processor.toneHighShelf.state != nullptr, "toneHighShelf не инициализирован");
        expect(processor.midCut.state != nullptr, "midCut не инициализирован");
    }

    void testIRBypassLogic()
    {
        beginTest("IR обход");

        AudioPluginAudioProcessor processor;
        processor.setIRBypass(true);
        expect(processor.isIRBypassed() == true);

        processor.setIRBypass(false);
        expect(processor.isIRBypassed() == false);
    }

    void testProcessBlock()
    {
        beginTest("processBlock");

        AudioPluginAudioProcessor processor;
        processor.prepareToPlay(44100.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        buffer.clear();

        processor.processBlock(buffer, midi);

        // Убедимся, что буфер не NaN после обработки
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample(ch, i);
                expect(!std::isnan(sample), "Сэмпл содержит NaN");
            }
        }
    }
};

static PluginUnitTests pluginTests;
