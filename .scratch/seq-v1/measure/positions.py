"""Where each onset of a recording falls, in 8th notes from the file's start, with its peak: to read which Hits
sounded when a Pattern is not a regular grid (the ticket "Locks").

Usage: python -I positions.py FILE.wav --bpm 126 [--threshold 0.05] [--gap-ms 40]
"""
import argparse
import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from onsets import onsets, read_wav  # noqa: E402


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("wav")
	ap.add_argument("--bpm", type=float, required=True)
	ap.add_argument("--threshold", type=float, default=0.05)
	ap.add_argument("--gap-ms", type=float, default=40.0)
	args = ap.parse_args()

	rate, signal = read_wav(args.wav)
	eighth = rate * 60.0 / args.bpm / 2
	names = ["1", "1+", "2", "2+", "3", "3+", "4", "4+"]
	for onset in onsets(signal, rate, args.threshold, args.gap_ms):
		position = onset / eighth
		nearest = round(position)
		peak = max(abs(v) for v in signal[onset:onset + int(rate * 0.08)])
		print("%8d  8th %6.2f  bar %d beat %-2s  off %+6.1f samples  peak %6.1f dBFS" % (
			onset, position, nearest // 8 + 1, names[nearest % 8], onset - nearest * eighth, 20 * math.log10(peak)))


if __name__ == "__main__":
	main()
