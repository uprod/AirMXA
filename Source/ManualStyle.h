#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace airmxa
{

// Monde visuel "Service Manual" : negatif diazo d'une page de manuel technique.
// Film sombre, encre pale, une encre spot par plugin de la famille MXA
// (Air = ciel d'altitude, l'encre des harmoniques qu'on ajoute en haut).
// Deux voix typographiques : lettrage de dessinateur (Routed Gothic) pour les
// legendes, machine a ecrire (Courier Prime) pour les chiffres.
namespace palette
{
    inline const juce::Colour film     { 0xff17140f };   // fond film sombre
    inline const juce::Colour filmHigh { 0xff231e15 };   // mouchetis clair du film
    inline const juce::Colour ink      { 0xffe6dcc2 };   // encre principale
    inline const juce::Colour inkMid   { 0xff8d826b };   // encre secondaire (annotations)
    inline const juce::Colour inkFaint { 0xff3b3527 };   // filets, grilles, traits eteints
    inline const juce::Colour spot     { 0xff8fd3ff };   // encre spot de l'Air (ciel d'altitude)
}

namespace fonts
{
    juce::Font lettering (float height);    // Routed Gothic (lettrage technique)
    juce::Font wide      (float height);    // Routed Gothic Wide (titres)
    juce::Font mono      (float height);    // Courier Prime (chiffres, tallies)
    juce::Font monoBold  (float height);
}

// Texture de film generee une fois : mouchetis + leger vignettage.
juce::Image makeFilmTexture (int width, int height);

// Dessine les cadrans "symbole schematique" de la famille MXA.
// Les zones de texte des sliders passent en Courier.
class ManualLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ManualLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
};

}
