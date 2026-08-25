#include "SchematicDiagram.h"
#include "ManualStyle.h"
#include "AirEngine.h"

namespace airmxa
{

namespace
{
    float weightFor (float amount01)
    {
        return 0.7f + 2.4f * juce::jlimit (0.0f, 1.0f, amount01);
    }

    void drawArrowHead (juce::Graphics& g, juce::Point<float> tip, juce::Point<float> dir, float size)
    {
        dir = dir / (dir.getDistanceFromOrigin() + 1.0e-6f);
        const juce::Point<float> n (-dir.y, dir.x);
        juce::Path p;
        p.addTriangle (tip, tip - dir * size + n * (size * 0.55f),
                             tip - dir * size - n * (size * 0.55f));
        g.fillPath (p);
    }

    void drawDashedLine (juce::Graphics& g, juce::Line<float> line, float thickness)
    {
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine (line, dashes, 2, thickness);
    }

    void drawSummingNode (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (palette::film);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.1f);
        g.drawLine (c.x - r * 0.5f, c.y, c.x + r * 0.5f, c.y, 1.0f);
        g.drawLine (c.x, c.y - r * 0.5f, c.x, c.y + r * 0.5f, 1.0f);
    }

    void drawBlock (juce::Graphics& g, juce::Rectangle<float> block, const juce::String& name)
    {
        g.setColour (palette::film);
        g.fillRect (block);
        g.setColour (palette::ink);
        g.drawRect (block, 1.2f);
        g.setFont (fonts::lettering (10.0f));
        g.drawText (name, block.withHeight (13.0f), juce::Justification::centred);
    }

    void drawValue (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> under, bool live = true)
    {
        const auto font = fonts::mono (9.0f);
        const float tw  = juce::GlyphArrangement::getStringWidth (font, text);
        auto area = juce::Rectangle<float> (tw + 6.0f, 11.0f)
                        .withCentre ({ under.getCentreX(), under.getBottom() + 9.0f });
        g.setColour (palette::film);
        g.fillRect (area);
        g.setFont (font);
        g.setColour (live ? palette::ink : palette::inkMid);
        g.drawText (text, area, juce::Justification::centred);
    }
}

SchematicDiagram::SchematicDiagram (AirProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    freq  = apvts.getRawParameterValue ("freq");
    drive = apvts.getRawParameterValue ("drive");
    mode  = apvts.getRawParameterValue ("mode");
    mix   = apvts.getRawParameterValue ("mix");
    solo  = apvts.getRawParameterValue ("solo");

    setInterceptsMouseClicks (false, false);
}

void SchematicDiagram::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    auto caption  = getLocalBounds().toFloat().removeFromBottom (16.0f);

    const float freqV  = freq->load();
    const float driveV = drive->load();
    const bool  evenV  = mode->load() > 0.5f;
    const float mixV   = mix->load();
    const bool  soloV  = solo->load() > 0.5f;

    const float dryY = 20.0f;
    const float airY = 54.0f;
    const float inX  = 12.0f;
    const float tapX = 40.0f;
    const float blockW = 58.0f, blockH = 26.0f;
    const float hpX  = w * 0.14f;
    const float drvX = w * 0.32f;
    const float shpX = w * 0.46f;
    const float hp2X = w * 0.64f;
    const float mixX = w * 0.86f;
    const float outX = w - 16.0f;

    const juce::Rectangle<float> hp  (hpX,  airY - blockH * 0.5f, blockW, blockH);
    const juce::Rectangle<float> drv (drvX, airY - blockH * 0.5f, blockW, blockH);
    const juce::Rectangle<float> shp (shpX, airY - blockH * 0.5f, blockW + 32.0f, blockH);
    const juce::Rectangle<float> hp2 (hp2X, airY - blockH * 0.5f, blockW, blockH);

    // --- Entree et derivation ---------------------------------------------------------
    const float ioY = (dryY + airY) * 0.5f;
    g.setColour (palette::ink);
    g.drawEllipse (inX - 3.0f, ioY - 3.0f, 6.0f, 6.0f, 1.1f);
    g.drawLine (inX + 3.0f, ioY, tapX, ioY, 1.4f);
    g.fillEllipse (tapX - 2.2f, ioY - 2.2f, 4.4f, 4.4f);
    g.drawLine (tapX, ioY, tapX, dryY, 1.2f);
    g.drawLine (tapX, ioY, tapX, airY, 1.2f);
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("IN", juce::Rectangle<float> (24.0f, 10.0f).withPosition (inX - 8.0f, ioY - 17.0f),
                juce::Justification::centredLeft);

    // --- Le direct : intact, coupe par SOLO ----------------------------------------------
    {
        const float sx = (tapX + mixX) * 0.5f;
        g.setColour (palette::ink.withAlpha (0.9f));
        if (soloV)
        {
            drawDashedLine (g, { { tapX, dryY }, { sx - 8.0f, dryY } }, 0.7f);
            drawDashedLine (g, { { sx + 8.0f, dryY }, { mixX, dryY } }, 0.7f);
            drawDashedLine (g, { { mixX, dryY }, { mixX, ioY - 8.0f } }, 0.7f);
        }
        else
        {
            g.drawLine (tapX, dryY, sx - 8.0f, dryY, 1.4f);
            g.drawLine (sx + 8.0f, dryY, mixX, dryY, 1.4f);
            g.drawLine (mixX, dryY, mixX, ioY - 8.0f, 1.4f);
        }
        // La lame du commutateur SOLO sur le direct.
        g.setColour (palette::ink);
        g.fillEllipse (sx - 8.0f - 1.6f, dryY - 1.6f, 3.2f, 3.2f);
        g.fillEllipse (sx + 8.0f - 1.6f, dryY - 1.6f, 3.2f, 3.2f);
        if (soloV) g.drawLine (sx - 8.0f, dryY, sx + 4.0f, dryY - 9.0f, 1.6f);
        else       g.drawLine (sx - 8.0f, dryY, sx + 8.0f, dryY, 1.4f);
        g.setFont (fonts::lettering (9.0f));
        g.setColour (palette::inkMid);
        g.drawText ("DIRECT - UNTOUCHED", juce::Rectangle<float> (130.0f, 10.0f).withPosition (tapX + 10.0f, dryY - 14.0f),
                    juce::Justification::centredLeft);
        g.setFont (fonts::mono (8.0f));
        g.setColour (soloV ? palette::spot : palette::ink);
        g.drawText (soloV ? "SOLO - DIRECT MUTED" : "PASS", juce::Rectangle<float> (120.0f, 10.0f).withCentre ({ sx, dryY - 14.0f }),
                    juce::Justification::centred);
    }

    // --- Le rail d'excitation ---------------------------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (tapX, airY, hp.getX(), airY, 1.2f);
    drawArrowHead (g, { hp.getX(), airY }, { 1.0f, 0.0f }, 5.0f);
    drawBlock (g, hp, "HPF");
    {
        auto r = hp.reduced (8.0f, 3.0f).withTrimmedTop (12.0f);
        const float knee = juce::jmap (std::log (freqV / 1000.0f) / std::log (12.0f), 0.0f, 1.0f, r.getX() + 4.0f, r.getRight() - 8.0f);
        juce::Path p;
        p.startNewSubPath (knee - 10.0f, r.getBottom());
        p.quadraticTo (knee - 2.0f, r.getBottom() - 1.0f, knee, r.getY() + 1.0f);
        p.lineTo (r.getRight(), r.getY() + 1.0f);
        g.setColour (palette::inkMid);
        g.strokePath (p, juce::PathStrokeType (1.0f));
    }
    drawValue (g, freqV >= 1000.0f ? juce::String (freqV / 1000.0f, 1) + " kHz" : juce::String ((int) freqV) + " Hz", hp);

    g.setColour (palette::ink);
    g.drawLine (hp.getRight(), airY, drv.getX(), airY, 1.2f);
    drawArrowHead (g, { drv.getX(), airY }, { 1.0f, 0.0f }, 5.0f);
    drawBlock (g, drv, "DRIVE");
    {
        // Glyphe : le triangle d'amplificateur, epaisseur = gain reel.
        auto r = drv.reduced (10.0f, 3.0f).withTrimmedTop (12.0f);
        juce::Path p;
        p.addTriangle (r.getX() + 8.0f, r.getY(), r.getX() + 8.0f, r.getBottom(), r.getX() + 8.0f + r.getHeight(), r.getCentreY());
        g.setColour (palette::inkMid);
        g.strokePath (p, juce::PathStrokeType (weightFor (driveV) * 0.5f));
    }
    drawValue (g, "x " + juce::String (AirEngine::gainFor (driveV), 1), drv);

    g.setColour (palette::ink);
    g.drawLine (drv.getRight(), airY, shp.getX(), airY, weightFor (driveV) * 0.6f);
    drawArrowHead (g, { shp.getX(), airY }, { 1.0f, 0.0f }, 5.0f);
    drawBlock (g, shp, "SHAPER  x2 OS");
    {
        // Glyphe : LA courbe de transfert reelle (shape), tracee point par point.
        auto r = shp.reduced (10.0f, 3.0f).withTrimmedTop (12.0f);
        g.setColour (palette::inkFaint);
        g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());
        g.drawVerticalLine ((int) r.getCentreX(), r.getY(), r.getBottom());
        juce::Path p;
        for (int i = 0; i <= 24; ++i)
        {
            const float x01 = (float) i / 24.0f;
            const float xin = (x01 * 2.0f - 1.0f) * 2.5f;
            const float y = AirEngine::shape (xin, evenV);
            const juce::Point<float> pt (r.getX() + x01 * r.getWidth(), r.getCentreY() - y * r.getHeight() * 0.5f);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (palette::spot);
        g.strokePath (p, juce::PathStrokeType (1.2f));
    }
    drawValue (g, evenV ? "EVEN 2f 4f" : "ODD 3f 5f", shp);

    g.setColour (palette::ink);
    g.drawLine (shp.getRight(), airY, hp2.getX(), airY, 1.2f);
    drawArrowHead (g, { hp2.getX(), airY }, { 1.0f, 0.0f }, 5.0f);
    drawBlock (g, hp2, "DC BLOCK");
    {
        // Glyphe : un condensateur de liaison (deux plaques) — le continu ne passe pas.
        auto r = hp2.reduced (10.0f, 3.0f).withTrimmedTop (12.0f);
        g.setColour (palette::inkMid);
        g.drawLine (r.getX(), r.getCentreY(), r.getCentreX() - 3.0f, r.getCentreY(), 1.0f);
        g.drawLine (r.getCentreX() - 3.0f, r.getY() + 1.0f, r.getCentreX() - 3.0f, r.getBottom() - 1.0f, 1.4f);
        g.drawLine (r.getCentreX() + 3.0f, r.getY() + 1.0f, r.getCentreX() + 3.0f, r.getBottom() - 1.0f, 1.4f);
        g.drawLine (r.getCentreX() + 3.0f, r.getCentreY(), r.getRight(), r.getCentreY(), 1.0f);
    }
    drawValue (g, "20 Hz", hp2);

    // Rail des harmoniques vers le sommateur : epaisseur = MIX.
    g.setColour (palette::ink.withAlpha (0.9f));
    if (mixV < 0.005f)
    {
        drawDashedLine (g, { { hp2.getRight(), airY }, { mixX, airY } }, 0.7f);
        drawDashedLine (g, { { mixX, airY }, { mixX, ioY + 8.0f } }, 0.7f);
    }
    else
    {
        g.drawLine (hp2.getRight(), airY, mixX, airY, weightFor (mixV));
        g.drawLine (mixX, airY, mixX, ioY + 8.0f, weightFor (mixV));
    }
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("MIX " + juce::String (juce::roundToInt (mixV * 100.0f)) + " %",
                juce::Rectangle<float> (70.0f, 10.0f).withPosition (hp2.getRight() + 8.0f, airY + 5.0f),
                juce::Justification::centredLeft);

    // --- Sommateur et sortie ------------------------------------------------------------------
    drawSummingNode (g, { mixX, ioY }, 8.0f);
    g.setColour (palette::ink);
    g.drawLine (mixX + 8.0f, ioY, outX - 3.0f, ioY, 1.4f);
    g.fillEllipse (outX - 3.0f, ioY - 3.0f, 6.0f, 6.0f);
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("OUT", juce::Rectangle<float> (28.0f, 10.0f).withPosition (outX - 24.0f, ioY - 17.0f),
                juce::Justification::centredRight);

    // --- Legende de figure ----------------------------------------------------
    const juce::String cap = "FIG. 2 - SIGNAL PATH, HIGH BAND, DRIVE, REAL SHAPER CURVE, DC BLOCK, MIX, SOLO";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
