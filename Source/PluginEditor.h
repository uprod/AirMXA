#pragma once

/*  AirMXA — feuille "Service Manual" de la famille MXA.
    Encre spot ciel d'altitude. FIG. 1 = le spectre reel (FFT) : la source
    en encre, les harmoniques ajoutees en spot, la frequence de coupure ;
    FIG. 2 = le chemin du signal (passe-haut, drive, shaper, passe-haut,
    melange, solo) ; cinq commandes (FREQ, DRIVE, MODE, MIX, SOLO) — deux
    commutateurs sur une feuille.
    Le gabarit de la feuille (cadre, cartouche, figures, cadrans) suit
    ../PhaserMXA/DESIGN.md, l'autorite de design de la famille.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ManualStyle.h"
#include "ScopePlot.h"
#include "SchematicDiagram.h"

namespace airmxa
{

class AirEditor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    explicit AirEditor (AirProcessor& proc);
    ~AirEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    using APVTS   = juce::AudioProcessorValueTreeState;
    using SAttach = APVTS::SliderAttachment;

    struct Dial
    {
        juce::Slider slider;
        juce::Label  name;
        std::unique_ptr<SAttach> attachment;
    };

    void setupDial (Dial& d, const juce::String& labelText, const juce::String& paramID);
    void setupSwitch (Dial& d, const juce::String& labelText, const juce::String& paramID,
                      const juce::String& componentID);
    void timerCallback() override;

    void drawSheetFrame (juce::Graphics& g);
    void drawHeader (juce::Graphics& g);

    AirProcessor& airProcessor;

    ManualLookAndFeel lookAndFeel;
    juce::Image       filmTexture;

    ScopePlot        plot;
    SchematicDiagram schematic;

    Dial freqDial, driveDial, modeSwitch, mixDial, soloSwitch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirEditor)
};

}
