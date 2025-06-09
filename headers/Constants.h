#pragma once

namespace UIConstants {
    constexpr int textboxWidth = 0;
    constexpr int textboxHeight = 0;

    constexpr int windowWidth = 500;
    constexpr int windowHeight = 500;

    constexpr int sliderTopOffset = 40;
    constexpr int sliderRowHeight = 160;
    constexpr int sliderLeftOffset = 105;

    constexpr int buttonX = 40;
    constexpr int buttonY1 = 153;
    constexpr int buttonY2 = 177;
    constexpr int buttonWidth = 100;
    constexpr int buttonHeight = 30;
    constexpr int IrX = 210;
    constexpr int IrY = 305;
    constexpr int IrWidth = 80;
    constexpr int IrHeight = 30;
}

namespace DSPConstants {
    constexpr float inputHPFFrequency = 400.0f;
    constexpr float postLPFFrequency = 6000.0f;
    constexpr float midCutFrequency = 700.0f;
    constexpr float midCutQ = 2.0f;
    constexpr float midCutGain = -1.5f;
    constexpr float interstageHPFFreq = 120.0f;
    constexpr float interstageMidFreq = 600.0f;
    constexpr float interstageMidQ = 1.5f;
    constexpr float interstageMidGain = -2.5f;
    constexpr float interstageLPFFreq = 6000.0f;
    constexpr float LowShelfFreq = 300.0f;
    constexpr float LowShelfGain = 6.0f;
    constexpr float toneFreq = 4000.0f;
    constexpr float defaultQ = 0.7f;
    constexpr float inputMin = 1.0f;
    constexpr float inputMax = 10.0f;
    constexpr float inputDefault = 5.0f;
    constexpr float gainMin = 0.0f;
    constexpr float gainMax = 20.0;
    constexpr float gainDefault = 10.0;
    constexpr float toneMin = 0.0f;
    constexpr float toneMax = 1.0;
    constexpr float toneDefault = 0.5;
    constexpr float volumeMin = 0.0f;
    constexpr float volumeMax = 1.0;
    constexpr float volumeDefault = 0.5;
}