#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace airmxa
{

namespace IDs
{
    constexpr auto freq  = "freq";
    constexpr auto drive = "drive";
    constexpr auto mode  = "mode";
    constexpr auto mix   = "mix";
    constexpr auto solo  = "solo";
}

juce::AudioProcessorValueTreeState::ParameterLayout AirProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto pctAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });

    const auto hzAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int)
        {
            return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
        })
        .withValueFromStringFunction ([] (const juce::String& t)
        {
            return t.containsIgnoreCase ("k") ? t.getFloatValue() * 1000.0f : t.getFloatValue();
        });

    // FREQ : au-dessus, on excite (echelle log, 1 a 12 kHz).
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::freq, 1 },
        "Freq", juce::NormalisableRange<float> (1000.0f, 12000.0f, 1.0f, 0.5f), 3000.0f, hzAttr));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::drive, 1 },
        "Drive", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f, pctAttr));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::mode, 1 }, "Mode",
        juce::StringArray { "Odd", "Even" }, 0));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::mix, 1 },
        "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f, pctAttr));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::solo, 1 }, "Solo",
        juce::StringArray { "Off", "On" }, 0));

    return { params.begin(), params.end() };
}

AirProcessor::AirProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

AirProcessor::~AirProcessor() = default;

void AirProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    pushParameterUpdatesToEngine();
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void AirProcessor::releaseResources()
{
    engine.reset();
}

bool AirProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == main;
}

void AirProcessor::pushParameterUpdatesToEngine()
{
    engine.setFreqHz (apvts.getRawParameterValue (IDs::freq)->load());
    engine.setDrive  (apvts.getRawParameterValue (IDs::drive)->load());
    engine.setEven   (apvts.getRawParameterValue (IDs::mode)->load() > 0.5f);
    engine.setMix    (apvts.getRawParameterValue (IDs::mix)->load());
    engine.setSolo   (apvts.getRawParameterValue (IDs::solo)->load() > 0.5f);
}

void AirProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    pushParameterUpdatesToEngine();
    engine.process (buffer);
}

juce::AudioProcessorEditor* AirProcessor::createEditor()
{
    return new AirEditor (*this);
}

void AirProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AirProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

}

// Point d'entree du plugin JUCE — doit etre au niveau global.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new airmxa::AirProcessor();
}
