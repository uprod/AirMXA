#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

namespace airmxa
{

// Exciteur harmonique, a la maniere de l'Aphex Aural Exciter (1975).
//
// Un egaliseur ne peut que remonter ce qui existe deja ; un exciteur
// FABRIQUE des aigus. La recette : on isole le haut du spectre (passe-haut
// a FREQ), on le sature legerement (DRIVE) — une saturation cree des
// harmoniques aux multiples des frequences presentes, donc plus haut
// encore — on retire ce qui n'est pas nouveau (la part de la sortie du
// shaper correlee a son entree, par projection glissante : un pur
// compresseur ne laisserait rien), et on remelange une petite dose (MIX)
// au signal intact. MODE choisit la symetrie du shaper :
// ODD (tanh, symetrique) donne les harmoniques impaires (3f, 5f : le
// tranchant), EVEN (asymetrique) les harmoniques paires (2f, 4f : le
// soyeux). L'etage de saturation est surechantillonne x2 : les harmoniques
// qu'on cree tout en haut ne doivent pas se replier en aigreur.
// SOLO fait entendre uniquement ce qu'on ajoute — l'oreille du reparateur.
class AirEngine
{
public:
    static constexpr int kScopeSize = 2048;   // FIG. 1 : fenetre de FFT

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void setFreqHz (float hz)  noexcept { freqTarget = hz; }
    void setDrive  (float d01) noexcept { driveTarget = d01; }
    void setEven   (bool e)    noexcept { even = e; }
    void setMix    (float m01) noexcept { mixTarget = m01; }
    void setSolo   (bool s)    noexcept { soloTarget = s ? 1.0f : 0.0f; }

    void process (juce::AudioBuffer<float>& buffer);

    // La loi du shaper — UNE seule source de verite, partagee avec FIG. 2.
    // Gain d'entree du shaper pour DRIVE (1 -> 12 : de l'effleurement au mordant).
    static float gainFor (float drive01) noexcept { return 1.0f + 11.0f * juce::jlimit (0.0f, 1.0f, drive01); }
    // Le shaper lui-meme. ODD : tanh, symetrique -> harmoniques impaires.
    // EVEN : on ajoute un terme en x^2 (borne par tanh) : une fonction paire
    // ne produit QUE des harmoniques paires (2f, 4f) et du continu, que le
    // bloqueur de continu retire ensuite.
    static float shape (float x, bool evenMode) noexcept
    {
        if (! evenMode)
            return std::tanh (x);
        const float t = std::tanh (x);
        return x + 0.5f * t * t;
    }

    // Verites d'affichage (FIG. 1, ~30 Hz).
    float getUiAddedDb() const noexcept { return uiAddedDb.load(); }   // niveau des harmoniques ajoutees vs source
    const std::array<float, kScopeSize>& getScopeDry() const noexcept { return scopeDry; }
    const std::array<float, kScopeSize>& getScopeAir() const noexcept { return scopeAir; }
    int getScopeWriteIndex() const noexcept { return scopeIdx.load(); }

private:
    double fs = 48000.0;
    int    numCh = 2;

    juce::dsp::LinkwitzRileyFilter<float> hpIn[2], hpOut[2];   // hpOut : les produits d'intermodulation retombes sous FREQ (et le continu) s'en vont
    juce::dsp::Oversampling<float> oversampler    { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    // La bande intacte traverse un suréchantillonneur jumeau (sans shaper) :
    // meme retard, donc alignee a l'echantillon avec la version saturee.
    juce::dsp::Oversampling<float> oversamplerDry { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::AudioBuffer<float> band, shaped;   // la bande a exciter et sa version saturee (pre-allouees)
    // Projection glissante de la sortie du shaper sur son entree : ce qui est
    // correle a x (le fondamental, comprime) est retire, il ne reste que le neuf.
    float projNum[2] = {}, projDen[2] = {};

    juce::SmoothedValue<float> drive, mix, solo;
    float freqTarget = 3000.0f, freq = 3000.0f;
    float driveTarget = 0.4f, mixTarget = 0.3f, soloTarget = 0.0f;
    bool  even = false;

    float dryE = 0.0f, airE = 0.0f;
    std::atomic<float> uiAddedDb { -60.0f };

    std::array<float, kScopeSize> scopeDry {}, scopeAir {};
    std::atomic<int> scopeIdx { 0 };
};

}
