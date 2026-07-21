#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "EffectModule.h"

/** Owns all six vocal-chain modules and a user-reorderable processing order.
    Order is persisted alongside plugin state (it's a UI drag-reorder concept,
    not host-automatable, so it's stored as a ValueTree property, not a parameter). */
class EffectChain
{
public:
    EffectChain();

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout);
    void attachToState (juce::AudioProcessorValueTreeState& apvts);

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    /** Sum of all modules' current getLatencySamples() (0 for bypassed
        modules). PluginProcessor polls this and reports it to the host via
        setLatencySamples() whenever it changes, for delay compensation. */
    int getTotalLatencySamples() const;

    /** Module pointers in current processing order, for UI display/reordering. */
    std::vector<EffectModule*> getModulesInOrder();

    /** Moves the module currently at fromIndex (in processing order) to toIndex. */
    void moveModule (int fromIndex, int toIndex);

    void appendOrderToState (juce::ValueTree& state) const;
    void restoreOrderFromState (const juce::ValueTree& state);
    /** Resets to the default 0-1-2-… slot order. */
    void resetOrder();

    /** Returns the first module in the chain that is of type T, or nullptr. */
    template <typename T>
    T* getModuleOfType()
    {
        for (auto& m : modules)
            if (auto* t = dynamic_cast<T*> (m.get()))
                return t;
        return nullptr;
    }

    static constexpr int numModules = 10;

private:
    std::vector<std::unique_ptr<EffectModule>> modules; // fixed storage, indexed by identity
    std::vector<int> order;                              // indices into `modules`; defines signal flow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectChain)
};
