"""Writes the device MoDule Seq.amxd: the approved v12 interface (Design/HANDOFF.md, Design/v12-test.html) over the
external mdseq.

The strip (992 x 169) plays, the edit window (1440 x 868, a subpatcher opened by Edit) edits. Both lcd objects are
drawn by mdseq; the patch only routes, through device-local send and receive names (---). The Pattern, the Fill, the
Mutes and the Output are Live parameters; every other control is a hidden parameter: neither automated, nor stored,
nor in Live's undo history.

Its colours are v12's own, fixed: the device no longer follows Live's theme. Its type is Ableton Sans, which Max
carries inside Live (Resources/Max/resources/fonts), by the names Live's own devices give it.

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


def lcd_geometry(name):
	"""The strip's or the grid's lcd as the core draws it (mdseqView.h): sizes, cell width, pitch."""
	return VOCABULARY["lcd"][name]


def grid_x(column):
	"""A grid column's left edge in its lcd, the second page of 16 a gap further (grid::cellX)."""
	cells = lcd_geometry("grid")
	return column * cells["pitch"] + (cells["pageGap"] if column >= 16 else 0)


def fitted(name, width, size, bold=False):
	"""Checks that a box holds what the Editor fits into it (mdseqText.h fit::): the same face and size, and a width
	beyond the fitted one, where Live broke or cut a longer text."""
	box = VOCABULARY["fit"][name]
	if box["size"] != size or box["bold"] != bold or box["width"] >= width:
		raise ValueError("the box of %s (%d px, %d px%s) does not hold fit::%s %r" % (name, width, size,
			" bold" if bold else "", name, box))


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


def rgb(hex_colour):
	return [round(int(hex_colour[i:i + 2], 16) / 255, 6) for i in (1, 3, 5)] + [1.0]


# v12's colours (Design/v12-ui.css, .fam-v9 to .fam-v12)
FACE = rgb("#2a2b2f")			# the device's face
KEY = rgb("#3d3e43")			# a key, a closed menu
KEY_EMPTY = rgb("#323338")		# an empty Pattern's key
LINE = rgb("#55585e")			# keys' borders, rules
RING = rgb("#4a4c51")			# a dial's ring, a slider's middle
WELL = rgb("#1c1d20")			# a slider's well
INK = rgb("#f2f2f2")			# content, values
INK_DIM = rgb("#c3c6cb")		# names
INK_FAINT = rgb("#9a9da3")		# group titles, hints, the Kit's values
ACCENT = rgb("#6fd1c4")			# MoDule's turquoise: what is chosen, a mode on, the Locks, the screens' text
ON_ACCENT = rgb("#10201e")		# text on the accent, and the screens' ground
SCREEN_BORDER = rgb("#2c5650")
CORAL = rgb("#ff7a5c")			# a muted Track
ON_CORAL = rgb("#111111")
AMBER = rgb("#e8b35a")			# a newer release
OFF_TEXT = rgb("#5f6267")		# an inactive key's text: a page past the length, Capture without a take

# Ableton Sans as Live's own devices name it
MEDIUM = "Ableton Sans Medium"
BOLD = "Ableton Sans Bold Regular"

# Where a live.comment puts the middle of its capitals below its box's top, by font size, and how far right of its
# box's left edge its text starts. Measured in Live on 2026-10-20 (dbg36, PrintWindow): the ink of 10 and 12 px texts
# centred within half a pixel of the mockup's boxes, that of 14 px a pixel low until 11; every text 4 px right.
CAP_MIDDLE = {10: 8, 12: 9, 14: 11}
COMMENT_INSET = 4

# A live.line's box: its line on the box's top or left edge, the rest empty.
RULE_BOX = 4

# Live's parameter visibility
AUTOMATED, STORED_ONLY, HIDDEN = 0, 1, 2


class Patch:
	"""Boxes and lines of one patcher, with ids made here. Within a layer, Max draws the boxes listed first in front
	(seen in Live on 2026-10-20: a panel listed before a live.menu or a live.comment hid it): boxes added with back=True
	are listed after all the others, behind them."""

	def __init__(self, prefix):
		self.prefix = prefix
		self.front = []
		self.back = []
		self.lines = []
		self.count = 0
		self.y = 20	# patching-view placement, one box per row, for whoever opens the patch
		self.core = None	# the external's box, in the patcher that holds it
		self.to_core = None	# elsewhere, the one send its messages go through
		self.loaded = None	# the receive that opens the guarded() gates once the device has loaded
		self.script = None	# a thispatcher, for the scripts that hide and show boxes

	@property
	def boxes(self):
		return self.front + self.back

	def add(self, maxclass, inlets, outlets, text=None, outlettype=None, present=None, rect=None, back=False,
		**extra):
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
		(self.back if back else self.front).append({"box": b})
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

	def thispatcher(self):
		if self.script is None:
			self.script = self.obj("thispatcher", 1, 2, ["", ""])
		return self.script

	# ---- interface objects ----

	def comment(self, x, y, w, h, text, size=10, receive=None, bold=False, colour=None, **extra):
		"""A live.comment whose text starts at x."""
		box = self.add("live.comment", 1, 0, text, None, [x - COMMENT_INSET, y, w + COMMENT_INSET, h],
			fontsize=float(size), textjustification=0, fontname=BOLD if bold else MEDIUM, textcolor=colour or INK_DIM,
			**extra)
		if receive:
			set_from(self, receive, box)
		return box

	def text(self, x, y, w, h, text, size=10, **extra):
		"""A text where the mockup draws it: its box (x, y, w, h) with the capitals centred on its height, as the
		mockup's line box centres them."""
		top = y + h // 2 - CAP_MIDDLE[size]
		return self.comment(x, top, w, size + 8, text, size=size, **extra)

	def label(self, x, y, w, text):
		"""A group's title: 10 px bold capitals in the faint ink, over the mockup's 12 px box."""
		return self.text(x, y, w, 12, text.upper(), bold=True, colour=INK_FAINT)

	def rule(self, x, y, w, h, colour=None):
		"""A 1 px line along the top of (x, y, w, 1) or down the left of (x, y, 1, h): a live.line, as a panel is never
		thinner than 4 px (measured in Live on 2026-10-20)."""
		horizontal = w > h
		box = [x, y, w, RULE_BOX] if horizontal else [x, y, RULE_BOX, h]
		return self.add("live.line", 1, 0, None, None, box, linecolor=colour or LINE, border=1.0, justification=0,
			orientation=1 if horizontal else 0, ignoreclick=1)

	def well(self, present, colour, border, varname=None):
		"""A panel bordered as a key, on the background layer, behind a menu or a screen's text; varname names it for a
		script that hides it."""
		return self.add("panel", 1, 0, None, None, present, bgcolor=colour, border=1, bordercolor=border, rounded=2,
			background=1, ignoreclick=1, **({"varname": varname} if varname else {}))

	def screen(self, present, lines, varname=None, colour=None):
		"""One of the device's small displays: MoDule's dark ground, the accent's text, a state in sentence case. lines
		are (text, receive) pairs, 12 px apart, centred on the screen's height as the mockup centres them (screen() in
		v12-ui.js), 7 px in from the left (its border and padding)."""
		x, y, w, h = present
		self.well(present, ON_ACCENT, SCREEN_BORDER, varname)
		for i, (text, receive) in enumerate(lines):
			self.text(x + 7, y + (h - 12 * len(lines)) // 2 + 12 * i, w - 14, 12, text, receive=receive,
				colour=colour or ACCENT, **({"varname": "%s%d" % (varname, i)} if varname else {}))


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


# A key and a tab item: v12's key, filled with the accent once chosen; inactive, sunk into the face.
KEY_COLOURS = {"activebgcolor": KEY, "activebgoncolor": ACCENT, "bordercolor": LINE, "focusbordercolor": ACCENT,
	"bgcolor": FACE, "bgoncolor": FACE, "rounded": 2.0, "fontname": MEDIUM, "fontsize": 10.0}


def tab(p, present, longname, items):
	"""A live.tab of one row (multiline 0: measured in Live, issue 15, multiline 1 lays out twice num_lines rows)."""
	return p.add("live.tab", 1, 3, None, ["", "", "float"], present, parameter_enable=1, varname=longname,
		num_lines_presentation=1, num_lines_patching=1, multiline=0, spacing_x=4.0, spacing_y=4.0, active=1,
		textcolor=INK, textoncolor=ON_ACCENT, **KEY_COLOURS,
		saved_attribute_attributes=valueof(longname, longname, 2, HIDDEN, enum=items, initial=0))


def text_button(p, present, longname, text, visibility=HIDDEN, initial=0, active=1, **extra):
	"""A live.text toggle: its button mode draws a pill rather than a key (seen in Live on 2026-10-20), so a key that
	acts is a toggle too (key_button). Its "text" attribute is the box's own "text" key, in capitals as v12 writes a
	key's."""
	colours = dict(KEY_COLOURS, activetextcolor=INK, activetextoncolor=ON_ACCENT, textcolor=OFF_TEXT,
		textoffcolor=OFF_TEXT)
	colours.update(extra)
	return p.add("live.text", 1, 2, text.upper(), ["", ""], present, parameter_enable=1, varname=longname,
		mode=1, texton=text.upper(), active=active,
		saved_attribute_attributes=valueof(longname, longname, 2, visibility, enum=["off", "on"], mmax=1,
			initial=initial), **colours)


def key_button(p, present, longname, text, message, to, active=1):
	"""A key that sends message to the box to once per click: a toggle set back off at once. No initial value: it
	would act as the device loads. Returns the live.text and the message box."""
	button = text_button(p, present, longname, text, initial=None, active=active)
	clicked = p.obj("t b b", 1, 2, ["bang", "bang"])
	p.line(button, 0, clicked)
	off = p.message("set 0")
	p.line(clicked, 1, off)
	p.line(off, 0, button)
	act = p.message(message)
	p.line(clicked, 0, act)
	p.line(act, 0, to)
	return button, act


# A live.menu draws its text at the top of a box about 15 px high, whatever its own (measured in Live on 2026-10-17:
# its text 4 px above that of a button beside it): the mockup's key is a panel, the menu centred in it.
MENU_HEIGHT = 16

# A closed menu or a number is a key (v12), drawn on a well(); a menu's open list keeps Live's ground, the item under
# the mouse in the accent.
ON_KEY = {"activebgcolor": KEY, "bordercolor": KEY, "focusbordercolor": KEY, "textcolor": INK, "fontname": MEDIUM,
	"fontsize": 10.0}
MENU_COLOURS = dict(ON_KEY, tricolor=INK_DIM, hltcolor=ACCENT, hlttextcolor=ON_ACCENT)


def menu(p, present, longname, items, initial=0, well=None):
	x, y, w, h = present
	# well names the panel for a script that hides it.
	p.well(present, KEY, LINE, well)
	return p.add("live.menu", 1, 3, None, ["", "", "float"], [x + 2, y + (h - MENU_HEIGHT) // 2, w - 4, MENU_HEIGHT],
		parameter_enable=1, varname=longname, active=1, **MENU_COLOURS,
		saved_attribute_attributes=valueof(longname, longname, 2, HIDDEN, enum=items, initial=initial))


def numbox(p, present, longname, mmin, mmax, well=None):
	# A number on a key, as a menu's: a live.numbox, dragged or typed. No initial value: the core tells it.
	x, y, w, h = present
	p.well(present, KEY, LINE, well)
	return p.add("live.numbox", 1, 2, None, ["", "float"], [x + 2, y + (h - MENU_HEIGHT) // 2, w - 4, MENU_HEIGHT],
		parameter_enable=1, varname=longname, active=1, **ON_KEY,
		saved_attribute_attributes=valueof(longname, longname, 1, HIDDEN, mmin=mmin, mmax=mmax))


def dial(p, present, longname, shortname, mmin, mmax, initial):
	# The dial alone: its name and value are texts beside it, as the Locks' are. The velocity is content: its arc in
	# ink on the ring.
	return p.add("live.dial", 1, 2, None, ["", "float"], present, parameter_enable=1, varname=longname,
		active=1, showname=0, shownumber=0, activedialcolor=INK, activefgdialcolor=RING, activeneedlecolor=INK,
		saved_attribute_attributes=valueof(longname, shortname, 1, HIDDEN, mmin=mmin, mmax=mmax, initial=initial))


def lock_items():
	"""A Lock's dial: 128 positions, each the value it locks, but the middle one, where NoLock (63) would be (63 cannot
	travel to MD Drums), which is the Kit. Without a Lock the dial sits unlit on the Kit's value, so that turning it
	starts the Lock there; double-clicking goes back to the Kit."""
	return ["Kit" if v == VOCABULARY["noLock"] else str(v) for v in range(128)]


def lock_dial(p, present, longname, shortname):
	# No number on the dial: the text beside it says what it holds, the Lock or the Kit's value. Always active: an
	# inactive live.dial sends nothing when turned (Max's reference; seen in Live: no Lock was ever made), so a Lock
	# shows by its colours instead: without one, its arc the ring's colour and a faint needle.
	return p.add("live.dial", 1, 2, None, ["", "float"], present, parameter_enable=1, varname=longname,
		active=1, showname=0, shownumber=0, activeneedlecolor=INK_FAINT, activedialcolor=RING, activefgdialcolor=RING,
		saved_attribute_attributes=valueof(longname, shortname, 2, HIDDEN, enum=lock_items(),
			initial=VOCABULARY["noLock"]))


def look(on=None, **colours):
	"""A message box's text that sets a box's colours (and its value, "set on", when given), one attribute each."""
	words = [] if on is None else ["set %d" % on]
	return ", ".join(words + ["%s %s" % (a, " ".join(str(c) for c in colour)) for a, colour in colours.items()])


def show_looks(p, chooser, looks, dst):
	"""Each outlet of chooser (a sel) sends dst the look of the same index."""
	for outlet, text in enumerate(looks):
		message = p.message(text)
		p.line(chooser, outlet, message)
		p.line(message, 0, dst)


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
	line and a triangle), in ink, in a well bordered as a key. Over it, a transparent live.slider takes the mouse: it
	jumps to the click and gives back 0 on a double-click, its initial value. Returns the slider, whose output goes
	through guarded(), and the bar, which shows what the core says. A line marks the middle."""
	x, y, w, h = present
	p.well(present, WELL, LINE)
	bar = p.add("multislider", 1, 2, None, ["", ""], [x + 1, y + 1, w - 2, h - 2], size=1, orientation=0, setstyle=1,
		signed=1, settype=0, setminmax=[float(low), float(high)], contdata=1, bgcolor=WELL, slidercolor=INK,
		border_top=0, border_bottom=0, border_left=0, border_right=0, ignoreclick=1)
	p.rule(x + w // 2, y + 3, 1, h - 6, RING)
	slider = p.add("live.slider", 1, 2, None, ["", "float"], present, parameter_enable=1, varname=longname,
		orientation=1, relative=0, active=1, showname=0, shownumber=0, slidercolor=CLEAR, tricolor=CLEAR,
		trioncolor=CLEAR, tribordercolor=CLEAR, textcolor=CLEAR, focusbordercolor=CLEAR, modulationcolor=CLEAR,
		saved_attribute_attributes=valueof(longname, longname, 1, HIDDEN, mmin=low, mmax=high, initial=0))
	set_on(p, slider, bar)
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
	"""A Track's Mute drawn as a key with its number while the Track plays, coral once muted. Its parameter is on
	while the Track plays, the core's Mute its inverse both ways."""
	button = text_button(p, present, longname, str(track + 1), visibility=visibility, initial=1,
		activebgoncolor=KEY, activetextoncolor=INK, activebgcolor=CORAL, activetextcolor=ON_CORAL)
	to_core_value = p.obj("!- 1")
	p.line(button, 0, to_core_value)
	to_core(p, to_core_value, "mute %d" % track)
	from_core = p.obj("!- 1")
	p.receive("---mute%d" % track, from_core)
	set_on(p, from_core, button)
	return button


def fill_button(p, present, longname, visibility=HIDDEN):
	"""A FILL button: held with the mouse, toggled by a controller; the strip's and the window's show each other's
	state. In 14 px bold, as the mockup draws it."""
	fill = text_button(p, present, longname, "FILL", visibility=visibility, fontsize=14.0, fontname=BOLD)
	momentary(p, present, fill)
	to_core(p, fill, "fill")
	p.send(fill, 0, "---fillstate")
	set_from(p, "---fillstate", fill)
	return fill


# A Pattern's button for each PatternState the core sends (mdseqProtocol.h, in its order): faint on a sunk key while
# empty, in ink while it holds a Hit, filled with the accent while shown, outlined in the accent while queued (the
# core blinks it). Each look sets every colour another one changes.
PATTERN_LOOKS = [
	look(0, activetextcolor=INK_FAINT, activebgcolor=KEY_EMPTY, bordercolor=LINE),
	look(0, activetextcolor=INK, activebgcolor=KEY, bordercolor=LINE),
	look(1, bordercolor=LINE),
	look(0, activetextcolor=INK, activebgcolor=KEY, bordercolor=ACCENT),
]


def pattern_buttons(p, x, y, width, pitch, columns, row_pitch, name):
	"""The 16 Patterns as buttons, each drawn as the core says its Pattern is (---patternstateN, PATTERN_LOOKS); a click
	chooses its Pattern through the strip's "Pattern" parameter, so that the set keeps it, or in Chain mode appends it to
	the chain (---patternclick, routed by the strip). No
	initial value: sent at load, it would choose its Pattern."""
	for i in range(limit("patterns")):
		button = text_button(p, [x + pitch * (i % columns), y + row_pitch * (i // columns), width, 24], name % (i + 1),
			"%02d" % (i + 1), initial=None, activetextcolor=INK_FAINT, activebgcolor=KEY_EMPTY)
		choose = p.obj("t b", 1, 1, ["bang"])
		p.line(button, 0, choose)
		index = p.message(str(i))
		p.line(choose, 0, index)
		p.send(index, 0, "---patternclick")
		states = p.obj("sel " + " ".join(str(s) for s in range(len(PATTERN_LOOKS))), 2, len(PATTERN_LOOKS) + 1,
			["bang"] * len(PATTERN_LOOKS) + [""])
		p.receive("---patternstate%d" % i, states)
		show_looks(p, states, PATTERN_LOOKS, button)


def lcd(p, present):
	# Transparent: its clear shows the face between the cells.
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


def set_on(p, src, dst, outlet=0):
	"""src's output sets dst without making it output."""
	pre = p.obj("prepend set")
	p.line(src, outlet, pre)
	p.line(pre, 0, dst)


def set_from(p, name, dst):
	"""Values the core publishes under name set dst without making it output."""
	set_on(p, p.obj("r " + name, 0, 1), dst)


def bind(p, control, prefix, key):
	"""A control that sends the core prefix and its value, and shows what the core publishes under ---key."""
	to_core(p, control, prefix)
	set_from(p, "---" + key, control)


def show_by(p, name, groups):
	"""The boxes (by varname) of groups[v] shown while the core's name says v, those of the other groups hidden."""
	everyone = [box for group in groups for box in group]
	values = p.obj("sel " + " ".join(str(v) for v in range(len(groups))), 1, len(groups) + 1,
		["bang"] * len(groups) + [""])
	p.receive(name, values)
	for v, group in enumerate(groups):
		script = p.message(", ".join("script sendbox %s hidden %d" % (box, 0 if box in group else 1)
			for box in everyone))
		p.line(values, v, script)
		p.line(script, 0, p.thispatcher())


def patcher(title, boxes, lines, rect, width=None, window=False):
	p = {
		"fileversion": 1,
		"appversion": {"major": 9, "minor": 0, "revision": 7, "architecture": "x64", "modernui": 1},
		"classnamespace": "box",
		"rect": rect,
		"openinpresentation": 1,
		"default_fontsize": 10.0,
		"default_fontface": 0,
		"default_fontname": MEDIUM,
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
			"title": title, "bgcolor": FACE, "editing_bgcolor": FACE, "locked_bgcolor": FACE})
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
	# The core draws the grid only while the window shows: shown when it takes the focus (the Edit key says so too),
	# closed by its closebang.
	focus = p.obj("active", 1, 1, ["int"])
	shown = p.obj("sel 1", 2, 2, ["bang", ""])
	p.line(focus, 0, shown)
	shown_msg = p.message(selector("window 1", prepend=False))
	p.line(shown, 0, shown_msg)
	p.line(shown_msg, 0, core_inlet(p))
	closed = p.obj("closebang", 1, 1, ["bang"])
	closed_msg = p.message(selector("window 0", prepend=False))
	p.line(closed, 0, closed_msg)
	p.line(closed_msg, 0, core_inlet(p))
	# The window's face is the patcher's own background colour (patcher()): a panel there, on the background layer,
	# hid the menus' keys drawn on that layer too (seen in Live), and in front it hid everything.

	# Top bar, in groups after rules: the Patterns; the Pattern shown (Copy to, Length, x2, Scale); the view (halves,
	# Follow); the Fill; Print.
	p.label(16, 22, 72, "Pattern")
	# The Patterns as the strip shows them: a button each, choosing through the strip's "Pattern" parameter.
	pattern_buttons(p, 96, 16, 28, 32, 16, 1, "Window Pattern Button %d")
	p.rule(616, 16, 1, 24)
	# Controls that edit the Pattern take no initial value: a live.* object sends its initial value when the device
	# loads, which would edit the Pattern (measured in Live: "hit 0" took Track 1's first Hit off). The core shows
	# their values when it redraws.
	# Copy to: the Pattern shown over the one chosen; the core sets the menu back on its label.
	copy_to = menu(p, [628, 16, 64, 24], "Copy to",
		["COPY TO"] + ["%02d" % (i + 1) for i in range(limit("patterns"))], initial=None)
	bind(p, copy_to, "copyto", "copyto")
	p.label(704, 22, 44, "Length")
	length = menu(p, [748, 16, 52, 24], "Length", [str(i + 1) for i in range(limit("steps"))], initial=None)
	bind(p, length, "length", "lengthindex")
	# x2: twice as long, the steps again after them, as Live's Duplicate Loop
	key_button(p, [804, 16, 28, 24], "Double", "×2", selector("double", prepend=False), core_inlet(p))
	p.label(844, 22, 36, "Scale")
	scale = menu(p, [880, 16, 60, 24], "Scale", VOCABULARY["scales"], initial=None)
	bind(p, scale, "scale", "scaleindex")
	p.rule(952, 16, 1, 24)
	half = tab(p, [964, 16, 116, 24], "Half", ["1-32", "33-64"])
	bind(p, half, "half", "half")
	follow = text_button(p, [1084, 16, 56, 24], "Follow", "Follow", initial=1)
	to_core(p, follow, "follow")
	p.rule(1152, 16, 1, 24)
	fill_button(p, [1164, 16, 56, 24], "Window Fill")
	p.rule(1232, 16, 1, 24)
	# Print: the Pattern shown over 1 to 16 Passes (issue 07, decision 10), or the last take as Capture writes it
	# (issue 23), into a Session clip per Track: the menu chooses, the button prints. The menu tells the core at once;
	# no initial value: the core tells the menu what it holds at each redraw.
	print_menu = menu(p, [1244, 16, 120, 24], "Print",
		["PATTERN ×%d" % (n + 1) for n in range(limit("passes"))] + ["LAST TAKE"], initial=None)
	bind(p, print_menu, "printsource", "printsource")
	key_button(p, [1368, 16, 56, 24], "Print Now", "Print", selector("print", prepend=False), core_inlet(p))
	p.rule(0, 52, WINDOW_W, 1)

	# Fill edit, above the Tracks' rows: the Hits the mouse puts take the Condition FILL, the others fade.
	fill_edit = text_button(p, [16, 60, 72, 20], "Fill Edit", "Fill edit")
	bind(p, fill_edit, "filledit", "filledit")
	# Clear: the Pattern shown back to an empty one (Live's undo gives it back). No initial value: it edits the Pattern.
	key_button(p, [96, 60, 64, 20], "Clear", "Clear", selector("clear", prepend=False), core_inlet(p))

	# Grid, with its column numbers every 4 steps, a row per Track, then the trig row
	x0, y0 = 176, 88
	cells = lcd_geometry("grid")
	numbers = next(k["indexed"] for k in VOCABULARY["panel"] if k["key"] == "colnum")
	for i in range(numbers):
		column = 4 * i
		p.text(x0 + grid_x(column), 66, 4 * cells["pitch"], 12, str(column + 1), receive="---colnum%d" % i,
			colour=INK_FAINT)
	grid = lcd(p, [x0, y0, cells["width"], cells["height"]])
	p.receive("---grid", grid)
	to_core(p, grid, "gridclick", 0)
	to_core(p, grid, "gridmouse", 2)
	for t in range(limit("tracks")):
		y = y0 + cells["rowPitch"] * t
		# The number, its Machine once MD Drums tells it (the Track the strip shows in ink), and its Mute
		p.text(16, y + 6, 20, 12, "%02d" % (t + 1), colour=INK_FAINT)
		fitted("trackName", 92, 12)
		name = p.text(40, y + 5, 92, 14, "", size=12, receive="---trackname%d" % t)
		# Its colour only when the Track shown comes to it or leaves it: change starts at 0, the dim look's.
		shown = p.obj("== %d" % t, 2, 1, ["int"])
		p.receive("---trackindex", shown)
		changed = p.obj("change", 1, 3, ["", "int", "int"])
		p.line(shown, 0, changed)
		looks = p.obj("sel 1 0", 1, 3, ["bang", "bang", ""])
		p.line(changed, 0, looks)
		show_looks(p, looks, [look(textcolor=INK), look(textcolor=INK_DIM)], name)
		activator(p, [136, y, 24, cells["cellHeight"]], "Window Track %d On" % (t + 1), t)
	# The trig row, the Machinedrum's trigs for every Track (issue 26), one at a time as the menu at its left
	# chooses: Swing (its steps play late by the Pattern's swing, the menu beside it, its index 50 % at 0), Accent
	# (every Hit of its steps plays at 128 + 2 x the Pattern's amount, the number beside it: note 88 to MD Drums), or
	# Slide (nothing beside it). The swing's menu and the Accent's number stand in one place, a script showing the
	# one the row needs; the core tells the row at each redraw.
	y = y0 + cells["rowPitch"] * limit("tracks")
	trig = menu(p, [16, y, 60, 24], "Trig Row", ["SWING", "ACCENT", "SLIDE"], initial=None)
	bind(p, trig, "trigrow", "trigrow")
	swing = menu(p, [80, y, 72, 24], "Swing", ["%d%%" % s for s in range(limit("minSwing"), limit("maxSwing") + 1)],
		initial=None, well="swingwell")
	bind(p, swing, "swing", "swing")
	accent = numbox(p, [80, y, 72, 24], "Accent", 0, limit("maxAccent"), well="accentwell")
	bind(p, accent, "accent", "accent")
	show_by(p, "---trigrow", [["Swing", "swingwell"], ["Accent", "accentwell"], []])
	p.rule(0, 576, WINDOW_W, 1)

	# Step panel: the step's Track as its title, the step in a screen beside it
	fitted("trackTitle", 168, 14, bold=True)
	p.text(16, 594, 168, 18, "Track 01", size=14, bold=True, colour=INK, receive="---tracktitle")
	p.screen([192, 592, 148, 20], [("Step 1, page 1", "---steptitle")])
	hit = text_button(p, [16, 644, 64, 24], "Step Hit", "Hit", initial=None)
	bind(p, hit, "hit", "hit")
	# The velocity's dial alone, its name and value beside it as the Locks' are
	# Double-clicked, back to 127, a new Hit's velocity (guarded: not sent as the device loads)
	velocity = dial(p, [96, 636, 40, 40], "Step Velocity", "Velocity", 1, 127, 127)
	to_core(p, guarded(p, velocity), "velocity")
	set_from(p, "---velocity", velocity)
	p.text(144, 638, 80, 12, "VELOCITY")
	velocity_value = p.text(144, 656, 64, 14, "", size=12, bold=True, colour=INK)
	set_from(p, "---velocity", velocity_value)
	set_on(p, velocity, velocity_value)
	p.label(16, 692, 140, "Condition")
	condition = menu(p, [16, 708, 140, 24], "Condition", [c.upper() for c in VOCABULARY["conditions"]],
		initial=None)
	bind(p, condition, "condition", "condition")
	p.label(172, 692, 168, "Micro-timing")
	micro, micro_bar = bipolar_bar(p, [172, 708, 168, 24], "Micro-timing", -limit("micro"), limit("micro"))
	to_core(p, guarded(p, micro), "micro")
	set_from(p, "---micro", micro)
	set_from(p, "---micro", micro_bar)
	p.screen([172, 734, 168, 18], [("On the grid", "---microtext")])
	p.label(16, 756, 140, "Ratchets")
	ratchets = menu(p, [16, 772, 64, 24], "Ratchets", ["×%d" % (i + 1) for i in range(limit("ratchets"))],
		initial=None)
	bind(p, ratchets, "ratchets", "ratchetsindex")
	p.label(172, 756, 168, "Ratchet velocity")
	curve, curve_bar = bipolar_bar(p, [172, 772, 168, 24], "Ratchet velocity", -limit("curve"), limit("curve"))
	to_core(p, guarded(p, curve), "curve")
	set_from(p, "---curve", curve)
	set_from(p, "---curve", curve_bar)
	p.screen([172, 798, 168, 18], [("One Hit", "---curvetext")])
	p.rule(352, 592, 1, 248)

	assert len(SYN + EFX + ROUTING) == limit("locks")
	for g, (group, names) in enumerate([("SYN", SYN), ("EFX", EFX), ("ROUTING", ROUTING)]):
		y = 596 + 88 * g
		p.label(368, y + 14, 72, group)
		for i, name in enumerate(names):
			param = 8 * g + i
			x = 448 + 120 * i
			lock = lock_dial(p, [x, y, 40, 40], "Lock %d" % (param + 1), name)
			to_core(p, guarded(p, lock), "lock %d" % param)
			# SYN1-8 take the names the step selected's Machine gives them, once MD Drums tells it; under the name,
			# what the dial holds: the Lock, in the accent, or the Kit's value, in the faint ink
			fitted("lockName", 64, 10)
			label = p.text(x + 48, y + 2, 64, 12, name, receive="---lockname%d" % param if g == 0 else None)
			value = p.text(x + 48, y + 20, 72, 14, "Kit", size=12, bold=True, colour=INK_FAINT,
				receive="---lockvalue%d" % param)
			# lock <value> <locked> from the core: the position, and the accent on the dial and its value while the
			# step holds a Lock, its name in ink
			values = p.obj("unpack 0 0", 1, 2, ["int", "int"])
			p.receive("---lock%d" % param, values)
			set_on(p, values, lock)
			# Colours only when a Lock comes or goes, not at each step shown: change starts at 0, the unlocked look's.
			changed = p.obj("change", 1, 3, ["", "int", "int"])
			p.line(values, 1, changed)
			locked = p.obj("sel 1", 2, 2, ["bang", ""])
			p.line(changed, 0, locked)
			show_looks(p, locked, [look(activeneedlecolor=INK, activedialcolor=ACCENT),
				look(activeneedlecolor=INK_FAINT, activedialcolor=RING)], lock)
			show_looks(p, locked, [look(textcolor=ACCENT), look(textcolor=INK_FAINT)], value)
			show_looks(p, locked, [look(textcolor=INK), look(textcolor=INK_DIM)], label)

	# Footer: what Print last did, in a screen, and the device's own setting, away from the step's
	p.rule(0, 840, WINDOW_W, 1)
	p.screen([16, 846, 1264, 18], [("", "---printstatus")])
	# Whether the device looks for a newer release, for every set (the external keeps it in its settings file). No
	# initial value: the external tells the switch's state.
	updates = text_button(p, [1296, 847, 128, 16], "Check Updates", "Check for updates", initial=None)
	bind(p, updates, "updates", "updates")

	return p


def page_buttons(p):
	"""The strip's four pages of 16 steps as keys: the one shown filled, those past the Pattern's length inactive.
	A click asks for its page; the core answers "pages p n" at once, which fills the page shown again."""
	pages = p.obj("unpack 0 0", 1, 2, ["int", "int"])
	p.receive("---pages", pages)
	s = lcd_geometry("strip")
	count = limit("steps") // 16
	for pg in range(count):
		# Under the 4 x 4 cells of its page, as wide, less the gap after the last
		button, _ = key_button(p, [172 + 4 * s["pitch"] * pg, 122, 4 * s["pitch"] - (s["pitch"] - s["cellWidth"]), 16],
			"Page %d" % (pg + 1),
			"%d-%d" % (16 * pg + 1, 16 * pg + 16), selector("page %d" % pg, prepend=False), core_inlet(p))
		on = p.obj("== %d" % pg, 2, 1, ["int"])
		p.line(pages, 0, on)
		set_on(p, on, button)
		within = p.obj("> %d" % pg, 2, 1, ["int"])
		p.line(pages, 1, within)
		set_active = p.obj("prepend active")
		p.line(within, 0, set_active)
		p.line(set_active, 0, button)


def device():
	p = Patch("s")

	# The face, behind everything: v12's own colour, not Live's theme.
	p.add("panel", 1, 0, None, None, [0, 0, STRIP_W, STRIP_H], bgcolor=FACE, rounded=0, background=1, ignoreclick=1,
		back=True)

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

	# Push 2 (issue 33). The core takes Push's controls through the LOM while the strip's switch is on, the device on
	# and its track selected: it learns the device's on/off from live.thisdevice, and the track selected from an
	# observer of Live's view. Each control it takes, a live.observer reports to it ("pushobs i id n" points
	# observer i at it, "pushvalue i ..." comes back), the pad matrix's first, then the buttons', then the
	# encoders'.
	to_core(p, device_ready, "deviceon", outlet=1)
	view = p.obj("live.path", 1, 3, ["", "", ""])
	view_path = p.message("goto live_set view")
	p.line(device_ready, 0, view_path)
	p.line(view_path, 0, view)
	selected = p.obj("live.observer selected_track", 2, 2, ["", ""])
	p.line(view, 0, selected, 1)
	to_core(p, selected, "selectedtrack")
	observers = next(k["indexed"] for k in VOCABULARY["panel"] if k["key"] == "pushobs")
	for n in range(observers):
		observer = p.obj("live.observer value", 2, 2, ["", ""])
		p.receive("---pushobs%d" % n, observer, 1)
		to_core(p, observer, "pushvalue %d" % n)

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
	p.label(8, 6, 140, "Pattern")
	# The Pattern chosen: a Live parameter, so that the set keeps it and the Arrangement can automate the switches. Its
	# live.tab stays out of the strip: one on several rows wraps by its width (measured in Live: 140 px hold two items a
	# row, whatever num_lines), so the strip shows 16 buttons in 4 x 4, each choosing its Pattern through the tab, lit
	# while the core says its Pattern is the one chosen.
	items = ["%02d" % (i + 1) for i in range(limit("patterns"))]
	pattern = p.add("live.tab", 1, 3, None, ["", "", "float"], None, rect=[20, 3000, 140, 24], parameter_enable=1,
		varname="Pattern", num_lines_presentation=1, num_lines_patching=1, multiline=0, active=1,
		saved_attribute_attributes=valueof("Pattern", "Pattern", 2, AUTOMATED, enum=items, initial=0))
	bind(p, pattern, "pattern", "patternindex")
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
	# What plays and the Pattern's length, in one screen of two lines
	p.screen([8, 132, 148, 30], [("Stopped", "---status"), ("16 steps, 1/16", "---lengthtext")])
	p.rule(160, 8, 1, 153)

	# Tracks, Mutes, the steps of the Track shown, pages
	p.label(172, 6, 300, "Track")
	s = lcd_geometry("strip")
	# One tab per cell of the steps' lcd, as wide; its item holds what fit::TrackTab cuts the label to
	fitted("trackTab", s["cellWidth"], 10)
	tracks = tab(p, [172, 22, s["width"], 24], "Track", ["%02d" % (t + 1) for t in range(limit("tracks"))])
	to_core(p, tracks, "track")
	# Each tab names its Track's Machine once MD Drums tells it (ADR 0003), by the short name a cell of 36 px holds
	# ("01 BD"), then the core sets the tab chosen again.
	names = p.obj("prepend _parameter_range")
	p.line(names, 0, tracks)
	p.receive("---tracknames", names)
	set_from(p, "---trackindex", tracks)
	for t in range(limit("tracks")):
		activator(p, [172 + s["pitch"] * t, 50, s["cellWidth"], 16], "Track %d On" % (t + 1), t, visibility=AUTOMATED)
	steps = lcd(p, [172, 74, s["width"], s["height"]])
	p.line(core, 1, steps)
	to_core(p, steps, "stripclick", 0)
	to_core(p, steps, "stripmouse", 2)
	page_buttons(p)
	# A newer release, once a day at most (the external asks GitHub), in a screen of its own; nothing at all without
	# one
	p.screen([172, 144, s["width"], 18], [("", "---update")], varname="news", colour=AMBER)
	show_by(p, "---updateshown", [[], ["news", "news0"]])
	p.rule(816, 8, 1, 153)

	# Play: what the hands do while it plays, and the chain playing in a screen under them
	p.label(828, 6, 152, "Play")
	# Held, the Fill plays FILL steps (read when a step is planned); a Live parameter, for controllers.
	fill_button(p, [828, 22, 72, 36], "Fill", visibility=AUTOMATED)
	# Chain: on, the Pattern buttons clicked make a new chain; off, it plays, each Pattern for one Pass, round and round
	chain = text_button(p, [908, 22, 72, 36], "Chain", "Chain")
	bind(p, chain, "chain", "chainmode")
	p.screen([828, 62, 152, 18], [("", "---chaintext")])

	# Output: who plays MD Drums, and the take written as clips, its length in a screen
	p.label(828, 86, 152, "Output")
	# Clips only: the device sends no note-on, so that captured or printed clips play MD Drums alone. A Live
	# parameter, which the Arrangement can automate; "Output" as before v12, off for the device, on for the clips.
	output = text_button(p, [828, 102, 72, 20], "Output", "Clips only", visibility=AUTOMATED)
	to_core(p, output, "output")
	# Capture (issue 23): the last take into Session clips, the button active once a take holds a note. A button: no
	# initial value, which would capture as the device loads.
	capture, _ = key_button(p, [908, 102, 72, 20], "Capture", "Capture", selector("capture", prepend=False),
		core_inlet(p), active=0)
	ready = p.obj("prepend active")
	p.line(ready, 0, capture)
	p.receive("---takeready", ready)
	p.screen([828, 126, 152, 18], [("No take", "---take")])

	# The window, and Push 2 played as the device's while its track is selected (issue 33): off as the device loads.
	pcontrol = p.obj("pcontrol", 1, 1)
	_, open_msg = key_button(p, [828, 148, 72, 16], "Edit", "Edit", "open", pcontrol)
	opened = p.obj("t b", 1, 1, ["bang"])
	p.line(open_msg, 0, opened)
	opened_msg = p.message(selector("window 1", prepend=False))
	p.line(opened, 0, opened_msg)
	p.line(opened_msg, 0, core)
	if CLASS != "mdseq":
		# A development build opens its edit window once loaded, so that it can be looked at without a click.
		open_later = p.obj("delay 1000")
		p.line(device_ready, 0, open_later)
		p.line(open_later, 0, open_msg)
	window = edit_window()
	window_box = p.add("newobj", 1, 0, "patcher window", None, None, rect=[20, 2000, 120, 22],
		patcher=patcher("MoDule Seq: Edit", window.boxes, window.lines, [80.0, 80.0, WINDOW_W, WINDOW_H],
			window=True))
	p.line(pcontrol, 0, window_box)
	push = text_button(p, [908, 148, 72, 16], "Push", "Push")
	to_core(p, push, "push")

	return patcher("MoDule Seq", p.boxes, p.lines, [100.0, 100.0, 1200.0, 800.0], width=STRIP_W)


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
