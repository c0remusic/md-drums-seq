# CLAUDE.md

Guidance for Claude Code in this repository.

## Project

MoDule Seq (md-drums-seq, and MD Drums Seq in Live, up to 0.1.1) is a Max for Live sequencer for MoDule (MD Drums
up to its 0.1.1: the Elektron Machinedrum's engines as a VST3,
`C:\dev\gearmulator-md-drums`), mixing Elektron and Cirklon ideas. It plays MD Drums live through its own MIDI output,
and on
request prints Hits and their Locks into Live clips, which then play MD Drums without it; Live's API (LOM) writes
clips only for printing, never while playing, since every write lands in Live's undo history. The contract with the
plug-in is MD Drums' ADR 0003
(`docs/adr/0003-locks-travel-as-notes.md` in gearmulator-md-drums): Hits are notes 36-51, Locks are notes 64-87 on
their Track's MIDI channel, velocity 1-63 giving values 0-62 and 64-127 themselves.

## Where things are

- `source/mdseq/`: the core, a C++17 library without Max (ADR 0002). `mdseqSequencer.*` plans each step one step
  ahead and hands out the MIDI due in Live's ticks; it reads Live's transport raw, as the adapter reads it (the ticks,
  playing or not, the time and what a millisecond is worth), and tells a start, a stop and a jump itself, a jump's
  slack a 64th, 30 ticks, at every scale (a loop brace or a seek longer than that is always a jump, so a loop brace
  of a 32nd is one; one of a 64th or less is never told). It is built on the main thread, then called from Max's
  scheduler thread only, and never allocates, locks or blocks there; what the main thread shows of a wake, its
  `Playhead` (the step, the Pattern played and the one queued), packs into one word that the adapter hands over
  atomically, as it does the last start's seed. It places events in subticks, 840 to a step, so that
  ratchets (1-8 to a step) and micro-timing (24ths) fall on integers. Played live, a Lock's note-off follows its
  note-on at once: Live keeps the notes a Max device holds in a set of 128 and quits when it overflows (issue 24);
  printed (`prepareRender`), a Lock keeps its length, which a clip's note needs. It produces a step's events already
  in order: the heaviest step holds 6400 of them, and a wake must stay well under Live's 1.45 ms vector
  (`mdseqSequencerTest` times it). Swing is the Machinedrum's: the Pattern's amount, 50-80 %, delays the steps of its
  swing row (every second step by default, every Track's) by (2 x amount - 1) of a step, rounded to a subtick, before
  their micro-timing; a step's last note-offs can then fall 2.56 steps after its place, so the queue holds four steps.
  Accent is the Machinedrum's too: the Pattern's amount, 0-127, and an Accent row for every Track; each Hit of an
  accented step is followed, after its Locks, by note 88, its velocity the amount on the Locks' scale, which MD Drums
  binds as a Lock and plays at 128 + 2 x the amount (issue 26; MD Drums' ADR 0003 amended). Slide too, the
  Machinedrum's: a Slide row for every Track; the last Hit of a step there is followed, after its Accent, by note 89,
  its velocity the clocks (24 to a quarter note) to the Track's next Hit in the Pattern as both play (whatever that
  Hit's Condition, around the Pattern's end), notes 90-113 that Hit's Locks for the Locks the step has, and note 114
  when the clocks pass 127, 127 more for each unit of its velocity (`Sequencer::stageRatchet`; issue 26, decided on
  2026-10-19 on MoDule's proposal, which MoDule implements). `mdseqEditor.*` is the
  interface state on the main thread,
  with its own copy of the bank of 16 Patterns: `applyEdit` (`mdseqEdit.*`) edits both copies alike, and every edit
  names its Pattern. A copy names the Pattern it writes and the one it copies (`CopyPattern`), which `applyEdit`,
  seeing one Pattern, leaves to the two banks' holders (the Sequencer through `load`); x2 (`DoubleLength`) doubles the
  length, up to 64, and copies steps 1-L after them. It also words and numbers what the step panel, the window's controls and the strip's status line
  show: each `PanelValue` is a key the patch routes on with its integers and its texts, which the adapter sends as it
  is. It names the Tracks after the Kit MD Drums tells (`mdseqKit.*`, ADR 0003): each Machine's short name on the
  strip's tabs ("01 BD", what a cell of 36 px holds, through the tab's `_parameter_range`, then the tab chosen again),
  its whole name on the window's rows and in the panel's title, and SYN1-8 after the step selected's Machine; numbers
  and "SYN1" while MD Drums says nothing. A name from a user's bank (a User Machine's, a user's Sample's file name)
  can be longer than its box: `mdseqText.*` measures it in Ableton Sans' own advances and cuts it to the box
  (`fitText`, the widths in `fit::`), with an ellipsis, where a live.comment would break the line. A Lock's dial without a Lock sits unlit on the Kit's value ("Kit 40" under
  it), except where the dial itself sent the Kit, which it leaves on the middle until `settle` (every Kit poll): moving
  it at once would jump a drag going on through the middle. `mdseqView.*` draws a step's cell as `Design/HANDOFF.md` says and places clicks in the two `lcd`
  objects. `mdseqStore.*` writes a Pattern as the integers its `pattr` Blob keeps in the set (one per Pattern), and
  its `PatternStore` decides the rest of keeping the bank there: which edits leave a Pattern to write, which Patterns
  are due once the edits stop (none whose words its Blob already holds), and what a Blob's value received is (an
  echo, a Pattern to load, which drops the write pending, words this build cannot read, which load as an empty
  Pattern that the next edit writes over and which the external tells in the Max console, or a Pattern outside the
  bank).
  `mdseqPrint.*` renders N Passes of a Pattern as a clip's notes per Track by running a Sequencer of its own from the
  last start's seed (the stream played is the stream printed). `mdseqPrinter.*` runs a print from its start to its
  status line against `LiveSet`, the few reads and writes it needs of the set (the adapter's goes through the LOM,
  `mdseqPrinterTest`'s is a set in memory): each Track's clip on the track sent to the device's track on "N-MD
  Drums", one created otherwise and given its channel once Live lists it, in the selected scene or a new one below,
  and no clip at all when a track it planned for is no longer at its index (tracks are addressed by index; a track
  created is checked by its name, a track found by its output to the device's Track channel, since Live renumbers
  "2-MIDI" and names with a '#' when a track is created before them) or Live did not create one; the adapter keeps
  only the clock that calls its second step again.
  `mdseqCapture.*` captures a performance (issue 23): a `Journal` of the notes the device sends, a ring of a million
  written on the scheduler thread without allocating (two atomic words a note, read on the main thread with what was
  overwritten meanwhile dropped), in take time (from the bar line before the transport's start, Live's jumps joined,
  which the Sequencer's `Wake` reports with its start and stop); `takeClips` turns the last take into a clip per
  Track, a Lock, the Accent and the Slide lasting as their Hit, and `Printer::capture` places them as a print's, named
  "Take n" and not looping (`mdseqCaptureTest` checks that a Pass captured is the Pass printed). The strip's Device /
  Clips switch keeps the note-ons in the device on Clips, its note-offs going out so that Live holds no note. The
  take to the Arrangement waits for Live 12.4 (issue 11).
  `mdseqProtocol.*` is the vocabulary between the patch and the external, defined once: each message's selector, the
  integers its prepend fixes before the control's value (a Mute's Track, a Lock's parameter), the fewest integers
  the external takes it with (a Mute's Track and state), and the kind of edit it makes the Editor forward (whether
  it edits a Pattern is `isPatternEdit`'s rule), the call `toEditor` makes for it (none with fewer integers than it
  needs, which the external reports in the Max console), the panel keys and how the patch routes them, the Scale
  menu's labels; the external registers its methods from it. A menu's value goes both ways as its index (Length,
  Ratchets, Swing and Passes count from their first item), and the column numbers are one indexed key.
  `mdseqKit.*` is the device's side of ADR 0003: `KitLink` draws its 24-bit id and reads
  `<temp>/md-drums-seq/<id>.kit`, MD Drums' answer: each Track's Machine, the Machine's names for SYN1-8 and the Kit's
  values, in a text format the device reads at version 1, skipping the lines it does not know; a file there that does
  not open (`KitFile::busy()`: Windows refuses to open a file while MD Drums' rename replaces it) keeps the Kit.
  `HelloPace` says when to send the hello (notes 124-127 whose velocities carry the id: at the first tick; without a
  file 1, 2, 4 and 8 s later, then every 10 s; every 10 s with one), on the scheduler thread: MD Drums deletes the
  file of a device it has not heard for 15 s, and Max's main thread stops for seconds in Live (issue 32).
  `mdseqPush.*` is Push 2 as the device's (issue 33): `PushMode` takes the pads, buttons and encoders the adapter
  grabs, edits through the Editor as the window does, and says which pads and buttons to light (only those that
  changed) in palette slots from 101 it defines (`paletteEntry`, Push 2's SysEx), and which notes a Track pad plays.
  The pads as Push's Drum Rack: the 32 steps of the Track the strip shows on the four top rows, the Tracks bottom left
  (Track 1 at the corner), the Patterns bottom right (in the strip's order); a step pressed puts its Hit at 127, or,
  let go within `HoldMs` with no Lock turned, takes it off; held, the encoders turn its Locks, a page of eight. The
  adapter takes Push (`takePush`: the second control surface listing the pads and the encoders, `grab_control` of
  each control, the palette by `send_midi`) while the strip's Push switch is on, the device on (`live.thisdevice`)
  and its track the one Live selects (an observer of `live_set view`), as Tuple does; 19 `live.observer` objects
  report the controls (`pushobs i id n` points one, `pushvalue i ...` comes back); the lights go out by `send_value`,
  again three times 150 ms apart after taking Push, and a Track pad's notes by the scheduler thread. Not tried in
  Live yet.
- `source/mdseqTest/`: the core's tests, one executable per module with a plain `main`, driven in virtual time.
- `source/mdseqExternal/`: `mdseq.mxe64`, the adapter over the C SDK (`max-sdk-base`, fetched and pinned by the root
  `CMakeLists.txt`): it hands the core what it reads of Live's transport every millisecond, wakes on an ITM timer at
  the tick the core asks for, and sends MIDI bytes to `midiout`. It reaches the LOM through the device's `live.path`
  and `live.object` ("lom path|id|obj ..."), whose answers come back within the call; the formats are noted above
  `lomValue`. Every half second it reads MD Drums' file on the main thread (opened, read whole and closed at once: MD
  Drums renames its new file over it) and sends the hello from the scheduler thread, between two wakes. As it is
  created, unless the window's "Check for updates" is off, a thread of its own asks GitHub for the public
  repository's newest release, at most once a day (`updates.*`: WinHTTP, and `%LOCALAPPDATA%\md-drums-seq\
  settings.txt`, which keeps the switch, the day and the answer for every device), and hands the strip's line
  (`mdseqUpdates.*` words it) to the main thread through a qelem it sets only while the device lives.
- `tools/make_device.py`: writes the device `MoDule Seq.amxd` next to the external, in `build/device`: the v12
  strip, and the edit window as a subpatcher: the strip's messages go straight into the external, the window's
  through one `s ---in`, and the external's outlets through device-local names (`---grid`, `---panel`, each panel
  key's receive named after the key). It builds its prefixes, its panel route and its menus (Scale, Conditions, the
  Lock dials' Kit) from `build/tools/vocabulary.json`, which `tools/mdseqVocabulary` writes from `mdseqProtocol`
  before the device. `tools/check_device.py`, run by ctest, checks it without Max (lines, sends and receives,
  parameter names, presentation bounds read from the patch itself) and against that JSON: every message reaching the
  external with a registered selector, a prepend with exactly the integers it fixes and a message box with at least
  those the external needs (`message_problem`, which make_device.py also raises on), and every registered one sent,
  the panel route listing exactly the declared keys and each reaching a receive, no initial value into a message that
  edits a Pattern. `tools/check_selectors.py`, run by ctest too, holds both tools to that rule on messages the device
  does not send.
- Max never reloads an external it has loaded: after a new `mdseq.mxe64`, Live has to restart; a loaded one is
  locked, so the build cannot overwrite it (rename it aside first). When Live crashes, read the Windows Application
  log (Event 1000) and `%LOCALAPPDATA%\CrashDumps` before naming a cause: on 2026-10-13 a copy into the User Library
  was blamed on timing alone, and the log put the crash 35 s earlier, in Max for Live (issue 24). To iterate in a
  running Live, configure
  with
  `-DMDSEQ_CLASS=mdseq_dbgN` (a new N each time): a class Max has never seen, which also starts with a demonstration
  Pattern and logs what it sees to `%TEMP%\mdseq-log.txt`. Releases are `mdseq`, with neither.
- In a device, Live's transport is the ITM of a time object (`time_getitm`, named "live"); `itm_getglobal()` is Max's
  own, which never runs there.
- Live can be driven while it runs through the `TupleMCP` control surface of `C:\dev\tuple\tools\live-mcp` (README
  there): LOM reads and writes, the browser (`load_item`), track meters. It writes no undo history: work on a set the
  user has saved, check names before deleting anything. Max ignores mouse messages posted to its windows
  (`PostMessage`): the device's clicks are the user's to make.
- `.scratch/seq-v1/`: the wayfinder map and its tickets; `Design/`: the approved UI mockup (v12) and its handoff.

## Build and gate

Windows, Visual Studio 2022, build directory `build/` (ignored by git); the first configure fetches `max-sdk-base`:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release -j 6
ctest --test-dir build -C Release --output-on-failure
```

The gate is the build exiting 0, then ctest all passed, before every commit and push. What ctest cannot reach (the
external inside Max, the device in Live, the LOM) is checked in Live with the ticket's checklist.

A release's zip, `build/MModuleSeq-<version>-win64.zip` (the folder `MoDule Seq/` with the device, `mdseq.mxe64`,
README, LICENSE and NOTICE), comes from `cmake --build build --config Release --target zip_release`
(`tools/package.py`, which refuses a development build). The version is the root `project()`'s; the external posts it
in the Max console. The licence is GPL-3.0-or-later (`LICENSE.md`; the Max SDK's in `NOTICE.md`). A release is a tag:
never one without the user's word.

Two repositories, renamed on 2026-10-20 (GitHub redirects the old names, the update check of 0.1.0 and 0.1.1
included): `origin`, c0remusic/MModuleSeq-dev (private, md-drums-seq-dev before), holds the history of the work;
`public`, c0remusic/MModuleSeq (md-drums-seq before), holds the releases, and is where the device's update check
looks. The public one never gets the private history: a release is one commit on `public/main` with the tree of the release's commit here and the
previous release as its parent (`git commit-tree <commit>^{tree} -p public/main -F <message>`, pushed to `public` as
`main`), then a tag and a GitHub release there with the zip, a prerelease while the version is below 1.0. 0.1.0 is
public 4e23a2d, the tree of f9f0d6a here; 0.1.1 is public d14b827, the tree of b5974fa; 0.2.0 is public 48f22fa, the tree of d3855c1. The release's notes are kept
in `.scratch/release/<version>-notes.md`.

Code conventions are gearmulator-md-drums': tabs (size 4), 120 columns, braces on their own line, namespace content
indented (`source/.clang-format`); PascalCase classes, camelCase functions and variables, `m_` members, `_`
parameters; early returns; C++17. A new test's `main` sets `setvbuf(stdout, nullptr, _IONBF, 0)`.

## Agent skills

### Issue tracker

Issues live as local markdown files under `.scratch/<feature>/`. See `docs/agents/issue-tracker.md`.

### Triage labels

The five default triage roles, each label equal to its name. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `CONTEXT.md` and `docs/adr/` at the repo root. See `docs/agents/domain.md`.
