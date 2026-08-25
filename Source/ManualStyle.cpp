#include "ManualStyle.h"
#include "BinaryData.h"

namespace airmxa
{

//==============================================================================
// Polices embarquees (voir Assets/LICENSES.md)

static juce::Font withTypeface (const juce::Typeface::Ptr& tf, float height)
{
    return juce::Font (juce::FontOptions (tf).withHeight (height));
}

namespace fonts
{
    juce::Font lettering (float h)
    {
        static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (
            BinaryData::RoutedGothicRegular_ttf, (size_t) BinaryData::RoutedGothicRegular_ttfSize);
        return withTypeface (tf, h);
    }

    juce::Font wide (float h)
    {
        static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (
            BinaryData::RoutedGothicWideRegular_ttf, (size_t) BinaryData::RoutedGothicWideRegular_ttfSize);
        return withTypeface (tf, h);
    }

    juce::Font mono (float h)
    {
        static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (
            BinaryData::CourierPrimeRegular_ttf, (size_t) BinaryData::CourierPrimeRegular_ttfSize);
        return withTypeface (tf, h);
    }

    juce::Font monoBold (float h)
    {
        static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (
            BinaryData::CourierPrimeBold_ttf, (size_t) BinaryData::CourierPrimeBold_ttfSize);
        return withTypeface (tf, h);
    }
}

//==============================================================================
// Texture film : mouchetis deterministe + vignettage discret.

juce::Image makeFilmTexture (int width, int height)
{
    juce::Image img (juce::Image::ARGB, juce::jmax (1, width), juce::jmax (1, height), false);

    {
        juce::Graphics g (img);
        g.fillAll (palette::film);
    }

    juce::Random rng (0x4d5841);   // "MXA" : meme grain a chaque ouverture

    juce::Image::BitmapData bits (img, juce::Image::BitmapData::writeOnly);
    for (int i = 0; i < width * height / 22; ++i)
    {
        const int px = rng.nextInt (width);
        const int py = rng.nextInt (height);
        const float a = 0.03f + rng.nextFloat() * 0.10f;
        bits.setPixelColour (px, py, palette::filmHigh.interpolatedWith (palette::film, 1.0f - a * 6.0f));
    }
    // Quelques poussieres d'encre, plus rares.
    for (int i = 0; i < width * height / 900; ++i)
    {
        const int px = rng.nextInt (width);
        const int py = rng.nextInt (height);
        bits.setPixelColour (px, py, palette::inkFaint.withAlpha (0.5f));
    }

    {
        juce::Graphics g (img);
        juce::ColourGradient vig (juce::Colours::transparentBlack,
                                  (float) width * 0.5f, (float) height * 0.5f,
                                  juce::Colours::black.withAlpha (0.22f),
                                  0.0f, 0.0f, true);
        vig.addColour (0.72, juce::Colours::transparentBlack);
        g.setGradientFill (vig);
        g.fillRect (0, 0, width, height);
    }

    return img;
}

//==============================================================================
// LookAndFeel

ManualLookAndFeel::ManualLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,       palette::ink);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,  palette::spot.withAlpha (0.35f));

    setColour (juce::Label::textColourId,               palette::ink);

    setColour (juce::TextEditor::textColourId,            palette::ink);
    setColour (juce::TextEditor::backgroundColourId,      palette::filmHigh);
    setColour (juce::TextEditor::outlineColourId,         palette::inkFaint);
    setColour (juce::TextEditor::focusedOutlineColourId,  palette::spot);
    setColour (juce::TextEditor::highlightColourId,       palette::spot.withAlpha (0.35f));
    setColour (juce::TextEditor::highlightedTextColourId, palette::ink);
    setColour (juce::CaretComponent::caretColourId,       palette::spot);
}

juce::Label* ManualLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox (s);
    label->setFont (fonts::mono (12.0f));
    label->setJustificationType (juce::Justification::centred);

    // Saisie au double-clic seulement : au simple clic, le chiffre ouvrirait un
    // editeur de texte et avalerait le geste destine au cadran juste au-dessus.
    label->setEditable (false, true, false);
    return label;
}

void ManualLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float sliderPos, float a0, float a1, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const bool  active   = s.isMouseOverOrDragging();
    const bool  focused  = s.hasKeyboardFocus (true);
    const bool  enabled  = s.isEnabled();
    const float angle    = a0 + sliderPos * (a1 - a0);

    if (s.getComponentID().startsWith ("switch"))
    {
        // Commutateurs rotatifs : MODE (ODD, EVEN) ou SOLO (OFF, ON) selon le
        // ComponentID. Rayon plus court que sur les cadrans : les etiquettes du haut
        // doivent rester dans le cadre.
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 20.0f;
        static const char* modeNames[] = { "ODD", "EVEN" };
        static const char* soloNames[] = { "OFF", "ON" };
        const char** names = s.getComponentID() == "switchMode" ? modeNames : soloNames;
        const int numPos   = 2;
        const int selected = juce::roundToInt (sliderPos * (float) (numPos - 1));

        for (int i = 0; i < numPos; ++i)
        {
            const float ang = a0 + (float) i / (float) (numPos - 1) * (a1 - a0);
            const auto  dir = juce::Point<float> (std::sin (ang), -std::cos (ang));

            g.setColour (i == selected ? palette::ink : palette::inkMid);
            g.drawLine ({ juce::Point<float> (cx, cy) + dir * (r + 1.0f),
                          juce::Point<float> (cx, cy) + dir * (r + 5.0f) },
                        i == selected ? 1.6f : 1.0f);

            const auto tp = juce::Point<float> (cx, cy) + dir * (r + 13.0f);
            g.setFont (i == selected ? fonts::monoBold (11.0f) : fonts::mono (10.0f));
            g.drawText (names[i],
                        juce::Rectangle<float> (36.0f, 12.0f).withCentre (tp),
                        juce::Justification::centred);
        }

        g.setColour (active || focused ? palette::ink : palette::inkMid);
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);

        // Lame plate du commutateur.
        const auto dir = juce::Point<float> (std::sin (angle), -std::cos (angle));
        g.setColour (palette::spot);
        g.drawLine ({ juce::Point<float> (cx, cy) - dir * (r * 0.30f),
                      juce::Point<float> (cx, cy) + dir * (r * 0.82f) },
                    active ? 4.5f : 3.5f);
        g.setColour (palette::ink);
        g.fillEllipse (cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
        return;
    }

    {
        // Cadran "symbole schematique" : couronne de graduations gravees,
        // arc de course en encre spot, aiguille simple.
        const float rOuter = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 3.0f;

        for (int i = 0; i <= 20; ++i)
        {
            const bool  major = (i % 5 == 0);
            const float ang   = a0 + (float) i / 20.0f * (a1 - a0);
            const auto  dir   = juce::Point<float> (std::sin (ang), -std::cos (ang));
            const float len   = major ? 5.5f : 3.0f;

            auto col = major ? palette::inkMid : palette::inkFaint;
            if (active || focused)
                col = col.interpolatedWith (palette::ink, 0.35f);

            g.setColour (col);
            g.drawLine ({ juce::Point<float> (cx, cy) + dir * (rOuter - len),
                          juce::Point<float> (cx, cy) + dir * rOuter },
                        major ? 1.2f : 0.8f);
        }

        // Arc de course parcourue. Un cadran desactive (TIME en mode SYNC)
        // perd son arc et passe en aiguille filiforme : la geometrie dit
        // "hors service", jamais la couleur seule.
        if (enabled)
        {
            juce::Path travel;
            const float rArc = rOuter - 8.5f;
            travel.addCentredArc (cx, cy, rArc, rArc, 0.0f, a0, angle, true);
            g.setColour (palette::spot);
            g.strokePath (travel, juce::PathStrokeType (active ? 2.0f : 1.4f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::butt));
        }

        // Corps du cadran + aiguille.
        const float rBody = rOuter - 12.0f;
        g.setColour (active || focused ? palette::ink : palette::inkMid);
        g.drawEllipse (cx - rBody, cy - rBody, rBody * 2.0f, rBody * 2.0f, 1.1f);

        const auto dir = juce::Point<float> (std::sin (angle), -std::cos (angle));
        g.setColour (enabled ? palette::spot : palette::inkMid);
        g.drawLine ({ juce::Point<float> (cx, cy),
                      juce::Point<float> (cx, cy) + dir * (rBody - 1.5f) },
                    ! enabled ? 1.0f : (active ? 3.0f : 2.2f));

        g.setColour (palette::ink);
        g.fillEllipse (cx - 1.8f, cy - 1.8f, 3.6f, 3.6f);

        if (focused)
        {
            // Anneau pointille : focus clavier (epaisseur + motif, pas couleur seule).
            juce::Path ring, dashed;
            ring.addEllipse (cx - rOuter - 2.0f, cy - rOuter - 2.0f,
                             (rOuter + 2.0f) * 2.0f, (rOuter + 2.0f) * 2.0f);
            const float dashes[] = { 2.5f, 3.5f };
            juce::PathStrokeType (1.0f).createDashedStroke (dashed, ring, dashes, 2);
            g.setColour (palette::spot);
            g.fillPath (dashed);
        }
    }
}

}
