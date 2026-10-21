// What the lcd objects draw and where a click lands: the cell's colours and order as Design/v12-ui.js draws them, its
// ratchet notches where the mockup cuts them, its micro-timing arrow, the steps past the length and the line after the
// last, the strip's and the grid's cells with their gaps.

#include "mdseqTestSupport.h"
#include "mdseqText.h"
#include "mdseqView.h"

#include <cstdio>
#include <string>

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

	// The mockup's own rounding (Math.round), as Design/v12-ui.js mixes them.
	constexpr Rgb Velocity40{180, 181, 182};
	constexpr Rgb Played{70, 89, 91};

	void colourMixes()
	{
		CHECK(mix(colours::Panel, colours::Hit, 1.0) == colours::Hit);
		CHECK(mix(colours::Panel, colours::Hit, 0.0) == colours::Panel);
		CHECK(mix(colours::Panel, colours::Hit, 0.5 + 0.5 * 40 / 127.0) == Velocity40);
		CHECK(mix(colours::Panel, colours::Accent, 0.18) == Played);
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
		CHECK(ops[1] == paint(colours::Hit, 2, 2, 34, 38));

		// A Hit is never darker than halfway to the ink, whatever its velocity.
		n = drawCell(hitStep(0), CellLook{}, 0, 0, 36, 24, ops);
		CHECK(ops[1].colour == (Rgb{152, 153, 155}));

		// Faded (a Hit the Fill neither plays alone nor skips, while the Fill is edited): a third of its colour over
		// the background.
		CellLook faded;
		faded.faded = true;
		n = drawCell(hitStep(127), faded, 0, 0, 36, 40, ops);
		CHECK(n == 2);
		CHECK(ops[1] == paint(Rgb{124, 126, 129}, 2, 2, 34, 38));

		// Everything at once, in the mockup's order: background, Hit, played frame, selected frame of 2 px.
		CellLook all;
		all.played = true;
		all.selected = true;
		n = drawCell(hitStep(40), all, 10, 20, 36, 24, ops);
		CHECK(n == 5);
		CHECK(ops[0] == paint(Played, 10, 20, 46, 44));
		CHECK(ops[1] == paint(Velocity40, 12, 22, 44, 42));
		CHECK(ops[2] == frame(colours::Accent, 10, 20, 46, 44));
		CHECK(ops[3] == frame(colours::Accent, 10, 20, 46, 44));
		CHECK(ops[4] == frame(colours::Accent, 11, 21, 45, 43));
	}

	// Past the Pattern's length: off, a Hit only hinted, nothing else of it, whatever the step holds or the cell shows.
	void pastCells()
	{
		DrawOp ops[MaxCellOps];
		CellLook past;
		past.past = true;
		CHECK(drawCell(Step{}, past, 0, 0, 36, 24, ops) == 1);
		CHECK(ops[0] == paint(colours::Past, 0, 0, 36, 24));

		Step full = hitStep(100);
		full.micro = 5;
		full.ratchets = 3;
		full.condition = 22;
		full.locks[5] = 90;
		past.played = true;
		past.selected = true;
		past.accented = true;
		past.beat = true;
		CHECK(drawCell(full, past, 0, 0, 36, 24, ops) == 2);
		CHECK(ops[0] == paint(colours::Past, 0, 0, 36, 24));
		CHECK(ops[1] == paint(Rgb{70, 71, 74}, 2, 2, 34, 22));

		DrawOp row[MaxMarkCellOps];
		CHECK(drawSwingCell(true, 62, past, 0, 0, 36, 24, row) == 1 && row[0].colour == colours::Past);
		CHECK(drawMarkCell(true, colours::Sliding, past, 0, 0, 36, 24, row) == 1 && row[0].colour == colours::Past);
	}

	void lockMark()
	{
		DrawOp ops[MaxCellOps];
		Step s = hitStep(127);
		s.locks[17] = 0;
		CellLook selected;
		selected.selected = true;
		const size_t n = drawCell(s, selected, 0, 0, 36, 24, ops);
		CHECK(n == 6);
		// After the Hit, before the frames: an 8 px square of the panel's colour in the top right corner, the 4 px
		// accent inside it.
		CHECK(ops[2] == paint(colours::Panel, 26, 2, 34, 10));
		CHECK(ops[3] == paint(colours::Accent, 28, 4, 32, 8));
		CHECK(ops[4].kind == DrawOp::Kind::FrameRect);

		// No mark without a Hit, whatever the step holds.
		Step noHit;
		noHit.locks[3] = 40;
		CHECK(drawCell(noHit, CellLook{}, 0, 0, 36, 24, ops) == 1);
	}

	void conditionLabels()
	{
		// Ableton Sans Bold's advances at 10 px, in hundredths of a pixel: tabular digits.
		CHECK(textWidth("1:2", Face::Bold, LcdFontSize) == 590 + 328 + 590);
		CHECK(textWidth("50%", Face::Bold, LcdFontSize) == 590 + 590 + 864);
		CHECK(textWidth("!FILL", Face::Bold, LcdFontSize) == 338 + 545 + 299 + 500 + 500);
		CHECK(textWidth("", Face::Bold, LcdFontSize) == 0);
		// Medium at 12 px: thousandths of an em times 12 / 10.
		CHECK(textWidth("NFX-D2", Face::Medium, 12) == (717 + 551 + 634 + 410 + 685 + 590) * 12 / 10);

		// Cut to the box's width, at a character: names from a user's bank (MoDule's User Machines, a user's Sample's
		// file name), measured in Live where they broke their line or lost half a letter.
		CHECK(fitText("NFX-D2", fit::TrackName) == "NFX-D2");
		CHECK(fitText("gretsch_tom_06", fit::TrackName) == "gretsch_tom\xE2\x80\xA6");
		CHECK(fitText("01 gretsch_tom_06", fit::TrackTab) == "01 gret");
		CHECK(fitText("01 R14", fit::TrackTab) == "01 R14");
		CHECK(fitText("Track 01 gretsch_tom_06", fit::TrackTitle) ==
			"Track 01 gretsch_tom\xE2\x80\xA6");
		CHECK(fitText("REVERBSIZE2", fit::LockName) == "REVERBSIZ\xE2\x80\xA6");
		// No space left before the ellipsis, and never half a UTF-8 sequence: each counts as one average letter.
		CHECK(fitText("AB CDEFGHIJ", {Face::Medium, 10, 24, true}) == "AB\xE2\x80\xA6");
		CHECK(fitText("\xC3\xA9\xC3\xA9\xC3\xA9", {Face::Medium, 12, 20, true}) == "\xC3\xA9\xE2\x80\xA6");
		CHECK(std::string(fontName(Face::Bold)) == "Ableton Sans Bold Regular");

		DrawOp ops[MaxCellOps];
		Step s = hitStep(100);
		s.condition = 22;	// 1:2
		s.locks[0] = 10;
		CellLook all;
		all.played = true;
		all.selected = true;
		const size_t n = drawCell(s, all, 38, 28, 36, 24, ops);
		CHECK(n == 8);
		// After the Locks' mark, before the frames: the label alone, no strip, centred (15.08 px wide in 36: 10.46 px
		// in), its baseline 6 px up from the bottom.
		CHECK(ops[4] == (DrawOp{DrawOp::Kind::Text, colours::Text, 48, 46, 0, 0, "1:2"}));
		CHECK(ops[5].kind == DrawOp::Kind::FrameRect);
		CHECK(n <= MaxCellOps);
	}

	void ratchetNotches()
	{
		DrawOp ops[MaxCellOps];

		// One ratchet is the plain Hit: no notch.
		Step one = hitStep(127);
		CHECK(drawCell(one, CellLook{}, 0, 0, 36, 24, ops) == 2);

		// Three: 2 x 4 px of the background's colour in the Hit's top edge, on the even pixels where the mockup's
		// Math.round puts them (v12-ui.js), and before the Locks' mark.
		Step three = hitStep(127);
		three.ratchets = 3;
		three.locks[0] = 10;
		CellLook played;
		played.played = true;
		size_t n = drawCell(three, played, 0, 0, 36, 24, ops);
		CHECK(n == 8);
		CHECK(ops[2] == paint(Played, 6, 2, 8, 6));
		CHECK(ops[3] == paint(Played, 16, 2, 18, 6));
		CHECK(ops[4] == paint(Played, 26, 2, 28, 6));
		CHECK(ops[5] == paint(colours::Panel, 26, 2, 34, 10));

		Step two = hitStep(127);
		two.ratchets = 2;
		n = drawCell(two, CellLook{}, 40, 0, 36, 40, ops);
		CHECK(n == 4);
		CHECK(ops[2] == paint(colours::Panel, 48, 2, 50, 6));
		CHECK(ops[3] == paint(colours::Panel, 64, 2, 66, 6));

		// Everything a cell can hold fills MaxCellOps exactly.
		Step full = hitStep(100);
		full.micro = 5;
		full.ratchets = MaxRatchets;
		full.condition = 22;
		full.locks[5] = 90;
		CellLook all;
		all.played = true;
		all.selected = true;
		all.accented = true;
		CHECK(drawCell(full, all, 0, 0, 36, 24, ops) == MaxCellOps);
	}

	// The Accent (issue 26): in the strip, an accented step takes the Accented colour whole, its Hit's or a well of a
	// quarter of it; in the grid's Accent row, a mark inset as a Hit is.
	void accentMarks()
	{
		DrawOp ops[MaxCellOps];
		CellLook accented;
		accented.accented = true;
		size_t n = drawCell(hitStep(100), accented, 40, 0, 36, 40, ops);
		CHECK(n == 2);
		CHECK(ops[1] == paint(Rgb{214, 167, 88}, 42, 2, 74, 38));
		n = drawCell(Step{}, accented, 40, 0, 36, 40, ops);
		CHECK(n == 2);
		CHECK(ops[1] == paint(Rgb{104, 92, 74}, 42, 2, 74, 38));

		// Its micro-timing's arrow in its own colour.
		Step late = hitStep(127);
		late.micro = 3;
		n = drawCell(late, accented, 0, 0, 36, 40, ops);
		CHECK(ops[n - 1].kind == DrawOp::Kind::Triangle && ops[n - 1].colour == colours::Accented);

		DrawOp row[MaxMarkCellOps];
		CHECK(drawMarkCell(false, colours::Accented, CellLook{}, 0, 0, 36, 24, row) == 1);
		CHECK(drawMarkCell(true, colours::Accented, CellLook{}, 0, 0, 36, 24, row) == 2);
		CHECK(row[1] == paint(colours::Accented, 2, 2, 34, 22));
		CellLook played;
		played.played = true;
		CHECK(drawMarkCell(true, colours::Sliding, played, 0, 0, 36, 24, row) == MaxMarkCellOps);
		CHECK(row[1].colour == colours::Sliding);
		CHECK(row[2].kind == DrawOp::Kind::FrameRect);
	}

	// The swing row: the mark's left edge moves by half the delay, on an even pixel; dimmer at 50 %.
	void swingMarks()
	{
		DrawOp row[MaxMarkCellOps];
		CHECK(drawSwingCell(false, 62, CellLook{}, 0, 0, 36, 24, row) == 1);
		CHECK(drawSwingCell(true, 62, CellLook{}, 0, 0, 36, 24, row) == 2);
		CHECK(row[1] == paint(Rgb{112, 115, 120}, 6, 2, 34, 22));
		CHECK(drawSwingCell(true, MinSwing, CellLook{}, 0, 0, 36, 24, row) == 2);
		CHECK(row[1] == paint(Rgb{89, 91, 97}, 2, 2, 34, 22));
		CHECK(drawSwingCell(true, 80, CellLook{}, 0, 0, 36, 24, row) == 2);
		CHECK(row[1].left == 12);
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
		CHECK(ops[4] == (DrawOp{DrawOp::Kind::Triangle, Velocity40, 40, 36, 38, 44}));

		// Late, in the strip's taller cell: in the right margin.
		Step late = hitStep(127);
		late.micro = 12;
		n = drawCell(late, CellLook{}, 40, 0, 36, 40, ops);
		CHECK(n == 3);
		CHECK(ops[2] == (DrawOp{DrawOp::Kind::Triangle, colours::Hit, 74, 16, 76, 24}));

		// On the grid, no arrow; without a Hit, none either.
		CHECK(drawCell(hitStep(127), CellLook{}, 0, 0, 36, 24, ops) == 2);
		Step none;
		none.micro = 4;
		CHECK(drawCell(none, CellLook{}, 0, 0, 36, 24, ops) == 1);
	}

	// The line after the Pattern's last step: 2 px of End centred in the gap after it, when a gap follows it.
	void endLines()
	{
		DrawOp end{};
		CHECK(!strip::endLine(16, 0, end));		// the strip's last cell ends the lcd
		CHECK(strip::endLine(12, 0, end));
		CHECK(end == paint(colours::End, 477, 0, 479, strip::Height));
		CHECK(!strip::endLine(12, 1, end));
		CHECK(!strip::endLine(20, 0, end));
		CHECK(strip::endLine(20, 1, end) && end.left == 157);

		// In the grid, as the mockup draws it at 48 steps: in the gap between the pages of 16.
		CHECK(grid::endLine(48, 1, end));
		CHECK(end == paint(colours::End, 608, 0, 610, grid::Height));
		CHECK(grid::endLine(16, 0, end) && end.left == 608);
		CHECK(grid::endLine(20, 0, end) && end.left == 762 && end.right == 764);
		CHECK(grid::endLine(32, 0, end) && end.left == 1218 && end.right == grid::Width);
		CHECK(!grid::endLine(64, 0, end));
		CHECK(!grid::endLine(16, 1, end));
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
		// The trig row under the Tracks' ends the lcd, and a click there gives its row.
		CHECK(grid::cellY(grid::TrigRow) + grid::CellHeight == grid::Height);
		const grid::Cell trig = grid::cellAt(grid::cellX(20) + 1, grid::cellY(grid::TrigRow) + 1);
		CHECK(trig.row == grid::TrigRow && trig.column == 20);
		CHECK(strip::cellX(strip::Cells - 1) + strip::CellWidth == strip::Width);
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	colourMixes();
	cells();
	pastCells();
	lockMark();
	conditionLabels();
	ratchetNotches();
	microArrow();
	accentMarks();
	swingMarks();
	endLines();
	stripClicks();
	gridClicks();
	return mdseqTest::finish("mdseqViewTest");
}
