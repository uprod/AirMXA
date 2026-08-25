#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace airmxa
{

// FIG. 2 - Le chemin du signal : le direct intact en haut ; en bas le rail
// d'excitation HPF (FREQ) -> DRIVE -> SHAPER (la courbe reelle, ODD ou
// EVEN) -> HPF (intermodulation) -> x MIX, puis le sommateur et le commutateur SOLO qui
// coupe le direct. Les epaisseurs de trait SONT les gains reels.
class SchematicDiagram : public juce::Component
{
public:
    explicit SchematicDiagram (AirProcessor&);

    void paint (juce::Graphics&) override;

private:
    AirProcessor& processor;

    std::atomic<float>* freq  = nullptr;
    std::atomic<float>* drive = nullptr;
    std::atomic<float>* mode  = nullptr;
    std::atomic<float>* mix   = nullptr;
    std::atomic<float>* solo  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchematicDiagram)
};

}
