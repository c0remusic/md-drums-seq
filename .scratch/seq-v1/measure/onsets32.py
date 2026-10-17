"""Which 32nd notes of a recording start a Hit, bar by bar, for the ticket "Banque de Patterns": a Pattern of beats
switched at a bar line to a Pattern of 32nds at 2x shows 4, then 32 onsets a bar. A 32nd starts a Hit when the peak of
its first milliseconds (after the engine's delay) is well above the end of the 32nd before it.

Usage: python -I onsets32.py FILE.wav --bpm 126 [--bars 3] [--skip 128] [--rise-db 6]
"""
import argparse
import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from onsets import read_wav  # noqa: E402


def db(value):
	return 20 * math.log10(value) if value > 0 else -200.0


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("wav")
	ap.add_argument("--bpm", type=float, required=True)
	ap.add_argument("--bars", type=int, default=3)
	ap.add_argument("--skip", type=int, default=128)
	ap.add_argument("--rise-db", type=float, default=6.0)
	args = ap.parse_args()

	rate, signal = read_wav(args.wav)
	slot = rate * 60.0 / args.bpm / 8
	look = int(rate * 0.012)

	def peak(begin, end):
		chunk = signal[max(0, int(begin)):max(0, int(end))]
		return max((abs(v) for v in chunk), default=0.0)

	print("%s: %d Hz, a 32nd = %.1f samples" % (args.wav, rate, slot))
	for bar in range(args.bars):
		marks = []
		for k in range(32):
			s = (bar * 32 + k) * slot
			attack = db(peak(s + args.skip, s + args.skip + look))
			tail = db(peak(s - look, s)) if s > 0 else -200.0
			marks.append("x" if attack > -60 and attack - tail >= args.rise_db else ".")
		line = "".join(marks)
		print("bar %d  %s  %d onsets" % (bar + 1, " ".join(line[i:i + 8] for i in range(0, 32, 8)), line.count("x")))


if __name__ == "__main__":
	main()
