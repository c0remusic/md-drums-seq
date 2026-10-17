// What the lcd objects draw and where a click lands: the cell's colours and order from Design/HANDOFF.md, its ratchet
// notches where the mockup cuts them, its micro-timing arrow, the strip's and the grid's cells with their gaps.

#include "mdseqTestSupport.h"
#include "mdseqView.h"

#include <cstdio>

namespace
{
	using namespace mdseq;

	DrawOp paint(const Rgb _c, const int _l, const int _t, const int _r, const int _b)
	{
		return {DrawOp::Kind::PaintRect, _c, static_cast<int16_t>(_l), static_cast<int16_t>(_t), static_cast<int16_t>(_r),
			static_cast<int16_t>(_b)};
	}

	DrawOp frame(const Rgb _c, const int _l, const int _t, const int _r, const int _b)
	{
		return {DrawOp::Kind::FrameRect, _c, static_cast<int16_t>(_l), static_cast<int16_t>(_t), static_cast<int16_t>(_r),
			static_cast<int16_t>(_b)};
	}

	Step hitStep(const int _velocity)
	{
		Step s;
		s.hit = true;
		s.velocity = static_cast<uint8_t>(_velocity);
		return s;
	}

	void colourMixes()
	{
		CHECK(mix(colours::Panel, colours::Hit, 1.0) == colours::Hit);
		CHECK(mix(colours::Panel, colours::Hit, 0.0) == colours::Panel);
		// The mockup's own rounding (Math.round), velocity 40 and the played tint.
		CHECK(mix(colours::Panel, colours::Hit, 0.35 + 0.65 * 40 / 127.0) == (Rgb{155, 105, 39}));
		CHECK(mix(colours::Panel, colours::Accent, 0.18) == (Rgb{47, 60, 71}));
	}

	void cells()
	{
		DrawOp ops[MaxCellOps];

		size_t n = drawCell(Step{}, CellLook{}, 0, 0, 36, 24, ops);
		CHECK(n == 1);
		CHECK(ops[0] == paint(colours::Panel, 0, 0, 36, 24));

		CellLook beat;
		beat.beat = true;
		n = drawCell(Step{}, beat, 38, 28, 36, 24, ops);
		CHECK(n == 1);
		CHECK(ops[0] == paint(colours::Beat, 38, 28, 74, 52));

		n = drawCell(hitStep(127), CellLook{}, 0, 0, 36, 40, ops);
		CHECK(n == 2);
		CHECK(ops[1] == paint(colours::Hit, 3, 3, 33, 37));

		// Faded (a Hit the Fill neither plays alone nor skips, while the Fill is edited): a third of its colour over
		// the background.
		CellLook faded;
		faded.faded = true;
		n = drawCell(hitStep(127), faded, 0, 0, 36, 40, ops);
		CHECK(n == 2);
		CHECK(ops[1] == paint(mix(colours::Panel, colours::Hit, FadedHit), 3, 3, 33, 37));

		// Everything at once, in HANDOFF's order: background, Hit, played frame, selected frame of 2 px.
		CellLook all;
		all.played = true;
		all.selected = true;
		n = drawCell(hitStep(40), all, 10, 20, 36, 24, ops);
		CHECK(n == 5);
		CHECK(ops[0] == paint(Rgb{47, 60, 71}, 10, 20, 46, 44));
		CHECK(ops[1] == paint(Rgb{155, 105, 39}, 13, 23, 43, 41));
		CHECK(ops[2] == frame(colours::Accent, 10, 20, 46, 44));
		CHECK(ops[3] == frame(colours::Accent, 10, 20, 46, 44));
		CHECK(ops[4] == frame(colours::Accent, 11, 21, 45, 43));
	}

	void lockDot()
	{
		DrawOp ops[MaxCellOps];
		Step s = hitStep(127);
		s.locks[17] = 0;
		CellLook selected;
		selected.selected = true;
		const size_t n = drawCell(s, selected, 0, 0, 36, 24, ops);
		CHECK(n == 6);
		// After the Hit, before the frames: a 7 px disc of the panel's colour, the 5 px accent inside it.
		CHECK(ops[2] == (DrawOp{DrawOp::Kind::PaintOval, colours::Panel, 27, 1, 34, 8}));
		CHECK(ops[3] == (DrawOp{DrawOp::Kind::PaintOval, colours::Accent, 28, 2, 33, 7}));
		CHECK(ops[4].kind == DrawOp::Kind::FrameRect);

		// No dot without a Hit, whatever the step holds.
		Step noHit;
		noHit.locks[3] = 40;
		CHECK(drawCell(noHit, CellLook{}, 0, 0, 36, 24, ops) == 1);
	}

	void conditionStrip()
	{
		DrawOp ops[MaxCellOps];
		Step s = hitStep(100);
		s.condition = 22;	// 1:2
		s.locks[0] = 10;
		CellLook all;
		all.played = true;
		all.selected = true;
		const size_t n = drawCell(s, all, 38, 28, 36, 24, ops);
		CHECK(n == 9);
		// After the Locks' dot, before the frames: the strip in the bottom margin, then the label centred above it.
		CHECK(ops[4] == (DrawOp{DrawOp::Kind::PaintRect, colours::Condition, 41, 49, 71, 52}));
		CHECK(ops[5] == (DrawOp{DrawOp::Kind::Text, colours::Text, 47, 46, 0, 0, "1:2"}));
		CHECK(ops[6].kind == DrawOp::Kind::FrameRect);
		CHECK(n <= MaxCellOps);
	}

	void ratchetNotches()
	{
		DrawOp ops[MaxCellOps];

		// One ratchet is the plain Hit: no notch.
		Step one = hitStep(127);
		CHECK(drawCell(one, CellLook{}, 0, 0, 36, 24, ops) == 2);

		// Three: 2 x 4 px of the background's colour in the Hit's top edge, at the middle of each third of the block,
		// where the mockup's Math.round puts them (v4-ui.js), and before the Locks' dot.
		Step three = hitStep(127);
		three.ratchets = 3;
		three.locks[0] = 10;
		CellLook played;
		played.played = true;
		size_t n = drawCell(three, played, 0, 0, 36, 24, ops);
		CHECK(n == 8);
		const Rgb tinted{47, 60, 71};
		CHECK(ops[2] == paint(tinted, 7, 3, 9, 7));
		CHECK(ops[3] == paint(tinted, 17, 3, 19, 7));
		CHECK(ops[4] == paint(tinted, 27, 3, 29, 7));
		CHECK(ops[5].kind == DrawOp::Kind::PaintOval);

		// Halves round up as Math.round does: 7.5 px gives 8.
		Step two = hitStep(127);
		two.ratchets = 2;
		n = drawCell(two, CellLook{}, 40, 0, 36, 40, ops);
		CHECK(n == 4);
		CHECK(ops[2] == paint(colours::Panel, 50, 3, 52, 7));
		CHECK(ops[3] == paint(colours::Panel, 65, 3, 67, 7));

		// Everything a cell can hold fills MaxCellOps exactly.
		Step full = hitStep(100);
		full.micro = 5;
		full.ratchets = MaxRatchets;
		full.condition = 22;
		full.locks[5] = 90;
		CellLook all;
		all.played = true;
		all.selected = true;
		CHECK(drawCell(full, all, 0, 0, 36, 24, ops) == MaxCellOps);
	}

	void microArrow()
	{
		DrawOp ops[MaxCellOps];

		// Early: the arrow's base on the Hit's left edge, its tip on the cell's, halfway down; drawn last, over the
		// selected frame, in the Hit's colour.
		Step early = hitStep(40);
		early.micro = -8;
		CellLook selected;
		selected.selected = true;
		size_t n = drawCell(early, selected, 38, 28, 36, 24, ops);
		CHECK(n == 5);
		CHECK(ops[4] == (DrawOp{DrawOp::Kind::Triangle, Rgb{155, 105, 39}, 41, 36, 38, 44}));

		// Late, in the strip's taller cell: in the right margin.
		Step late = hitStep(127);
		late.micro = 12;
		n = drawCell(late, CellLook{}, 40, 0, 36, 40, ops);
		CHECK(n == 3);
		CHECK(ops[2] == (DrawOp{DrawOp::Kind::Triangle, colours::Hit, 73, 16, 76, 24}));

		// On the grid, no arrow; without a Hit, none either.
		CHECK(drawCell(hitStep(127), CellLook{}, 0, 0, 36, 24, ops) == 2);
		Step none;
		none.micro = 4;
		CHECK(drawCell(none, CellLook{}, 0, 0, 36, 24, ops) == 1);
	}

	void stripClicks()
	{
		CHECK(strip::cellAt(0, 0) == 0);
		CHECK(strip::cellAt(35, 39) == 0);
		CHECK(strip::cellAt(36, 10) == -1);		// gap
		CHECK(strip::cellAt(39, 10) == -1);
		CHECK(strip::cellAt(40, 10) == 1);
		CHECK(strip::cellAt(635, 10) == 15);
		CHECK(strip::cellAt(636, 10) == -1);
		CHECK(strip::cellAt(10, 40) == -1);
		CHECK(strip::cellAt(-1, 10) == -1);
	}

	void gridClicks()
	{
		const auto at = [](const int _x, const int _y, const int _row, const int _column)
		{
			const grid::Cell c = grid::cellAt(_x, _y);
			return c.row == _row && c.column == _column;
		};
		CHECK(at(0, 0, 0, 0));
		CHECK(at(37, 0, -1, -1));		// gap between columns
		CHECK(at(38, 0, 0, 1));
		CHECK(at(605, 0, 0, 15));
		CHECK(at(606, 0, -1, -1));
		CHECK(at(610, 0, -1, -1));		// gap between the pages of 16
		CHECK(at(612, 0, 0, 16));
		CHECK(at(1217, 0, 0, 31));
		CHECK(at(1218, 0, -1, -1));
		CHECK(at(0, 24, -1, -1));		// gap between rows
		CHECK(at(0, 28, 1, 0));
		CHECK(at(0, 443, 15, 0));
		CHECK(at(0, 444, -1, -1));

		// Every cell's corners land in that cell.
		int wrong = 0;
		for(int r = 0; r < grid::Rows; ++r)
		{
			for(int c = 0; c < grid::Columns; ++c)
			{
				const int x = grid::cellX(c);
				const int y = grid::cellY(r);
				if(!at(x, y, r, c) || !at(x + grid::CellWidth - 1, y + grid::CellHeight - 1, r, c))
					++wrong;
			}
		}
		CHECK(wrong == 0);
		CHECK(grid::cellX(grid::Columns - 1) + grid::CellWidth <= grid::Width);
		// The swing row under the Tracks' ends the lcd, and a click there gives its row.
		CHECK(grid::cellY(grid::SwingRow) + grid::CellHeight == grid::Height);
		const grid::Cell swing = grid::cellAt(grid::cellX(20) + 1, grid::cellY(grid::SwingRow) + 1);
		CHECK(swing.row == grid::SwingRow && swing.column == 20);
		CHECK(strip::cellX(strip::Cells - 1) + strip::CellWidth == strip::Width);
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	colourMixes();
	cells();
	lockDot();
	conditionStrip();
	ratchetNotches();
	microArrow();
	stripClicks();
	gridClicks();
	return mdseqTest::finish("mdseqViewTest");
}
