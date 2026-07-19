SimpleVocals
============
A JUCE-based vocal channel-strip plugin (VST3 / Standalone) for macOS and
Linux, built around a reorderable chain of eight processing modules.


OVERVIEW
--------
SimpleVocals gives you a complete vocal processing chain in a single
plugin: EQ, Autotune, De-Esser, Compressor, Saturation, Harmonizer,
Reverb, and Delay. Every module can be individually enabled/disabled,
and the order of the chain can be freely rearranged to suit your signal
flow. A built-in preset browser ships with 50 factory presets covering
everything from clean broadcast voice to heavy creative effects.


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

3. De-Esser
   Frequency-targeted sibilance control with adjustable center
   frequency, threshold, and reduction range.

4. Compressor
   Threshold, ratio, attack, release, knee, and makeup gain, with a
   live transfer-curve display and gain-reduction meter.

5. Saturation
   Four drive characters (Tape, Tube, Clip, Foldback) with drive,
   tone, mix, and output controls.

6. Harmonizer
   Two independently tunable harmony voices (in semitones), with mix,
   grain size, humanize, and stereo width controls.

7. Reverb
   Size, damping, width, pre-delay, mix, and a freeze mode for infinite
   sustained pads.

8. Delay
   Time, feedback, mix, tone (damping filter), and a ping-pong stereo
   mode.


AUTO CHAIN
----------
The "Auto Chain" button (top right, next to the preset bar) listens to
about 4 seconds of dry vocal and measures level, dynamics (crest
factor), and high-frequency/sibilance energy, then applies a starting
configuration across the whole chain: a standard vocal low-cut plus a
touch of presence on EQ 1, a compressor and de-esser scaled to what it
measured, light saturation and reverb as "glue", and a neutral
chromatic Autotune (pair it with Auto Key if you want a specific key
locked in). Harmonizer, Delay, and EQ 2 are left off, since those are
stylistic choices rather than corrective ones. It also resets the
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


BUILDING FROM SOURCE
---------------------
Requirements:
  - CMake 3.22+
  - A C++17 compiler
  - JUCE (either a local checkout at $HOME/JUCE, or it will be fetched
    automatically via CMake's FetchContent)

To build:

    ./build.sh          (macOS / Linux)
    build.bat           (Windows — run from a shell with CMake and a C++
                          compiler on PATH, e.g. the "Developer Command
                          Prompt for VS" or a regular prompt with the
                          Visual Studio Build Tools installed)

This configures a Release build in ./build and compiles the VST3 and
Standalone targets. On macOS, macrun.sh / run.sh are provided as
convenience scripts for launching the Standalone build during
development.


PLUGIN FORMATS
---------------
  - VST3
  - Standalone


SUPPORT / FEEDBACK
-------------------
This is an independent, in-development project. If you run into
issues or have ideas for new modules or presets, keep notes and file
them alongside the project source for the next round of development.
