"""Peak level of each beat of a recording that starts on a bar line, for the Accent (issue 26).

The development build's check Pattern plays Track 1 on the four beats at velocity 127, beats 3 and 4 in the Accent
row at an amount of 64: MD Drums plays those at 128 + 2 x 64 = 256, about twice velocity 127 (+6 dB), beats 1 and 2
at 127.

Usage: python -I accent_levels.py FILE.wav --bpm 126 [--bars 2] [--window-ms 80]
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
	ap.add_argument("--window-ms", type=float, default=80.0)
	args = ap.parse_args()

	rate, signal = read_wav(args.wav)
	beat = rate * 60.0 / args.bpm
	window = int(rate * args.window_ms / 1000.0)
	print("%s: %d Hz, %.2f s" % (args.wav, rate, len(signal) / rate))
	plain, accented = [], []
	for b in range(4 * args.bars):
		start = int(b * beat)
		peak = max((abs(v) for v in signal[start:start + window]), default=0.0)
		(accented if b % 4 >= 2 else plain).append(peak)
		print("beat %d%s: peak %.4f (%.1f dB)" % (b % 4 + 1, " accented" if b % 4 >= 2 else "", peak,
			20 * math.log10(peak) if peak > 0 else float("-inf")))
	if plain and accented and min(plain) > 0:
		ratio = (sum(accented) / len(accented)) / (sum(plain) / len(plain))
		print("accented / plain: %.3f (%.2f dB); 256 / 127 = %.3f" % (ratio, 20 * math.log10(ratio), 256 / 127))


if __name__ == "__main__":
	main()
