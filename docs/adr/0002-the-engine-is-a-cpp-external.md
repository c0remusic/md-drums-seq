# The engine is a C++ external over a core without Max

The sequencer's logic lives in a Max external written in C++ (`mdseq.mxe64`), not in boxes of a patch. Its core is a
C++17 library with no Max header: the Pattern, the drawing of Conditions, the events of each step, the rendering of N
Passes for printing, the capture journal and the drawing of a step's cell. A thin adapter, written against the C SDK
(`max-sdk-base`), runs the core on Max's scheduler thread, which is Live's audio thread in a Max for Live device: it
keeps an ITM timer in ticks, sends the due MIDI bytes to `midiout`, moves the UI's edits onto the scheduler thread with
`schedule()` and hands its outputs to the main thread with `defer_low`. The patch keeps the UI (`live.*`, `lcd`),
`live.object` for printing, and `midiout`.

ADR 0001 keeps JavaScript out of the device for its jitter; a compiled external is native code on the high-priority
thread, so it stands within that decision. What a pure patch could not give is a test gate: the core is tested under
ctest in virtual time, as gearmulator-md-drums is, before every commit and push.

## Considered Options

- **Pure patch** (`dict`, `zl`, `expr`, `if`, `pipe`): nothing to compile, but `PRE`/`NEI`, the hashed `X%` draws,
  ratchets and micro-timing become a patch nobody can review, testable only inside Max or Live, where `max-test` is not
  known to run on Live's bundled Max.
- **`gen~`**: compiled and sample-accurate inside Max, but its events reach `midiout` through the scheduler a vector
  late, its data would live in a `buffer~`, and nothing tests it outside Max.
- **Min (`min-api`) for the adapter**: nicer C++, but every message from the scheduler thread is deferred to the main
  thread unless declared `threadsafe::yes`, through a queue of two slots, and the repository is maintained less.

## Consequences

- One binary per platform, rebuilt and embedded by freezing the device; Windows only until someone builds it on a Mac.
- The core never allocates, locks or blocks on the scheduler thread: queues and the journal are preallocated for the
  worst case (16 Tracks × 25 notes × 8 ratchets per step, plus note-offs).
- The adapter, the patch and the LOM are checked in Live by hand, with a checklist per ticket.
- Patterns are saved as 16 `pattr` Blobs, one per Pattern, written at the end of each gesture; every write is an undo
  step in Live (research `08-max-natif.md`).
