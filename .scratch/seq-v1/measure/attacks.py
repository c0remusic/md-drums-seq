"""Attacks around each beat of a recording that starts on a bar: the level in blocks of 16 samples from 400 before the
beat to 3000 after it, and the attacks found in it (a block at least 9 dB above the loudest of the 8 before it, and
above -50 dBFS), in samples from the beat.

Usage: python -I attacks.py FILE.wav BPM [beats]
"""
import importlib.util
import math
import sys

spec = importlib.util.spec_from_file_location("onsets", r"C:\dev\md-drums-seq\.scratch\seq-v1\measure\onsets.py")
onsets = importlib.util.module_from_spec(spec)
spec.loader.exec_module(onsets)

path, bpm = sys.argv[1], float(sys.argv[2])
rate, mono = onsets.read_wav(path)
beat = rate * 60.0 / bpm
count = int(sys.argv[3]) if len(sys.argv) > 3 else int(len(mono) / beat)
B = 16


def db(v):
	return 20 * math.log10(max(v, 1e-9))


print("%s: %d samples, %.2f beats" % (path.split("\\")[-1], len(mono), len(mono) / beat))
for k in range(count):
	t = int(round(k * beat))
	start = max(0, t - 400)
	blocks = []
	for s in range(start, min(len(mono), t + 3000), B):
		blocks.append((s - t, max(abs(v) for v in mono[s:s + B])))
	found = []
	for i, (at, level) in enumerate(blocks):
		before = max((l for _, l in blocks[max(0, i - 8):i]), default=0.0)
		if level > 0.00316 and level >= before * 2.82 and (not found or at - found[-1][0] > 4 * B):
			# The first sample of the block over half the block's peak: the attack within 16 samples.
			s0 = at + t
			first = next(j for j in range(s0, s0 + B) if abs(mono[j]) >= level * 0.5)
			found.append((first - t, db(level)))
	print("beat %2d: attacks %s" % (k, ", ".join("%+d (%.1f dBFS)" % f for f in found) or "none"))
