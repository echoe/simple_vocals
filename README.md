SimpleVocals
============

<img src=https://raw.githubusercontent.com/echoe/simple_vocals/refs/heads/main/simplevocals.png width="600" height="300" />

## Human Area
- Wow, I wrote this little part before the overview, and took the picture! Wow. Anyways, this is a plugin made mostly with an LLM, providing a semi-opinionated, single-screen series of plugins that you can use in order to make your vocals sound better. A human wrote this little part before the main readme.
## How to build
- I always forget how to do this ... so it's back in the readme for now.
### Windows
- I installed CMake and Git from the respective websites (using the windows executables) and then installed Visual Studio Community Edition: "Desktop development with C++" workload, and then the build went without issues.
### Linux
- Just install development tools, CMAKE dependencies, and then a whole bundle of tools for JUCE ...
- sudo dnf group install development-tools
- sudo dnf install cmake gcc-c++ git alsa-lib-devel freetype-devel fontconfig-devel libX11-devel libXinerama-devel libXext-devel libXrandr-devel
- sudo dnf install libXcursor-devel libXcomposite-devel gtk3-devel webkit2gtk4.1-devel freetype-devel curl-devel
### MacOS
- xcode-select --install
- brew install cmake
- (everything else is already there by default)

This is a JUCE-based vocal channel-strip plugin (VST3 / Standalone) for macOS and Linux, built around a reorderable chain of ten (!) processing modules. Made chiefly with an LLM.
The LLM readme is below.

OVERVIEW
--------
SimpleVocals gives you a complete vocal processing chain in a single
plugin: two independent EQs, Autotune, Pitch/Formant, De-Esser,
Compressor, Saturation, Reverb, Delay, and Denoise. Every module can
be individually enabled/disabled, and the order of the chain can be
freely rearranged to suit your signal flow. A built-in preset browser
ships with 50 factory presets covering everything from clean
broadcast voice to heavy creative effects.

Module panels are laid out as a 2-wide x 3-tall grid, with every
control on a single row so each panel stays short. Denoise and
Pitch/Formant don't need a visualiser the way the others do, so
they're each half height and share one grid cell, stacked on top of
each other.


MODULES
-------
1. EQ (x2)
   Two independent, fully parametric EQ instances ("EQ 1" and "EQ 2"),
   switchable via tabs above the curve display. Each has up to 8 bell
   bands with its own frequency, gain, Q, and an optional "dynamic"
   (compressing) mode with its own threshold and ratio. Because each EQ
   has its own slot in the chain strip, the two can be placed at
   different points in the signal flow — e.g. one early for corrective
   work, one later for tonal shaping.

2. Autotune
   Real-time pitch correction with adjustable speed, amount, mix,
   formant shift, and a Hard Tune control for dialing in anything from
   subtle correction to a fully quantized, robotic effect. Notes can be
   enabled/disabled individually via the on-screen keyboard, set in one
   click using the Scale Preset menu (Major, Natural Minor, Harmonic
   Minor, pentatonic scales, Blues, Dorian, Mixolydian, and more)
   combined with a selectable root key, or detected automatically:
   the Auto Key button listens for a few seconds and picks the closest
   matching major/minor key from the sung pitch (Krumhansl-Schmuckler
   key-finding).

   A Live / Studio toggle controls the pitch-shifter's grain size,
   which is the source of this module's processing latency: Live uses
   a short ~10ms grain for near-real-time monitoring (some loss of
   smoothness); Studio uses the original ~120ms grain for the
   smoothest correction. The plugin reports its total current latency
   (this module plus Pitch/Formant's, if both are active — see below)
   to the host for automatic delay compensation, so tracks stay in
   sync regardless of which modules or modes are active.

3. De-Esser
   Frequency-targeted sibilance control with adjustable center
   frequency, threshold, and reduction range.

4. Compressor
   Threshold, ratio, attack, release, knee, and makeup gain, with a
   live transfer-curve display and gain-reduction meter.

5. Saturation
   Four drive characters (Tape, Tube, Clip, Foldback) with drive,
   tone, mix, and output controls.

6. Reverb
   Size, damping, width, pre-delay, mix, and a freeze mode for infinite
   sustained pads.

7. Delay
   Time, feedback, mix, tone (damping filter), and a ping-pong stereo
   mode.

8. Denoise
   A standard module panel like the others (Learn Noise button, a
   Static/Adaptive mode pair, Reduction and Sensitivity knobs, all on
   one row). Enable/disable is via the chain strip, same as every
   other module — no separate on/off switch on the panel itself.
   Classical spectral-subtraction noise reduction: click "Learn Noise"
   while only room tone/hiss is playing (no vocal) — after about 1.5
   seconds it captures an average noise spectrum, which never changes
   afterward (clicking Learn also turns the module on automatically,
   since it can't listen while bypassed).

   Two modes control how that fixed profile gets used:
     - Static (default): a fixed attenuation curve is computed once
       from the profile and applied identically to every frame,
       regardless of what's currently playing. Since the gain never
       reacts to the current signal, pumping is structurally
       impossible — it behaves like a learned EQ notch matching the
       noise's spectral shape.
     - Adaptive: compares each frame's spectrum against the profile
       live, so bins close to the noise level get attenuated while
       bins clearly above it (your voice) pass through mostly
       untouched — potentially deeper reduction in the gaps between
       words, but because the gain reacts to the current signal it's
       inherently more prone to pumping, even with frame-to-frame
       smoothing applied.
   Reduction controls how strongly either mode attenuates; Sensitivity
   scales the learned profile up or down if you want it to catch
   quieter or louder noise than what it originally measured. A
   spectral floor keeps any bin from being fully muted in either mode,
   which avoids "musical noise" (random tonal blips) that naive noise
   subtraction is prone to.

   This is well-understood classical DSP (Boll-style spectral
   subtraction), not a trained model — it works well on steady
   background noise (hiss, hum, fans, AC, room tone) but won't do much
   for non-stationary noise like passing traffic or someone talking in
   the background. Off by default (needs a learned profile to be
   useful, and — like Autotune and Pitch/Formant — it has its own real
   processing latency reported to the host for delay compensation
   while active).

9. Pitch/Formant
   A standard module panel like the others (Pitch, Formant, Mix
   knobs, all on one row), sharing a grid cell with Denoise since
   neither needs a visualiser. This is NOT pitch correction — unlike
   Autotune, it never detects what note you're singing or tries to
   snap it toward a scale. It just transposes your whole voice by a
   fixed number of semitones (Pitch, +/-12), with an independent
   Formant control (a spectral tilt) so a shifted voice doesn't
   automatically sound like a chipmunk (shifted up) or a giant
   (shifted down). Mix blends shifted and dry signal, useful for
   subtler character changes rather than a full transpose.

   Uses a 4-tap granular pitch shifter (the highest-quality pitch
   engine in this plugin) so expect clean results for modest shifts
   (a few semitones) and progressively more grain/flutter at larger
   ones. The Formant control is a lightweight brightness-tilt
   approximation (the same technique Autotune's Formant knob uses),
   not a true LPC-based formant-preserving algorithm, so it won't
   perfectly hold exact vowel character the way dedicated formant-
   correction software does — but it's a solid, low-artifact way to
   warm up or thin out a shifted voice. Off by default (real
   processing latency while active, reported to the host like every
   other pitch-shifting module here).


AUTO CHAIN
----------
The "Auto Chain" button (top right, next to the preset bar) listens to
about 4 seconds of dry vocal and measures level, dynamics (crest
factor), and high-frequency/sibilance energy, then applies a starting
configuration across the whole chain: a standard vocal low-cut plus a
touch of presence on EQ 1, a compressor and de-esser scaled to what it
measured, light saturation and reverb as "glue", and a neutral
chromatic Autotune (pair it with Auto Key if you want a specific key
locked in). Delay, Pitch/Formant, and EQ 2 are left off, since those
are stylistic choices rather than corrective ones. It also resets the
chain to its default processing order.

This is rule-based signal analysis, not a trained model — think of it
as a fast, reasonable starting point to tweak from rather than a
finished mix.


PRESETS
-------
50 factory presets are included, organized loosely by use case:
broadcast/podcast/voiceover, mainstream pop, R&B/hip-hop/trap, rock and
metal, vintage/lo-fi character, ambient/cinematic textures, live/room
sounds, choir/harmony-forward settings, and a handful of genre and
special-purpose effects (telephone FX, robot voice, ASMR, etc.).

You can also save your own presets. User presets are stored as
.svpreset XML files in:

    ~/Documents/SimpleVocals/Presets/

Use the preset bar's Prev/Next arrows to step through the full preset
list (factory presets first, then your saved user presets), or open
the preset menu to jump directly to one by name.
