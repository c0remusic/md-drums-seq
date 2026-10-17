"""Onset timing of a recording of regular hits, for the ticket "Timing et canaux du MIDI joué en direct".

Reads a WAV (PCM 16/24/32-bit or float 32/64, any channel count; the loudest channel is used), finds each hit's first
sample above a threshold after a silence, fits a straight line onset = start + k * period, and prints the residuals:
how far each hit lands from a perfectly regular grid. With --bpm and --division, the period is the grid's instead of
the fitted one, and the offset from the file's first sample is printed too (meaningful when the recording starts on a
bar line).

Usage: python -I onsets.py FILE.wav [--bpm 120] [--division 16] [--threshold 0.1] [--gap-ms 40]
Compare two files by running it on each: the reference (clip) shows Live and the instrument; the probe adds midiout.
"""
import argparse
import struct
import sys


def read_wav(path):
	data = open(path, "rb").read()
	if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
		raise SystemExit(path + ": not a RIFF WAVE file")
	pos, fmt, samples = 12, None, None
	while pos + 8 <= len(data):
		cid, size = data[pos:pos + 4], struct.unpack("<I", data[pos + 4:pos + 8])[0]
		body = data[pos + 8:pos + 8 + size]
		if cid == b"fmt ":
			tag, channels, rate = struct.unpack("<HHI", body[:8])
			bits = struct.unpack("<H", body[14:16])[0]
			if tag == 0xFFFE and len(body) >= 26:	# WAVE_FORMAT_EXTENSIBLE: the sub-format's first two bytes
				tag = struct.unpack("<H", body[24:26])[0]
			fmt = (tag, channels, rate, bits)
		elif cid == b"data":
			samples = body
		pos += 8 + size + (size & 1)
	if not fmt or samples is None:
		raise SystemExit(path + ": no fmt or data chunk")
	tag, channels, rate, bits = fmt
	width = bits // 8
	frames = len(samples) // (width * channels)
	decode = {
		(1, 16): lambda b: struct.unpack("<h", b)[0] / 32768.0,
		(1, 24): lambda b: int.from_bytes(b, "little", signed=True) / 8388608.0,
		(1, 32): lambda b: struct.unpack("<i", b)[0] / 2147483648.0,
		(3, 32): lambda b: struct.unpack("<f", b)[0],
		(3, 64): lambda b: struct.unpack("<d", b)[0],
	}.get((tag, bits))
	if decode is None:
		raise SystemExit(path + ": unsupported format tag %d, %d bits" % (tag, bits))
	chans = [[0.0] * frames for _ in range(channels)]
	step = width * channels
	for f in range(frames):
		base = f * step
		for c in range(channels):
			o = base + c * width
			chans[c][f] = decode(samples[o:o + width])
	loudest = max(chans, key=lambda ch: max(abs(v) for v in ch))
	return rate, loudest


def onsets(signal, rate, threshold, gap_ms):
	peak = max(abs(v) for v in signal) or 1.0
	level, gap = threshold * peak, int(rate * gap_ms / 1000)
	found, quiet = [], gap
	for i, v in enumerate(signal):
		if abs(v) >= level:
			if quiet >= gap:
				found.append(i)
			quiet = 0
		else:
			quiet += 1
	return found


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("wav")
	ap.add_argument("--bpm", type=float)
	ap.add_argument("--division", type=int, default=16, help="grid: 16 = sixteenth notes")
	ap.add_argument("--threshold", type=float, default=0.1, help="fraction of the file's peak")
	ap.add_argument("--gap-ms", type=float, default=40.0, help="silence needed before a new onset")
	args = ap.parse_args()

	rate, signal = read_wav(args.wav)
	hits = onsets(signal, rate, args.threshold, args.gap_ms)
	if len(hits) < 3:
		raise SystemExit("fewer than 3 onsets found: lower --threshold or --gap-ms")
	n = len(hits)
	if args.bpm:
		period = rate * 60.0 / args.bpm * 4.0 / args.division
		start = sum(h - k * period for k, h in enumerate(hits)) / n
	else:
		ks = list(range(n))
		mk, mh = sum(ks) / n, sum(hits) / n
		period = sum((k - mk) * (h - mh) for k, h in zip(ks, hits)) / sum((k - mk) ** 2 for k in ks)
		start = mh - period * mk
	res = [h - (start + k * period) for k, h in enumerate(hits)]
	mean = sum(res) / n
	sd = (sum((r - mean) ** 2 for r in res) / n) ** 0.5
	ms = 1000.0 / rate
	print("%s: %d onsets at %d Hz, period %.2f samples (%.3f ms)" % (args.wav, n, rate, period, period * ms))
	print("residuals: min %.1f, max %.1f, spread %.1f samples (%.3f ms), sd %.2f samples" % (
		min(res), max(res), max(res) - min(res), (max(res) - min(res)) * ms, sd))
	if args.bpm:
		print("first onset %d samples (%.3f ms) after the file's start; mean offset from the grid %.1f samples" % (
			hits[0], hits[0] * ms, start))
	print("per hit:", " ".join("%.0f" % r for r in res))


if __name__ == "__main__":
	sys.exit(main())
