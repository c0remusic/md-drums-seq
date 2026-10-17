# MD Drums tells the device its Kit through a file the device names

The device shows each Track's Machine ("01 TRX-BD"), and later the names of SYN1-8 and the Kit's values behind the
Locks. MD Drums knows them; the LOM shows a plug-in's parameters only once the user has configured them in Live, 128 at
most, which MD Drums' own Push list already fills (issue 12). So MD Drums writes them into a file, and the device finds
the file by a name it gives MD Drums through the MIDI it already sends.

**The hello.** The device draws a 24-bit id at its creation, never 0, and sends four note-ons at one moment, pitches
124, 125, 126 and 127 in that order, each with velocity 1 + six bits of the id (bits 23-18, 17-12, 11-6, 5-0), each
followed by its note-off. MD Drums takes four consecutive note-ons 124 to 127, in that order and on any channel, as one
hello, whether the transport runs or not; any other note between them cancels it. They trigger nothing and lock
nothing. The device sends a hello when it is created, then every second while it finds no file for its id, and every
five seconds while it finds one.

**The file.** `<temp>/md-drums-seq/<id>.kit`, where `<temp>` is the system's temporary folder as both sides take it,
`std::filesystem::temp_directory_path` (`GetTempPath` on Windows: `TMP`, then `TEMP`; `$TMPDIR` elsewhere), and
`<id>` the id in six lowercase hexadecimal digits. MD Drums writes it within a second of a hello from an id it does
not hold, and of every change to what it holds, at most four times a second, off the audio thread; whole, as
`<id>.kit.tmp` renamed over the file. It deletes the file fifteen seconds after that id's last hello, and when the
instance goes away. UTF-8 text, one record per line, fields separated by one space:

```
md-drums-kit 1
kit <the Kit's name, to the end of the line>
track <1-16> <Machine id 0-191> <Machine name> <SYN1 name> <SYN2 name> ... <SYN8 name>
values <1-16> <the Kit's 24 values of the Track's parameters, 0-127, in the Locks' order>
```

A SYN name the Machine does not use is `-`. A Track whose Machine MD Drums does not know yet has no `track` line. The
first line names the format's version: the device reads version 1 and ignores a file of another, and ignores lines it
does not know, so that a later version can add some.

## Considered Options

- **The LOM, on configured parameters** (issue 12's plan): nothing to build in MD Drums, but the user configures the 16
  Machines in Live once, they take 16 of the 128 places that "Prepare Push list: a page per Track" fills, and the 384
  Kit values never fit.
- **Shared memory between the two DLLs**, both in Live's process: no file, but a binary layout and its versions to
  keep identical in two repositories, a lock-free protocol to read it, and slots to reclaim after a crash.
- **CC or SysEx for the hello**: MD Drums takes only notes from the host (ADR 0002 of gearmulator-md-drums), CCs 16-119
  belong to the Machinedrum's map, and whether Live passes SysEx from a Max for Live device to a VST3 is not known.
- **MD Drums answering through MIDI**: a plug-in's MIDI output reaches a device only through another track's input,
  which the device's own track already uses for its notes.

## Consequences

- Amends ADR 0003 of gearmulator-md-drums: notes 124-127 from the host are the device's hello, MD Drums' only notes
  outside Hits and Locks.
- The device's name for its MD Drums lives a process at most: an id is drawn at each creation, so a file left by a
  crash names no device. A device routed to another MD Drums is found by the new one within five seconds, once the old
  one has forgotten it after fifteen.
- A MIDI clip recorded from the device's output keeps its hellos; played, they make MD Drums write a file that no
  device reads, deleted fifteen seconds later. A print carries none: it renders the Pattern only.
- The names reach the device a second at most after they change, on its main thread; nothing of it touches the
  Sequencer, which only sends the hello's eight messages when the main thread asks.
