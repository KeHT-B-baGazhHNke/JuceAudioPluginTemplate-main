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
    constexpr float postLPFFrequency = 6500.0f;
    constexpr float interstageHPFFreq = 120.0f;
    constexpr float interstageMidFreq = 600.0f;
    constexpr float interstageLPFFreq = 8000.0f;
    constexpr float toneFreq = 4000.0f;
    float defaultQ = 0.7f;
}