"""Writes a release's zip: MModuleSeq-<version>-win64.zip, holding the folder "MoDule Seq/" that a user drops into
Live's User Library (Presets/MIDI Effects/Max MIDI Effect): the device, the external it loads from its own folder,
the README, the licence and the notices.

It refuses a development build: its external has another class (-DMDSEQ_CLASS=mdseq_dbgN), a demonstration Pattern
and a log, and is no release.

Run: python -I package.py <device folder> <version> <source folder> <output folder>
(the build's target zip_release runs it with build/device, the project's version, the repository and build/)
"""
import pathlib
import sys
import zipfile

# Run isolated, Python does not look beside the script; and it leaves no compiled copies in the source tree.
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
sys.dont_write_bytecode = True
from check_device import read  # noqa: E402

FILES_FROM_BUILD = ["MoDule Seq.amxd", "mdseq.mxe64"]
FILES_FROM_SOURCE = ["README.md", "LICENSE.md", "NOTICE.md"]


def main():
	if len(sys.argv) != 5:
		sys.exit("usage: package.py <device folder> <version> <source folder> <output folder>")
	device, version, source, output = (pathlib.Path(sys.argv[1]), sys.argv[2], pathlib.Path(sys.argv[3]),
		pathlib.Path(sys.argv[4]))
	for name in FILES_FROM_BUILD:
		if not (device / name).is_file():
			sys.exit("package.py: no %s in %s (a release build writes both)" % (name, device))
	for name in FILES_FROM_SOURCE:
		if not (source / name).is_file():
			sys.exit("package.py: no %s in %s" % (name, source))
	# The device names its external's class in its patch: "mdseq" for a release.
	texts = {e["box"].get("text") for e in read(device / "MoDule Seq.amxd")["boxes"]}
	if "mdseq" not in texts:
		sys.exit("package.py: %s is not a release device (its external is not the class mdseq)" %
			(device / "MoDule Seq.amxd"))

	output.mkdir(parents=True, exist_ok=True)
	zip_path = output / ("MModuleSeq-%s-win64.zip" % version)
	with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as z:
		for name in FILES_FROM_BUILD:
			z.write(device / name, "MoDule Seq/" + name)
		for name in FILES_FROM_SOURCE:
			z.write(source / name, "MoDule Seq/" + name)
	print("written:", zip_path)


if __name__ == "__main__":
	main()
