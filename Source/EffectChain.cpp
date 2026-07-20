#include "EffectChain.h"
#include "Modules/EQModule.h"
#include "Modules/DeEsserModule.h"
#include "Modules/CompressorModule.h"
#include "Modules/SaturationModule.h"
#include "Modules/HarmonizerModule.h"
#include "Modules/ReverbModule.h"
#include "Modules/AutotuneModule.h"
#include "Modules/DelayModule.h"

namespace
{
    constexpr const char* orderStateKey = "moduleOrder";
}

EffectChain::EffectChain()
{
    // Default signal flow: EQ -> Autotune -> De-esser -> Compressor -> Saturation
    // -> Harmonizer -> Reverb -> Delay -> EQ 2.
    // EQ 2 is a second, independent parametric EQ instance (its own band
    // parameters, prefixed "eq2_") that can be dragged to any position in the
    // chain via the chain strip — e.g. one EQ before the compressor for
    // corrective work, a second one after everything for tonal shaping.
    // At its default band gains (0 dB) it's audibly transparent, so adding
    // it doesn't change the sound of existing presets/projects.
    // Index in `modules` is each module's permanent identity; `order` is what the UI reorders.
    modules.push_back (std::make_unique<EQModule>());
    modules.push_back (std::make_unique<AutotuneModule>());
    modules.push_back (std::make_unique<DeEsserModule>());
    modules.push_back (std::make_unique<CompressorModule>());
    modules.push_back (std::make_unique<SaturationModule>());
    modules.push_back (std::make_unique<HarmonizerModule>());
    modules.push_back (std::make_unique<ReverbModule>());
    modules.push_back (std::make_unique<DelayModule>());
    modules.push_back (std::make_unique<EQModule> ("eq2", "EQ 2"));

    jassert ((int) modules.size() == numModules);

    order.resize ((size_t) numModules);
    for (int i = 0; i < numModules; ++i)
        order[(size_t) i] = i;
}

void EffectChain::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for (auto& m : modules)
        m->addParameters (layout);
}

void EffectChain::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    for (auto& m : modules)
        m->attachToState (apvts);
}

void EffectChain::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (auto& m : modules)
        m->prepare (spec);
}

void EffectChain::process (juce::AudioBuffer<float>& buffer)
{
    for (int idx : order)
    {
        auto& m = modules[(size_t) idx];
        if (! m->isBypassed())
            m->process (buffer);
    }
}

int EffectChain::getTotalLatencySamples() const
{
    int total = 0;
    for (auto& m : modules)
        total += m->getLatencySamples();
    return total;
}

void EffectChain::reset()
{
    for (auto& m : modules)
        m->reset();
}

std::vector<EffectModule*> EffectChain::getModulesInOrder()
{
    std::vector<EffectModule*> result;
    result.reserve (order.size());
    for (int idx : order)
        result.push_back (modules[(size_t) idx].get());
    return result;
}

void EffectChain::moveModule (int fromIndex, int toIndex)
{
    if (fromIndex == toIndex
        || ! juce::isPositiveAndBelow (fromIndex, (int) order.size())
        || ! juce::isPositiveAndBelow (toIndex, (int) order.size()))
        return;

    auto moved = order[(size_t) fromIndex];
    order.erase (order.begin() + fromIndex);
    order.insert (order.begin() + toIndex, moved);
}

void EffectChain::appendOrderToState (juce::ValueTree& state) const
{
    juce::String orderString;
    for (int idx : order)
        orderString << idx << " ";

    state.setProperty (orderStateKey, orderString.trim(), nullptr);
}

void EffectChain::restoreOrderFromState (const juce::ValueTree& state)
{
    if (! state.hasProperty (orderStateKey))
        return;

    auto tokens = juce::StringArray::fromTokens (state[orderStateKey].toString(), " ", "");
    if (tokens.size() != numModules)
        return;

    std::vector<int> restored;
    restored.reserve ((size_t) tokens.size());
    for (auto& t : tokens)
        restored.push_back (t.getIntValue());

    order = restored;
}

void EffectChain::resetOrder()
{
    for (int i = 0; i < numModules; ++i)
        order[(size_t) i] = i;
}
