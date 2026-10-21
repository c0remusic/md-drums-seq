#pragma once

// What the device's two lcd objects show, as Design/HANDOFF.md places and draws it: the strip's 16 steps of the Track
// shown, and the edit window's grid of 16 Tracks by 32 steps. Pure functions; the external turns the operations into
// lcd messages.

#include "mdseqTypes.h"

#include <cstddef>
#include <cstdint>

namespace mdseq
{
	struct Rgb
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;

		bool operator==(const Rgb& _other) const { return r == _other.r && g == _other.g && b == _other.b; }
	};

	// Design/v12's: lcd neither follows Live's theme nor blends, so the colours are opaque. Each empty step is a key on
	// the device's face (#2a2b2f, the lcd's own background), every fourth lighter; past the Pattern's length a step is
	// darker than the face, off.
	namespace colours
	{
		constexpr Rgb Panel{0x3d, 0x3f, 0x44};
		constexpr Rgb Beat{0x4a, 0x4c, 0x52};
		constexpr Rgb Past{0x20, 0x21, 0x25};
		constexpr Rgb Hit{0xf2, 0xf2, 0xf2};		// the device's ink
		constexpr Rgb Accent{0x6f, 0xd1, 0xc4};		// MoDule's turquoise: the step played, the selection, the Locks
		constexpr Rgb Accented{0xe8, 0xb3, 0x5a};	// the Accent row's steps
		constexpr Rgb Sliding{0x9f, 0xb7, 0xff};	// the Slide row's steps
		constexpr Rgb Swing{0x9a, 0x9d, 0xa3};		// the swing row's marks, quieter than the accent
		constexpr Rgb Text{0x1c, 0x1d, 0x20};		// a Condition's label, on its Hit
		constexpr Rgb End{0xa2, 0xa2, 0xa2};		// the line after the Pattern's last step
	}

	// The lcd's text: Ableton Sans bold (Face::Bold in mdseqText.h) at 10 px.
	constexpr int LcdFontSize = 10;

	// _a towards _b by _t, rounded per channel.
	Rgb mix(Rgb _a, Rgb _b, double _t);

	// One lcd drawing message: clear, paintrect / framerect with left, top, right and bottom, a triangle
	// (paintpoly) whose base stands on x = left from top to bottom and whose tip is at x = right, halfway, or text
	// written in bold 10 px from (left, top), its baseline.
	struct DrawOp
	{
		enum class Kind : uint8_t
		{
			Clear,
			PaintRect,
			FrameRect,
			Triangle,
			Text
		};

		Kind kind;
		Rgb colour;
		int16_t left;
		int16_t top;
		int16_t right;
		int16_t bottom;
		char text[8];

		bool operator==(const DrawOp& _other) const
		{
			for(size_t i = 0; i < sizeof(text); ++i)
				if(text[i] != _other.text[i])
					return false;
			return kind == _other.kind && colour == _other.colour && left == _other.left && top == _other.top &&
				right == _other.right && bottom == _other.bottom;
		}
	};

	struct CellLook
	{
		bool beat = false;		// the first step of each group of 4
		bool played = false;
		bool selected = false;
		bool faded = false;		// a Hit drawn at FadedHit of its colour: one the Fill does not concern, in Fill edit
		bool accented = false;	// a step of the Pattern's Accent row (the strip's cells only)
		bool past = false;		// past the Pattern's length: off, a Hit kept there only hinted
	};

	// How much of its colour a faded Hit keeps over the background.
	constexpr double FadedHit = 0.35;

	// How much of the ink a Hit kept past the Pattern's length shows over Past.
	constexpr double PastHit = 0.18;

	// The background, the Hit (or an accented step's well), a notch per ratchet, the Locks' square and mark, the
	// Condition's label, the played frame, the selected frame's two and the micro-timing's arrow.
	constexpr size_t MaxCellOps = 1 + 1 + MaxRatchets + 2 + 1 + 1 + 2 + 1;

	// One cell of _w by _h at (_x, _y), as Design/v12-ui.js draws it, every inset 2 px. Past the Pattern's length: the
	// Past background and, for a Hit, a block of PastHit of the ink, nothing more. Otherwise: the background (lighter
	// on a beat, tinted when played); on a step of the Accent row the Accented colour takes the cell whole, as the
	// Hit's own colour or, without a Hit, as a well of a quarter of it; the Hit, from halfway to its colour at velocity
	// 0 to all of it at 127; its ratchets (from 2, a 2 x 4 px notch of the background's colour in the Hit's top edge,
	// on an even pixel at the middle of each ratchet's share); the Locks' mark (a 4 px accent square in an 8 px square
	// of the panel's colour, in the top right corner); the Condition's label, centred, its baseline 6 px up from the
	// bottom; the played frame (1 px), the selected frame (2 px), and over the frames the micro-timing (an arrow of the
	// Hit's colour, 2 px long and 8 px high, halfway down the cell's left margin when early, its right one when late;
	// the Hit itself stays on the grid). Returns the number of operations written to _out, at most MaxCellOps.
	size_t drawCell(const Step& _step, const CellLook& _look, int _x, int _y, int _w, int _h, DrawOp* _out);

	// One cell of the swing row: the background as drawCell paints it (Past alone past the length), and on a step that
	// swings a mark in the Swing colour, inset as a Hit is, its left edge moved right by half the swing's delay on an
	// even pixel so that the row shows how late those steps fall; at 50 % the swing plays nothing and the mark is
	// dimmer (drawMarkCell's, moved). Then the played frame. Returns the number of operations written to _out, at
	// most MaxMarkCellOps.
	size_t drawSwingCell(bool _swings, int _swing, const CellLook& _look, int _x, int _y, int _w, int _h, DrawOp* _out);

	// The background, the mark and the played frame.
	constexpr size_t MaxMarkCellOps = 3;

	// One cell of the Accent or the Slide row: the background as drawCell paints it (Past alone past the length), on a
	// step of the row a mark of _mark's colour, inset as a Hit is and its left edge _shift further, then the played
	// frame. Returns the number of operations written to _out, at most MaxMarkCellOps.
	size_t drawMarkCell(bool _marked, Rgb _mark, const CellLook& _look, int _x, int _y, int _w, int _h, DrawOp* _out,
		int _shift = 0);

	// The strip's lcd: 16 cells of 36 x 40 at a pitch of 40.
	namespace strip
	{
		constexpr int Width = 636;
		constexpr int Height = 40;
		constexpr int Cells = 16;
		constexpr int CellWidth = 36;
		constexpr int Pitch = 40;

		constexpr int cellX(const int _cell) { return _cell * Pitch; }

		// The cell under a click, -1 in a gap or outside.
		int cellAt(int _x, int _y);

		// The line after a Pattern of _length steps on page _page (16 steps each): 2 px of End down the lcd, centred in
		// the gap after the last step, when that step is on the page and a gap follows it. False otherwise.
		bool endLine(int _length, int _page, DrawOp& _out);
	}

	// The edit window's lcd: 32 columns of 36 x 24 at a pitch of 38, 4 px more between the two pages of 16, and 16
	// rows of Tracks, then the trig row, at a pitch of 28: the swing, Accent or Slide row, as the menu at its left
	// chooses.
	namespace grid
	{
		constexpr int Width = 1220;
		constexpr int Height = 472;
		constexpr int Columns = 32;
		constexpr int Rows = TrackCount;	// the Tracks' rows
		constexpr int TrigRow = TrackCount;
		constexpr int CellWidth = 36;
		constexpr int CellHeight = 24;
		constexpr int Pitch = 38;
		constexpr int PageGap = 4;
		constexpr int RowPitch = 28;

		constexpr int cellX(const int _column) { return _column * Pitch + (_column >= 16 ? PageGap : 0); }
		constexpr int cellY(const int _row) { return _row * RowPitch; }

		struct Cell
		{
			int row;
			int column;
		};

		// The cell under a click, {-1, -1} in a gap or outside.
		Cell cellAt(int _x, int _y);

		// The line after a Pattern of _length steps in half _half (32 steps each): 2 px of End down the lcd, the trig
		// row's too, centred in the gap after the last step, when that step is in the half. False otherwise.
		bool endLine(int _length, int _half, DrawOp& _out);
	}
}
