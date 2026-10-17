# md-drums-seq

A step sequencer for MD Drums, as a Max for Live device. It plays MD Drums' 16 Tracks live from Ableton Live, with
the Machinedrum's ideas: Hits with velocity, parameter Locks on each step, trig Conditions, ratchets, micro-timing,
swing, a Fill, 16 Patterns and chains. It can also print a Pattern into Live clips that play MD Drums without it.

Version 0.1.0, a preview: Windows only.

## Requirements

- Windows 10 or 11, 64-bit.
- Ableton Live 12 with Max for Live (developed and measured on Live 12.2.1).
- MD Drums, the Machinedrum's engines as a VST3 ([releases](https://github.com/c0remusic/MModule/releases)): 0.1.0 or
  later plays the Hits and the Locks; 0.1.1 or later also names the Tracks after their Machines and shows the Kit's
  values on the Lock knobs (with 0.1.0 the Tracks show their numbers). From 0.1.2 the plug-in is called MoDule: the
  same plug-in, which the device works with as it does with MD Drums.

## Install

1. Unzip `md-drums-seq-0.1.0-win64.zip`.
2. Put the `md-drums-seq` folder into Live's User Library, under `Presets/MIDI Effects/Max MIDI Effect/`. Keep its
   two files together: the device loads its engine, `mdseq.mxe64`, from its own folder.
3. Start Live (or restart it: Max never loads a new version of an engine it has already loaded).

## Set up

Put **MD Drums Seq** on a MIDI track, in front of **MD Drums**. The device sends each Track's Hits (notes 36-51) and
its Locks (notes 64-87) to MD Drums through the track. MD Drums tells the device its Kit through a small file in the
system's temporary folder: the device then names each Track after its Machine and shows the Kit's values.

## Use

The device has two surfaces.

**The strip**, in Live's device chain, plays:
- the 16 Patterns (lit: playing; bright: holds Hits; faint: empty; blinking blue outline: queued for the end of the
  Pass). Choosing a Pattern while Live plays queues it;
- the Tracks, by their Machines, and their activators (yellow: plays);
- the 16 steps of the Track shown, page by page; a click toggles a Hit;
- FILL, held to play the steps whose Condition is FILL (and silence those with !FILL);
- Chain: on, the Pattern buttons clicked make a chain; off, it plays, each Pattern for one Pass, round and round. A
  Pattern chosen by hand ends it;
- Edit, which opens the window.

**The edit window** edits:
- the grid, 16 Tracks by 32 steps (two halves for 64): a click on an empty cell puts a Hit and selects the step; a
  click on a Hit selects it; a click on the selected step toggles its Hit, so a double-click takes a Hit off; a drag
  paints Hits, or takes them off when it starts on one;
- the swing row under the grid: the steps that play late by the Pattern's swing;
- Copy to (the Pattern shown over another), Length, x2 (twice as long, the steps again after them), Scale, Follow,
  Print;
- Fill edit (Hits put with the mouse take the Condition FILL) and Clear (the Pattern back to empty);
- the step panel: Hit, velocity, Condition, micro-timing, ratchets and their velocity curve, and the 24 Locks. A
  Lock's knob sits on the Kit's value until turned; a double-click gives a control back its default (the Kit for a
  Lock).

Live's undo (Ctrl+Z) gives back a Pattern copied over, cleared or edited, and a chain: the set keeps them.

The "Pattern" parameter can be automated in the Arrangement, set per scene by a clip envelope in Session, or mapped
to a controller. A change takes effect at the end of the Pass playing, as on Elektron's machines.

**Print** writes the Pattern shown, over 1 to 16 Passes, into a Session clip per Track, on the MIDI tracks routed to
MD Drums' Track channels ("3-MD Drums" for Track 3), creating those that are missing. Turn the device off, or mute it,
to hear the clips alone.

**Updates.** At most once a day, as it loads, the device asks GitHub for the newest release of md-drums-seq (one
HTTPS request to api.github.com; nothing else is sent, nothing is downloaded or installed) and shows a line under the
strip's pages when it is newer than the one running. "Check for updates", in the edit window, turns this off for
every set. The day of the last check and the switch are kept in `%LOCALAPPDATA%\md-drums-seq\settings.txt`.

## Timing

Measured in Live 12.2.1 at 44.1 kHz: played live by the device, from its printed clips, and from a clip made by hand,
MD Drums' attacks land on the same sample, 2 samples after the grid once Live has compensated MD Drums' latency, with
no measurable jitter.

## Build from source

Windows, Visual Studio 2022, CMake 3.25 or later, Python 3; the first configure fetches Cycling '74's `max-sdk-base`.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release -j 6
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target zip_release
```

The device and its engine land in `build/device`, the zip in `build`.

## Licence

GPL-3.0-or-later: see `LICENSE.md`. Third-party notices, the Max SDK's licence among them: `NOTICE.md`.
