#pragma once

// The device's texts in Ableton Sans, the typeface Max carries inside Live: their widths from the font's own advances,
// and a text cut to the box that shows it, since a live.comment breaks a long line in two and a live.tab cuts it
// mid-letter. Pure functions.

#include <cstdint>
#include <string>
#include <string_view>

namespace mdseq
{
	// The faces the device uses.
	enum class Face : uint8_t
	{
		Medium,		// keys, menus, names, screens
		Bold		// titles, values, the lcd's labels
	};

	// The face's name as Live's own devices give it: "Ableton Sans Medium", "Ableton Sans Bold Regular".
	const char* fontName(Face _face);

	// _text's width in _face at _size px, in hundredths of a pixel: the advances read from the font files Max carries
	// in Live 12 (Resources/Max/resources/fonts) for printable ASCII and the ellipsis; a character beyond them, one
	// UTF-8 sequence, counts as an average letter.
	int textWidth(std::string_view _text, Face _face, int _size);

	// What a text is fitted to: its face and size, the width it may take in px, and whether a cut text ends in an
	// ellipsis.
	struct TextBox
	{
		Face face;
		int size;
		int width;
		bool ellipsis;
	};

	// _text whole when it is _box.width px wide at most, else cut after its last character that leaves room, trailing
	// spaces dropped, and an ellipsis after it when the box takes one. Never cuts a UTF-8 sequence.
	std::string fitText(std::string_view _text, const TextBox& _box);

	// The panel's fitted texts, each a little narrower than where Live broke or cut a longer one (measured on
	// 2026-10-20 with a user's Sample named gretsch_tom_06): a strip Track tab (36 px), a window row's Machine (a box
	// of 92 px broke the line between 84 and 91 px), the step panel's title (168 px, broken at 168), a Lock's name
	// (64 px). The vocabulary carries them, and tools/make_device.py refuses a box narrower or in another face.
	namespace fit
	{
		constexpr TextBox TrackTab{Face::Medium, 10, 34, false};
		constexpr TextBox TrackName{Face::Medium, 12, 84, true};
		constexpr TextBox TrackTitle{Face::Bold, 14, 160, true};
		constexpr TextBox LockName{Face::Medium, 10, 60, true};
	}
}
