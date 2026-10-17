"""The three hits' envelopes: peak level in windows after each beat, and when each first comes within 6 and 12 dB
of its own peak (over 300 ms).

Usage: python -I three_envelopes.py FILE.wav BPM
"""
import importlib.util
import math
import sys

spec = importlib.util.spec_from_file_location("onsets", r"C:\dev\md-drums-seq\.scratch\seq-v1\measure\onsets.py")
onsets = importlib.util.module_from_spec(spec)
spec.loader.exec_module(onsets)

path, bpm, NAME = sys.argv[1], float(sys.argv[2]), sys.argv[3]
rate, mono = onsets.read_wav(path)
beat = rate * 60.0 / bpm
ms = rate / 1000.0


def db(v):
	return 20 * math.log10(max(abs(v), 1e-12))


edges = [0, 1, 2, 5, 10, 20, 50, 100, 200, 300]
print("window (ms)                     " + "".join("%8s" % ("%d-%d" % (a, b)) for a, b in zip(edges, edges[1:])))
for name, beats in ((NAME, 0), (NAME + " beat 2", 2)):
	t = int(round(beats * beat))
	cells = []
	for a, b in zip(edges, edges[1:]):
		seg = mono[t + int(a * ms):t + int(b * ms)]
		cells.append("%8.1f" % db(max((abs(v) for v in seg), default=0.0)))
	whole = mono[t:t + int(300 * ms)]
	peak = max(abs(v) for v in whole)
	within = {}
	for d in (12, 6):
		level = peak * 10 ** (-d / 20)
		within[d] = next(i for i, v in enumerate(whole) if abs(v) >= level) / ms
	print("%-32s%s   peak %.1f dBFS; within 12 dB at %.2f ms, within 6 dB at %.2f ms" % (
		name, "".join(cells), db(peak), within[12], within[6]))
