#include "AutotuneModule.h"

// ── GrainVoice ────────────────────────────────────────────────────────────────

float AutotuneModule::GrainVoice::lerp (const std::vector<float>& b, float p, int sz)
{
    int i0 = (int) std::floor (p) % sz;
    if (i0 < 0) i0 += sz;
    float frac = p - std::floor (p);
    return b[(size_t)i0]*(1.0f-frac) + b[(size_t)((i0+1)%sz)]*frac;
}

float AutotuneModule::GrainVoice::dist (float rp) const
{
    float d = writePos - rp;
    while (d <  0.0f)            d += (float)bufSize;
    while (d >= (float)bufSize)  d -= (float)bufSize;
    return d;
}

void AutotuneModule::GrainVoice::prepare (double sr, float maxGrainMs)
{
    maxGrain  = (int)(sr * maxGrainMs / 1000.0) + 1;
    grainSize = maxGrain;
    bufSize   = maxGrain * 4;
    bufL.assign ((size_t)bufSize, 0.0f);
    bufR.assign ((size_t)bufSize, 0.0f);
    resetBuffers();
}

void AutotuneModule::GrainVoice::resetBuffers()
{
    std::fill (bufL.begin(), bufL.end(), 0.0f);
    std::fill (bufR.begin(), bufR.end(), 0.0f);
    writePos = 0.0f;
    readPos1 = (float)(bufSize - grainSize);
    readPos2 = (float)(bufSize - grainSize / 2);
}

void AutotuneModule::GrainVoice::processSample (float inL, float inR,
                                                  float& outL, float& outR, float ratio)
{
    if (bufSize == 0 || grainSize == 0) { outL = outR = 0.0f; return; }

    int wi = (int)writePos % bufSize;
    bufL[(size_t)wi] = inL;
    bufR[(size_t)wi] = inR;

    auto window = [&](float d) {
        float c = juce::jlimit (0.0f, (float)grainSize, d);
        float s = std::sin (juce::MathConstants<float>::pi * c / (float)grainSize);
        return s * s;
    };
    float w1 = window (dist (readPos1)), w2 = window (dist (readPos2));
    outL = lerp (bufL, readPos1, bufSize)*w1 + lerp (bufL, readPos2, bufSize)*w2;
    outR = lerp (bufR, readPos1, bufSize)*w1 + lerp (bufR, readPos2, bufSize)*w2;

    writePos = std::fmod (writePos + 1.0f, (float)bufSize);
    auto advance = [&](float& rp) {
        rp = std::fmod (rp + ratio, (float)bufSize);
        if (rp < 0.0f) rp += (float)bufSize;
    };
    advance (readPos1); advance (readPos2);

    auto maybeReset = [&](float& rp) {
        float d = dist (rp);
        if (d > (float)grainSize * 2.0f)
            rp = std::fmod (writePos - (float)grainSize + (float)bufSize*2.0f, (float)bufSize);
        else if (d > (float)grainSize * 1.15f)
            rp = writePos;
    };
    maybeReset (readPos1); maybeReset (readPos2);
}

// ── Parameters ────────────────────────────────────────────────────────────────

void AutotuneModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    EffectModule::addParameters (layout);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "auto_speed",  1 }, "Speed",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "auto_amount", 1 }, "Amount",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "auto_mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "auto_formant", 1 }, "Formant",
        juce::NormalisableRange<float> (-5.0f, 5.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "auto_character", 1 }, "Character",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "auto_key",   1 }, "Key",
        juce::StringArray { "C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "auto_latency_mode", 1 }, "Latency Mode",
        juce::StringArray { "Live", "Studio" }, 1));   // default Studio: matches the original fixed behaviour

    // 12 individual note toggles — the actual scale the corrector targets.
    // Default: all on (= chromatic, snap to nearest semitone).
    for (int n = 0; n < 12; ++n)
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "auto_note_" + juce::String (n), 1 },
            "Note " + juce::String (n), true));
}

void AutotuneModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    EffectModule::attachToState (apvts);
    speedParam     = apvts.getRawParameterValue ("auto_speed");
    amountParam    = apvts.getRawParameterValue ("auto_amount");
    mixParam       = apvts.getRawParameterValue ("auto_mix");
    formantParam   = apvts.getRawParameterValue ("auto_formant");
    characterParam = apvts.getRawParameterValue ("auto_character");
    keyParam       = apvts.getRawParameterValue ("auto_key");
    latencyModeParam = apvts.getRawParameterValue ("auto_latency_mode");
    for (int n = 0; n < 12; ++n)
        noteParams[(size_t) n] = apvts.getRawParameterValue ("auto_note_" + juce::String (n));
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void AutotuneModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    analysisBuf.assign (kAnalysisN, 0.0f);
    analysisWriteIdx = hopCounter = 0;
    smoothedCorrSt   = 0.0f;
    corrVoiceLive.prepare   (sampleRate, kLiveGrainMs);
    corrVoiceStudio.prepare (sampleRate, kStudioGrainMs);

    juce::dsp::ProcessSpec mono { spec.sampleRate, spec.maximumBlockSize, 1 };
    auto flat = FilterCoefs::makeLowShelf  (sampleRate, 800.0f, 0.707f, 1.0f);
    auto flat2= FilterCoefs::makeHighShelf (sampleRate, 800.0f, 0.707f, 1.0f);
    formantLowL.coefficients  = flat;  formantLowR.coefficients  = flat;
    formantHighL.coefficients = flat2; formantHighR.coefficients = flat2;
    formantLowL.prepare (mono);  formantLowR.prepare (mono);
    formantHighL.prepare (mono); formantHighR.prepare (mono);
}

void AutotuneModule::reset()
{
    std::fill (analysisBuf.begin(), analysisBuf.end(), 0.0f);
    analysisWriteIdx = hopCounter = 0;
    smoothedCorrSt = 0.0f;
    corrVoiceLive.resetBuffers();
    corrVoiceStudio.resetBuffers();
    formantLowL.reset();  formantLowR.reset();
    formantHighL.reset(); formantHighR.reset();
    detectedHz.store (0.0f); targetHz.store (0.0f);
    correctionSt.store (0.0f); confidence.store (0.0f);
}

// ── Pitch detection ───────────────────────────────────────────────────────────

void AutotuneModule::runPitchDetection()
{
    std::vector<float> win (kAnalysisN);
    for (int i = 0; i < kAnalysisN; ++i)
        win[(size_t)i] = analysisBuf[(size_t)((analysisWriteIdx + i) % kAnalysisN)];

    float rms = 0.0f;
    for (float s : win) rms += s * s;
    rms = std::sqrt (rms / (float)kAnalysisN);
    if (rms < 0.005f) { detectedHz.store(0.0f); confidence.store(0.0f); return; }
    for (float& s : win) s /= (rms + 1e-6f);

    float r0 = 0.0f;
    for (float s : win) r0 += s * s;

    int lagMin = juce::jmax (1, (int)(sampleRate / 1050.0));
    int lagMax = juce::jmin (kAnalysisN / 2, (int)(sampleRate / 65.0));

    float bestCorr = 0.0f;
    int   bestLag  = lagMin;
    for (int lag = lagMin; lag <= lagMax; ++lag)
    {
        float r = 0.0f, rx = 0.0f;
        int   n = kAnalysisN - lag;
        for (int i = 0; i < n; ++i)
        {
            r  += win[(size_t)i] * win[(size_t)(i+lag)];
            rx += win[(size_t)(i+lag)] * win[(size_t)(i+lag)];
        }
        float norm = r / (std::sqrt (r0 * rx) + 1e-6f);
        if (norm > bestCorr) { bestCorr = norm; bestLag = lag; }
    }

    if (bestCorr > 0.45f)
    {
        float y0 = 0.0f, y2 = 0.0f;
        if (bestLag > lagMin && bestLag < lagMax)
        {
            int n0 = kAnalysisN - (bestLag-1), n2 = kAnalysisN - (bestLag+1);
            float r0a=0,r2a=0,rx0=0,rx2=0;
            for (int i = 0; i < juce::jmin(n0,n2); ++i)
            {
                r0a += win[(size_t)i]*win[(size_t)(i+bestLag-1)];
                r2a += win[(size_t)i]*win[(size_t)(i+bestLag+1)];
                rx0 += win[(size_t)(i+bestLag-1)]*win[(size_t)(i+bestLag-1)];
                rx2 += win[(size_t)(i+bestLag+1)]*win[(size_t)(i+bestLag+1)];
            }
            y0 = r0a / (std::sqrt(r0*rx0)+1e-6f);
            y2 = r2a / (std::sqrt(r0*rx2)+1e-6f);
        }
        float denom = 2.0f*(2.0f*bestCorr - y0 - y2);
        float refined = (std::abs(denom) > 1e-6f)
                        ? (float)bestLag + (y2-y0)/denom : (float)bestLag;
        detectedHz.store ((float)sampleRate / refined, std::memory_order_relaxed);
        confidence.store (bestCorr,                    std::memory_order_relaxed);
    }
    else
    {
        detectedHz.store (0.0f, std::memory_order_relaxed);
        confidence.store (0.0f, std::memory_order_relaxed);
    }
}

// ── Quantiser ─────────────────────────────────────────────────────────────────

float AutotuneModule::quantisePitch (float hz) const
{
    if (hz < 20.0f) return hz;
    float midi   = 69.0f + 12.0f * std::log2 (hz / 440.0f);
    int   semOct = ((int) std::round (midi) % 12 + 12) % 12;

    int   best = -1;
    float bestDist = 99.0f;
    for (int n = 0; n < 12; ++n)
    {
        if (noteParams[(size_t)n] == nullptr || noteParams[(size_t)n]->load() < 0.5f)
            continue;
        float dist = std::abs ((float)(semOct - n));
        if (dist > 6.0f) dist = 12.0f - dist;
        if (dist < bestDist) { bestDist = dist; best = n; }
    }

    if (best < 0) return hz;   // all notes disabled

    float tgt = std::floor (midi / 12.0f) * 12.0f + (float) best;
    while (tgt - midi >  6.0f) tgt -= 12.0f;
    while (tgt - midi < -6.0f) tgt += 12.0f;
    return 440.0f * std::pow (2.0f, (tgt - 69.0f) / 12.0f);
}

// ── Process ───────────────────────────────────────────────────────────────────

void AutotuneModule::process (juce::AudioBuffer<float>& buffer)
{
    auto speed     = speedParam     ? speedParam->load()     : 0.15f;
    auto amount    = amountParam    ? amountParam->load()    : 1.0f;
    auto character = characterParam ? characterParam->load() : 0.25f;

    auto numSamples  = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Character=0 (Natural): Speed param controls retune fully, up to ~500 ms.
    // Character=1 (Hard Tune): pitch snaps instantly, sample-to-sample, with
    // zero glide between notes — no exponential smoothing at all. This is
    // the classic "T-Pain" hard-autotune behaviour: every note is corrected
    // the instant it's detected, producing audible discrete pitch steps
    // instead of a slide between notes.
    bool  hardSnap    = character >= 0.995f;
    float retuneSecs  = (0.001f + speed * 0.499f) * (1.0f - character * 0.99f);
    retuneSecs = juce::jmax (0.001f, retuneSecs);
    float smoothCoeff = hardSnap ? 1.0f
                                  : (1.0f - std::exp (-1.0f / (float)(sampleRate * (double)retuneSecs)));

    // Dead zone: natural character lets small pitch variations (vibrato, slides)
    // pass through without correction. Robotic snaps everything to grid.
    // 0.15 semitones ≈ 15 cents — a musically meaningful threshold.
    float deadZoneSt = 0.15f * (1.0f - character);

    // Latency mode: switch which grain voice is active. On a switch, reset
    // the newly-active voice so it doesn't play back stale/silent buffer
    // content — this trades a brief (one grain length) dip for avoiding a
    // garbled splice.
    int modeIdx = latencyModeParam ? (int) std::round (latencyModeParam->load()) : 1;
    modeIdx = juce::jlimit (0, 1, modeIdx);
    if (modeIdx != currentLatencyMode)
    {
        currentLatencyMode = modeIdx;
        (currentLatencyMode == 0 ? corrVoiceLive : corrVoiceStudio).resetBuffers();
    }
    GrainVoice& activeVoice = (currentLatencyMode == 0) ? corrVoiceLive : corrVoiceStudio;

    for (int s = 0; s < numSamples; ++s)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) mono += buffer.getSample(ch, s);
        if (numChannels > 1) mono *= 0.5f;
        analysisBuf[(size_t)(analysisWriteIdx++ % kAnalysisN)] = mono;
        if (++hopCounter >= kHopSize) { hopCounter = 0; runPitchDetection(); }
    }

    float detHz = detectedHz.load (std::memory_order_relaxed);
    float tgtHz = detHz > 20.0f ? quantisePitch (detHz) : detHz;
    targetHz.store (tgtHz, std::memory_order_relaxed);

    float rawCorrSt = (detHz > 20.0f && tgtHz > 20.0f)
                      ? 12.0f * std::log2 (tgtHz / detHz) : 0.0f;

    // Soft dead zone: ramp correction in from the dead-zone edge so there's
    // no sudden jump as pitch crosses the threshold.
    float targetCorrSt = 0.0f;
    float absRaw = std::abs (rawCorrSt);
    if (absRaw > deadZoneSt)
    {
        // Linear ramp over one extra dead-zone width for smoothness.
        float ramp = (deadZoneSt > 0.001f)
                     ? juce::jmin (1.0f, (absRaw - deadZoneSt) / deadZoneSt)
                     : 1.0f;
        targetCorrSt = rawCorrSt * ramp;
    }

    for (int s = 0; s < numSamples; ++s)
    {
        smoothedCorrSt += (targetCorrSt - smoothedCorrSt) * smoothCoeff;
        correctionSt.store (smoothedCorrSt, std::memory_order_relaxed);

        float ratio = std::pow (2.0f, smoothedCorrSt * amount / 12.0f);
        float inL = numChannels >= 1 ? buffer.getSample(0, s) : 0.0f;
        float inR = numChannels >= 2 ? buffer.getSample(1, s) : inL;
        float corrL, corrR;
        activeVoice.processSample (inL, inR, corrL, corrR, ratio);
        float mix = mixParam ? mixParam->load() : 1.0f;
        if (numChannels >= 1) buffer.setSample(0, s, inL + (corrL - inL) * mix);
        if (numChannels >= 2) buffer.setSample(1, s, inR + (corrR - inR) * mix);
    }

    // ── Formant tilt (applied post-mix to the full output) ───────────────
    float formant = formantParam ? formantParam->load() : 0.0f;
    if (std::abs (formant) > 0.05f)
    {
        // Each semitone of formant shift ≈ 2 dB of tilt around 800 Hz.
        // Positive formant → boost highs / cut lows  (brighter, "smaller" voice)
        // Negative formant → boost lows  / cut highs (darker,   "larger"  voice)
        float tiltDb   = formant * 2.0f;
        auto  lowCoefs = FilterCoefs::makeLowShelf  (sampleRate, 800.0f, 0.707f,
                             juce::Decibels::decibelsToGain (-tiltDb));
        auto  hiCoefs  = FilterCoefs::makeHighShelf (sampleRate, 800.0f, 0.707f,
                             juce::Decibels::decibelsToGain (+tiltDb));

        formantLowL.coefficients  = lowCoefs; formantLowR.coefficients  = lowCoefs;
        formantHighL.coefficients = hiCoefs;  formantHighR.coefficients = hiCoefs;

        if (numChannels >= 1)
        {
            auto* ch = buffer.getWritePointer (0);
            for (int s = 0; s < numSamples; ++s)
                ch[s] = formantHighL.processSample (formantLowL.processSample (ch[s]));
        }
        if (numChannels >= 2)
        {
            auto* ch = buffer.getWritePointer (1);
            for (int s = 0; s < numSamples; ++s)
                ch[s] = formantHighR.processSample (formantLowR.processSample (ch[s]));
        }
    }
}
