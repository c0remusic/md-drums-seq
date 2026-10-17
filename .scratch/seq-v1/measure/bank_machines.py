"""Each Kit of MD Drums' Bank.syx: its name and its 16 Tracks' machine ids, from the dump layout of mdkit.h.

Usage: python -I bank_machines.py Bank.syx
"""
import sys

DUMP = 0x4d1
NAME, MACHINES = 0x00a, 0x1aa
data = open(sys.argv[1], "rb").read()


def unpack7(packed, count):
	"""Elektron's 7-bit packing: a byte of high bits (bit 6 for the first) before each run of up to 7 bytes."""
	out, i = [], 0
	while len(out) < count:
		high = packed[i]
		run = packed[i + 1:i + 8]
		for k, b in enumerate(run):
			if len(out) == count:
				break
			out.append(b | (0x80 if high & (0x40 >> k) else 0))
		i += 8
	return out


kits = len(data) // DUMP
for k in range(kits):
	dump = data[k * DUMP:(k + 1) * DUMP]
	if dump[0] != 0xF0 or dump[6] != 0x52:
		print(k + 1, "not a Kit dump")
		continue
	name = bytes(dump[NAME:NAME + 16]).split(b"\0")[0].decode("latin1").rstrip()
	machines = unpack7(dump[MACHINES:], 64)
	ids = [machines[4 * t + 3] for t in range(16)]
	print("%2d %-16s %s" % (k + 1, name, " ".join("%3d" % m for m in ids)))
