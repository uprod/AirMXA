# AirMXA

A harmonic exciter in the Aphex Aural Exciter tradition: the band above FREQ is isolated, gently saturated (DRIVE) to generate new harmonics that were not in the source, cleaned up below FREQ and blended back in small doses (MIX). MODE picks odd harmonics (symmetric tanh — edge) or even harmonics (asymmetric — silk). The saturation stage runs 2× oversampled so the harmonics you create up top do not alias. SOLO lets you hear only what is being added. FIG. 1 is a live FFT: source in ink, generated harmonics in spot.

Audio plugin (AU / VST3 / Standalone) built with [JUCE](https://juce.com). Part of the [MXA plugin suite](https://mxaudio.mescalina.fr/). macOS 11+ and Windows — Windows builds (VST3 + Standalone) are available in [Releases](https://github.com/uprod/AirMXA/releases).

## Build

```sh
git clone --recurse-submodules https://github.com/uprod/AirMXA.git
cd AirMXA
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

If you already have the MXA suite checked out with a shared `../JUCE` folder, the submodule is optional — the build falls back to the sibling folder automatically.
