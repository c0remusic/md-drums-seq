# Driving Live for checks and measurements

Node scripts over the `TupleMCP` control surface (`C:\dev\tuple\tools\live-mcp`, its README says how to install it
and select it in Live's preferences). Each imports that bridge by its absolute path, talks to the Live that is open,
and writes no undo history: run them on a set the user has saved, and expect them to check names before deleting.

| Script | Does |
|---|---|
| `live-look.mjs` | Lists the tracks and their devices (read only) |
| `live-device.mjs <track> <device>` | A device's parameters, the track's clips and meter (read only) |
| `live-browse.mjs <root> <name>…` | Walks the browser by names, prints the path of the item found (read only) |
| `live-playtest.mjs <track> <seconds>` | Plays, samples the track's output meter, stops, puts the position back |
| `live-swap.mjs <track> <old name> <browser path> <new name>` | Loads a device from the browser, deletes the old one if the names check |
| `live-timing-setup.mjs <device path> <Operator path>` | Builds the timing measurement on four `MDSEQ …` tracks |
| `live-timing-rec.mjs` | Routes and arms the two `MDSEQ REC …` tracks (Post FX, monitoring off) |
| `live-timing-run.mjs` | Records four bars of probe and reference from the set's start, prints the files |
| `live-cleanup.mjs` | Deletes the `MDSEQ …` tracks, and nothing else |
| `live-print-run.mjs <MD Drums path> <dev device path> [--play] [--hold s]` | Prints through a development device on temporary tracks, reads the clips, records them playing, removes it all |
| `live-print-auto.mjs <MD Drums path> <dev device path> [--play]` | The same, Track 1's track keeping the name Live gives it ("8-MIDI"), which a print that creates tracks before it renumbers |
| `live-loop-run.mjs <MD Drums path> <dev device path> <loop beats> <seconds>` | A development device inside a short arrangement loop, its MIDI recorded (Post FX): the notes and their spacing; puts the loop back |
| `live-same-track.mjs`, `live-user-kicks.mjs`, `live-trxbd-clip.mjs`, `live-three-kicks.mjs` | Clip against sequencer on one track, and the user's own track recorded at Main (issue 04) |
| `capture-window.ps1 -Title <title> -Out <png>` | One window of Live, through `PrintWindow`: never the screen |
| `live-banks-probe.mjs <probe path>` | What Live hands Push for `md-seq-banks-probe`'s banks, on a temporary track |
| `live-kick-latency.mjs <MD Drums path> <device path> [bars]` | MD Drums played by the device and by a clip, recorded together on temporary `MDSEQ …` tracks, then removed |
| `live-swing-run.mjs <MD Drums path> <device path> [bars]` | What a development device sends, recorded as a MIDI clip from its track (Post FX) on temporary `MDSEQ …` tracks: the notes in beats, then removed |
| `live-patch-test.mjs setup <MD Drums path> <device path>`, then `run`, `params`, `cleanup` | The strip's parameters that the LOM reaches (Pattern, Mute 1-16, Fill) set at given beats while a MIDI track records what the device sends; the temporary `MDSEQ …` tracks stay until `cleanup`, for captures between the steps |

| `live-kit-test.mjs setup <MD Drums path> <device path>`, then `drop`, `cleanup` | MD Drums and a development device on a temporary `MDSEQ KIT` track (`KIT_TRACK` names another), MD Drums deleted on `drop`: the Track names of issue 27, between captures |
| `live-kit-readd.mjs <MD Drums path> <kit file>` | MD Drums loaded again on `MDSEQ KIT`, after the device, and the time its Kit file takes to come back |
| `live-load-probe.mjs <browser path>`, then `cleanup` | Any probe device on a temporary `MDSEQ TABS` track, for a capture (`../tabs_probe.py`'s live.tab probes) |
| `live-rename-device.mjs <track> <name> <new name>` | Renames a device whose name checks: a development build shown as "MD Drums Seq" for the release's screenshots (`docs/images`) |
| `live-quiet-check.mjs <track name>` | Whether starting the transport sounds anything but that track: every other track's Arrangement clips and Session clips playing (read only) |
| `live-track-volume.mjs <track name> [value]` | Reads or sets a track's fader: at 0 during `live-record-track.mjs`, which records Post FX, nothing reaches the speakers |
| `live-crash-test.mjs setup <MD Drums path> <device path>`, then `run <s>`, `cleanup` | Issue 24's crash scenario: two development devices on the dense Pattern in front of MD Drums, fader at 0; only on a set the user accepts to lose |
| `live-set-param.mjs <track> <device> <parameter> <value>` | Sets a device's parameter by name (the Push probe's "Probe Surface") |

The Push probe of issue 33 (`../md-seq-push-probe.amxd`, `../push_probe.js`, written by `../make_devices.py`) logs what
Push 2's grabbed encoders send to `%TEMP%\mdseq-push-probe.txt`; put both over `md-seq-channel-probe` in the User
Library for the test, and the original back after. `../accent_levels.py` measures the Accent's check Pattern.

Max ignores mouse messages posted to its windows (`PostMessage` of `WM_LBUTTONDOWN`/`UP` to the edit window: no Hit,
nothing in the device's log): a click in the device needs the real pointer, which only the user moves.

Live's browser lists only the device files it has indexed: a new `.amxd` in the User Library did not show up, copied
while Live ran (dbg24, dbg25) or before it started (the tabs probe, 2026-10-16). Write over a file it lists instead
(a development build over an older `dbgN` that Live has not loaded since it started, a probe over another probe, kept
aside and copied back after).

Live keeps a recorded file locked while a clip uses it: delete the recording clip (`clip_slots 0 delete_clip`) before
copying the WAV, then analyse it with `../onsets.py`, and compare recordings that start on the same bar line with
`../compare.py` (the lag at which they match best: 0 when they play to the sample).

Run from `C:\dev\tuple\tools\live-mcp` (its `node_modules`), e.g. `node <this folder>\live-look.mjs`.
