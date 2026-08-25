// Outil de capture hors-ecran pour la revue de design : instancie le
// processeur et l'editeur sans peripherique audio ni fenetre, joue un
// sinus a 4 kHz (au-dessus de FREQ = 2 kHz) et verifie la physique :
//   1. en SOLO, mode ODD : de l'energie apparait a 12 kHz (3f), presque rien a 8 kHz ;
//   2. en SOLO, mode EVEN : de l'energie apparait a 8 kHz (2f) ;
//   3. hors SOLO, le fondamental a 4 kHz traverse au niveau d'entree ;
//   4. un sinus a 1 kHz (sous FREQ) ne recoit AUCUNE harmonique.
// Puis peint l'editeur en 2x.
//   usage : AirMXASnapshot <sortie.png> [alt]
//   "alt" : valeurs non par defaut (EVEN, drive fort, mix fort, SOLO)

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../Source/PluginProcessor.h"

#include <vector>

namespace
{
    float energyAt (const std::vector<float>& x, float hz, float fs)
    {
        double s = 0.0, c = 0.0;
        for (size_t n = 0; n < x.size(); ++n)
        {
            const double ph = 2.0 * juce::MathConstants<double>::pi * hz * (double) n / fs;
            s += x[n] * std::sin (ph);
            c += x[n] * std::cos (ph);
        }
        return (float) std::sqrt ((s * s + c * c)) * 2.0f / (float) x.size();   // amplitude du partiel
    }
}

int main (int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: AirMXASnapshot <sortie.png> [alt]\n";
        return 1;
    }

    juce::ScopedJuceInitialiser_GUI juceInit;

    airmxa::AirProcessor proc;

    auto setReal = [&proc] (const juce::String& id, float real)
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (proc.getAPVTS().getParameter (id)))
            p->setValueNotifyingHost (p->convertTo0to1 (real));
    };

    const bool alt = argc > 2 && juce::String (argv[2]) == "alt";
    const float fs = 48000.0f;

    proc.prepareToPlay (fs, 512);
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    // Joue 'hz' pendant 60 blocs (les lissages se posent), mesure sur les 40 derniers.
    float phase = 0.0f;
    auto run = [&] (float hz, float amp) -> std::vector<float>
    {
        std::vector<float> out;
        for (int i = 0; i < 100; ++i)
        {
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                const float x = amp * std::sin (phase);
                phase += 2.0f * juce::MathConstants<float>::pi * hz / fs;
                if (phase > 2.0f * juce::MathConstants<float>::pi) phase -= 2.0f * juce::MathConstants<float>::pi;
                buffer.setSample (0, n, x);
                buffer.setSample (1, n, x);
            }
            proc.processBlock (buffer, midi);
            if (i >= 60)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    out.push_back (buffer.getSample (0, n));
        }
        return out;
    };

    setReal ("freq", 2000.0f); setReal ("drive", 0.8f); setReal ("mix", 0.5f);

    // 1. ODD en solo : 3f.
    setReal ("mode", 0.0f); setReal ("solo", 1.0f);
    auto y = run (4000.0f, 0.4f);
    const float odd4 = energyAt (y, 4000.0f, fs), odd8 = energyAt (y, 8000.0f, fs), odd12 = energyAt (y, 12000.0f, fs);
    std::cout << "ODD solo  : 4k " << odd4 << "  8k " << odd8 << "  12k " << odd12 << "\n";

    // 2. EVEN en solo : 2f.
    setReal ("mode", 1.0f);
    y = run (4000.0f, 0.4f);
    const float ev4 = energyAt (y, 4000.0f, fs), ev8 = energyAt (y, 8000.0f, fs), ev12 = energyAt (y, 12000.0f, fs);
    std::cout << "EVEN solo : 4k " << ev4 << "  8k " << ev8 << "  12k " << ev12 << "\n";

    // 3. Hors solo : le fondamental traverse.
    setReal ("solo", 0.0f); setReal ("mode", 0.0f);
    y = run (4000.0f, 0.4f);
    const float thru4 = energyAt (y, 4000.0f, fs);
    std::cout << "direct    : 4k " << thru4 << " (entree 0.4)\n";

    // 4. Sous FREQ : rien n'est cree.
    setReal ("solo", 1.0f);
    y = run (1000.0f, 0.4f);
    const float low3 = energyAt (y, 3000.0f, fs), low2 = energyAt (y, 2000.0f, fs);
    std::cout << "1 kHz solo: 2k " << low2 << "  3k " << low3 << " (doit rester ~0)\n";

    const bool ok = odd12 > 0.01f && odd12 > 4.0f * odd8
                 && ev8 > 0.01f && ev8 > 2.0f * ev12
                 && std::abs (thru4 - 0.4f) < 0.04f   // le fondamental traverse intact (+/-0.9 dB)
                 && low2 < 0.002f && low3 < 0.002f;
    if (! ok)
    {
        std::cerr << "ERREUR: l'exciteur ne fabrique pas les harmoniques attendues\n";
        return 4;
    }

    // --- Reglages de la capture ------------------------------------------------------
    if (alt)
    {
        setReal ("freq", 5000.0f); setReal ("drive", 0.9f); setReal ("mode", 1.0f);
        setReal ("mix", 0.7f); setReal ("solo", 1.0f);
    }
    else
    {
        setReal ("freq", 3000.0f); setReal ("drive", 0.4f); setReal ("mode", 0.0f);
        setReal ("mix", 0.3f); setReal ("solo", 0.0f);
    }

    // Un signal riche pour la feuille : une dent de scie a 220 Hz (harmoniques
    // en 1/h, donc encore du contenu au-dessus de FREQ a saturer).
    for (int i = 0; i < 80; ++i)
    {
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float x = 0.0f;
            for (int h = 1; h <= 90; ++h)
                x += std::sin ((float) h * phase) / (float) h;
            x *= 0.3f;
            phase += 2.0f * juce::MathConstants<float>::pi * 220.0f / fs;
            if (phase > 2.0f * juce::MathConstants<float>::pi) phase -= 2.0f * juce::MathConstants<float>::pi;
            buffer.setSample (0, n, x);
            buffer.setSample (1, n, x);
        }
        proc.processBlock (buffer, midi);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    if (editor == nullptr)
        return 2;

    // Deux passes de peinture : la premiere pose le lissage du spectre.
    const int w = editor->getWidth();
    const int h = editor->getHeight();
    juce::Image img (juce::Image::ARGB, w * 2, h * 2, true);
    for (int pass = 0; pass < 8; ++pass)
    {
        juce::Graphics g (img);
        g.addTransform (juce::AffineTransform::scale (2.0f));
        editor->paintEntireComponent (g, true);
    }

    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    out.deleteFile();
    juce::FileOutputStream os (out);
    if (! os.openedOk())
        return 3;

    juce::PNGImageFormat().writeImageToStream (img, os);
    std::cout << "ecrit: " << out.getFullPathName() << " (" << w * 2 << "x" << h * 2 << ")\n";
    return 0;
}
