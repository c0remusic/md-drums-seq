# md-drums-seq

A Max for Live sequencer that plays MD Drums from Live and prints what it plays into Live clips, which then play MD
Drums without it.

## Language

### From MD Drums

The plug-in's own terms (`CONTEXT.md` in gearmulator-md-drums), used here with the same meaning.

**Track**:
One of MD Drums' sixteen voices; in Live, the MIDI track that plays it.
_Avoid_: voice, part, channel

**Hit**:
One trigger of a Track and the sound it makes.
_Avoid_: note, trig

**Lock**:
A value of one Track parameter that a single Hit plays in place of the Kit's; the Track's next Hit without one plays the
Kit's value again.
_Avoid_: p-lock, parameter lock, automation

**Kit**:
The whole sound of the sixteen Tracks, held by MD Drums; the sequencer never changes it. MD Drums tells the device its
Machines, their names for SYN1-8 and its values through a file (ADR 0003), which name the Tracks in the device.
_Avoid_: preset, patch

### Sequencing

**Pattern**:
What the sequencer plays: up to 64 steps shared by the sixteen Tracks, each step holding a Track's Hit with its
velocity, Locks, Condition, micro-timing and ratchets. It lives in the sequencer; clips only receive what it plays.
_Avoid_: sequence, clip (a clip is what a Pattern prints into)

**Pass**:
One time through the Pattern, from its first step to its last; Conditions count and draw per Pass.
_Avoid_: loop, cycle, iteration

**Condition**:
A rule on a step that decides, each time the step comes round, whether its Hit plays: a chance, a pass count (`A:B`),
the Fill, the previous condition's outcome (`PRE`), the neighbouring Track's (`NEI`), or the first pass (`1ST`).
_Avoid_: trig condition, probability (one kind of Condition only)

**Ratchet**:
One of the Hits a step plays when it repeats itself: two to eight, spread evenly over the step, with the same Locks.
_Avoid_: retrig, roll, pulse

**Swing**:
The Pattern's lateness of the steps in its swing row, as on the Machinedrum: an amount from 50 % (straight) to 80 %,
by which each pair of steps splits; the row, every second step until edited, is the Pattern's, for every Track.
_Avoid_: shuffle, groove

**Print**:
Writing, on request and in one go, what the sequencer plays into Live clips, Conditions already drawn: some Passes of
the Pattern, or a performance it played. Live then plays the clips without the sequencer.
_Avoid_: export, render, bounce (those make audio)

**Fill**:
A momentary mode the player holds, during which steps with a `FILL` Condition play and those with `!FILL` do not.
_Avoid_: fill pattern
