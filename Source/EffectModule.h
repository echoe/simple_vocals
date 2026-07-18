#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

/** Base class for every processing block in the SimpleVocals chain.

    The enabled/bypass convention:
      - Parameter "{id}_enabled" defaults to TRUE  (module is on).
      - UI shows the toggle CHECKED when the module is active.
      - UNCHECKED (parameter value 0) means the module is bypassed.
      - This maps naturally to JUCE's ButtonAttachment:
          toggle on  → value 1 → module active
          toggle off → value 0 → module bypassed               */
class EffectModule
{
public:
    explicit EffectModule (juce::String idIn) : moduleId (std::move (idIn)) {}
    virtual ~EffectModule() = default;

    virtual void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { moduleId + "_enabled", 1 },
            getDisplayName() + " Enabled", true));
    }

    virtual void attachToState (juce::AudioProcessorValueTreeState& apvts)
    {
        enabledParam = apvts.getRawParameterValue (moduleId + "_enabled");
    }

    virtual void prepare (const juce::dsp::ProcessSpec& spec) = 0;
    virtual void process (juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;

    virtual juce::String getDisplayName() const = 0;

    const juce::String& getId() const noexcept { return moduleId; }
    /** Returns true when the module should be skipped (parameter = 0). */
    bool isBypassed() const noexcept { return enabledParam == nullptr || enabledParam->load() < 0.5f; }

protected:
    juce::String moduleId;
    std::atomic<float>* enabledParam = nullptr;
};
