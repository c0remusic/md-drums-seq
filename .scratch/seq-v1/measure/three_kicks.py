"""Where each of the three hits of live-three-kicks.mjs starts and peaks, against its beat in the recording.

Usage: python -I three_kicks.py FILE.wav BPM
The recording starts on the bar of the first hit; hits are at beats 0, 4 and 8 of it.
"""
import importlib.util
import math
import sys

spec = importlib.util.spec_from_file_location("onsets", r"C:\dev\md-drums-seq\.scratch\seq-v1\measure\onsets.py")
onsets = importlib.util.module_from_spec(spec)
spec.loader.exec_module(onsets)

path, bpm = sys.argv[1], float(sys.argv[2])
# onsets.read_wav gives the rate and the loudest channel.
rate, mono = onsets.read_wav(path)
beat = rate * 60.0 / bpm
print("rate", rate, "samples", len(mono), "beat", beat, "samples")


def db(v):
	return 20 * math.log10(max(abs(v), 1e-12))


for name, beats in (("909 clip on 1 KICK (DRUM BUS)", 0), ("MD Drums from a clip", 4), ("MD Drums from MD Drums Seq", 8)):
	t = int(round(beats * beat))
	lo, hi = max(0, t - 4000), min(len(mono), t + 8000)
	floor = max((abs(v) for v in mono[max(0, t - 4000):max(0, t - 200)]), default=0.0)
	first = {}
	for level in (-60, -40, -20):
		threshold = 10 ** (level / 20)
		first[level] = next((i - t for i in range(lo, hi) if abs(mono[i]) >= threshold), None)
	peak = max(range(lo, hi), key=lambda i: abs(mono[i]))
	print("%-32s floor %6.1f dBFS; first above -60 dBFS %s, -40 %s, -20 %s samples; peak %d samples (%.1f dBFS)" % (
		name, db(floor), first[-60], first[-40], first[-20], peak - t, db(mono[peak])))
