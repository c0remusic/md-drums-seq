"""Writes the two throwaway Max for Live MIDI effects of the ticket "Timing et canaux du MIDI joué en direct".

md-seq-probe.amxd     while Live plays, note 36 (velocity 100, 30 ms) on every 16th note, the MIDI channel stepping
                      1, 2, ... 16, 1 ...; incoming MIDI passes through.
md-seq-channels.amxd  lights one cell per MIDI channel it receives (1-16), shows the last one; Clear resets; incoming
                      MIDI passes through.

The .amxd container is the one Live 12's own "Max MIDI Effect.amxd" uses: "ampf", u32 4, "mmmm" (MIDI effect),
"meta", u32 4, 4 zero bytes, "ptch", u32 length, then the patcher JSON ending in a NUL.

Run: python -I make_devices.py   (writes next to this script)
"""
import json
import pathlib
import struct


def box(box_id, maxclass, rect, inlets, outlets, text=None, outlettype=None, present=None, **extra):
	b = {"id": box_id, "maxclass": maxclass, "numinlets": inlets, "numoutlets": outlets, "patching_rect": rect}
	if text is not None:
		b["text"] = text
	if outlettype is not None:
		b["outlettype"] = outlettype
	if present is not None:
		b["presentation"] = 1
		b["presentation_rect"] = present
	b.update(extra)
	return {"box": b}


def line(src, outlet, dst, inlet):
	return {"patchline": {"source": [src, outlet], "destination": [dst, inlet]}}


def patcher(title, boxes, lines, width):
	return {"patcher": {
		"fileversion": 1,
		"appversion": {"major": 9, "minor": 0, "revision": 7, "architecture": "x64", "modernui": 1},
		"classnamespace": "box",
		"rect": [100.0, 100.0, 640.0, 420.0],
		"openrect": [0.0, 0.0, width, 169.0],
		"openinpresentation": 1,
		"default_fontsize": 10.0,
		"default_fontface": 0,
		"default_fontname": "Arial Bold",
		"gridonopen": 1,
		"gridsize": [8.0, 8.0],
		"devicewidth": width,
		"description": "",
		"digest": "",
		"tags": "",
		"style": "",
		"title": title,
		"boxes": boxes,
		"lines": lines,
		"dependency_cache": [],
		"latency": 0,
		"project": {
			"version": 1, "creationdate": 3590052786, "modificationdate": 3590052786,
			"viewrect": [0.0, 0.0, 300.0, 500.0], "autoorganize": 1, "hideprojectwindow": 1, "showdependencies": 1,
			"autolocalize": 0, "contents": {"patchers": {}}, "layout": {}, "searchpath": {}, "detailsvisible": 0,
			"amxdtype": 1835887981, "readonly": 0, "devpathtype": 0, "devpath": ".", "sortmode": 0, "viewmode": 0,
		},
		"autosave": 0,
	}}


def write_amxd(path, patch):
	body = json.dumps(patch, indent="\t").encode("utf-8") + b"\n\x00"
	data = b"ampf" + struct.pack("<I", 4) + b"mmmm" + b"meta" + struct.pack("<I", 4) + b"\x00\x00\x00\x00"
	data += b"ptch" + struct.pack("<I", len(body)) + body
	path.write_bytes(data)


def probe():
	boxes = [
		box("obj-1", "newobj", [20, 20, 40, 20], 1, 1, "midiin", ["int"]),
		box("obj-2", "newobj", [20, 300, 47, 20], 1, 0, "midiout"),
		box("obj-3", "newobj", [120, 20, 330, 20], 2, 1, "metro 16n @quantize 16n @active 1 @autostart 1 @autostarttime 0",
			["bang"]),
		box("obj-4", "newobj", [120, 60, 80, 20], 5, 4, "counter 0 15", ["int", "", "", "int"]),
		box("obj-5", "newobj", [120, 100, 40, 20], 1, 2, "t b i", ["bang", "int"]),
		box("obj-6", "newobj", [220, 140, 30, 20], 2, 1, "+ 1", ["int"]),
		box("obj-7", "message", [120, 140, 50, 20], 2, 1, "36 100", [""]),
		box("obj-8", "newobj", [120, 180, 100, 20], 3, 2, "makenote 100 30", ["float", "float"]),
		box("obj-9", "newobj", [120, 220, 60, 20], 2, 1, "pack 0 0", [""]),
		box("obj-10", "newobj", [120, 260, 120, 20], 7, 1, "midiformat", ["int"]),
		box("obj-11", "number", [260, 180, 50, 22], 1, 2, None, ["", "bang"], present=[110, 40, 50, 22]),
		box("obj-12", "live.comment", [320, 100, 260, 18], 1, 0, "md-seq-probe: note 36 every 16th",
			present=[8, 8, 260, 18]),
		box("obj-13", "live.comment", [320, 120, 260, 18], 1, 0, "channel now:", present=[8, 42, 100, 18]),
		box("obj-14", "live.comment", [320, 140, 300, 18], 1, 0, "steps channels 1-16 while Live plays",
			present=[8, 76, 260, 18]),
	]
	lines = [
		line("obj-1", 0, "obj-2", 0),
		line("obj-3", 0, "obj-4", 0),
		line("obj-4", 0, "obj-5", 0),
		line("obj-5", 1, "obj-6", 0),
		line("obj-6", 0, "obj-10", 6),
		line("obj-6", 0, "obj-11", 0),
		line("obj-5", 0, "obj-7", 0),
		line("obj-7", 0, "obj-8", 0),
		line("obj-8", 1, "obj-9", 1),
		line("obj-8", 0, "obj-9", 0),
		line("obj-9", 0, "obj-10", 0),
		line("obj-10", 0, "obj-2", 0),
	]
	return patcher("md-seq-probe", boxes, lines, 280.0)


def channels():
	boxes = [
		box("obj-1", "newobj", [20, 20, 40, 20], 1, 1, "midiin", ["int"]),
		box("obj-2", "newobj", [20, 300, 47, 20], 1, 0, "midiout"),
		box("obj-3", "newobj", [120, 60, 160, 20], 1, 8, "midiparse", ["", "", "", "int", "int", "int", "int", ""]),
		box("obj-4", "newobj", [120, 100, 30, 20], 2, 1, "- 1", ["int"]),
		box("obj-5", "newobj", [120, 140, 70, 20], 3, 1, "pack 0 0 1", [""]),
		box("obj-6", "matrixctrl", [120, 180, 320, 20], 1, 2, None, ["list", "list"],
			present=[8, 40, 320, 20], columns=16, rows=1),
		box("obj-7", "number", [300, 100, 50, 22], 1, 2, None, ["", "bang"], present=[110, 72, 50, 22]),
		box("obj-8", "message", [300, 140, 40, 20], 2, 1, "clear", [""], present=[8, 104, 48, 20]),
		box("obj-9", "live.comment", [460, 20, 300, 18], 1, 0, "md-seq-channels: MIDI channels received 1-16",
			present=[8, 8, 320, 18]),
		box("obj-10", "live.comment", [460, 40, 120, 18], 1, 0, "last channel:", present=[8, 74, 100, 18]),
	]
	lines = [
		line("obj-1", 0, "obj-2", 0),
		line("obj-1", 0, "obj-3", 0),
		line("obj-3", 6, "obj-4", 0),
		line("obj-3", 6, "obj-7", 0),
		line("obj-4", 0, "obj-5", 0),
		line("obj-5", 0, "obj-6", 0),
		line("obj-8", 0, "obj-6", 0),
	]
	return patcher("md-seq-channels", boxes, lines, 340.0)


def numbox(box_id, rect, present, longname):
	return box(box_id, "live.numbox", rect, 1, 2, None, ["", "float"], present, parameter_enable=1,
		varname=longname, saved_attribute_attributes={"valueof": {
			"parameter_longname": longname, "parameter_shortname": longname, "parameter_type": 1,
			"parameter_mmin": 0, "parameter_mmax": 16, "parameter_initial_enable": 1, "parameter_initial": [0],
			"parameter_invisible": 0}})


def channel_probe():
	"""md-seq-channel-probe.amxd: the MIDI channel of the last Hit (notes 36-51) and of the last Lock (notes 64-87)
	it sees, as two Live parameters that LOM reads; incoming MIDI passes through."""
	boxes = [
		box("obj-1", "newobj", [20, 20, 40, 20], 1, 1, "midiin", ["int"]),
		box("obj-2", "newobj", [20, 400, 47, 20], 1, 0, "midiout"),
		box("obj-3", "newobj", [120, 60, 160, 20], 1, 8, "midiparse", ["", "", "", "int", "int", "int", "int", ""]),
		box("obj-4", "newobj", [120, 120, 240, 20], 2, 1, "expr ($i1 >= 64) * ($i1 <= 87) * ($i2 > 0)", [""]),
		box("obj-5", "newobj", [380, 120, 240, 20], 2, 1, "expr ($i1 >= 36) * ($i1 <= 51) * ($i2 > 0)", [""]),
		box("obj-6", "newobj", [120, 160, 40, 20], 2, 2, "sel 1", ["bang", ""]),
		box("obj-7", "newobj", [380, 160, 40, 20], 2, 2, "sel 1", ["bang", ""]),
		box("obj-8", "newobj", [120, 200, 30, 20], 2, 1, "i", ["int"]),
		box("obj-9", "newobj", [380, 200, 30, 20], 2, 1, "i", ["int"]),
		numbox("obj-10", [120, 240, 44, 15], [8, 30, 44, 15], "Lock Channel"),
		numbox("obj-11", [380, 240, 44, 15], [8, 60, 44, 15], "Hit Channel"),
		box("obj-12", "live.comment", [460, 20, 200, 18], 1, 0, "Lock / Hit channel", present=[8, 8, 120, 18]),
	]
	lines = [
		line("obj-1", 0, "obj-2", 0),
		line("obj-1", 0, "obj-3", 0),
		line("obj-3", 6, "obj-8", 1),
		line("obj-3", 6, "obj-9", 1),
		line("obj-3", 0, "obj-4", 0),
		line("obj-3", 0, "obj-5", 0),
		line("obj-4", 0, "obj-6", 0),
		line("obj-5", 0, "obj-7", 0),
		line("obj-6", 0, "obj-8", 0),
		line("obj-7", 0, "obj-9", 0),
		line("obj-8", 0, "obj-10", 0),
		line("obj-9", 0, "obj-11", 0),
	]
	return patcher("md-seq-channel-probe", boxes, lines, 140.0)


def banks_probe():
	"""md-seq-banks-probe.amxd: three numboxes, one per Live parameter visibility (Automated, Stored Only, Hidden), in
	two parameter banks, to read what Live hands Push (Device.parameters, get_bank_count, get_bank_parameters);
	incoming MIDI passes through."""
	names = [("Auto Knob", 0), ("Stored Knob", 1), ("Hidden Knob", 2)]
	boxes = [
		box("obj-1", "newobj", [20, 20, 40, 20], 1, 1, "midiin", ["int"]),
		box("obj-2", "newobj", [20, 400, 47, 20], 1, 0, "midiout"),
		box("obj-9", "live.comment", [460, 20, 200, 18], 1, 0, "Auto / Stored / Hidden", present=[8, 8, 160, 18]),
	]
	registry = {}
	for i, (name, visibility) in enumerate(names):
		box_id = "obj-%d" % (10 + i)
		b = numbox(box_id, [120, 100 + 40 * i, 44, 15], [8, 30 + 30 * i, 44, 15], name)
		b["box"]["saved_attribute_attributes"]["valueof"]["parameter_invisible"] = visibility
		boxes.append(b)
		registry[box_id] = [name, name, 0]
	registry["parameterbanks"] = {
		"0": {"index": 0, "name": "Probe", "parameters": ["Auto Knob", "Stored Knob", "Hidden Knob"] + ["-"] * 5},
		"1": {"index": 1, "name": "Not automated", "parameters": ["Stored Knob", "Hidden Knob"] + ["-"] * 6},
	}
	registry["inherited_shortname"] = 1
	patch = patcher("md-seq-banks-probe", boxes, [line("obj-1", 0, "obj-2", 0)], 180.0)
	patch["patcher"]["parameters"] = registry
	return patch


if __name__ == "__main__":
	here = pathlib.Path(__file__).resolve().parent
	write_amxd(here / "md-seq-probe.amxd", probe())
	write_amxd(here / "md-seq-channels.amxd", channels())
	write_amxd(here / "md-seq-channel-probe.amxd", channel_probe())
	write_amxd(here / "md-seq-banks-probe.amxd", banks_probe())
	print("written:", here / "md-seq-probe.amxd", here / "md-seq-channels.amxd")
