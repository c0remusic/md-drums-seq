# No JavaScript in the device

The device is built from native Max objects only: no `js`, `v8`, `jsui` or `v8ui`, for its logic, its timing or its
UI. Live 12.2 embeds `v8` (ES2022, CommonJS), and a pure JavaScript engine would have been testable under Node; but `js`
and `v8` run on Max's low-priority thread only, and the device plays MD Drums live through its own MIDI output, where a
step decided on that thread would come late by however busy Live's interface is. The user also wants the device native
throughout.

## Considered Options

- **JavaScript engine, native timing**: JavaScript decides each step a little ahead and leaves the events in a native
  structure that high-priority objects emit on time. Testable under Node and kept off the timing path, but still
  JavaScript, and the look-ahead delays every live gesture.
- **JavaScript everywhere**: simplest to write, with the jitter of the low-priority thread on every Hit.

## Consequences

- No Node test gate: the gate has to be built in Max or from recorded output (ticket « Structure du moteur en Max
  natif »). That ticket put the logic in a compiled C++ external, native code tested under ctest (ADR 0002).
- The edit window's grid cannot be drawn by `jsui`/`v8ui`: its cells (velocity, Locks, Condition, micro-timing,
  ratchets) take native objects (`lcd`, `matrixctrl`, `live.grid`, `live.step`), which the UI mockups have to respect.
