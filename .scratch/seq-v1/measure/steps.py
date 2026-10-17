"""Levels and onsets of the development build's 4-bar check Pattern, for the tickets "Ratchets", "Micro-timing",
"Conditions" and "Locks", from a recording that starts on a bar line one bar into the Pattern (live-record-track.mjs
starts recording on the second bar of the set).

The Pattern (%TEMP%\\mdseq-demo.txt, all on Track 3, velocity 120), in steps from the recording's start:
  0   4 ratchets falling by 75 %: velocities 120, 90, 60, 30
  8   3 ratchets rising by 100 %: velocities 1, 60, 120
  16  on the grid; 20 late by 12/24; 24 early by 12/24; 28 on the grid
  32  FILL: silent unless the Fill is held
  48  on the grid; 52 VOL locked to 0; 56 on the grid; 60 VOL locked to 0

Usage: python -I steps.py FILE.wav --bpm 126 [--skip 128]
"""
import argparse
import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from onsets import read_wav  # noqa: E402


def db(value):
	return 20 * math.log10(value) if value > 0 else float("-inf")


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("wav")
	ap.add_argument("--bpm", type=float, required=True)
	ap.add_argument("--skip", type=int, default=128,
		help="samples skipped after each slice's start: the Hit sounds 31+ samples after its note")
	args = ap.parse_args()

	rate, signal = read_wav(args.wav)
	step = rate * 60.0 / args.bpm / 4
	print("%s: %d Hz, %.2f s, a step = %.1f samples" % (args.wav, rate, len(signal) / rate, step))

	def peak(begin, end):
		chunk = signal[max(0, int(begin)):max(0, int(end))]
		return max((abs(v) for v in chunk), default=0.0)

	# Ratchets: the peak of each ratchet's share of its step.
	for first, count, velocities in ((0, 4, (120, 90, 60, 30)), (8, 3, (1, 60, 120))):
		share = step / count
		levels = [db(peak(first * step + r * share + args.skip, first * step + (r + 1) * share)) for r in range(count)]
		print("ratchets at step %d (velocities %s): %s dB" % (first, velocities,
			", ".join("%.1f" % v for v in levels)))

	# Micro-timing: each Hit's onset, the first sample above a third of its peak within a step either side of its
	# step, from the grid Hit at step 16.
	def onset(at):
		begin, end = int((at - 0.9) * step), int((at + 0.9) * step)
		top = peak(begin, end)
		for i in range(begin, end):
			if abs(signal[i]) >= top / 3:
				return i
		return None

	reference = onset(16)
	for at, expected in ((16, 0.0), (20, 0.5), (24, -0.5), (28, 0.0)):
		found = onset(at)
		offset = (found - reference) / step - (at - 16)
		print("Hit at step %d: onset %d, %+.4f steps from the grid (expected %+.3f), %+.1f samples off" % (at, found,
			offset, expected, (offset - expected) * step))

	# FILL and Locks: the peak of each step's first 80 ms.
	window = int(rate * 0.08)
	for at, what in ((28, "grid Hit before the FILL step"), (32, "FILL step"), (48, "beat 1"),
			(52, "beat 2, VOL locked to 0"), (56, "beat 3"), (60, "beat 4, VOL locked to 0")):
		print("step %d (%s): %.1f dB" % (at, what, db(peak(at * step + args.skip, at * step + args.skip + window))))


if __name__ == "__main__":
	main()
