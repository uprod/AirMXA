# Product

<!-- impeccable:product-schema 1 -->

## Platform

desktop

Native JUCE audio plugin (AU/VST3/Standalone, macOS 11+, Windows via CI). Sibling of the MXA suite; the family design authority is `../PhaserMXA/DESIGN.md` and the family product context is `../PhaserMXA/PRODUCT.md`.

## Product Purpose

A harmonic exciter (Aphex Aural Exciter lineage): LR4 high-pass at FREQ → DRIVE gain → waveshaper (2× oversampled), then a sliding least-squares projection removes the part of the shaper output correlated with its input (20 ms window) so only genuinely new harmonics remain whatever the drive → LR4 high-pass at FREQ (removes intermodulation difference tones that fell below FREQ, and DC) → × MIX, summed with the untouched direct signal; SOLO mutes the direct. MODE: ODD = tanh (3f, 5f), EVEN = x + ½·tanh²(x), an even function whose generated part is purely 2f/4f. An EQ can only lift what exists; this makes new highs. Success: a stranger adds "air" to a dull vocal, flips SOLO to hear exactly what was added, and sees it on FIG. 1's spectrum above the FREQ line only.

## Capabilities and Constraints

- Exactly five parameters: `freq` (1–12 kHz log, default 3 kHz), `drive` (0–100 %, default 40 → shaper input gain `gainFor` = 1 + 11·d), `mode` (choice Odd/Even — family switch "switchMode"), `mix` (0–100 %, default 30, internally ×2 since only new harmonics remain), `solo` (choice Off/On — family switch "switchSolo").
- Engine (`AirEngine`): per-channel LR4 high-pass in and out, `juce::dsp::Oversampling` 2× (half-band polyphase IIR) around the shaper, smoothed drive/mix/solo, cutoff glided per block; 2048-sample lock-free rings of dry and added signals for FIG. 1; `uiAddedDb` = RMS(added)/RMS(dry).
- `gainFor` and `shape` are the single source of truth — FIG. 2 draws the real transfer curve from `shape`.
- Physics-checked by the snapshot tool (FREQ 2 kHz, 4 kHz sine): ODD solo → strong 12 kHz (3f) with ≥4× less at 8 kHz; EVEN solo → strong 8 kHz (2f); direct passes the 4 kHz fundamental within ±0.9 dB of input amplitude even at DRIVE 80 % (the projection removed the compressed fundamental); a 1 kHz sine (below FREQ) generates nothing at 2/3 kHz.
- Editor: Service Manual family sheet, 820×470, spot ink altitude sky #8FD3FF, DWG NO. MXA-AI-01; two family switches. FIG. 1 = live 2048-point Hann FFT, log axis 100 Hz–20 kHz, 0…−84 dB, source spectrum in ink, added harmonics in spot (filled), FREQ line with veil to the right, legend, tally ADDED dB / ODD 3f 5f · EVEN 2f 4f / SOLO. FIG. 2 = direct rail with SOLO blade ("PASS" / "SOLO - DIRECT MUTED") + excitation rail HPF (knee glyph) → DRIVE (×gain) → SHAPER ×2 OS (real curve in spot) → HPF IM CLEAN-UP → MIX-weighted rail → summing node → OUT.

## Roadmap (later phases)

- V2 candidates: a low-band "bass exciter" mode (SubMXA territory), dynamic (level-dependent) harmonic amount, a tilt/timbre control on the added band, parallel dry/wet delay compensation display.
