"""Writes md-seq-tabs-probe.amxd, a throwaway Max for Live MIDI effect: the device's live.tab drawn several ways, to see
which attribute decides the rows a live.tab lays its items out in (critique of 2026-10-16: the device's 16-item tabs
showed two rows of 8 where num_lines_presentation says 1).

A  636 x 24, 16 items, num_lines 1: the strip's Track tab as make_device.py writes it
B  as A, multiline 0
C  as A, multiline 1
D  636 x 16, num_lines 1: as A, 16 px high (the Page tab is 16 px high and shows one row)
E  as A, its items "01 BD" ... "16 SD", the names the strip would show
P  140 x 108, 16 items, num_lines 4, multiline 0: the strip's Pattern tab

Run: python -I tabs_probe.py   (writes next to this script)
"""
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
sys.dont_write_bytecode = True
from make_devices import box, patcher, write_amxd  # noqa: E402

NUMBERS = ["%02d" % (t + 1) for t in range(16)]
NAMED = ["%02d %s" % (t + 1, n) for t, n in enumerate(["BD", "SD", "XT", "CP", "RS", "CB", "CH", "OH", "CY", "MA",
	"CL", "XC", "B2", "S2", "BD", "SD"])]


def tab(box_id, present, longname, items, lines, **extra):
	return box(box_id, "live.tab", [20, 20 + 30 * int(box_id.split("-")[1]), present[2], present[3]], 1, 3,
		outlettype=["", "", "float"], present=present, parameter_enable=1, varname=longname,
		num_lines_presentation=lines, num_lines_patching=lines, spacing_x=4.0, spacing_y=4.0, active=1,
		saved_attribute_attributes={"valueof": {"parameter_longname": longname, "parameter_shortname": longname,
			"parameter_type": 2, "parameter_invisible": 2, "parameter_enum": items}}, **extra)


def comment(box_id, present, text):
	return box(box_id, "live.comment", [700, 20 + 30 * int(box_id.split("-")[1]), present[2], present[3]], 1, 0,
		text=text, present=present, fontsize=9.0, textjustification=0)


def tabs_probe():
	boxes = [
		comment("obj-1", [4, 8, 40, 12], "A"),
		tab("obj-2", [48, 4, 636, 24], "Probe A", NUMBERS, 1),
		comment("obj-3", [4, 36, 40, 12], "B ml 0"),
		tab("obj-4", [48, 32, 636, 24], "Probe B", NUMBERS, 1, multiline=0),
		comment("obj-5", [4, 64, 40, 12], "C ml 1"),
		tab("obj-6", [48, 60, 636, 24], "Probe C", NUMBERS, 1, multiline=1),
		comment("obj-7", [4, 92, 40, 12], "D 16px"),
		tab("obj-8", [48, 88, 636, 16], "Probe D", NUMBERS, 1),
		comment("obj-9", [4, 120, 40, 12], "E names"),
		tab("obj-10", [48, 116, 636, 24], "Probe E", NAMED, 1),
		comment("obj-11", [700, 148, 140, 12], "P lines 4, ml 0"),
		tab("obj-12", [700, 4, 140, 108], "Probe P", NUMBERS, 4, multiline=0),
	]
	return patcher("md-seq-tabs-probe", boxes, [], 860.0)


FULL = ["%02d %s" % (t + 1, n) for t, n in enumerate(["TRX-BD", "TRX-SD", "TRX-XT", "TRX-CP", "TRX-RS", "TRX-CB",
	"TRX-CH", "TRX-OH", "TRX-CY", "TRX-MA", "TRX-CL", "TRX-XC", "ROM-14", "ROM-06", "EFM-BD", "EFM-SD"])]
SHORT = [n.replace("TRX-", "").replace("EFM-", "").replace("ROM-", "R") for n in FULL]


def tabs_probe2():
	"""md-seq-tabs-probe2.amxd, after the first probe found multiline 0 alone gives one row: the names in one row.

	F  636 x 24, multiline 0, "01 BD": the Machine's short name
	G  636 x 24, multiline 0, "01 TRX-BD": its full name
	H  508 x 24, multiline 0, 16 numbers: the window's Pattern tab
	I  116 x 24, multiline 0, "1-32" "33-64": the window's Half tab
	J  140 x 108, multiline 1, 4 items: does a Pattern tab of four rows of one item lay out by height too
	"""
	boxes = [
		comment("obj-1", [4, 8, 40, 12], "F short"),
		tab("obj-2", [48, 4, 636, 24], "Probe F", SHORT, 1, multiline=0),
		comment("obj-3", [4, 36, 40, 12], "G full"),
		tab("obj-4", [48, 32, 636, 24], "Probe G", FULL, 1, multiline=0),
		comment("obj-5", [4, 64, 40, 12], "H win"),
		tab("obj-6", [48, 60, 508, 24], "Probe H", NUMBERS, 1, multiline=0),
		comment("obj-7", [4, 92, 40, 12], "I half"),
		tab("obj-8", [48, 88, 116, 24], "Probe I", ["1-32", "33-64"], 1, multiline=0),
		comment("obj-9", [700, 148, 140, 12], "J 4 items, ml 1"),
		tab("obj-10", [700, 4, 140, 108], "Probe J", ["01", "02", "03", "04"], 4, multiline=1),
	]
	return patcher("md-seq-tabs-probe2", boxes, [], 860.0)


if __name__ == "__main__":
	here = pathlib.Path(__file__).resolve().parent
	write_amxd(here / "md-seq-tabs-probe.amxd", tabs_probe())
	write_amxd(here / "md-seq-tabs-probe2.amxd", tabs_probe2())
	print("written:", here / "md-seq-tabs-probe.amxd", here / "md-seq-tabs-probe2.amxd")
