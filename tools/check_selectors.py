"""Checks the rule make_device.py and check_device.py hold the patch's messages to, against the vocabulary the core
writes: a prepend gives exactly the integers a message fixes before the control's value, a message box at least those
the external needs (a Mute's Track and state, a Lock's parameter and value). A button that sends "mute 3" then fails
the gate rather than being refused in Live. The device itself has none such: these are messages it does not send.

Run: python -I check_selectors.py <vocabulary.json>; exits 1 on the first list of problems.
"""
import pathlib
import sys

# Run isolated, Python does not look beside the script; and it leaves no compiled copies in the source tree.
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
sys.dont_write_bytecode = True
import check_device  # noqa: E402
import make_device  # noqa: E402

# Each message as a box sends it to ---in, and whether the external takes it.
MESSAGE_BOXES = [("mute 3", False), ("mute 3 1", True), ("lock 7", False), ("lock 7 0", True), ("restore", False),
	("restore 2", True), ("hit 1", True), ("redraw", True)]
PREPENDS = [("mute 3", True), ("mute", False), ("mute 3 1", False), ("lock 7", True), ("lock", False), ("hit", True),
	("hit 1", False)]


def device_takes(vocabulary, text, prepend):
	"""Whether check_device finds nothing wrong with text sent to ---in, from a prepend or a message box, a
	registered message nothing sends aside."""
	source = {"id": "a", "maxclass": "newobj", "text": "prepend " + text} if prepend else {"id": "a",
		"maxclass": "message", "text": text}
	boxes = {"a": source, "b": {"id": "b", "maxclass": "newobj", "text": "s ---in"}}
	lines = [{"patchline": {"source": ["a", 0], "destination": ["b", 0]}}]
	problems = []
	check_device.check_messages([("test", boxes, lines)], vocabulary, problems)
	return not [p for p in problems if "nothing sends" not in p]


def generator_takes(text, prepend):
	try:
		make_device.selector(text, prepend)
		return True
	except ValueError:
		return False


def main():
	make_device.load_vocabulary(sys.argv[1])
	vocabulary = make_device.VOCABULARY
	problems = []
	for prepend, cases in ((False, MESSAGE_BOXES), (True, PREPENDS)):
		form = "prepend" if prepend else "message box"
		for text, takes in cases:
			verdict = "takes" if takes else "refuses"
			if generator_takes(text, prepend) != takes:
				problems.append("make_device.py should %s the %s %r" % (verdict[:-1], form, text))
			if device_takes(vocabulary, text, prepend) != takes:
				problems.append("check_device.py should %s the %s %r" % (verdict[:-1], form, text))
	if problems:
		print("\n".join(problems))
		sys.exit(1)
	print("selectors OK: %d message boxes, %d prepends" % (len(MESSAGE_BOXES), len(PREPENDS)))


if __name__ == "__main__":
	main()
