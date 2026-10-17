"""Checks the device make_device.py wrote, as far as can be checked without Max: the container, every patch line
between existing boxes and within their inlets and outlets, every receive with a send and the other way round,
unique Live parameter names, every presented box inside its surface (the strip, or the edit window), and no control
that edits the Pattern sending an initial value when the device loads.

Against the vocabulary the core defines (source/mdseq/mdseqProtocol.h, the JSON tools/mdseqVocabulary writes), it also
checks both ends of the external: every message sent to ---in has a selector the external registers and the integers
it reads before the control's value (a Mute's Track, a Lock's parameter), a message box all those it needs (a Mute's
state too), and every one it registers is sent; the
route on ---panel lists exactly the panel keys the external sends, an indexed key by its first
integer to as many receives as the core counts, and every key reaches a receive. The external's methods and the
Editor's keys come from the same table (mdseqProtocolTest), so a selector or a key the two sides do not share fails
here rather than in Live, as "doesn't understand".

Run: python -I check_device.py <device.amxd> <vocabulary.json>; exits 1 on the first list of problems.
"""
import json
import pathlib
import struct
import sys


def read(path):
	data = path.read_bytes()
	if data[:4] != b"ampf" or data[8:12] != b"mmmm" or data[24:28] != b"ptch":
		raise ValueError("not a Max MIDI Effect container")
	(length,) = struct.unpack("<I", data[28:32])
	body = data[32:32 + length]
	if not body.endswith(b"\n\x00"):
		raise ValueError("patcher JSON does not end in a NUL")
	return json.loads(body[:-1].decode("utf-8"))["patcher"]


def check(patcher, where, width, height, edits, problems, sends, receives, names, patchers):
	boxes = {e["box"]["id"]: e["box"] for e in patcher["boxes"]}
	patchers.append((where, boxes, patcher["lines"]))
	for entry in patcher["lines"]:
		(src, outlet), (dst, inlet) = entry["patchline"]["source"], entry["patchline"]["destination"]
		if src not in boxes or dst not in boxes:
			problems.append("%s: line %s -> %s between missing boxes" % (where, src, dst))
			continue
		if outlet >= boxes[src]["numoutlets"]:
			problems.append("%s: %s has no outlet %d" % (where, boxes[src].get("text", src), outlet))
		if inlet >= boxes[dst]["numinlets"]:
			problems.append("%s: %s has no inlet %d" % (where, boxes[dst].get("text", dst), inlet))

	for b in boxes.values():
		text = b.get("text", "")
		if b["maxclass"] == "newobj" and text.startswith("s "):
			sends.add(text[2:])
		if b["maxclass"] == "newobj" and text.startswith("r "):
			receives.add(text[2:])
		attrs = b.get("saved_attribute_attributes", {}).get("valueof", {})
		if "parameter_longname" in attrs:
			name = attrs["parameter_longname"]
			if name in names:
				problems.append("%s: parameter name %r used twice" % (where, name))
			names.add(name)
		if b.get("presentation"):
			x, y, w, h = b["presentation_rect"]
			if x < 0 or y < 0 or x + w > width or y + h > height:
				problems.append("%s: %s %s outside %dx%d" % (where, b["maxclass"], b["presentation_rect"], width,
					height))
		if "patcher" in b:
			# A subpatcher is the edit window, its size in its rect: x, y, width, height.
			_, _, w, h = b["patcher"]["rect"]
			check(b["patcher"], where + "/" + text, w, h, edits, problems, sends, receives, names, patchers)

	# A control that edits the Pattern must not send an initial value: a live.* object sends it when the device loads,
	# and it would edit the Pattern (measured in Live: "hit 0" took a Hit off). Unless a gate that ---loaded opens
	# after the load stands between them (make_device.py's guarded()): the trace stops there.
	downstream = {}
	for entry in patcher["lines"]:
		downstream.setdefault(entry["patchline"]["source"][0], []).append(entry["patchline"]["destination"][0])
	loaded = {i for i, b in boxes.items() if b.get("text") == "r ---loaded"}
	guards = {i for i, b in boxes.items() if b.get("text", "").startswith("gate ")
		and any(l in loaded and i in downstream.get(l, []) for l in loaded)}
	for b in boxes.values():
		attrs = b.get("saved_attribute_attributes", {}).get("valueof", {})
		if not attrs.get("parameter_initial_enable"):
			continue
		seen, frontier = set(), [b["id"]]
		for _ in range(3):
			frontier = [d for f in frontier if f not in guards for d in downstream.get(f, []) if d not in seen]
			seen.update(frontier)
		for d in seen:
			words = boxes[d].get("text", "").split()
			if len(words) >= 2 and words[0] == "prepend" and words[1] in edits:
				problems.append("%s: %r sends an initial value into %r" % (where, attrs["parameter_longname"],
					boxes[d]["text"]))


def into(boxes, lines, dst):
	"""The boxes with a line into dst."""
	return [boxes[l["patchline"]["source"][0]] for l in lines if l["patchline"]["destination"][0] == dst
		and l["patchline"]["source"][0] in boxes]


def out_of(boxes, lines, src, outlet):
	"""The boxes a line from src's outlet reaches."""
	return [boxes[l["patchline"]["destination"][0]] for l in lines if l["patchline"]["source"] == [src, outlet]
		and l["patchline"]["destination"][0] in boxes]


def message_problem(message, integers, prepend):
	"""What is wrong with a message to the external, None when nothing is: a prepend gives exactly the integers the
	external reads before the control's value (a Mute's Track, a Lock's parameter); a message box, which carries the
	value too, at least as many as it needs (a Mute's Track and state). make_device.py holds its messages to this too."""
	numbers = all(w.lstrip("-").isdigit() for w in integers)
	if prepend and (not numbers or len(integers) != message["fixed"]):
		return "%d integers before the value; the external reads %d" % (len(integers), message["fixed"])
	if not prepend and (not numbers or len(integers) < message["needs"]):
		return "%d integers; the external needs %d" % (len(integers), message["needs"])
	return None


def into_core(boxes, lines):
	"""The boxes whose messages reach the external: those with a line into "s ---in", and, in the patcher that holds
	the external (the box "r ---in" feeds), those with a line straight into it."""
	targets = [b["id"] for b in boxes.values() if b["maxclass"] == "newobj" and b.get("text") == "s ---in"]
	receive = [b["id"] for b in boxes.values() if b["maxclass"] == "newobj" and b.get("text") == "r ---in"]
	cores = [c["id"] for r in receive for c in out_of(boxes, lines, r, 0)]
	return [src for t in targets + cores for src in into(boxes, lines, t) if src["id"] not in receive]


def check_messages(patchers, vocabulary, problems):
	"""Every message to the external starts with a selector it registers, from a prepend or a message box, with the
	integers message_problem asks for. Every selector it registers is sent."""
	info = {m["selector"]: m for m in vocabulary["messages"]}
	sent = set()
	for where, boxes, lines in patchers:
		for src in into_core(boxes, lines):
			words = src.get("text", "").split()
			prepend = src["maxclass"] == "newobj" and words[:1] == ["prepend"] and len(words) >= 2
			if prepend:
				words = words[1:]
			elif src["maxclass"] != "message" or not words:
				problems.append("%s: %r sends the external no selector" % (where, src.get("text", src["maxclass"])))
				continue
			name, integers = words[0], words[1:]
			sent.add(name)
			if name not in info:
				problems.append("%s: %r sends the external %r, a message it does not register" % (where,
					src["text"], name))
				continue
			problem = message_problem(info[name], integers, prepend)
			if problem:
				problems.append("%s: %r sends the external %r with %s" % (where, src["text"], name, problem))
	for name in info:
		if name not in sent:
			problems.append("the external registers %r, which nothing sends it" % name)


def check_panel(patchers, vocabulary, receives, problems):
	"""The route on "r ---panel" lists exactly the panel keys the external sends, each reaching a receive: an indexed
	key through a route of its indices, one receive each."""
	declared = {k["key"]: k["indexed"] for k in vocabulary["panel"]}
	routes = []
	for where, boxes, lines in patchers:
		for b in boxes.values():
			if b["maxclass"] == "newobj" and b.get("text") == "r ---panel":
				routes += [(where, boxes, lines, r) for r in out_of(boxes, lines, b["id"], 0)]
	if len(routes) != 1 or not routes[0][3].get("text", "").startswith("route "):
		problems.append("the panel outlet reaches %d boxes; one route expected" % len(routes))
		return
	where, boxes, lines, route = routes[0]
	keys = route["text"].split()[1:]
	for key in keys:
		if key not in declared:
			problems.append("%s: the panel route lists %r, a key the external does not send" % (where, key))
	for key, count in declared.items():
		if key not in keys:
			problems.append("%s: the external sends the panel key %r, which the route does not list" % (where, key))
			continue
		# A send on each receive the key reaches: its own, or, indexed, one per index.
		targets = out_of(boxes, lines, route["id"], keys.index(key))
		if count:
			indices = [str(n) for n in range(count)]
			by_index = [t for t in targets if t.get("text", "").split() == ["route"] + indices]
			if not by_index:
				problems.append("%s: the panel key %r is not routed by its %d indices" % (where, key, count))
				continue
			targets = [t for n in range(count) for t in out_of(boxes, lines, by_index[0]["id"], n)]
			if len(targets) != count:
				problems.append("%s: the panel key %r reaches %d of its %d receives" % (where, key, len(targets),
					count))
		names = [t["text"][2:] for t in targets if t.get("text", "").startswith("s ")]
		if not names:
			problems.append("%s: the panel key %r is routed to no send" % (where, key))
		for name in names:
			if name not in receives:
				problems.append("%s: the panel key %r goes to %s, which nothing receives" % (where, key, name))


def main():
	patcher = read(pathlib.Path(sys.argv[1]))
	vocabulary = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))
	# The core's messages that edit a Pattern, as isPatternEdit says of what each forwards
	edits = {m["selector"] for m in vocabulary["messages"] if m["edits"]}
	problems, sends, receives, names, patchers = [], set(), set(), set(), []
	# The strip's size, as Live opens the device
	check(patcher, "device", patcher["devicewidth"], patcher["openrect"][3], edits, problems, sends, receives, names,
		patchers)
	for name in sorted(receives - sends):
		problems.append("receive %s has no send" % name)
	for name in sorted(sends - receives):
		problems.append("send %s has no receive" % name)
	check_messages(patchers, vocabulary, problems)
	check_panel(patchers, vocabulary, receives, problems)
	if problems:
		print("\n".join(problems))
		sys.exit(1)
	print("device OK: %d parameters, %d send names, %d messages, %d panel keys" % (len(names), len(sends),
		len(vocabulary["messages"]), len(vocabulary["panel"])))


if __name__ == "__main__":
	main()
