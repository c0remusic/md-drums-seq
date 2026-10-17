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

	// Live 12's Dark theme, approximated: lcd neither follows the theme nor blends, so the colours are opaque.
	namespace colours
	{
		constexpr Rgb Panel{0x1e, 0x1e, 0x1e};
		constexpr Rgb Beat{0x26, 0x26, 0x26};
		constexpr Rgb Hit{0xff, 0xa5, 0x2e};
		constexpr Rgb Accent{0x7c, 0xc6, 0xff};
		constexpr Rgb Condition{0xc9, 0xa0, 0xff};
		constexpr Rgb Text{0x1e, 0x1e, 0x1e};
	}

	// _a towards _b by _t, rounded per channel.
	Rgb mix(Rgb _a, Rgb _b, double _t);

	// One lcd drawing message: clear, paintrect / framerect / paintoval with left, top, right and bottom, a triangle
	// (paintpoly) whose base stands on x = left from top to bottom and whose tip is at x = right, halfway, or text
	// written in bold 10 px from (left, top), its baseline.
	struct DrawOp
	{
		enum class Kind : uint8_t
		{
			Clear,
			PaintRect,
			FrameRect,
			PaintOval,
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
	};

	// How much of its colour a faded Hit keeps over the background.
	constexpr double FadedHit = 0.35;

	// The background, the Hit, a notch per ratchet, the Locks' two discs, the Condition's strip and label, the played
	// frame, the selected frame's two and the micro-timing's arrow.
	constexpr size_t MaxCellOps = 2 + MaxRatchets + 2 + 2 + 1 + 2 + 1;

	// One cell of _w by _h at (_x, _y), in HANDOFF's order: background (lighter on a beat, tinted when played), the
	// Hit inset by 3 px and tinted by velocity, its ratchets (from 2, a 2 x 4 px notch of the background's colour in
	// the Hit's top edge, at the middle of each ratchet's share), the Locks' dot (a 5 px accent disc ringed in the
	// panel's colour, near the top right corner), the Condition (a 3 px strip in the bottom margin and its label
	// centred above it, 6 px up from the bottom), the played frame (1 px), the selected frame (2 px), and over the
	// frames the micro-timing (an arrow of the Hit's colour, 3 px long and 8 px high, halfway down the cell's left
	// margin when early, its right one when late; the Hit itself stays on the grid). Returns the number of operations
	// written to _out, at most MaxCellOps.
	size_t drawCell(const Step& _step, const CellLook& _look, int _x, int _y, int _w, int _h, DrawOp* _out);

	// The background, the swing mark and the played frame.
	constexpr size_t MaxSwingCellOps = 3;

	// One cell of the swing row: the background as drawCell paints it, and on a step that swings a mark in the accent
	// colour, inset as a Hit is, its left edge moved right by half the swing's delay so that the row shows how late
	// those steps fall; at 50 % the swing plays nothing and the mark is dimmed. Then the played frame. Returns the
	// number of operations written to _out, at most MaxSwingCellOps.
	size_t drawSwingCell(bool _swings, int _swing, const CellLook& _look, int _x, int _y, int _w, int _h, DrawOp* _out);

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
	}

	// The edit window's lcd: 32 columns of 36 x 24 at a pitch of 38, 4 px more between the two pages of 16, and 16
	// rows of Tracks then the swing row at a pitch of 28.
	namespace grid
	{
		constexpr int Width = 1220;
		constexpr int Height = 472;
		constexpr int Columns = 32;
		constexpr int Rows = TrackCount;	// the Tracks' rows
		constexpr int SwingRow = TrackCount;
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
	}
}
