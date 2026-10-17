"""Writes the device MD Drums Seq.amxd: the approved v5 interface (Design/HANDOFF.md) over the external mdseq.

The strip (992 x 169) plays, the edit window (1440 x 868, a subpatcher opened by Edit) edits. Both lcd objects are
drawn by mdseq; the patch only routes, through device-local send and receive names (---). Controls of what the
sequencer does not do yet (capture) are in place and inactive. The Pattern, the Fill and the Mutes are Live
parameters; every other control is a hidden parameter: neither automated, nor stored, nor in Live's undo history.

The device loads mdseq.mxe64 from its own folder: the build writes both into build/device.

The messages the patch sends the external, the panel keys it routes from it, and the menus whose indices the core
reads come from the vocabulary the core defines (source/mdseq/mdseqProtocol.h), which the build writes as JSON before
the device (tools/mdseqVocabulary).

The .amxd container is the one Live 12's own "Max MIDI Effect.amxd" uses: "ampf", u32 4, "mmmm" (MIDI effect),
"meta", u32 4, 4 zero bytes, "ptch", u32 length, then the patcher JSON ending in a NUL.

Run: python -I make_device.py <output.amxd> <vocabulary.json> [<external class, mdseq by default>]
"""
import json
import pathlib
import struct
import sys

# Run isolated, Python does not look beside the script; and it leaves no compiled copies in the source tree.
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
sys.dont_write_bytecode = True
from check_device import message_problem  # noqa: E402

# The external's Max class: a development build takes a new one, since Max never reloads a class it has loaded.
CLASS = "mdseq"

STRIP_W, STRIP_H = 992, 169
WINDOW_W, WINDOW_H = 1440, 868

# The vocabulary between the patch and the external, as the core's JSON gives it (load_vocabulary): "messages", each
# selector the external registers, whether it "edits" a Pattern, the integers a prepend "fixes" before the control's
# value and the fewest integers the external "needs" (the value too, for a Mute or a Lock); "panel", each key the
# external sends and how many receives an indexed one's first integer picks among; the "scales" and "conditions" menus
# in the core's index order; "noLock"; and the "limits" the menus and rows count.
VOCABULARY = None


def load_vocabulary(path):
	global VOCABULARY
	VOCABULARY = json.loads(pathlib.Path(path).read_text(encoding="utf-8"))


def limit(name):
	return VOCABULARY["limits"][name]


def selector(text, prepend=True):
	"""text, a message whose selector the external registers, then integers: in a prepend, exactly those it fixes
	before the control's value (a Mute's Track, a Lock's parameter); in a message box, which carries the value too, at
	least as many as the external needs (a Mute's Track and state). Otherwise it would show only in Live, as "doesn't
	understand" or as a Mute or a Lock refused."""
	words = text.split()
	message = next((m for m in VOCABULARY["messages"] if m["selector"] == words[0]), None)
	if message is None:
		raise ValueError("the external registers no message %r" % words[0])
	problem = message_problem(message, words[1:], prepend)
	if problem:
		raise ValueError("%r: %s" % (text, problem))
	return text


# Lock rows of the step panel (MD Drums' notes 64-87, ADR 0003)
SYN = ["SYN1", "SYN2", "SYN3", "SYN4", "SYN5", "SYN6", "SYN7", "SYN8"]
EFX = ["AMD", "AMF", "EQF", "EQG", "FLTF", "FLTW", "FLTQ", "SRR"]
ROUTING = ["DIST", "VOL", "PAN", "DEL", "REV", "LFOS", "LFOD", "LFOM"]

RULE = [0.35, 0.35, 0.35, 1.0]


def rgb(hex_colour):
	return [round(int(hex_colour[i:i + 2], 16) / 255, 6) for i in (1, 3, 5)] + [1.0]


# The mockup's colours (Design/v5-style-guide.md) where a live.* object's own, Live's theme, would differ from it
WINDOW_BG = rgb("#383838")		# the device face: cells are wells below it
WELL = rgb("#1e1e1e")			# a control's well, and the ink on a lit control
ACTIVE = rgb("#f2d33a")			# a Track that plays: Live's activator yellow
MUTED = rgb("#4a4a4a")
DARK_BORDER = rgb("#121212")
TEXT = rgb("#dadada")
TEXT_DIM = rgb("#a2a2a2")
TEXT_FAINT = rgb("#767676")
ACCENT = rgb("#7cc6ff")			# the step selected, Locks

# Live's parameter visibility
AUTOMATED, STORED_ONLY, HIDDEN = 0, 1, 2


class Patch:
	"""Boxes and lines of one patcher, with ids made here; z-order is the order of creation."""

	def __init__(self, prefix):
		self.prefix = prefix
		self.boxes = []
		self.lines = []
		self.count = 0
		self.y = 20	# patching-view placement, one box per row, for whoever opens the patch
		self.core = None	# the external's box, in the patcher that holds it
		self.to_core = None	# elsewhere, the one send its messages go through
		self.loaded = None	# the receive that opens the guarded() gates once the device has loaded

	def add(self, maxclass, inlets, outlets, text=None, outlettype=None, present=None, rect=None, **extra):
		self.count += 1
		box_id = "%s-%d" % (self.prefix, self.count)
		if rect is None:
			width = present[2] if present else 120
			height = present[3] if present else 20
			rect = [20 + 400 * (self.count % 3), self.y, width, height]
			self.y += max(24, min(height, 120) + 4)
		b = {"id": box_id, "maxclass": maxclass, "numinlets": inlets, "numoutlets": outlets, "patching_rect": rect}
		if text is not None:
			b["text"] = text
		if outlettype is not None:
			b["outlettype"] = outlettype
		if present is not None:
			b["presentation"] = 1
			b["presentation_rect"] = present
		b.update(extra)
		self.boxes.append({"box": b})
		return box_id

	def obj(self, text, inlets=1, outlets=1, outlettype=None):
		return self.add("newobj", inlets, outlets, text, outlettype or [""] * outlets)

	def message(self, text):
		return self.add("message", 2, 1, text, [""])

	def line(self, src, outlet, dst, inlet=0):
		self.lines.append({"patchline": {"source": [src, outlet], "destination": [dst, inlet]}})

	def send(self, src, outlet, name):
		s = self.obj("s " + name, 1, 0)
		self.line(src, outlet, s)

	def receive(self, name, dst, inlet=0):
		r = self.obj("r " + name, 0, 1)
		self.line(r, 0, dst, inlet)

	# ---- interface objects ----

	def comment(self, x, y, w, h, text, size=9.0, receive=None, **extra):
		box = self.add("live.comment", 1, 0, text, None, [x, y, w, h], fontsize=size, textjustification=0, **extra)
		if receive:
			set_from(self, receive, box)
		return box

	def rule(self, x, y, w, h, colour=None, **extra):
		return self.add("panel", 1, 0, None, None, [x, y, w, h], bgcolor=colour or RULE, rounded=0, **extra)


def valueof(longname, shortname, ptype, visibility, enum=None, mmin=None, mmax=None, initial=None):
	v = {"parameter_longname": longname, "parameter_shortname": shortname, "parameter_type": ptype,
		"parameter_invisible": visibility}
	if enum is not None:
		v["parameter_enum"] = enum
	if mmin is not None:
		v["parameter_mmin"] = mmin
	if mmax is not None:
		v["parameter_mmax"] = mmax
	if initial is not None:
		v["parameter_initial_enable"] = 1
		v["parameter_initial"] = [initial]
	return {"valueof": v}


def tab(p, present, longname, items, visibility=HIDDEN, lines=1, initial=0):
	"""A live.tab of one row, or of 2 x lines rows (measured in Live, issue 15: multiline at 1, its default, lays out
	twice num_lines rows; at 0, one row whatever num_lines)."""
	return p.add("live.tab", 1, 3, None, ["", "", "float"], present, parameter_enable=1, varname=longname,
		num_lines_presentation=lines, num_lines_patching=lines, multiline=0 if lines == 1 else 1, spacing_x=4.0,
		spacing_y=4.0, active=1,
		saved_attribute_attributes=valueof(longname, longname, 2, visibility, enum=items, initial=initial))


def text_button(p, present, longname, text, toggle, visibility=HIDDEN, initial=0, active=1, **extra):
	# A live.text's "text" attribute is the box's own "text" key.
	return p.add("live.text", 1, 2, text, ["", ""], present, parameter_enable=1, varname=longname,
		mode=1 if toggle else 0, texton=text, active=active,
		saved_attribute_attributes=valueof(longname, longname, 2, visibility, enum=["off", "on"], mmax=1,
			initial=initial), **extra)


# A live.menu draws its text at the top of a box about 15 px high, whatever its own (measured in Live on 2026-10-17:
# its text 4 px above that of a button beside it): the mockup's well is a panel, the menu centred in it.
MENU_HEIGHT = 16


def menu(p, present, longname, items, initial=0):
	x, y, w, h = present
	# On the background layer, as the window's face: in front, it hid the menu (seen in Live).
	p.add("panel", 1, 0, None, None, present, bgcolor=WELL, border=1, bordercolor=DARK_BORDER, rounded=2,
		background=1)
	return p.add("live.menu", 1, 3, None, ["", "", "float"], [x + 2, y + (h - MENU_HEIGHT) // 2, w - 4, MENU_HEIGHT],
		parameter_enable=1, varname=longname, active=1, activebgcolor=WELL, bordercolor=WELL, textcolor=TEXT,
		tricolor=TEXT_FAINT,
		saved_attribute_attributes=valueof(longname, longname, 2, HIDDEN, enum=items, initial=initial))


def dial(p, present, longname, shortname, mmin, mmax, initial):
	# The dial alone: its name and value are comments beside it, as the Locks' are. Its arc in the accent, on the
	# mockup's ring, rather than Live's own colour.
	return p.add("live.dial", 1, 2, None, ["", "float"], present, parameter_enable=1, varname=longname,
		active=1, showname=0, shownumber=0, activedialcolor=ACCENT, activefgdialcolor=MUTED, activeneedlecolor=TEXT,
		saved_attribute_attributes=valueof(longname, shortname, 1, HIDDEN, mmin=mmin, mmax=mmax, initial=initial))


def lock_items():
	"""A Lock's dial: 128 positions, each the value it locks, but the middle one, where NoLock (63) would be (63 cannot
	travel to MD Drums), which is the Kit. Without a Lock the dial sits unlit on the Kit's value, so that turning it
	starts the Lock there; double-clicking goes back to the Kit."""
	return ["Kit" if v == VOCABULARY["noLock"] else str(v) for v in range(128)]


def lock_dial(p, present, longname, shortname):
	# No number on the dial: the comment beside it says what it holds, the Lock or the Kit's value. Always active: an
	# inactive live.dial sends nothing when turned (Max's reference; seen in Live: no Lock was ever made), so a Lock
	# shows by its colours instead: without one, its arc (activedialcolor) the ring's colour and a faint needle.
	return p.add("live.dial", 1, 2, None, ["", "float"], present, parameter_enable=1, varname=longname,
		active=1, showname=0, shownumber=0, activeneedlecolor=TEXT_FAINT, activedialcolor=MUTED,
		activefgdialcolor=MUTED,
		saved_attribute_attributes=valueof(longname, shortname, 2, HIDDEN, enum=lock_items(),
			initial=VOCABULARY["noLock"]))


def colour_message(attributes, colour):
	"""One message box setting each attribute to the colour, in turn."""
	value = " ".join(str(c) for c in colour)
	return ", ".join("%s %s" % (a, value) for a in attributes)


CLEAR = [0.0, 0.0, 0.0, 0.0]


def guarded(p, src, outlet=0):
	"""src's output once the device has loaded: a gate that ---loaded opens 500 ms after live.thisdevice. A control
	that edits the Pattern may then take an initial value, the one Live's double-click gives back, without its being
	sent as the device loads (which would edit the Pattern: measured in Live, "hit 0" took a Hit off)."""
	if p.loaded is None:
		p.loaded = p.obj("r ---loaded", 0, 1)
	gate = p.obj("gate 1 0", 2, 1)
	p.line(p.loaded, 0, gate, 0)
	p.line(src, outlet, gate, 1)
	return gate


def bipolar_bar(p, present, longname, low, high):
	"""A value from low to high drawn as a bar from the middle of a well, as the mockup draws micro-timing and the
	ratchets' curve: a multislider of one horizontal bar, signed, that the mouse passes through (live.slider draws a
	line and a triangle). Over it, a transparent live.slider takes the mouse: it jumps to the click and gives back 0
	on a double-click, its initial value. Returns the slider, whose output goes through guarded(), and the bar, which
	shows what the core says. A line marks the middle."""
	x, y, w, h = present
	bar = p.add("multislider", 1, 2, None, ["", ""], present, size=1, orientation=0, setstyle=1, signed=1,
		settype=0, setminmax=[float(low), float(high)], contdata=1, bgcolor=WELL, slidercolor=ACCENT,
		border_top=0, border_bottom=0, border_left=0, border_right=0, ignoreclick=1)
	p.rule(x + w // 2, y + 3, 1, h - 6, MUTED, ignoreclick=1)
	slider = p.add("live.slider", 1, 2, None, ["", "float"], present, parameter_enable=1, varname=longname,
		orientation=1, relative=0, active=1, showname=0, shownumber=0, slidercolor=CLEAR, tricolor=CLEAR,
		trioncolor=CLEAR, tribordercolor=CLEAR, textcolor=CLEAR, focusbordercolor=CLEAR, modulationcolor=CLEAR,
		saved_attribute_attributes=valueof(longname, longname, 1, HIDDEN, mmin=low, mmax=high, initial=0))
	shown = p.obj("prepend set")
	p.line(slider, 0, shown)
	p.line(shown, 0, bar)
	return slider, bar


def momentary(p, present, button):
	"""Makes a toggle live.text momentary under the mouse: a transparent ubutton over it gives it 1 when the mouse
	goes down and 0 when it comes up. live.text's own button mode sends one bang on the click and nothing on the
	release (its Max reference; measured in Live: the Fill never held). A controller mapped to the live.text's
	parameter toggles it."""
	cover = p.add("ubutton", 1, 4, None, ["bang", "bang", "", "int"], present, hilite=0)
	down = p.message("1")
	up = p.message("0")
	p.line(cover, 1, down)
	p.line(cover, 0, up)
	p.line(down, 0, button)
	p.line(up, 0, button)


def activator(p, present, longname, track, visibility=HIDDEN):
	"""A Track's Mute drawn as Live's track activator: lit while the Track plays. Its parameter is on while the Track
	plays, the core's Mute its inverse both ways."""
	button = text_button(p, present, longname, str(track + 1), True, visibility=visibility, initial=1,
		activebgoncolor=ACTIVE, activetextoncolor=WELL, activebgcolor=MUTED, activetextcolor=TEXT_FAINT,
		bordercolor=DARK_BORDER)
	to_core_value = p.obj("!- 1")
	p.line(button, 0, to_core_value)
	to_core(p, to_core_value, "mute %d" % track)
	from_core = p.obj("!- 1")
	p.receive("---mute%d" % track, from_core)
	shown = p.obj("prepend set")
	p.line(from_core, 0, shown)
	p.line(shown, 0, button)
	return button


def fill_button(p, present, longname, visibility=HIDDEN, size=12.0):
	"""A FILL button: held with the mouse, toggled by a controller; the strip's and the window's show each other's
	state. Bold and larger than the other buttons, as the mockup draws it."""
	fill = text_button(p, present, longname, "FILL", True, visibility=visibility, fontsize=size, fontface=1)
	momentary(p, present, fill)
	to_core(p, fill, "fill")
	p.send(fill, 0, "---fillstate")
	set_from(p, "---fillstate", fill)
	return fill


# A Pattern's button for each PatternState the core sends (mdseqProtocol.h, in its order): faint while empty, bright
# while it holds a Hit, lit while shown, outlined in the accent while queued (the core blinks it).
PATTERN_LOOKS = [
	"set 0, " + colour_message(["activetextcolor"], TEXT_FAINT) + ", " + colour_message(["bordercolor"], DARK_BORDER),
	"set 0, " + colour_message(["activetextcolor"], TEXT) + ", " + colour_message(["bordercolor"], DARK_BORDER),
	"set 1, " + colour_message(["bordercolor"], DARK_BORDER),
	"set 0, " + colour_message(["activetextcolor", "bordercolor"], ACCENT),
]


def pattern_buttons(p, x, y, width, pitch, columns, row_pitch, name):
	"""The 16 Patterns as buttons, each drawn as the core says its Pattern is (---patternstateN, PATTERN_LOOKS); a click
	chooses its Pattern through the strip's "Pattern" parameter, so that the set keeps it, or in Chain mode appends it to
	the chain (---patternclick, routed by the strip). No
	initial value: sent at load, it would choose its Pattern."""
	for i in range(limit("patterns")):
		button = text_button(p, [x + pitch * (i % columns), y + row_pitch * (i // columns), width, 24], name % (i + 1),
			"%02d" % (i + 1), True, initial=None, activetextcolor=TEXT_FAINT, bordercolor=DARK_BORDER)
		choose = p.obj("t b", 1, 1, ["bang"])
		p.line(button, 0, choose)
		index = p.message(str(i))
		p.line(choose, 0, index)
		p.send(index, 0, "---patternclick")
		states = p.obj("sel " + " ".join(str(s) for s in range(len(PATTERN_LOOKS))), 2, len(PATTERN_LOOKS) + 1,
			["bang"] * len(PATTERN_LOOKS) + [""])
		p.receive("---patternstate%d" % i, states)
		for s, look in enumerate(PATTERN_LOOKS):
			shown = p.message(look)
			p.line(states, s, shown)
			p.line(shown, 0, button)


def lcd(p, present):
	return p.add("lcd", 1, 4, None, ["list", "list", "int", ""], present, bgtransparent=1, border=0, local=0)


def core_inlet(p):
	"""Where p's messages to mdseq go: the external itself in the patcher that holds it, else one send to it."""
	if p.core is not None:
		return p.core
	if p.to_core is None:
		p.to_core = p.obj("s ---in", 1, 0)
	return p.to_core


def to_core(p, src, prefix, outlet=0):
	"""src's output, prefixed, to mdseq's inlet."""
	pre = p.obj("prepend " + selector(prefix))
	p.line(src, outlet, pre)
	p.line(pre, 0, core_inlet(p))


def set_from(p, name, dst):
	"""Values the core publishes under name set dst without making it output."""
	pre = p.obj("prepend set")
	p.line(pre, 0, dst)
	p.receive(name, pre)


def patcher(title, boxes, lines, rect, width=None, window=False):
	p = {
		"fileversion": 1,
		"appversion": {"major": 9, "minor": 0, "revision": 7, "architecture": "x64", "modernui": 1},
		"classnamespace": "box",
		"rect": rect,
		"openinpresentation": 1,
		"default_fontsize": 10.0,
		"default_fontface": 0,
		"default_fontname": "Arial Bold",
		"gridonopen": 1,
		"gridsize": [8.0, 8.0],
		"description": "",
		"digest": "",
		"tags": "",
		"style": "",
		"boxes": boxes,
		"lines": lines,
	}
	if window:
		p.update({"toolbarvisible": 0, "statusbarvisible": 0, "enablehscroll": 0, "enablevscroll": 0,
			"title": title, "bgcolor": WINDOW_BG, "editing_bgcolor": WINDOW_BG, "locked_bgcolor": WINDOW_BG})
	else:
		p.update({"title": title, "openrect": [0.0, 0.0, width, STRIP_H], "devicewidth": width,
			"dependency_cache": [], "latency": 0, "autosave": 0,
			"project": {
				"version": 1, "creationdate": 3590052786, "modificationdate": 3590052786,
				"viewrect": [0.0, 0.0, 300.0, 500.0], "autoorganize": 1, "hideprojectwindow": 1,
				"showdependencies": 1, "autolocalize": 0, "contents": {"patchers": {}}, "layout": {},
				"searchpath": {}, "detailsvisible": 0, "amxdtype": 1835887981, "readonly": 0, "devpathtype": 0,
				"devpath": ".", "sortmode": 0, "viewmode": 0}})
	return p


def edit_window():
	p = Patch("w")
	# pcontrol opens a subpatcher through its inlet: the window needs one, unused.
	p.add("inlet", 0, 1, None, [""], None, rect=[20, 4, 30, 30])
	# The window's face is the patcher's own background colour (patcher()): a panel there, on the background layer,
	# hid the menus' wells drawn on that layer too (seen in Live), and in front it hid everything.

	# Top bar. A comment's text sits 4 px below its box's top, a 9 px one's centre 7 px below it, and a button's text
	# centre 11 px below its top (measured in Live): labels at the controls' top + 4 share their text's line.
	p.comment(16, 20, 72, 12, "Pattern")
	# The Patterns as the strip shows them: a button each, choosing through the strip's "Pattern" parameter.
	pattern_buttons(p, 96, 16, 28, 32, 16, 1, "Window Pattern Button %d")
	# Controls that edit the Pattern take no initial value: a live.* object sends its initial value when the device
	# loads, which would edit the Pattern (measured in Live: "hit 0" took Track 1's first Hit off). The core shows
	# their values when it redraws.
	# Copy to: the Pattern shown over the one chosen; the core sets the menu back on its label.
	copy_to = menu(p, [612, 16, 64, 24], "Copy to",
		["Copy to"] + ["%02d" % (i + 1) for i in range(limit("patterns"))], initial=None)
	to_core(p, copy_to, "copyto")
	set_from(p, "---copyto", copy_to)
	p.comment(688, 20, 44, 12, "Length")
	length = menu(p, [736, 16, 52, 24], "Length", [str(i + 1) for i in range(limit("steps"))], initial=None)
	to_core(p, length, "length")
	set_from(p, "---lengthindex", length)
	# x2: twice as long, the steps again after them, as Live's Duplicate Loop
	double = text_button(p, [792, 16, 28, 24], "Double", "×2", False, initial=None)
	to_core(p, double, "double")
	p.comment(832, 20, 36, 12, "Scale")
	scale = menu(p, [872, 16, 60, 24], "Scale", VOCABULARY["scales"], initial=None)
	to_core(p, scale, "scale")
	set_from(p, "---scaleindex", scale)
	half = tab(p, [944, 16, 116, 24], "Half", ["1-32", "33-64"])
	to_core(p, half, "half")
	set_from(p, "---half", half)
	follow = text_button(p, [1064, 16, 56, 24], "Follow", "Follow", True, initial=1)
	to_core(p, follow, "follow")
	fill_button(p, [1132, 16, 56, 24], "Window Fill")
	# Print: the Pattern shown into a Session clip per Track, over the Passes chosen (issue 07, decision 10), in one
	# control as the mockup's "Print 4 passes": choosing an item sets the Passes, then prints. No initial value, which
	# would print as the device loads: the menu shows the core's 4 Passes from the load on.
	print_menu = menu(p, [1200, 16, 116, 24], "Print",
		["Print 1 pass"] + ["Print %d passes" % (n + 1) for n in range(1, limit("passes"))], initial=None)
	then_print = p.obj("t b i", 1, 2, ["bang", "int"])
	p.line(print_menu, 0, then_print)
	to_core(p, then_print, "passes", outlet=1)
	print_now = p.message(selector("print", prepend=False))
	p.line(then_print, 0, print_now)
	p.line(print_now, 0, core_inlet(p))
	shown_passes = p.obj("loadmess set 3", 1, 1)
	p.line(shown_passes, 0, print_menu)
	text_button(p, [1328, 16, 96, 24], "Window Capture", "Session", False, active=0)
	p.rule(0, 52, WINDOW_W, 1)

	# Fill edit, above the Tracks' rows: the Hits the mouse puts take the Condition FILL, the others fade.
	fill_edit = text_button(p, [16, 60, 72, 20], "Fill Edit", "Fill edit", True)
	to_core(p, fill_edit, "filledit")
	set_from(p, "---filledit", fill_edit)
	# Clear: the Pattern shown back to an empty one (Live's undo gives it back). No initial value: it edits the Pattern.
	clear = text_button(p, [96, 60, 64, 20], "Clear", "Clear", False, initial=None)
	to_core(p, clear, "clear")

	# Grid, with its column numbers every 4 steps, a row per Track, then the swing row
	x0, y0 = 176, 88
	for i in range(8):
		column = 4 * i
		x = x0 + 38 * column + (4 if column >= 16 else 0)
		p.comment(x, 66, 152, 12, str(column + 1), receive="---colnum%d" % i)
	grid = lcd(p, [x0, y0, 1220, 28 * (limit("tracks") + 1) - 4])
	p.receive("---grid", grid)
	to_core(p, grid, "gridclick", 0)
	to_core(p, grid, "gridmouse", 2)
	for t in range(limit("tracks")):
		y = y0 + 28 * t
		# The number and its Machine, once MD Drums tells it, on one baseline, centred on the activator's text
		p.comment(16, y + 5, 20, 12, "%02d" % (t + 1), textcolor=TEXT_FAINT)
		p.comment(40, y + 3, 92, 14, "", size=11.0, receive="---trackname%d" % t, fontface=1, textcolor=TEXT)
		activator(p, [136, y, 24, 24], "Window Track %d On" % (t + 1), t)
	# The swing row, the Machinedrum's: its steps play late by the Pattern's swing, every Track's. The menu's index
	# goes both ways, 50 % (straight) at 0.
	y = y0 + 28 * limit("tracks")
	p.comment(16, y + 4, 56, 12, "Swing")
	swing = menu(p, [80, y, 72, 24], "Swing", ["%d%%" % s for s in range(limit("minSwing"), limit("maxSwing") + 1)],
		initial=None)
	to_core(p, swing, "swing")
	set_from(p, "---swing", swing)
	p.rule(0, 576, WINDOW_W, 1)

	# Step panel
	p.comment(16, 590, 320, 24, "Track 01", size=16.0, receive="---tracktitle", textcolor=TEXT)
	p.comment(16, 620, 320, 12, "Step 1, page 1", receive="---steptitle", textcolor=TEXT_FAINT)
	hit = text_button(p, [16, 644, 64, 24], "Step Hit", "Hit", True, initial=None)
	to_core(p, hit, "hit")
	set_from(p, "---hit", hit)
	# The velocity's dial alone, its name and value beside it as the Locks' are
	# Double-clicked, back to 127, a new Hit's velocity (guarded: not sent as the device loads)
	velocity = dial(p, [96, 636, 40, 40], "Step Velocity", "Velocity", 1, 127, 127)
	to_core(p, guarded(p, velocity), "velocity")
	set_from(p, "---velocity", velocity)
	p.comment(144, 642, 80, 12, "Velocity", textcolor=TEXT_DIM)
	velocity_value = p.comment(144, 658, 80, 14, "", size=11.0, textcolor=TEXT)
	set_from(p, "---velocity", velocity_value)
	shown = p.obj("prepend set")
	p.line(velocity, 0, shown)
	p.line(shown, 0, velocity_value)
	p.comment(16, 692, 140, 12, "Condition")
	condition = menu(p, [16, 708, 140, 24], "Condition", VOCABULARY["conditions"], initial=None)
	to_core(p, condition, "condition")
	set_from(p, "---condition", condition)
	p.comment(172, 692, 168, 12, "Micro-timing")
	micro, micro_bar = bipolar_bar(p, [172, 708, 168, 24], "Micro-timing", -limit("micro"), limit("micro"))
	to_core(p, guarded(p, micro), "micro")
	set_from(p, "---micro", micro)
	set_from(p, "---micro", micro_bar)
	p.comment(172, 736, 168, 12, "On the grid", receive="---microtext")
	p.comment(16, 756, 140, 12, "Ratchets")
	ratchets = menu(p, [16, 772, 64, 24], "Ratchets", ["x%d" % (i + 1) for i in range(limit("ratchets"))],
		initial=None)
	to_core(p, ratchets, "ratchets")
	set_from(p, "---ratchetsindex", ratchets)
	p.comment(172, 756, 168, 12, "Ratchet velocity")
	curve, curve_bar = bipolar_bar(p, [172, 772, 168, 24], "Ratchet velocity", -limit("curve"), limit("curve"))
	to_core(p, guarded(p, curve), "curve")
	set_from(p, "---curve", curve)
	set_from(p, "---curve", curve_bar)
	p.comment(172, 800, 168, 12, "One Hit", receive="---curvetext")
	# Whether the device looks for a newer release, for every set (the external keeps it in its settings file). No
	# initial value: the external tells the switch's state.
	updates = text_button(p, [16, 836, 140, 20], "Check Updates", "Check for updates", True, initial=None)
	to_core(p, updates, "updates")
	set_from(p, "---updates", updates)
	p.rule(352, 592, 1, 248)

	assert len(SYN + EFX + ROUTING) == limit("locks")
	for g, (group, names) in enumerate([("SYN", SYN), ("EFX", EFX), ("ROUTING", ROUTING)]):
		y = 596 + 88 * g
		p.comment(368, y + 14, 72, 12, group)
		for i, name in enumerate(names):
			param = 8 * g + i
			x = 448 + 120 * i
			lock = lock_dial(p, [x, y, 40, 40], "Lock %d" % (param + 1), name)
			to_core(p, guarded(p, lock), "lock %d" % param)
			# SYN1-8 take the names the step selected's Machine gives them, once MD Drums tells it; under the name,
			# what the dial holds: the Lock, in the accent, or the Kit's value, quieter
			p.comment(x + 48, y + 6, 64, 12, name, receive="---lockname%d" % param if g == 0 else None,
				textcolor=TEXT_DIM)
			value = p.comment(x + 48, y + 22, 72, 14, "Kit", size=11.0, receive="---lockvalue%d" % param,
				textcolor=TEXT_FAINT)
			# lock <value> <locked> from the core: the position, and the accent on the dial and its value while the
			# step holds a Lock
			values = p.obj("unpack 0 0", 1, 2, ["int", "int"])
			p.receive("---lock%d" % param, values)
			set_value = p.obj("prepend set")
			p.line(values, 0, set_value)
			p.line(set_value, 0, lock)
			locked = p.obj("sel 1", 2, 2, ["bang", ""])
			p.line(values, 1, locked)
			for outlet, colour, arc in ((0, ACCENT, ACCENT), (1, TEXT_FAINT, MUTED)):
				dial_colours = p.message(colour_message(["activeneedlecolor"], colour) + ", " +
					colour_message(["activedialcolor"], arc))
				p.line(locked, outlet, dial_colours)
				p.line(dial_colours, 0, lock)
				text_colour = p.message(colour_message(["textcolor"], colour))
				p.line(locked, outlet, text_colour)
				p.line(text_colour, 0, value)
	# What a print did, once it has
	p.comment(448, 840, 976, 12, "", receive="---printstatus")

	return p


def device():
	p = Patch("s")

	# The core and the MIDI path; incoming MIDI passes through.
	core = p.obj(CLASS, 1, 4, ["int", "", "", ""])
	p.core = core
	# The edit window's messages come through one send
	p.receive("---in", core)
	midiin = p.obj("midiin", 1, 1, ["int"])
	midiout = p.obj("midiout", 1, 0)
	p.line(midiin, 0, midiout)
	p.line(core, 0, midiout)
	p.send(core, 2, "---grid")
	p.send(core, 3, "---panel")
	# The bank, kept in the set: a Blob parameter per Pattern, Stored Only, that the core writes half a second after
	# the last edit of that Pattern ("store p words...") and that gives it back when the set loads or Live undoes a
	# write ("restore p words..."); thru 2 (the core also ignores the echo it gets anyway). A newobj keeps
	# parameter_enable among its object attributes, as Ableton's own pattr Blobs do (Dynamical, Waveshaper): at the
	# box's top level it is ignored, and Live kept nothing (seen when a track was duplicated). The initial value, 0,
	# loads in the core as an empty Pattern.
	stores = p.obj("route " + " ".join(str(i) for i in range(limit("patterns"))), 1, limit("patterns") + 1)
	p.receive("---store", stores)
	for i in range(limit("patterns")):
		store = p.add("newobj", 1, 3, "pattr pattern%d @thru 2" % (i + 1), ["", "", ""], None,
			varname="pattern%d" % (i + 1), saved_object_attributes={"parameter_enable": 1, "initial": [0]},
			saved_attribute_attributes=valueof("Pattern %d" % (i + 1), "Pattern %d" % (i + 1), 3, STORED_ONLY,
				initial=0))
		p.line(stores, i, store)
		to_core(p, store, "restore %d" % i)
	# The chain, kept the same way: "chainstore playing length patterns..." from the core each time a chain starts or
	# ends, given back as "restorechain ..." when the set loads or Live undoes a write. Its initial value, 0, restores
	# nothing.
	chain_store = p.add("newobj", 1, 3, "pattr chain @thru 2", ["", "", ""], None, varname="chain",
		saved_object_attributes={"parameter_enable": 1, "initial": [0]},
		saved_attribute_attributes=valueof("Pattern Chain", "Pattern Chain", 3, STORED_ONLY, initial=0))
	p.receive("---chainstore", chain_store)
	to_core(p, chain_store, "restorechain")
	# The LOM, for printing. The core sends "lom path <path>" (live.path goes there, live.object takes its id), "lom id
	# <n>" (live.object takes that object), "lom obj <get|set|call ...>" (live.object asks it); answers come back to the
	# core as "lomid ..." and "lomreply ...", within the same call, since Max messages run depth-first.
	lom = p.obj("route path id obj", 1, 4, ["", "", "", ""])
	p.receive("---lom", lom)
	goto = p.obj("prepend goto")
	p.line(lom, 0, goto)
	live_path = p.obj("live.path", 1, 3, ["", "", ""])
	p.line(goto, 0, live_path)
	live_object = p.obj("live.object", 2, 1, [""])
	p.line(live_path, 0, live_object, 1)
	to_core(p, live_path, "lomid")
	by_id = p.obj("prepend id")
	p.line(lom, 1, by_id)
	p.line(by_id, 0, live_object, 1)
	p.line(lom, 2, live_object, 0)
	to_core(p, live_object, "lomreply")
	device_ready = p.obj("live.thisdevice", 1, 3, ["bang", "int", "int"])
	redraw = p.message(selector("redraw", prepend=False))
	p.line(device_ready, 0, redraw)
	p.line(redraw, 0, core)
	# The window's guarded controls (guarded()) open half a second after the device has loaded, once their initial
	# values have gone by.
	loaded_later = p.obj("delay 500")
	p.line(device_ready, 0, loaded_later)
	opened = p.message("1")
	p.line(loaded_later, 0, opened)
	p.send(opened, 0, "---loaded")

	# The core's panel values, to whoever shows them: each key to the receive of its name, and an indexed key
	# ("mute 3 1") by its first integer to that one of its receives.
	keys = [k["key"] for k in VOCABULARY["panel"]]
	panel_in = p.obj("r ---panel", 0, 1)
	route = p.obj("route " + " ".join(keys), 1, len(keys) + 1)
	p.line(panel_in, 0, route)
	for i, k in enumerate(VOCABULARY["panel"]):
		count = k["indexed"]
		if count:
			by_index = p.obj("route " + " ".join(str(n) for n in range(count)), 1, count + 1)
			p.line(route, i, by_index)
			for n in range(count):
				p.send(by_index, n, "---%s%d" % (k["key"], n))
			continue
		p.send(route, i, "---" + k["key"])

	# Patterns
	p.comment(8, 6, 140, 12, "Pattern")
	# The Pattern chosen: a Live parameter, so that the set keeps it and the Arrangement can automate the switches. Its
	# live.tab stays out of the strip: one on several rows wraps by its width (measured in Live: 140 px hold two items a
	# row, whatever num_lines), so the strip shows 16 buttons in 4 x 4, each choosing its Pattern through the tab, lit
	# while the core says its Pattern is the one chosen.
	items = ["%02d" % (i + 1) for i in range(limit("patterns"))]
	pattern = p.add("live.tab", 1, 3, None, ["", "", "float"], None, rect=[20, 3000, 140, 24], parameter_enable=1,
		varname="Pattern", num_lines_presentation=1, num_lines_patching=1, multiline=0, active=1,
		saved_attribute_attributes=valueof("Pattern", "Pattern", 2, AUTOMATED, enum=items, initial=0))
	to_core(p, pattern, "pattern")
	set_from(p, "---patternindex", pattern)
	# A Pattern button clicked chooses its Pattern, or in Chain mode appends it to the chain built: a gate the core's
	# chainmode sets (outlet 1 to choose, 2 to append).
	clicked = p.obj("r ---patternclick", 0, 1)
	click_to = p.obj("gate 2 1", 2, 2)
	p.line(clicked, 0, click_to, 1)
	chain_mode = p.obj("r ---chainmode", 0, 1)
	which = p.obj("+ 1", 2, 1, ["int"])
	p.line(chain_mode, 0, which)
	p.line(which, 0, click_to, 0)
	p.line(click_to, 0, pattern)
	to_core(p, click_to, "chainadd", outlet=1)
	pattern_buttons(p, 8, 22, 32, 36, 4, 28, "Pattern Button %d")
	p.comment(8, 136, 148, 12, "Stopped", receive="---status")
	p.comment(8, 150, 148, 12, "16 steps, 1/16", receive="---lengthtext")
	p.rule(160, 8, 1, 153)

	# Tracks, Mutes, the steps of the Track shown, pages
	p.comment(172, 6, 300, 12, "Track")
	tracks = tab(p, [172, 22, 636, 24], "Track", ["%02d" % (t + 1) for t in range(limit("tracks"))])
	to_core(p, tracks, "track")
	# Each tab names its Track's Machine once MD Drums tells it (ADR 0003), by the short name a cell of 36 px holds
	# ("01 BD"), then the core sets the tab chosen again.
	names = p.obj("prepend _parameter_range")
	p.line(names, 0, tracks)
	p.receive("---tracknames", names)
	set_from(p, "---trackindex", tracks)
	for t in range(limit("tracks")):
		activator(p, [172 + 40 * t, 50, 36, 16], "Track %d On" % (t + 1), t, visibility=AUTOMATED)
	steps = lcd(p, [172, 74, 636, 40])
	p.line(core, 1, steps)
	to_core(p, steps, "stripclick", 0)
	to_core(p, steps, "stripmouse", 2)
	pages = tab(p, [172, 122, 636, 16], "Page", ["1-16", "17-32", "33-48", "49-64"])
	to_core(p, pages, "page")
	# A newer release, once a day at most (the external asks GitHub), empty otherwise
	p.comment(172, 146, 636, 12, "", receive="---update", textcolor=ACCENT)
	p.rule(816, 8, 1, 153)

	# Fill, Edit, Capture
	# Held, the Fill plays FILL steps (read when a step is planned); a Live parameter, for controllers.
	fill_button(p, [828, 22, 72, 64], "Fill", visibility=AUTOMATED, size=16.0)
	edit = text_button(p, [908, 22, 72, 24], "Edit", "Edit", False)
	open_msg = p.message("open")
	p.line(edit, 0, open_msg)
	pcontrol = p.obj("pcontrol", 1, 1)
	p.line(open_msg, 0, pcontrol)
	if CLASS != "mdseq":
		# A development build opens its edit window once loaded, so that it can be looked at without a click.
		open_later = p.obj("delay 1000")
		p.line(device_ready, 0, open_later)
		p.line(open_later, 0, open_msg)
	window = edit_window()
	window_box = p.add("newobj", 1, 0, "patcher window", None, None, rect=[20, 2000, 120, 22],
		patcher=patcher("MD Drums Seq: Edit", window.boxes, window.lines, [80.0, 80.0, WINDOW_W, WINDOW_H],
			window=True))
	p.line(pcontrol, 0, window_box)
	p.comment(908, 52, 72, 12, "Capture")
	text_button(p, [908, 70, 72, 16], "Capture", "Session", False, active=0)
	# Chain: on, the Pattern buttons clicked make a new chain; off, it plays, each Pattern for one Pass, round and round
	chain = text_button(p, [828, 96, 152, 20], "Chain", "Chain", True)
	to_core(p, chain, "chain")
	set_from(p, "---chainmode", chain)
	p.comment(828, 122, 152, 36, "", receive="---chaintext", textcolor=TEXT_DIM)

	return patcher("MD Drums Seq", p.boxes, p.lines, [100.0, 100.0, 1200.0, 800.0], width=STRIP_W)


def write_amxd(path, patch):
	body = json.dumps({"patcher": patch}, indent="\t").encode("utf-8") + b"\n\x00"
	data = b"ampf" + struct.pack("<I", 4) + b"mmmm" + b"meta" + struct.pack("<I", 4) + b"\x00\x00\x00\x00"
	data += b"ptch" + struct.pack("<I", len(body)) + body
	path.write_bytes(data)


if __name__ == "__main__":
	if len(sys.argv) not in (3, 4):
		sys.exit("usage: make_device.py <output.amxd> <vocabulary.json> [<external class>]")
	load_vocabulary(sys.argv[2])
	if len(sys.argv) == 4:
		CLASS = sys.argv[3]
	out = pathlib.Path(sys.argv[1])
	out.parent.mkdir(parents=True, exist_ok=True)
	write_amxd(out, device())
	print("written:", out)
