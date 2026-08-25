#include "AirEngine.h"

namespace airmxa
{

void AirEngine::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    fs = sampleRate;
    numCh = juce::jlimit (1, 2, numChannels);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 1 };
    for (int ch = 0; ch < 2; ++ch)
    {
        hpIn[ch].setType (juce::dsp::LinkwitzRileyFilterType::highpass);
        hpIn[ch].prepare (spec);
    }

    oversampler.reset();
    oversampler.initProcessing ((size_t) juce::jmax (1, samplesPerBlock));
    oversamplerDry.reset();
    oversamplerDry.initProcessing ((size_t) juce::jmax (1, samplesPerBlock));
    band.setSize (2, juce::jmax (1, samplesPerBlock));
    shaped.setSize (2, juce::jmax (1, samplesPerBlock));

    drive.reset (sampleRate, 0.02);
    mix.reset (sampleRate, 0.02);
    solo.reset (sampleRate, 0.01);

    reset();
}

void AirEngine::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        hpIn[ch].reset();
        hpIn[ch].setCutoffFrequency (freqTarget);
        dcState[ch] = dcPrev[ch] = 0.0f;
        projNum[ch] = 0.0f;
        projDen[ch] = 1.0e-6f;
    }
    freq = freqTarget;
    oversampler.reset();
    oversamplerDry.reset();
    drive.setCurrentAndTargetValue (driveTarget);
    mix.setCurrentAndTargetValue (mixTarget);
    solo.setCurrentAndTargetValue (soloTarget);
    dryE = airE = 0.0f;
    uiAddedDb.store (-60.0f);
    scopeDry.fill (0.0f);
    scopeAir.fill (0.0f);
    scopeIdx.store (0);
}

void AirEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSm = buffer.getNumSamples();
    const int chans = juce::jmin (numCh, buffer.getNumChannels());
    if (numSm <= 0 || chans <= 0 || numSm > band.getNumSamples())
        return;

    drive.setTargetValue (driveTarget);
    mix.setTargetValue (mixTarget);
    solo.setTargetValue (soloTarget);

    if (std::abs (freq - freqTarget) > 0.5f)
    {
        freq += 0.3f * (freqTarget - freq);
        for (int ch = 0; ch < 2; ++ch)
            hpIn[ch].setCutoffFrequency (freq);
    }

    // 1. La bande a exciter : le haut du spectre, passe-haut LR4.
    for (int ch = 0; ch < chans; ++ch)
    {
        const auto* in = buffer.getReadPointer (ch);
        auto* b = band.getWritePointer (ch);
        for (int n = 0; n < numSm; ++n)
            b[n] = hpIn[ch].processSample (0, in[n]);
    }

    // 2. Le shaper, surechantillonne x2 : DRIVE pousse la bande dans la
    //    courbe, on redescend, et on garde la bande intacte a cote.
    for (int ch = 0; ch < chans; ++ch)
        shaped.copyFrom (ch, 0, band, ch, 0, numSm);
    {
        juce::dsp::AudioBlock<float> block (shaped.getArrayOfWritePointers(), (size_t) chans, (size_t) numSm);
        auto up = oversampler.processSamplesUp (block);
        const float g = gainFor (drive.getTargetValue());
        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            auto* p = up.getChannelPointer (ch);
            for (size_t n = 0; n < up.getNumSamples(); ++n)
                p[n] = shape (p[n] * g, even) / g;
        }
        oversampler.processSamplesDown (block);

        // La bande intacte fait le meme voyage, sans shaper : meme retard.
        juce::dsp::AudioBlock<float> dryBlock (band.getArrayOfWritePointers(), (size_t) chans, (size_t) numSm);
        oversamplerDry.processSamplesUp (dryBlock);
        oversamplerDry.processSamplesDown (dryBlock);
    }

    // 3. Ne garder que le NEUF : on retire de la sortie du shaper sa part
    //    correlee a l'entree (projection <s.x>/<x.x>, fenetre 20 ms) — le
    //    fondamental comprime s'en va, les harmoniques restent, quel que
    //    soit DRIVE. Puis bloqueur de continu (le mode EVEN en produit) et
    //    melange, sans dephasage dans la bande.
    const float k    = 1.0f - std::exp (-1.0f / (0.1f * (float) fs));
    const float kp   = 1.0f - std::exp (-1.0f / (0.02f * (float) fs));
    const float dcR  = 1.0f - 2.0f * juce::MathConstants<float>::pi * 20.0f / (float) fs;
    for (int n = 0; n < numSm; ++n)
    {
        const float m = mix.getNextValue() * 2.0f;
        const float s = solo.getNextValue();
        drive.getNextValue();

        float dryMono = 0.0f, airMono = 0.0f;
        for (int ch = 0; ch < chans; ++ch)
        {
            auto* io = buffer.getWritePointer (ch);
            const float x  = band.getReadPointer (ch)[n];
            const float sh = shaped.getReadPointer (ch)[n];
            projNum[ch] += kp * (sh * x - projNum[ch]);
            projDen[ch] += kp * (x * x - projDen[ch]);
            const float c  = projDen[ch] > 1.0e-9f ? projNum[ch] / projDen[ch] : 1.0f;
            const float nw = sh - c * x;                              // le neuf seulement
            const float hp = nw - dcPrev[ch] + dcR * dcState[ch];     // y = x - x[n-1] + R y[n-1]
            dcPrev[ch] = nw;
            dcState[ch] = hp;
            const float air = hp * m;
            dryMono += io[n];
            airMono += air;
            io[n] = (1.0f - s) * io[n] + air;   // SOLO : le direct s'efface
        }
        dryMono /= (float) chans;
        airMono /= (float) chans;

        dryE += k * (dryMono * dryMono - dryE);
        airE += k * (airMono * airMono - airE);

        const int i = scopeIdx.load();
        scopeDry[(size_t) i] = dryMono;
        scopeAir[(size_t) i] = airMono;
        scopeIdx.store ((i + 1) % kScopeSize);
    }

    uiAddedDb.store (dryE > 1.0e-10f ? juce::Decibels::gainToDecibels (std::sqrt (airE / dryE), -60.0f) : -60.0f);
}

}
