"""Compares recordings of the same hits sample by sample, for the end-to-end timing (issue 04): each pair's largest
difference over their common length, in dBFS, and the lag (-8 to +8 samples) at which they match best. The files
must start on the same bar line (the live/ scripts record from one). A best lag of 0 says they play to the sample;
the difference itself is the sounds' (a velocity, an instrument's own variation).

Usage: python -I compare.py A.wav B.wav [C.wav ...]
"""
import math
import pathlib
import sys

# Run isolated, Python does not look beside the script; and it leaves no compiled copies in the source tree.
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
sys.dont_write_bytecode = True
from onsets import read_wav  # noqa: E402


def db(x):
	return 20 * math.log10(x) if x > 0 else float("-inf")


def largest_difference(a, b, lag, n):
	return max(abs(a[k] - b[k + lag]) for k in range(8, n - 8))


files = sys.argv[1:]
if len(files) < 2:
	sys.exit("usage: compare.py A.wav B.wav [C.wav ...]")
signals = {f: read_wav(f)[1] for f in files}	# the loudest channel of each
for i, a in enumerate(files):
	for b in files[i + 1:]:
		sa, sb = signals[a], signals[b]
		n = min(len(sa), len(sb)) - 16
		best = min(range(-8, 9), key=lambda lag: largest_difference(sa, sb, lag, n))
		print("%s vs %s: %d samples; largest difference %.1f dBFS at lag 0; best lag %+d (%.1f dBFS)" % (
			pathlib.Path(a).stem, pathlib.Path(b).stem, n, db(largest_difference(sa, sb, 0, n)), best,
			db(largest_difference(sa, sb, best, n))))
