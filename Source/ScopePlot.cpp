#include "ScopePlot.h"
#include "ManualStyle.h"
#include "AirEngine.h"

namespace airmxa
{

namespace
{
    constexpr float kMinHz = 100.0f, kMaxHz = 20000.0f;
    constexpr float kMinDb = -84.0f, kMaxDb = 0.0f;
}

ScopePlot::ScopePlot (AirProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    freq = apvts.getRawParameterValue ("freq");
    mode = apvts.getRawParameterValue ("mode");
    solo = apvts.getRawParameterValue ("solo");

    workDry.resize ((size_t) AirEngine::kScopeSize * 2, 0.0f);
    workAir.resize ((size_t) AirEngine::kScopeSize * 2, 0.0f);
    magDry.resize ((size_t) AirEngine::kScopeSize / 2, kMinDb);
    magAir.resize ((size_t) AirEngine::kScopeSize / 2, kMinDb);

    setInterceptsMouseClicks (false, false);
}

void ScopePlot::computeSpectrum (const std::array<float, AirEngine::kScopeSize>& ring, int head,
                                 std::vector<float>& work, std::vector<float>& mag)
{
    const int N = AirEngine::kScopeSize;
    for (int i = 0; i < N; ++i)
        work[(size_t) i] = ring[(size_t) ((head + i) % N)];
    std::fill (work.begin() + N, work.end(), 0.0f);

    window.multiplyWithWindowingTable (work.data(), (size_t) N);
    fft.performFrequencyOnlyForwardTransform (work.data());

    // En dB, normalise pour qu'un sinus pleine echelle fasse ~0 dB, et lisse
    // dans le temps (montee vive, descente lente : une aiguille de vu-metre).
    const float norm = 2.0f / ((float) N * 0.5f);
    for (int i = 0; i < N / 2; ++i)
    {
        const float db = juce::Decibels::gainToDecibels (work[(size_t) i] * norm, kMinDb);
        float& m = mag[(size_t) i];
        m += (db - m) * (db > m ? 0.6f : 0.15f);
    }
}

void ScopePlot::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    auto caption = full.removeFromBottom (16.0f);
    auto box = full;

    const auto& engine = processor.getEngine();
    const float freqV = freq->load();
    const bool  evenV = mode->load() > 0.5f;
    const bool  soloV = solo->load() > 0.5f;
    const float added = engine.getUiAddedDb();

    const int head = engine.getScopeWriteIndex();
    computeSpectrum (engine.getScopeDry(), head, workDry, magDry);
    computeSpectrum (engine.getScopeAir(), head, workAir, magAir);

    auto grid = box.withTrimmedLeft (44.0f).withTrimmedRight (16.0f)
                   .withTrimmedTop (26.0f).withTrimmedBottom (22.0f);

    auto xFor = [&] (float hz)
    {
        const float t = std::log (juce::jlimit (kMinHz, kMaxHz, hz) / kMinHz) / std::log (kMaxHz / kMinHz);
        return grid.getX() + t * grid.getWidth();
    };
    auto yFor = [&] (float db)
    {
        return grid.getBottom() - (juce::jlimit (kMinDb, kMaxDb, db) - kMinDb) / (kMaxDb - kMinDb) * grid.getHeight();
    };

    // --- Grille : decades et dB ------------------------------------------------------
    g.setFont (fonts::mono (8.0f));
    for (float hz : { 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        const float x = xFor (hz);
        g.setColour (hz == 1000.0f || hz == 10000.0f ? palette::inkMid.withAlpha (0.6f) : palette::inkFaint);
        g.drawVerticalLine ((int) x, grid.getY(), grid.getBottom() + 3.0f);
        g.setColour (palette::inkMid);
        g.drawText (hz >= 1000.0f ? juce::String (hz / 1000.0f, 0) + "k" : juce::String ((int) hz),
                    juce::Rectangle<float> (30.0f, 10.0f).withCentre ({ x, grid.getBottom() + 10.0f }),
                    juce::Justification::centred);
    }
    for (float db : { 0.0f, -24.0f, -48.0f, -72.0f })
    {
        const float y = yFor (db);
        g.setColour (palette::inkFaint);
        g.drawHorizontalLine ((int) y, grid.getX(), grid.getRight());
        g.setColour (palette::inkMid);
        g.drawText (juce::String ((int) db), juce::Rectangle<float> (30.0f, 10.0f).withPosition (box.getX() + 6.0f, y - 5.0f),
                    juce::Justification::centredRight);
    }

    // --- La zone excitee : a droite de FREQ, un leger voile ---------------------------
    const float fx = xFor (freqV);
    g.setColour (palette::spot.withAlpha (0.05f));
    g.fillRect (juce::Rectangle<float> (fx, grid.getY(), grid.getRight() - fx, grid.getHeight()));
    g.setColour (palette::spot.withAlpha (0.8f));
    g.drawVerticalLine ((int) fx, grid.getY() - 6.0f, grid.getBottom());
    g.setFont (fonts::lettering (9.0f));
    g.drawText ("FREQ " + (freqV >= 1000.0f ? juce::String (freqV / 1000.0f, 1) + " kHz" : juce::String ((int) freqV)),
                juce::Rectangle<float> (90.0f, 10.0f).withPosition (fx + 4.0f, grid.getY() - 4.0f),
                juce::Justification::centredLeft);

    // --- Les deux spectres : la source en encre, l'ajout en spot ----------------------
    auto drawSpectrum = [&] (const std::vector<float>& mag, juce::Colour col, float thick, bool fill)
    {
        juce::Path p;
        const int N = AirEngine::kScopeSize;
        bool started = false;
        for (int i = 1; i < N / 2; ++i)
        {
            const float hz = (float) i * 48000.0f / (float) N;   // l'axe est relatif : 48 k suppose pour l'echelle
            if (hz < kMinHz || hz > kMaxHz) continue;
            const float x = xFor (hz), y = yFor (mag[(size_t) i]);
            if (! started) { p.startNewSubPath (x, y); started = true; }
            else p.lineTo (x, y);
        }
        if (! started) return;
        if (fill)
        {
            juce::Path f (p);
            f.lineTo (grid.getRight(), grid.getBottom());
            f.lineTo (xFor (kMinHz), grid.getBottom());
            f.closeSubPath();
            g.setColour (col.withAlpha (0.12f));
            g.fillPath (f);
        }
        g.setColour (col);
        g.strokePath (p, juce::PathStrokeType (thick, juce::PathStrokeType::curved));
    };

    drawSpectrum (magDry, soloV ? palette::inkMid : palette::ink, 1.1f, false);
    drawSpectrum (magAir, palette::spot, 1.5f, true);

    // --- Tally : ce qu'on ajoute, mesure ----------------------------------------------
    {
        g.setColour (palette::film);
        g.fillRect (juce::Rectangle<float> (box.getRight() - 262.0f, box.getY() + 4.0f, 256.0f, 16.0f));
        auto tally = juce::Rectangle<float> (252.0f, 12.0f).withPosition (box.getRight() - 258.0f, box.getY() + 6.0f);
        g.setFont (fonts::mono (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("ADDED", tally.removeFromLeft (48.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText (added <= -59.0f ? juce::String ("-") : juce::String (added, 1) + " dB", tally.removeFromLeft (70.0f), juce::Justification::centredLeft);
        g.setColour (palette::inkMid);
        g.drawText (evenV ? "EVEN 2f 4f" : "ODD 3f 5f", tally.removeFromLeft (84.0f), juce::Justification::centredLeft);
        if (soloV)
        {
            g.setColour (palette::spot);
            g.drawText ("SOLO", tally, juce::Justification::centredLeft);
        }
    }

    // Legende des deux traces.
    {
        g.setFont (fonts::lettering (9.0f));
        auto leg = juce::Rectangle<float> (200.0f, 10.0f).withPosition (grid.getX() + 4.0f, grid.getY() - 4.0f);
        g.setColour (palette::ink);
        g.drawLine (leg.getX(), leg.getCentreY(), leg.getX() + 12.0f, leg.getCentreY(), 1.1f);
        g.setColour (palette::inkMid);
        g.drawText ("SOURCE", leg.withX (leg.getX() + 16.0f), juce::Justification::centredLeft);
        g.setColour (palette::spot);
        g.drawLine (leg.getX() + 66.0f, leg.getCentreY(), leg.getX() + 78.0f, leg.getCentreY(), 1.5f);
        g.setColour (palette::inkMid);
        g.drawText ("ADDED HARMONICS", leg.withX (leg.getX() + 82.0f), juce::Justification::centredLeft);
    }

    // --- Cadre + legende de figure ---------------------------------------------
    g.setColour (palette::ink);
    g.drawRect (box, 1.0f);

    const juce::String cap = "FIG. 1 - LIVE SPECTRUM, SOURCE IN INK, GENERATED HARMONICS IN SPOT, ABOVE FREQ ONLY";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
