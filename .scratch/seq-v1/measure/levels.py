"""Peak level of each 8th note of a recording that starts on a bar line, for the ticket "Locks".

The development build's demonstration Pattern plays Track 3 on the beats with VOL locked to 0 on 2 and 4, and Track 7
on the off-beats: where Locks reach their Track, beats 2 and 4 are near silence and 1 and 3 are not.

Usage: python -I levels.py FILE.wav --bpm 126 [--bars 2] [--window-ms 80]
"""
import argparse
import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from onsets import read_wav  # noqa: E402


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("wav")
	ap.add_argument("--bpm", type=float, required=True)
	ap.add_argument("--bars", type=int, default=2)
	ap.add_argument("--window-ms", type=float, default=80.0, help="how long after each 8th to look for its peak")
	ap.add_argument("--skip", type=int, default=0,
		help="samples skipped after each 8th: MD Drums' Hit sounds 31+ samples after its note, so the previous Hit's "
			"tail fills the first ones (128 is past it)")
	args = ap.parse_args()

	rate, signal = read_wav(args.wav)
	eighth = rate * 60.0 / args.bpm / 2
	window = int(rate * args.window_ms / 1000.0)
	print("%s: %d Hz, %.1f s, an 8th = %.1f samples" % (args.wav, rate, len(signal) / rate, eighth))
	print("bar  " + "  ".join("%-7s" % name for name in ("1", "1+", "2", "2+", "3", "3+", "4", "4+")))
	for bar in range(args.bars):
		cells = []
		for k in range(8):
			start = int(round((bar * 8 + k) * eighth)) + args.skip
			chunk = signal[start:start + window]
			peak = max((abs(v) for v in chunk), default=0.0)
			cells.append("%7.1f" % (20 * math.log10(peak)) if peak > 0 else "   -inf")
		print("%3d  %s" % (bar + 1, "  ".join(cells)))
	print("dBFS, peak within %.0f ms of each 8th" % args.window_ms)


if __name__ == "__main__":
	main()
