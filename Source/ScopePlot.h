#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

namespace airmxa
{

// FIG. 1 - Le spectre REEL : une FFT de 2048 points sur la source (encre)
// et sur les harmoniques ajoutees (spot), sur un axe de frequences
// logarithmique de 100 Hz a 20 kHz. La ligne verticale est FREQ : a
// gauche rien n'est touche, a droite l'exciteur travaille — et ce qu'il
// cree apparait plus haut encore que ce qui entre. Repaint ~30 Hz.
class ScopePlot : public juce::Component
{
public:
    explicit ScopePlot (AirProcessor&);

    void paint (juce::Graphics&) override;

private:
    AirProcessor& processor;

    std::atomic<float>* freq = nullptr;
    std::atomic<float>* mode = nullptr;
    std::atomic<float>* solo = nullptr;

    juce::dsp::FFT fft { 11 };   // 2048 points
    juce::dsp::WindowingFunction<float> window { 2048, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> workDry, workAir, magDry, magAir;   // lisses pour l'oeil

    void computeSpectrum (const std::array<float, AirEngine::kScopeSize>& ring, int head,
                          std::vector<float>& work, std::vector<float>& mag);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopePlot)
};

}
