// The interface state: clicks in the strip and the grid toggle and paint Hits, the grid selects the step for the
// panel, the panel edits the step selected, the step played moves its frame and the window's half, and every edit
// forwarded to the Sequencer, packed as the external packs it, leaves it with the Editor's own Pattern. The panel's
// values are checked as the patch receives them: their keys, integers and texts, in the order sent.

#include "mdseqEdit.h"
#include "mdseqCondition.h"
#include "mdseqEditor.h"
#include "mdseqSequencer.h"
#include "mdseqTestSupport.h"

#include <algorithm>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace
{
	using namespace mdseq;

	// The swing row drawn whole with the default row: 32 backgrounds and a mark on every second step, the steps past
	// the Pattern's length off and without one. At the default length of 16 in the first half; in a half all past the
	// length; in a half within it.
	constexpr size_t SwingRowOps = 32 + 8;
	constexpr size_t SwingRowPastOps = 32;
	constexpr size_t SwingRowFullOps = 32 + 16;

	// The centre of a cell, where a click lands.
	int stripX(const int _cell) { return strip::cellX(_cell) + strip::CellWidth / 2; }
	int gridX(const int _column) { return grid::cellX(_column) + grid::CellWidth / 2; }
	int gridY(const int _row) { return grid::cellY(_row) + grid::CellHeight / 2; }

	bool forwarded(const EditorOutput& _out, const Input::Kind _kind, const int _track, const int _step,
		const int _value)
	{
		if(_out.forward.size() != 1)
			return false;
		const Input& in = _out.forward[0];
		return in.kind == _kind && in.track == _track && in.step == _step && in.value == _value;
	}

	// A panel value of integers, one of text, and an indexed one of text.
	PanelValue ints(const char* _key, const std::initializer_list<int> _ints) { return {_key, _ints, {}}; }
	PanelValue text(const char* _key, const char* _text) { return {_key, {}, {_text}}; }
	PanelValue named(const char* _key, const int _index, const char* _text) { return {_key, {_index}, {_text}}; }

	bool hasPanel(const EditorOutput& _out, const PanelValue& _value)
	{
		for(const PanelValue& v : _out.panel)
			if(v == _value)
				return true;
		return false;
	}

	// What the patch ends up showing on _value's key (and, for a Lock's dial or value, on its parameter): the last value
	// sent there.
	bool lastPanel(const EditorOutput& _out, const PanelValue& _value)
	{
		const bool byParam = _value.key == "lock" || _value.key == "lockvalue" || _value.key == "patternstate";
		const PanelValue* last = nullptr;
		for(const PanelValue& v : _out.panel)
			if(v.key == _value.key && (!byParam || (!v.ints.empty() && v.ints[0] == _value.ints[0])))
				last = &v;
		return last && *last == _value;
	}

	// The last text sent on _key, empty when none was.
	std::string lastText(const EditorOutput& _out, const char* _key)
	{
		std::string t;
		for(const PanelValue& v : _out.panel)
			if(v.key == _key && v.texts.size() == 1)
				t = v.texts[0];
		return t;
	}

	std::vector<std::string> keys(const EditorOutput& _out)
	{
		std::vector<std::string> k;
		for(const PanelValue& v : _out.panel)
			k.push_back(v.key);
		return k;
	}

	void redrawAll()
	{
		Editor e;
		const EditorOutput& out = e.redrawAll();
		CHECK(!out.strip.empty() && out.strip[0].kind == DrawOp::Kind::Clear);
		CHECK(out.strip.size() == 1 + 16);
		CHECK(!out.grid.empty() && out.grid[0].kind == DrawOp::Kind::Clear);
		// The step selected, Track 1 step 1, has its frame of 2 px; then the trig row, the swing's at first, and the
		// line after step 16.
		CHECK(out.grid.size() == 1 + 512 + 2 + SwingRowOps + 1);
		CHECK(hasPanel(out, text("tracktitle", "Track 01")));
		CHECK(hasPanel(out, text("steptitle", "Step 1, page 1")));
		CHECK(hasPanel(out, ints("velocity", {DefaultVelocity})));
		CHECK(hasPanel(out, ints("hit", {0})));
		CHECK(hasPanel(out, ints("lengthindex", {DefaultLength - 1})));
		CHECK(hasPanel(out, ints("scaleindex", {DefaultScale})));
		CHECK(hasPanel(out, text("lengthtext", "16 steps, 1/16")));
		CHECK(hasPanel(out, ints("half", {0})));
		CHECK(hasPanel(out, text("status", "Stopped")));
		CHECK(hasPanel(out, ints("patternindex", {0})));
		CHECK(out.forward.empty());
	}

	// Everything the panel shows, in the order the patch receives it: the Tracks' names, the step selected with its
	// Track's names for SYN1-8, the length and the strip's pages, the half and its column numbers, the status line, then
	// the Pattern chosen with the status line again.
	void panelInOrder()
	{
		Editor e;
		std::vector<std::string> expected = {"tracknames", "trackindex"};
		for(int t = 0; t < TrackCount; ++t)
			expected.push_back("trackname");
		expected.push_back("tracktitle");
		for(int p = 0; p < SynCount; ++p)
			expected.push_back("lockname");
		for(const char* k : {"steptitle", "velocity", "hit", "condition", "micro", "microtext", "ratchetsindex",
			"curve", "curvetext"})
			expected.push_back(k);
		const size_t firstLock = expected.size();
		for(int p = 0; p < LockCount; ++p)
		{
			expected.push_back("lock");
			expected.push_back("lockvalue");
		}
		for(const char* k : {"lengthindex", "scaleindex", "lengthtext", "pages", "swing", "accent", "trigrow", "half"})
			expected.push_back(k);
		for(int i = 0; i < 8; ++i)
			expected.push_back("colnum");
		for(const char* k : {"status", "patternindex", "status"})
			expected.push_back(k);
		for(int p = 0; p < PatternCount; ++p)
			expected.push_back("patternstate");
		for(const char* k : {"copyto", "filledit", "chainmode", "chaintext"})
			expected.push_back(k);
		const EditorOutput& out = e.redrawAll();
		CHECK(keys(out) == expected);
		// Integers or texts, never neither; texts after integers only behind an indexed key's index.
		for(const PanelValue& v : out.panel)
			CHECK(!v.ints.empty() || !v.texts.empty());
		for(const PanelValue& v : out.panel)
			CHECK(v.ints.empty() || v.texts.empty() || (v.ints.size() == 1 && findPanelKey(v.key)->indexed));
		if(out.panel.size() == expected.size())
			for(size_t p = 0; p < 2 * LockCount; ++p)	// the Locks by parameter, each dial then its value
				CHECK(out.panel[firstLock + p].ints[0] == static_cast<int>(p / 2));

		// A step played in the other half: the status line, then the half and its numbers.
		expected = {"status", "half"};
		for(int i = 0; i < 8; ++i)
			expected.push_back("colnum");
		CHECK(keys(e.played(40)) == expected);

		// The Half tab clicked: the half and its numbers alone, as the step played sends them.
		Editor clicked;
		expected.erase(expected.begin());
		CHECK(keys(clicked.showHalf(1)) == expected);
	}

	void stripClicksAndDrags()
	{
		Editor e;
		e.stripMouse(true);
		const EditorOutput* out = &e.stripClick(stripX(2), 10);
		CHECK(forwarded(*out, Input::Kind::SetHit, 0, 2, 1));
		CHECK(out->strip.size() == 2);	// the cell's background and its Hit
		CHECK(out->strip[0].left == strip::cellX(2));
		CHECK(out->grid.size() == 2);	// the same step in the grid

		// Dragging on: the next cell takes the same value, the same cell nothing.
		CHECK(forwarded(e.stripClick(stripX(3), 10), Input::Kind::SetHit, 0, 3, 1));
		CHECK(e.stripClick(stripX(3) + 2, 12).forward.empty());
		CHECK(e.stripClick(strip::cellX(4) - 2, 10).forward.empty());	// a gap
		e.stripMouse(false);

		// A new gesture on a Hit takes it off, and the drag goes on taking off.
		e.stripMouse(true);
		CHECK(forwarded(e.stripClick(stripX(2), 10), Input::Kind::SetHit, 0, 2, 0));
		CHECK(forwarded(e.stripClick(stripX(3), 10), Input::Kind::SetHit, 0, 3, 0));
		e.stripMouse(false);

		// The Track and page shown decide which step a cell is, in a Pattern long enough for the third page.
		e.setLength(MaxSteps);
		CHECK(e.showTrack(5).strip.size() == 1 + 16);
		CHECK(e.showPage(2).strip.size() == 1 + 16);
		e.stripMouse(true);
		CHECK(forwarded(e.stripClick(stripX(0), 10), Input::Kind::SetHit, 5, 32, 1));
		CHECK(e.pattern().steps[5][32].hit);
		e.stripMouse(false);
		CHECK(e.showTrack(5).strip.empty());	// nothing to redraw
	}

	// The lcd's own sequence for one click, read in Live (the development build's log): the button down, the position,
	// the button up, and the position once more. One click puts one Hit: the last position used to take it off again.
	void oneClickAsTheLcdSendsIt()
	{
		Editor e;
		e.stripMouse(true);
		e.stripClick(stripX(5), 21);
		e.stripMouse(false);
		CHECK(e.stripClick(stripX(5), 21).forward.empty());
		CHECK(e.pattern().steps[0][5].hit);

		e.gridMouse(true);
		e.gridClick(gridX(7), gridY(2));
		e.gridMouse(false);
		CHECK(e.gridClick(gridX(7), gridY(2)).forward.empty());
		CHECK(e.pattern().steps[2][7].hit);

		// A position without the button down (the lcd's last one, or a stray one) changes nothing.
		CHECK(e.stripClick(stripX(9), 21).forward.empty());
		CHECK(e.gridClick(gridX(9), gridY(4)).forward.empty());
	}

	void gridSelectsAndPaints()
	{
		Editor e;
		e.gridMouse(true);
		const EditorOutput* out = &e.gridClick(gridX(5), gridY(3));
		CHECK(forwarded(*out, Input::Kind::SetHit, 3, 5, 1));
		CHECK(e.selectedTrack() == 3 && e.selectedStep() == 5);
		// The step panel once, and each cell once: the one left loses its frame, the one clicked gets its Hit.
		const std::vector<std::string> sent = keys(*out);
		CHECK(std::count(sent.begin(), sent.end(), "tracktitle") == 1);
		CHECK(out->grid.size() == 1 + 2 + 2);	// step 1: its background; step 6: background, Hit, frame of 2 px
		CHECK(lastText(*out, "tracktitle") == "Track 04");
		CHECK(lastText(*out, "steptitle") == "Step 6, page 1");
		CHECK(lastPanel(*out, ints("hit", {1})));
		e.gridMouse(false);

		// The panel edits the step selected, and redraws it.
		out = &e.setVelocity(40);
		CHECK(forwarded(*out, Input::Kind::SetVelocity, 3, 5, 40));
		CHECK(out->grid.size() == 2 + 2);	// background, Hit, frame of 2 px
		CHECK(e.setVelocity(40).forward.empty());
		CHECK(forwarded(e.setHit(false), Input::Kind::SetHit, 3, 5, 0));

		// The second half: columns are steps 33-64.
		CHECK(e.showHalf(1).grid.size() == 1 + 512 + SwingRowPastOps);	// the step selected is not in it
		e.gridMouse(true);
		CHECK(forwarded(e.gridClick(grid::cellX(16) + 1, gridY(0)), Input::Kind::SetHit, 0, 48, 1));
		CHECK(e.gridClick(grid::cellX(16) - 1, gridY(0)).forward.empty());	// the gap between the pages
		e.gridMouse(false);
	}

	// A click in the grid selects: an empty cell takes a Hit, a Hit stays with its Locks. The step selected's own click
	// toggles its Hit, so that a double-click takes a Hit off, as in a Live clip. A drag from an empty cell paints Hits,
	// one from a Hit takes off those of the cells it crosses, its own first.
	void gridSelectsWithoutTakingOff()
	{
		Editor e;
		const auto click = [&](const int _column, const int _row)
		{
			e.gridMouse(true);
			const EditorOutput out = e.gridClick(gridX(_column), gridY(_row));
			e.gridMouse(false);
			return out;
		};
		e.apply({Input::Kind::SetHit, 2, 4, 1});
		Input lock{Input::Kind::SetLock, 2, 4, 40};
		lock.param = 17;
		e.apply(lock);

		EditorOutput out = click(4, 2);
		CHECK(out.forward.empty() && e.selectedTrack() == 2 && e.selectedStep() == 4);
		CHECK(e.pattern().steps[2][4].hit && e.pattern().steps[2][4].locks[17] == 40);
		CHECK(lastPanel(out, ints("hit", {1})) && lastPanel(out, ints("lock", {17, 40, 1})));
		CHECK(!out.grid.empty());	// the frame moves to it
		CHECK(forwarded(click(4, 2), Input::Kind::SetHit, 2, 4, 0));
		CHECK(forwarded(click(4, 2), Input::Kind::SetHit, 2, 4, 1));

		out = click(9, 5);
		CHECK(forwarded(out, Input::Kind::SetHit, 5, 9, 1) && e.selectedTrack() == 5 && e.selectedStep() == 9);

		e.apply({Input::Kind::SetHit, 7, 1, 1});
		e.apply({Input::Kind::SetHit, 7, 2, 1});
		e.gridMouse(true);
		CHECK(e.gridClick(gridX(1), gridY(7)).forward.empty());
		e.gridClick(gridX(2), gridY(7));
		e.gridClick(gridX(3), gridY(7));
		e.gridMouse(false);
		CHECK(!e.pattern().steps[7][1].hit && !e.pattern().steps[7][2].hit && !e.pattern().steps[7][3].hit);
		CHECK(e.selectedTrack() == 7 && e.selectedStep() == 1);
	}

	// The panel's titles count Tracks, steps and the strip's pages of 16 steps from 1.
	void selectionTitles()
	{
		Editor e;
		e.gridMouse(true);
		const EditorOutput* out = &e.gridClick(gridX(16), gridY(2));
		CHECK(lastText(*out, "tracktitle") == "Track 03");
		CHECK(lastText(*out, "steptitle") == "Step 17, page 2");
		e.gridMouse(false);

		e.showHalf(1);
		e.gridMouse(true);
		out = &e.gridClick(gridX(31), gridY(15));
		CHECK(lastText(*out, "tracktitle") == "Track 16");
		CHECK(lastText(*out, "steptitle") == "Step 64, page 4");
		e.gridMouse(false);
	}

	void mutesAndLength()
	{
		Editor e;
		const EditorOutput* out = &e.setMute(2, true);
		CHECK(forwarded(*out, Input::Kind::SetMute, 2, 0, 1));
		CHECK(out->panel.size() == 1 && hasPanel(*out, ints("mute", {2, 1})));
		CHECK(e.setMute(2, true).forward.empty());
		CHECK(hasPanel(e.setMute(2, false), ints("mute", {2, 0})));

		// The length's menu holds 1 to 64 steps at indices 0 to 63.
		out = &e.setLength(32);
		CHECK(forwarded(*out, Input::Kind::SetLength, 0, 0, 32));
		CHECK(hasPanel(*out, ints("lengthindex", {31})));
		CHECK(hasPanel(*out, ints("scaleindex", {DefaultScale})));
		CHECK(hasPanel(*out, text("lengthtext", "32 steps, 1/16")));
		CHECK(e.setLength(32).forward.empty());
		out = &e.setLength(200);
		CHECK(out->forward[0].value == 200);	// forwarded as asked, clamped by applyEdit on both sides
		CHECK(e.pattern().length == MaxSteps);
		CHECK(hasPanel(*out, ints("lengthindex", {MaxSteps - 1})));
		CHECK(hasPanel(*out, text("lengthtext", "64 steps, 1/16")));
		CHECK(hasPanel(e.setLength(1), ints("lengthindex", {0})));

		// A length drawn again whole: the steps past it off, the line after the last step in the gap after it.
		out = &e.setLength(12);
		CHECK(!out->strip.empty() && out->strip[0].kind == DrawOp::Kind::Clear);
		CHECK(out->strip.size() == 1 + 16 + 1);
		CHECK(out->strip[12].colour == colours::Panel && out->strip[13].colour == colours::Past);
		DrawOp end{};
		CHECK(strip::endLine(12, 0, end) && out->strip.back() == end);
		CHECK(grid::endLine(12, 0, end) && !out->grid.empty() && out->grid.back() == end);
	}

	// The strip's pages: those past the length are off, the page shown stays within it, and the patch's four buttons
	// hear the page and the count again after each click.
	void pagesWithinTheLength()
	{
		Editor e;
		CHECK(hasPanel(e.redrawAll(), ints("pages", {0, 1})));
		const EditorOutput* out = &e.showPage(2);
		CHECK(out->strip.empty() && lastPanel(*out, ints("pages", {0, 1})));
		out = &e.setLength(40);
		CHECK(lastPanel(*out, ints("pages", {0, 3})));
		out = &e.showPage(2);
		CHECK(!out->strip.empty() && lastPanel(*out, ints("pages", {2, 3})));
		CHECK(e.showPage(2).strip.empty());
		out = &e.setLength(20);
		CHECK(lastPanel(*out, ints("pages", {1, 2})));
		DrawOp end{};
		CHECK(strip::endLine(20, 1, end) && out->strip.back() == end);	// on page 2, which holds the last step
		// Another Pattern, of 16 steps: its first page.
		CHECK(lastPanel(e.setPattern(3), ints("pages", {0, 1})));
	}

	void playedStep()
	{
		Editor e;
		e.redrawAll();
		e.setLength(MaxSteps);
		const EditorOutput* out = &e.played(5);
		CHECK(out->panel.size() == 1 && hasPanel(*out, text("status", "Playing step 6")));
		// A column of 16 cells, background and frame, and its swing cell: step 6 swings, so its mark too.
		CHECK(out->grid.size() == 16 * 2 + 3);
		CHECK(out->strip.size() == 2);
		CHECK(e.played(5).grid.empty());

		// Moving on redraws the old column and the new, with their swing cells (step 7 does not swing).
		out = &e.played(6);
		CHECK(out->grid.size() == 16 + 2 + 16 * 2 + 2);
		CHECK(out->strip.size() == 1 + 2);

		// Following, the window turns to the half played.
		out = &e.played(40);
		CHECK(e.half() == 1);
		CHECK(hasPanel(*out, ints("half", {1})));
		// All redrawn, the column played framed, its swing cell too, and the line after step 64.
		CHECK(out->grid.size() == 1 + 512 + 16 + SwingRowFullOps + 1 + 1);
		CHECK(out->strip.size() == 1);	// step 7 back to plain; step 41 is on another page

		e.setFollow(false);
		e.played(3);
		CHECK(e.half() == 1);

		out = &e.played(-1);
		CHECK(hasPanel(*out, text("status", "Stopped")));
		CHECK(out->strip.size() == 1);	// step 4 back to plain
	}

	// The numbers over the grid's columns: every fourth step of the half shown, from its first.
	void columnNumbers()
	{
		Editor e;
		const EditorOutput* out = &e.redrawAll();
		for(int i = 0; i < 8; ++i)
			CHECK(hasPanel(*out, ints("colnum", {i, 4 * i + 1})));
		CHECK(hasPanel(*out, ints("colnum", {7, 29})));

		// The window's Half tab clicked: the numbers follow the half, with no redraw of the window.
		out = &e.showHalf(1);
		CHECK(hasPanel(*out, ints("half", {1})));
		for(int i = 0; i < 8; ++i)
			CHECK(hasPanel(*out, ints("colnum", {i, 32 + 4 * i + 1})));
		CHECK(hasPanel(*out, ints("colnum", {0, 33})) && hasPanel(*out, ints("colnum", {7, 61})));
		CHECK(e.showHalf(1).panel.empty());	// the half shown already: nothing changes
		out = &e.showHalf(0);
		CHECK(hasPanel(*out, ints("half", {0})));
		CHECK(hasPanel(*out, ints("colnum", {0, 1})) && hasPanel(*out, ints("colnum", {7, 29})));

		// A redraw numbers the half shown.
		e.showHalf(1);
		out = &e.redrawAll();
		CHECK(hasPanel(*out, ints("half", {1})) && hasPanel(*out, ints("colnum", {7, 61})));

		// Following the step played back to the first half numbers it again.
		out = &e.played(2);
		CHECK(hasPanel(*out, ints("half", {0})) && hasPanel(*out, ints("colnum", {1, 5})));
	}

	// The strip's status line: stopped, the step played, or the Pattern queued and the step it waits for.
	void statusLine()
	{
		Editor e;
		CHECK(lastText(e.redrawAll(), "status") == "Stopped");
		CHECK(lastText(e.played(0), "status") == "Playing step 1");
		CHECK(lastText(e.played(15), "status") == "Playing step 16");

		// A Pattern queued while playing: named from 1 and on two digits, after the last step of the one played.
		e.setLength(32);
		const EditorOutput* out = &e.setPattern(1);
		CHECK(lastPanel(*out, ints("patternindex", {1})));
		CHECK(lastText(*out, "status") == "02 next, after step 32");
		CHECK(lastText(e.played(16), "status") == "02 next, after step 32");	// the queue outlasts the steps
		CHECK(lastText(e.setPattern(11), "status") == "12 next, after step 32");

		// Taken back: the step played again.
		CHECK(lastText(e.setPattern(0), "status") == "Playing step 17");

		// The switch: the new Pattern shown, nothing queued.
		e.setPattern(1);
		out = &e.follow(1, -1);
		CHECK(lastPanel(*out, ints("patternindex", {1})));
		CHECK(lastText(*out, "status") == "Playing step 17");
		CHECK(lastText(e.played(-1), "status") == "Stopped");

		// Stopped, a Pattern chosen is shown at once: nothing waits.
		CHECK(lastText(e.setPattern(4), "status") == "Stopped");
	}

	void editsFromElsewhere()
	{
		Editor e;
		const EditorOutput* out = &e.apply({Input::Kind::SetHit, 0, 4, 1});
		CHECK(forwarded(*out, Input::Kind::SetHit, 0, 4, 1));
		CHECK(out->strip.size() == 2 && out->grid.size() == 2);
		CHECK(e.apply({Input::Kind::SetHit, 0, 4, 1}).forward.empty());

		// The step selected changes under the panel: the panel follows.
		out = &e.apply({Input::Kind::SetVelocity, 0, 0, 30});
		CHECK(hasPanel(*out, ints("velocity", {30})));
		CHECK(hasPanel(e.apply({Input::Kind::SetMute, 6, 0, 1}), ints("mute", {6, 1})));
		out = &e.apply({Input::Kind::SetLength, 0, 0, 8});
		CHECK(hasPanel(*out, ints("lengthindex", {7})) && hasPanel(*out, text("lengthtext", "8 steps, 1/16")));
		CHECK(hasPanel(e.apply({Input::Kind::SetScale, 0, 0, 2}), text("lengthtext", "8 steps, 1/8")));
	}

	void locksFromThePanel()
	{
		constexpr int Vol = 17;
		Editor e;

		// lock <param> <value> <active>: the Kit is 63 and unlit, a Lock its value and lit.
		// The step selected has no Hit: the Lock is refused, the dial shows the Kit again.
		const EditorOutput* out = &e.setLock(Vol, 40);
		CHECK(out->forward.empty());
		CHECK(out->panel.size() == 2 && lastPanel(*out, ints("lock", {Vol, NoLock, 0})));
		CHECK(lastPanel(*out, named("lockvalue", Vol, "Kit")));

		e.setHit(true);
		out = &e.setLock(Vol, 40);
		CHECK(out->forward.size() == 1 && out->forward[0].kind == Input::Kind::SetLock &&
			out->forward[0].param == Vol && out->forward[0].value == 40);
		CHECK(lastPanel(*out, ints("lock", {Vol, 40, 1})));
		CHECK(out->grid.size() == 2 + 2 + 2);	// background, Hit, the Locks' dot, frame of 2 px
		CHECK(out->strip.size() == 4);			// background, Hit, dot

		// The ends of the range are Locks too, lit.
		CHECK(lastPanel(e.setLock(Vol, 0), ints("lock", {Vol, 0, 1})));
		CHECK(lastPanel(e.setLock(Vol, 127), ints("lock", {Vol, 127, 1})));
		CHECK(lastPanel(e.setLock(0, 62), ints("lock", {0, 62, 1})));
		e.setLock(Vol, 40);

		// Selecting the step shows its Locks, each parameter's own; taking its Hit off gives every parameter back to
		// the Kit.
		out = &e.redrawAll();
		CHECK(lastPanel(*out, ints("lock", {Vol, 40, 1})));
		CHECK(lastPanel(*out, ints("lock", {0, 62, 1})));
		CHECK(lastPanel(*out, ints("lock", {1, NoLock, 0})));
		CHECK(lastPanel(*out, ints("lock", {LockCount - 1, NoLock, 0})));
		out = &e.setHit(false);
		CHECK(lastPanel(*out, ints("lock", {Vol, NoLock, 0})));
		CHECK(lastPanel(*out, ints("lock", {0, NoLock, 0})));
		CHECK(!e.pattern().steps[0][0].locked());

		// Back to the Kit from the dial (its middle position, where 63 would be).
		e.setHit(true);
		e.setLock(Vol, 90);
		out = &e.setLock(Vol, NoLock);
		CHECK(out->forward.size() == 1 && out->forward[0].value == NoLock);
		CHECK(lastPanel(*out, ints("lock", {Vol, NoLock, 0})));
		CHECK(e.setLock(LockCount, 10).panel.empty());
	}

	void conditionsFromThePanel()
	{
		Editor e;
		// No Hit: refused, the menu back to none.
		const EditorOutput* out = &e.setCondition(22);
		CHECK(out->forward.empty());
		CHECK(out->panel.size() == 1 && lastPanel(*out, ints("condition", {0})));

		e.setHit(true);
		out = &e.setCondition(22);
		CHECK(out->forward.size() == 1 && out->forward[0].kind == Input::Kind::SetCondition &&
			out->forward[0].value == 22);
		CHECK(lastPanel(*out, ints("condition", {22})));
		CHECK(out->grid.size() == 2 + 1 + 2);	// background, Hit, label, frame of 2 px
		CHECK(lastPanel(e.redrawAll(), ints("condition", {22})));

		// Taking the Hit off takes the Condition with it.
		CHECK(lastPanel(e.setHit(false), ints("condition", {0})));

		// The Fill is forwarded, never applied to the Pattern.
		out = &e.setFill(true);
		CHECK(out->forward.size() == 1 && out->forward[0].kind == Input::Kind::SetFill && out->forward[0].value == 1);
	}

	void ratchetsFromThePanel()
	{
		Editor e;
		// The menu holds 1 to 8 at indices 0 to 7; the text says how the curve shares the velocity out.
		// No Hit: refused, the panel back to one Hit without a curve.
		const EditorOutput* out = &e.setRatchets(3);
		CHECK(out->forward.empty());
		CHECK(lastPanel(*out, ints("ratchetsindex", {0})) && lastPanel(*out, ints("curve", {0})));
		CHECK(lastText(*out, "curvetext") == "One Hit");
		CHECK(e.setCurve(-50).forward.empty());

		e.setHit(true);
		out = &e.setRatchets(3);
		CHECK(forwarded(*out, Input::Kind::SetRatchets, 0, 0, 3));
		CHECK(lastPanel(*out, ints("ratchetsindex", {2})) && lastPanel(*out, ints("curve", {0})));
		CHECK(lastText(*out, "curvetext") == "Even over the 3");
		CHECK(out->grid.size() == 2 + 3 + 2);	// background, Hit, three notches, frame of 2 px
		CHECK(out->strip.size() == 2 + 3);

		// The curve shows in the panel only.
		out = &e.setCurve(-50);
		CHECK(forwarded(*out, Input::Kind::SetCurve, 0, 0, -50));
		CHECK(lastPanel(*out, ints("ratchetsindex", {2})) && lastPanel(*out, ints("curve", {-50})));
		CHECK(lastText(*out, "curvetext") == "-50 %, falling over the 3");
		CHECK(out->grid.empty() && out->strip.empty());
		out = &e.redrawAll();
		CHECK(lastPanel(*out, ints("ratchetsindex", {2})) && lastPanel(*out, ints("curve", {-50})));
		out = &e.setRatchets(20);
		CHECK(lastPanel(*out, ints("ratchetsindex", {7})));
		CHECK(lastText(*out, "curvetext") == "-50 %, falling over the 8");
		out = &e.setCurve(75);
		CHECK(lastPanel(*out, ints("curve", {75})) && lastText(*out, "curvetext") == "+75 %, rising over the 8");
		CHECK(lastText(e.setCurve(0), "curvetext") == "Even over the 8");
		// One Hit has no curve to speak of, whatever it holds.
		e.setCurve(40);
		out = &e.setRatchets(1);
		CHECK(lastPanel(*out, ints("curve", {40})) && lastText(*out, "curvetext") == "One Hit");

		// Taking the Hit off takes them with it.
		out = &e.setHit(false);
		CHECK(lastPanel(*out, ints("ratchetsindex", {0})) && lastPanel(*out, ints("curve", {0})));
		CHECK(lastText(*out, "curvetext") == "One Hit");
	}

	void microFromThePanel()
	{
		Editor e;
		// No Hit: refused, the slider back on the grid.
		const EditorOutput* out = &e.setMicro(-8);
		CHECK(out->forward.empty());
		CHECK(out->panel.size() == 2 && lastPanel(*out, ints("micro", {0})));
		CHECK(lastText(*out, "microtext") == "On the grid");

		e.setHit(true);
		out = &e.setMicro(-8);
		CHECK(forwarded(*out, Input::Kind::SetMicro, 0, 0, -8));
		// The slider shows no number: the text says where it is in its range.
		CHECK(lastPanel(*out, ints("micro", {-8})) && lastText(*out, "microtext") == "-8 of 23, early");
		CHECK(out->grid.size() == 2 + 2 + 1);	// background, Hit, frame of 2 px, the arrow over it
		CHECK(out->grid.back().kind == DrawOp::Kind::Triangle);
		CHECK(lastPanel(e.redrawAll(), ints("micro", {-8})));
		out = &e.setMicro(99);
		CHECK(lastPanel(*out, ints("micro", {MaxMicro})) && lastText(*out, "microtext") == "+23 of 23, late");
		CHECK(lastText(e.setMicro(1), "microtext") == "+1 of 23, late");
		CHECK(lastText(e.setMicro(0), "microtext") == "On the grid");

		// Taking the Hit off takes it with it.
		e.setMicro(5);
		out = &e.setHit(false);
		CHECK(lastPanel(*out, ints("micro", {0})) && lastText(*out, "microtext") == "On the grid");
	}

	// The scale's menu holds 1/8x to 2x at indices 0 to 6; the length's text names a step's note at each.
	void scaleFromTheWindow()
	{
		Editor e;
		const EditorOutput* out = &e.setScale(6);
		CHECK(forwarded(*out, Input::Kind::SetScale, 0, 0, 6));
		CHECK(lastPanel(*out, ints("scaleindex", {6})) && lastPanel(*out, ints("lengthindex", {DefaultLength - 1})));
		CHECK(lastText(*out, "lengthtext") == "16 steps, 1/32");
		CHECK(e.setScale(6).forward.empty());
		CHECK(e.setScale(ScaleCount).forward.empty());
		CHECK(e.pattern().scale == 6);

		const char* notes[ScaleCount] = {"1/2", "1/4", "1/8", "1/12", "1/16", "1/24", "1/32"};
		e.setLength(48);
		for(int s = 0; s < ScaleCount; ++s)
		{
			out = &e.setScale(s);
			CHECK(lastPanel(*out, ints("scaleindex", {s})));
			CHECK(lastText(*out, "lengthtext") == std::string("48 steps, ") + notes[s]);
		}
	}

	// The swing: its menu from the window, its row under the grid, where a click toggles a step for every Track and a
	// drag paints, the step selected staying.
	void swingFromTheWindow()
	{
		Editor e;
		e.redrawAll();
		const EditorOutput* out = &e.setSwing(66);
		CHECK(forwarded(*out, Input::Kind::SetSwing, 0, 0, 66));
		CHECK(lastPanel(*out, ints("swing", {16})));
		CHECK(out->grid.size() == SwingRowOps);	// the row again: its marks move with the amount
		CHECK(e.setSwing(66).forward.empty());
		CHECK(e.pattern().swing == 66);

		// Step 2 swings by default: a click takes it off, a drag over steps 3 and 4 takes them off too (step 3 did
		// not swing: nothing to forward), the selection still Track 1's step 1.
		e.gridMouse(true);
		out = &e.gridClick(gridX(1), gridY(grid::TrigRow));
		CHECK(forwarded(*out, Input::Kind::SetSwingStep, 0, 1, 0));
		CHECK(out->grid.size() == 1 && out->panel.empty());	// the cell without its mark
		CHECK(e.gridClick(gridX(2), gridY(grid::TrigRow)).forward.empty());
		CHECK(forwarded(e.gridClick(gridX(3), gridY(grid::TrigRow)), Input::Kind::SetSwingStep, 0, 3, 0));
		e.gridMouse(false);
		CHECK((e.pattern().swingSteps & 0xf) == 0);
		CHECK(e.selectedTrack() == 0 && e.selectedStep() == 0);

		// A new gesture on a step without swing puts it on, in the half shown.
		e.showHalf(1);
		e.gridMouse(true);
		CHECK(forwarded(e.gridClick(gridX(2), gridY(grid::TrigRow)), Input::Kind::SetSwingStep, 0, 34, 1));
		e.gridMouse(false);
		CHECK((e.pattern().swingSteps >> 34 & 1) == 1);

		// From elsewhere: the amount and a step, shown as from the window.
		out = &e.apply({Input::Kind::SetSwing, 0, 0, 80});
		// Steps 33-64 are past the length: the half's row is off, step 35's swing kept for when it grows.
		CHECK(lastPanel(*out, ints("swing", {30})) && out->grid.size() == SwingRowPastOps);
		out = &e.apply({Input::Kind::SetSwingStep, 0, 33, 0});
		CHECK(out->grid.size() == 1 && out->forward.size() == 1);
		out = &e.apply({Input::Kind::SetSwingStep, 0, 3, 1});	// in the other half: forwarded, nothing drawn
		CHECK(out->grid.empty() && out->forward.size() == 1);
	}

	// The Accent (issue 26): its amount in the window, its row the trig row once the menu chooses it, which the strip
	// marks.
	void accentFromTheWindow()
	{
		Editor e;
		e.redrawAll();
		CHECK(e.showTrigRow(1).grid.size() == 32 && e.trigRow() == Editor::TrigRow::Accent);	// no step accented
		const EditorOutput* out = &e.setAccent(40);
		CHECK(forwarded(*out, Input::Kind::SetAccent, 0, 0, 40));
		CHECK(lastPanel(*out, ints("accent", {40})) && out->grid.empty());	// the row shows the steps, not the amount
		CHECK(e.setAccent(40).forward.empty());
		CHECK(e.pattern().accent == 40);

		// A click puts step 2 in the row, a drag over steps 3 and 4 too, the selection still Track 1's step 1; the
		// strip, on page 1, marks each.
		e.gridMouse(true);
		out = &e.gridClick(gridX(1), gridY(grid::TrigRow));
		CHECK(forwarded(*out, Input::Kind::SetAccentStep, 0, 1, 1));
		CHECK(out->grid.size() == 2 && out->strip.size() == 2 && out->panel.empty());	// the cell and its mark
		CHECK(forwarded(e.gridClick(gridX(2), gridY(grid::TrigRow)), Input::Kind::SetAccentStep, 0, 2, 1));
		CHECK(forwarded(e.gridClick(gridX(3), gridY(grid::TrigRow)), Input::Kind::SetAccentStep, 0, 3, 1));
		e.gridMouse(false);
		CHECK((e.pattern().accentSteps & 0xf) == 0xe);
		CHECK(e.selectedTrack() == 0 && e.selectedStep() == 0);
		// A new gesture on an accented step takes it off.
		e.gridMouse(true);
		CHECK(forwarded(e.gridClick(gridX(2), gridY(grid::TrigRow)), Input::Kind::SetAccentStep, 0, 2, 0));
		e.gridMouse(false);
		CHECK((e.pattern().accentSteps & 0xf) == 0xa);

		// From elsewhere: the amount and a step, shown as from the window.
		out = &e.apply({Input::Kind::SetAccent, 0, 0, 127});
		CHECK(lastPanel(*out, ints("accent", {127})) && out->forward.size() == 1);
		out = &e.apply({Input::Kind::SetAccentStep, 0, 40, 1});	// in the other half and page: forwarded, nothing drawn
		CHECK(out->grid.empty() && out->strip.empty() && out->forward.size() == 1);
	}

	// The trig row's menu (issue 26): the swing row at first, then the Accent or the Slide row, each drawn whole in
	// its colour; a click edits the row shown, an edit of another row from elsewhere draws nothing in the grid.
	void trigRowMenu()
	{
		Editor e;
		CHECK(lastPanel(e.redrawAll(), ints("trigrow", {0})));
		const EditorOutput* out = &e.showTrigRow(2);
		CHECK(e.trigRow() == Editor::TrigRow::Slide && lastPanel(*out, ints("trigrow", {2})));
		CHECK(out->grid.size() == 32 && out->forward.empty());	// the Slide row, empty
		CHECK(e.showTrigRow(9).panel.size() == 1 && e.trigRow() == Editor::TrigRow::Slide);	// past the last: the last
		e.gridMouse(true);
		out = &e.gridClick(gridX(4), gridY(grid::TrigRow));
		CHECK(forwarded(*out, Input::Kind::SetSlideStep, 0, 4, 1));
		CHECK(out->grid.size() == 2 && out->grid[1].colour == colours::Sliding && out->strip.empty());
		e.gridMouse(false);
		CHECK(e.pattern().inSlideRow(4));
		// The swing row from elsewhere while the Slide row shows: forwarded, nothing drawn; the Accent's step, the strip
		// marks.
		out = &e.apply({Input::Kind::SetSwingStep, 0, 2, 1});
		CHECK(out->grid.empty() && out->forward.size() == 1);
		out = &e.apply({Input::Kind::SetAccentStep, 0, 2, 1});
		CHECK(out->grid.empty() && out->strip.size() == 2);
		// Back to the swing row: drawn whole again, step 3 swinging now.
		out = &e.showTrigRow(0);
		CHECK(out->grid.size() == SwingRowOps + 1 && e.pattern().inSwingRow(2));
	}

	void patterns()
	{
		// Stopped: the Pattern chosen is shown at once, and forwarded.
		Editor e;
		const EditorOutput* out = &e.setPattern(3);
		CHECK(forwarded(*out, Input::Kind::SetPattern, 0, 0, 3));
		CHECK(e.shown() == 3 && out->grid.size() == 1 + 512 + 2 + SwingRowOps + 1);
		CHECK(lastPanel(*out, ints("patternindex", {3})));

		// Its edits name it.
		out = &e.setHit(true);
		CHECK(out->forward.size() == 1 && out->forward[0].pattern == 3);
		CHECK(e.pattern(3).steps[0][0].hit && !e.pattern(0).steps[0][0].hit);

		// Playing: queued, shown when the Sequencer switches. The tab shows the Pattern chosen, the one queued.
		e.played(5);
		out = &e.setPattern(7);
		CHECK(forwarded(*out, Input::Kind::SetPattern, 0, 0, 7));
		CHECK(out->grid.empty() && e.shown() == 3);
		CHECK(lastPanel(*out, ints("patternindex", {7})));
		CHECK(lastPanel(e.setPattern(3), ints("patternindex", {3})));	// taken back
		e.setPattern(7);
		out = &e.follow(7, -1);
		CHECK(e.shown() == 7 && out->grid.size() > 512);
		CHECK(lastPanel(*out, ints("patternindex", {7})));
		CHECK(e.follow(7, -1).panel.empty());

		// The Sequencer reports a Pattern queued without a switch: the tab, the status line and the Patterns' buttons
		// alone.
		out = &e.follow(7, 2);
		CHECK(out->panel.size() == 2 + PatternCount && out->grid.empty());
		CHECK(lastPanel(*out, ints("patternindex", {2})) && lastText(*out, "status") == "03 next, after step 16");

		// A Pattern loaded into the bank is redrawn only if shown.
		Pattern p;
		p.length = 12;
		CHECK(e.load(2, p).grid.empty() && e.pattern(2).length == 12);
		CHECK(!e.load(7, p).grid.empty());
	}

	void loadsAPattern()
	{
		Editor e;
		Pattern p;
		p.length = 32;
		p.steps[4][3].hit = true;
		const EditorOutput& out = e.load(0, p);
		CHECK(out.forward.empty());	// the external hands the Sequencer the same Pattern
		// Every cell, the selected frame, the Hit loaded.
		CHECK(out.strip.size() == 1 + 16 && out.grid.size() == 1 + 512 + 2 + 1 + SwingRowFullOps + 1);
		CHECK(hasPanel(out, ints("lengthindex", {31})) && hasPanel(out, text("lengthtext", "32 steps, 1/16")));
		CHECK(e.pattern().steps[4][3].hit && e.pattern().length == 32);
	}

	// "Copy to": the Pattern shown over another, forwarded as an edit of the Pattern written; the menu back on its
	// label whatever was chosen.
	void copiesAPattern()
	{
		Editor e;
		Sequencer seq;	// fed what the Editor forwards: it copies its own the same
		const auto feed = [&](const EditorOutput& _out) -> const EditorOutput&
		{
			for(const Input& in : _out.forward)
				seq.input(unpack(pack(in)));
			return _out;
		};
		feed(e.setHit(true));
		feed(e.setLength(12));
		const EditorOutput* out = &feed(e.copyPattern(4));
		CHECK(out->forward.size() == 1 && out->forward[0].kind == Input::Kind::CopyPattern &&
			out->forward[0].pattern == 4 && out->forward[0].value == 0);
		CHECK(e.pattern(4) == e.pattern(0) && e.shown() == 0);
		CHECK(seq.pattern(4) == e.pattern(4) && seq.pattern(4).length == 12);
		CHECK(out->grid.empty() && out->strip.empty());	// the Pattern shown has not changed
		CHECK(lastPanel(*out, ints("copyto", {0})));
		// Onto itself, onto a Pattern equal already, outside the bank: nothing forwarded.
		CHECK(e.copyPattern(0).forward.empty() && e.copyPattern(4).forward.empty());
		CHECK(e.copyPattern(-1).forward.empty());
		out = &e.copyPattern(PatternCount);
		CHECK(out->forward.empty() && lastPanel(*out, ints("copyto", {0})));
		// Over a Pattern that differs again.
		feed(e.setHit(false));
		feed(e.copyPattern(4));
		CHECK(seq.pattern(4) == e.pattern(4) && !seq.pattern(4).steps[0][0].hit);
	}

	// x2, as Live's Duplicate Loop: the length doubled, up to 64, and steps 1-L again after them on every Track, with
	// their Locks, Conditions and swing.
	void doublesThePattern()
	{
		constexpr int Vol = 17;
		Editor e;
		e.setHit(true);
		e.setVelocity(90);
		e.setLock(Vol, 40);
		e.setCondition(3);
		e.apply({Input::Kind::SetHit, 5, 15, 1});
		e.apply({Input::Kind::SetSwingStep, 0, 2, 1});	// step 3 joins the swing row
		const EditorOutput* out = &e.doubleLength();
		CHECK(forwarded(*out, Input::Kind::DoubleLength, 0, 0, 0));
		const Pattern& p = e.pattern();
		CHECK(p.length == 32);
		CHECK(p.steps[0][16] == p.steps[0][0] && p.steps[0][16].locks[Vol] == 40 && p.steps[0][16].condition == 3);
		CHECK(p.steps[5][31].hit && !p.steps[5][15 + 32].hit);
		CHECK(p.inSwingRow(18) && p.inSwingRow(17) && !p.inSwingRow(16));
		// Everything drawn again: the strip's page, the grid's half, the length.
		CHECK(!out->strip.empty() && out->strip[0].kind == DrawOp::Kind::Clear);
		CHECK(!out->grid.empty() && out->grid[0].kind == DrawOp::Kind::Clear);
		CHECK(lastPanel(*out, ints("lengthindex", {31})) && lastText(*out, "lengthtext") == "32 steps, 1/16");
		CHECK(lastPanel(*out, ints("pages", {0, 2})));	// steps 17-32 within the length: their page key on

		// From 40 steps: 64, steps 41-64 being steps 1-24. At 64, nothing left to double into.
		e.setLength(40);
		e.doubleLength();
		CHECK(e.pattern().length == 64 && e.pattern().steps[0][40] == e.pattern().steps[0][0]);
		CHECK(e.doubleLength().forward.empty());
	}

	// The last SetPattern forwarded, -1 for none.
	int choice(const EditorOutput& _out)
	{
		int chosen = -1;
		for(const Input& in : _out.forward)
			if(in.kind == Input::Kind::SetPattern)
				chosen = in.value;
		return chosen;
	}

	// The chain as the Editor forwards it: its Patterns, then how many play.
	bool chainForwarded(const EditorOutput& _out, const std::vector<int>& _patterns, const int _plays)
	{
		std::vector<Input> expected;
		for(size_t i = 0; i < _patterns.size(); ++i)
			expected.push_back({Input::Kind::ChainEntry, 0, static_cast<int>(i), _patterns[i]});
		expected.push_back({Input::Kind::SetChain, 0, 0, _plays});
		if(_out.forward.size() < expected.size())
			return false;
		const size_t from = _out.forward.size() - expected.size();
		for(size_t i = 0; i < expected.size(); ++i)
		{
			const Input& in = _out.forward[from + i];
			if(in.kind != expected[i].kind || in.step != expected[i].step || in.value != expected[i].value)
				return false;
		}
		return true;
	}

	// A chain: built in Chain mode, a Pattern clicked appended; once the mode is off, forwarded to the Sequencer,
	// which plays each Pattern for one Pass, round and round, and reports its place; ended by a Pattern chosen by hand.
	void chains()
	{
		Editor e;
		EditorOutput out = e.setChainMode(true);
		CHECK(lastPanel(out, ints("chainmode", {1})) && lastText(out, "chaintext") == "Chain:");
		e.chainAdd(0);
		e.chainAdd(2);
		e.chainAdd(2);
		out = e.chainAdd(4);
		CHECK(lastText(out, "chaintext") == "Chain: 01 03 03 05" && out.forward.empty());
		CHECK(e.shown() == 0);	// building chooses nothing

		// Stopped: the chain starts on its first Pattern, shown already here.
		out = e.setChainMode(false);
		CHECK(lastPanel(out, ints("chainmode", {0})) && lastText(out, "chaintext") == "Chain 1/4: 01 03 03 05");
		CHECK(choice(out) == -1 && chainForwarded(out, {0, 2, 2, 4}, 4));

		// Playing: the place the Sequencer reports, with the Pattern it plays.
		e.played(0);
		CHECK(lastText(e.follow(0, 2, 0), "chaintext").empty());	// the Pattern queued, no new place
		out = e.follow(2, -1, 1);
		CHECK(lastText(out, "chaintext") == "Chain 2/4: 01 03 03 05");
		CHECK(lastText(e.follow(2, 4, 2), "chaintext") == "Chain 3/4: 01 03 03 05");

		// A Pattern chosen by hand ends the chain, in the Sequencer too.
		out = e.setPattern(7);
		CHECK(choice(out) == 7 && out.forward.size() == 1 && lastText(out, "chaintext").empty());
		CHECK(lastText(e.follow(2, 7, 3), "chaintext").empty());	// a place reported late shows nothing

		// A new chain ends the one playing: the Pattern it queued still follows.
		e.setChainMode(true);
		e.chainAdd(1);
		e.chainAdd(3);
		e.setChainMode(false);
		out = e.setChainMode(true);
		CHECK(forwarded(out, Input::Kind::SetChain, 0, 0, 0));
		CHECK(e.setChainMode(false).forward.empty());

		// One Pattern in Chain mode is a choice like another; none chooses nothing.
		e.setChainMode(true);
		e.chainAdd(9);
		out = e.setChainMode(false);
		CHECK(choice(out) == 9 && out.forward.size() == 1);
		e.setChainMode(true);
		CHECK(e.setChainMode(false).forward.empty());

		// Sixteen at most.
		e.setChainMode(true);
		for(int i = 0; i < 20; ++i)
			e.chainAdd(i % PatternCount);
		CHECK(e.chainAdd(1).panel.empty());	// full
		CHECK(lastText(e.redrawAll(), "chaintext") == "Chain: 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16");
	}

	// The edit window closed: its grid is not drawn, neither by the step played nor by an edit, the strip still is;
	// shown again, the grid is drawn whole, once.
	void gridOnlyWhileTheWindowShows()
	{
		Editor e;
		CHECK(e.setWindow(true).grid.empty());	// shown already
		const EditorOutput& closed = e.setWindow(false);
		CHECK(closed.grid.empty() && closed.strip.empty());
		e.played(0);
		const EditorOutput& played = e.played(1);
		CHECK(played.grid.empty() && !played.strip.empty());
		CHECK(e.apply({Input::Kind::SetHit, 2, 5, 1}).grid.empty());
		CHECK(e.redrawAll().grid.empty());
		const EditorOutput& shown = e.setWindow(true);
		CHECK(!shown.grid.empty() && shown.grid.front().kind == DrawOp::Kind::Clear);
		CHECK(e.setWindow(true).grid.empty());
	}

	// The chain is kept in the set: "chainstore playing length patterns..." each time it starts or ends, for its pattr
	// Blob; what the Blob gives back when the set opens restores it, playing or not, and forwards it. Words this build
	// cannot read restore nothing.
	void chainKeptInTheSet()
	{
		Editor e;
		e.setChainMode(true);
		for(const int p : {0, 2, 2, 4})
			e.chainAdd(p);
		EditorOutput out = e.setChainMode(false);
		CHECK(lastPanel(out, ints("chainstore", {1, 4, 0, 2, 2, 4})));
		out = e.setPattern(7);
		CHECK(lastPanel(out, ints("chainstore", {0, 4, 0, 2, 2, 4})));

		Editor opened;
		out = opened.restoreChain({1, 3, 5, 6, 7});
		CHECK(lastText(out, "chaintext") == "Chain -/3: 06 07 08" && chainForwarded(out, {5, 6, 7}, 3));
		CHECK(opened.restoreChain({1, 3, 5, 6, 7}).panel.empty());	// the same: nothing
		out = opened.restoreChain({0, 2, 1, 2});
		CHECK(lastText(out, "chaintext").empty() && chainForwarded(out, {1, 2}, 0));	// kept, not playing
		CHECK(opened.restoreChain({0}).panel.empty());	// the Blob's initial value: no chain
		for(const std::vector<int>& bad : std::vector<std::vector<int>>{{1, 2, 1}, {1, 17}, {1, 2, 1, 16}, {2, 2, 1, 2}})
			CHECK(opened.restoreChain(bad).panel.empty());
	}

	// Clear: the Pattern shown back to an empty one, its length, scale and swing too, redrawn whole and forwarded as an
	// edit of it; the Sequencer clears its own.
	void clearsThePattern()
	{
		Editor e;
		Sequencer seq;
		const auto feed = [&](const EditorOutput& _out) -> const EditorOutput&
		{
			for(const Input& in : _out.forward)
				seq.input(unpack(pack(in)));
			return _out;
		};
		feed(e.setHit(true));
		feed(e.setLength(32));
		feed(e.setSwing(66));
		const EditorOutput& out = feed(e.clearPattern());
		CHECK(forwarded(out, Input::Kind::ClearPattern, 0, 0, 0) && out.forward[0].pattern == 0);
		CHECK(e.pattern() == Pattern{} && seq.pattern(0) == Pattern{});
		CHECK(!out.grid.empty() && out.grid[0].kind == DrawOp::Kind::Clear);
		CHECK(lastPanel(out, ints("lengthindex", {DefaultLength - 1})) && lastPanel(out, ints("swing", {0})));
		CHECK(e.clearPattern().forward.empty());	// empty already
	}

	// Each Pattern's button: lit while shown, outlined while queued, blinking with each blink(), bright while it holds a
	// Hit, faint while empty.
	void patternStates()
	{
		const auto state = [](const int _p, const PatternState _s)
		{
			return ints("patternstate", {_p, static_cast<int>(_s)});
		};
		Editor e;
		EditorOutput out = e.redrawAll();
		CHECK(lastPanel(out, state(0, PatternState::Shown)) && lastPanel(out, state(1, PatternState::Empty)));
		CHECK(lastPanel(out, state(PatternCount - 1, PatternState::Empty)));

		// A Pattern the set gives back, and one a copy writes, tell whether they hold a Hit.
		Pattern p;
		p.steps[2][5].hit = true;
		CHECK(lastPanel(e.load(3, p), state(3, PatternState::Used)));
		CHECK(lastPanel(e.load(3, Pattern{}), state(3, PatternState::Empty)));
		e.setHit(true);
		CHECK(lastPanel(e.copyPattern(5), state(5, PatternState::Used)));

		// Playing, a Pattern chosen is queued: outlined, then its own look and back at each blink.
		e.played(4);
		out = e.setPattern(7);
		CHECK(lastPanel(out, state(7, PatternState::Queued)) && lastPanel(out, state(0, PatternState::Shown)));
		out = e.blink();
		CHECK(out.panel.size() == 1 && lastPanel(out, state(7, PatternState::Empty)));
		CHECK(lastPanel(e.blink(), state(7, PatternState::Queued)));
		e.setPattern(5);
		CHECK(lastPanel(e.blink(), state(5, PatternState::Used)));

		// Switched: the new one lit, the old one bright as it holds a Hit; nothing queued, nothing blinks.
		out = e.follow(5, -1);
		CHECK(lastPanel(out, state(5, PatternState::Shown)) && lastPanel(out, state(0, PatternState::Used)));
		CHECK(e.blink().panel.empty());
	}

	// Fill edit: a Hit the mouse puts takes the Condition FILL, in the grid and in the strip, whether clicked or
	// painted; one it takes off goes as always. The grid and the strip draw again, the Hits that are neither FILL nor
	// !FILL faded.
	void fillEdit()
	{
		const int fill = conditionIndex("FILL");
		Editor e;
		e.apply({Input::Kind::SetHit, 2, 5, 1});	// a Hit before the mode, without a Condition
		EditorOutput out = e.setFillEdit(true);
		CHECK(lastPanel(out, ints("filledit", {1})));
		CHECK(!out.grid.empty() && out.grid[0].kind == DrawOp::Kind::Clear);
		CHECK(!out.strip.empty() && out.strip[0].kind == DrawOp::Kind::Clear);
		CHECK(e.setFillEdit(true).panel.empty());	// already on: nothing

		e.gridMouse(true);
		out = e.gridClick(gridX(3), gridY(1));
		e.gridMouse(false);
		CHECK(out.forward.size() == 2 && out.forward[0].kind == Input::Kind::SetHit &&
			out.forward[1].kind == Input::Kind::SetCondition && out.forward[1].value == fill);
		CHECK(e.pattern().steps[1][3].hit && e.pattern().steps[1][3].condition == fill);
		CHECK(lastPanel(out, ints("condition", {fill})));

		// A drag puts FILL Hits on the cells it crosses; the strip does the same.
		e.gridMouse(true);
		e.gridClick(gridX(6), gridY(4));
		e.gridClick(gridX(7), gridY(4));
		e.gridMouse(false);
		CHECK(e.pattern().steps[4][6].condition == fill && e.pattern().steps[4][7].condition == fill);
		e.stripMouse(true);
		e.stripClick(stripX(9), 5);
		e.stripMouse(false);
		CHECK(e.pattern().steps[0][9].hit && e.pattern().steps[0][9].condition == fill);

		// Double-clicking a FILL Hit (the first click selects it) takes it off, Condition and all.
		for(int i = 0; i < 2; ++i)
		{
			e.gridMouse(true);
			e.gridClick(gridX(3), gridY(1));
			e.gridMouse(false);
		}
		CHECK(!e.pattern().steps[1][3].hit && e.pattern().steps[1][3].condition == 0);

		// Off: a click puts a plain Hit again, and the grid draws again.
		out = e.setFillEdit(false);
		CHECK(lastPanel(out, ints("filledit", {0})) && !out.grid.empty());
		e.gridMouse(true);
		out = e.gridClick(gridX(3), gridY(1));
		e.gridMouse(false);
		CHECK(out.forward.size() == 1 && e.pattern().steps[1][3].condition == 0);
		CHECK(e.pattern().steps[2][5].condition == 0);	// the Hit from before was never touched
	}

	// Whatever the gestures, the Sequencer fed the forwarded edits, through the words the external hands between its
	// threads, holds the Editor's Pattern.
	void forwardedEditsKeepTheSequencerEqual()
	{
		Editor e;
		Sequencer seq;
		std::mt19937 random(7);
		const auto feed = [&](const EditorOutput& _out)
		{
			for(const Input& in : _out.forward)
				seq.input(unpack(pack(in)));
		};
		for(int i = 0; i < 2000; ++i)
		{
			switch(random() % 23)
			{
			case 22: if(random() % 8 == 0) feed(e.clearPattern()); break;
			case 21: feed(e.setFillEdit(random() % 2 != 0)); break;
			case 20: feed(e.setLength(1 + static_cast<int>(random() % 20))); break;
			case 19: feed(e.doubleLength()); break;
			case 18: feed(e.copyPattern(static_cast<int>(random() % 3))); break;
			case 17: feed(e.setPattern(static_cast<int>(random() % 3))); break;
			case 16: feed(e.setScale(static_cast<int>(random() % ScaleCount))); break;
			case 15: feed(e.setMicro(static_cast<int>(random() % 51) - 25)); break;
			case 13: feed(e.setRatchets(1 + static_cast<int>(random() % 8))); break;
			case 14: feed(e.setCurve(static_cast<int>(random() % 201) - 100)); break;
			case 11: feed(e.setCondition(static_cast<int>(random() % ConditionCount))); break;
			case 12: feed(e.setFill(random() % 2 != 0)); break;
			case 9: feed(e.setLock(static_cast<int>(random() % LockCount), static_cast<int>(random() % 128))); break;
			case 10: feed(e.setLock(static_cast<int>(random() % LockCount), NoLock)); break;
			case 0: feed(e.stripClick(static_cast<int>(random() % strip::Width), 5)); break;
			case 1: feed(e.stripMouse(random() % 2 != 0)); break;
			case 2: feed(e.gridClick(static_cast<int>(random() % grid::Width), static_cast<int>(random() % grid::Height))); break;
			case 3: feed(e.gridMouse(random() % 2 != 0)); break;
			case 4: feed(e.showTrack(static_cast<int>(random() % 16))); break;
			case 5: feed(e.showPage(static_cast<int>(random() % 4))); break;
			case 6: feed(e.setVelocity(static_cast<int>(random() % 140))); break;
			case 7: feed(e.setHit(random() % 2 != 0)); break;
			default: feed(e.showHalf(static_cast<int>(random() % 2))); break;
			}
		}
		int differing = 0;
		for(int i = 0; i < PatternCount; ++i)
			if(!(e.pattern(i) == seq.pattern(i)))
				++differing;
		CHECK(differing == 0);
		CHECK(e.shown() == seq.current());
	}

	PanelValue tabs(const std::vector<std::string>& _labels) { return {"tracknames", {}, _labels}; }

	std::vector<std::string> numbers()
	{
		std::vector<std::string> n;
		for(int t = 0; t < TrackCount; ++t)
			n.push_back((t < 9 ? "0" : "") + std::to_string(t + 1));
		return n;
	}

	// The Kit MD Drums tells names the Tracks: their tabs in the strip, their rows in the window, the panel's title and
	// SYN1-8; without one, numbers and SYN1-8.
	void kitNames()
	{
		Editor e;
		EditorOutput out = e.redrawAll();
		CHECK(hasPanel(out, tabs(numbers())));
		CHECK(hasPanel(out, named("trackname", 3, "")));
		CHECK(hasPanel(out, named("lockname", 0, "SYN1")) && hasPanel(out, named("lockname", 7, "SYN8")));

		Kit kit;
		kit.name = "TRX UW";
		kit.tracks[0].machine = 16;
		kit.tracks[0].machineName = "TRX-BD";
		kit.tracks[0].synNames = {"PTCH", "DEC", "RAMP", "RDEC", "STRT", "NOIS", "HARM", "CLIP"};
		kit.tracks[3].machine = 19;
		kit.tracks[3].machineName = "TRX-CP";
		kit.tracks[3].synNames = {"CLPY", "TONE", "HARD", "RICH", "RATE", "ROOM", "RSIZ", "RTUN"};
		kit.tracks[5].machine = 22;
		kit.tracks[5].machineName = "TRX-CH";
		kit.tracks[5].synNames = {"GAP", "DEC", "HPF", "LPF", "MTAL", "-", "-", "-"};
		kit.tracks[6].machine = 141;
		kit.tracks[6].machineName = "ROM-14";
		kit.tracks[7].machine = 72;
		kit.tracks[7].machineName = "P-I-HH";
		kit.tracks[8].machine = 0;
		kit.tracks[8].machineName = "GND---";
		e.showTrack(5);
		out = e.setKit(kit);
		// A tab of 36 px holds the number and the Machine's short name: its last part, a numbered one after its
		// family's initial.
		std::vector<std::string> labels = numbers();
		labels[0] = "01 BD";
		labels[3] = "04 CP";
		labels[5] = "06 CH";
		labels[6] = "07 R14";
		labels[7] = "08 HH";
		labels[8] = "09 -";
		CHECK(hasPanel(out, tabs(labels)));
		CHECK(lastPanel(out, ints("trackindex", {5})));	// the strip's Track, again after the names
		// The window's rows have room for the whole name.
		CHECK(hasPanel(out, named("trackname", 0, "TRX-BD")) && hasPanel(out, named("trackname", 3, "TRX-CP")));
		CHECK(hasPanel(out, named("trackname", 1, "")));
		// The step selected is Track 1's.
		CHECK(lastText(out, "tracktitle") == "Track 01 TRX-BD");
		CHECK(hasPanel(out, named("lockname", 0, "PTCH")) && hasPanel(out, named("lockname", 7, "CLIP")));
		CHECK(out.strip.empty() && out.grid.empty() && out.forward.empty());
		CHECK(e.setKit(kit).panel.empty());	// the same Kit: nothing new

		// A step of Track 4 selected: its Machine's names; one of Track 2, whose Machine is not told: SYN1-8.
		e.gridMouse(true);
		out = e.gridClick(gridX(2), gridY(3));
		e.gridMouse(false);
		CHECK(lastText(out, "tracktitle") == "Track 04 TRX-CP" && hasPanel(out, named("lockname", 2, "HARD")));
		e.gridMouse(true);
		out = e.gridClick(gridX(2), gridY(1));
		e.gridMouse(false);
		CHECK(lastText(out, "tracktitle") == "Track 02" && hasPanel(out, named("lockname", 2, "SYN3")));
		// Track 6's Machine uses five of SYN1-8: the others say so, and show no value.
		e.gridMouse(true);
		out = e.gridClick(gridX(2), gridY(5));
		e.gridMouse(false);
		CHECK(hasPanel(out, named("lockname", 4, "MTAL")) && hasPanel(out, named("lockname", 5, "unused")));
		CHECK(lastPanel(out, named("lockvalue", 5, "")) && lastPanel(out, named("lockvalue", 4, "Kit")));

		// MD Drums gone: numbers again.
		out = e.setKit(std::nullopt);
		CHECK(hasPanel(out, tabs(numbers())) && hasPanel(out, named("trackname", 0, "")));
		CHECK(lastText(out, "tracktitle") == "Track 06" && hasPanel(out, named("lockname", 5, "SYN6")));
	}

	// A user's bank (seen in Live on 2026-10-20): MoDule's User Machines, from id 194, named as their packages name
	// them, and a ROM Machine named after the user's Sample it plays. Each name as long as its box holds: the tab cut
	// where Live would cut a letter in two, the row, the title and a SYN name with an ellipsis where Live broke the line.
	void userBankNames()
	{
		Kit kit;
		kit.tracks[0].machine = 176;
		kit.tracks[0].machineName = "gretsch_tom_06";
		kit.tracks[0].synNames = {"PTCH", "DEC", "HOLD", "BRR", "STRT", "END", "RTRG", "REVERBSIZE2"};
		kit.tracks[1].machine = 194;
		kit.tracks[1].machineName = "NFX-D2";
		kit.tracks[1].synNames = {"DRV", "TONE", "LVL", "OUT", "BRR", "MIX", "-", "-"};
		Editor e;
		e.redrawAll();
		const EditorOutput out = e.setKit(kit);
		std::vector<std::string> labels = numbers();
		labels[0] = "01 gret";
		labels[1] = "02 D2";
		CHECK(hasPanel(out, tabs(labels)));
		CHECK(hasPanel(out, named("trackname", 0, "gretsch_tom\xE2\x80\xA6")));
		CHECK(hasPanel(out, named("trackname", 1, "NFX-D2")));
		CHECK(lastText(out, "tracktitle") == "Track 01 gretsch_tom\xE2\x80\xA6");
		CHECK(hasPanel(out, named("lockname", 7, "REVERBSIZ\xE2\x80\xA6")));
	}

	// The Kit's values on the Lock dials: a dial without a Lock sits unlit on the Kit's value, which its text gives, so
	// that turning it starts the Lock from there; while MD Drums tells no value, on its middle, the Kit's position.
	void kitValuesOnLocks()
	{
		constexpr int Vol = 17;
		constexpr int Pan = 18;
		constexpr int Lfom = 23;
		Editor e;
		EditorOutput out = e.redrawAll();
		CHECK(lastPanel(out, ints("lock", {Vol, NoLock, 0})) && lastPanel(out, named("lockvalue", Vol, "Kit")));

		Kit kit;
		kit.tracks[0].machine = 16;
		kit.tracks[0].machineName = "TRX-BD";
		kit.tracks[0].values.fill(10);
		kit.tracks[0].values[Vol] = 100;
		kit.tracks[0].values[Pan] = NoLock;	// a value of the Kit like another, which no Lock can take
		kit.tracks[0].values[Lfom] = -1;	// not told
		out = e.setKit(kit);
		CHECK(lastPanel(out, ints("lock", {Vol, 100, 0})) && lastPanel(out, named("lockvalue", Vol, "Kit 100")));
		CHECK(lastPanel(out, ints("lock", {Pan, NoLock, 0})) && lastPanel(out, named("lockvalue", Pan, "Kit 63")));
		CHECK(lastPanel(out, ints("lock", {Lfom, NoLock, 0})) && lastPanel(out, named("lockvalue", Lfom, "Kit")));

		// A step without a Hit refuses the Lock: the dial goes back to the Kit's value at once.
		out = e.setLock(Vol, 101);
		CHECK(out.forward.empty() && lastPanel(out, ints("lock", {Vol, 100, 0})));

		e.setHit(true);
		out = e.setLock(Vol, 40);
		CHECK(lastPanel(out, ints("lock", {Vol, 40, 1})) && lastPanel(out, named("lockvalue", Vol, "40")));

		// Back to the Kit from the dial, a double-click or a drag through the middle: the dial stays where it is, on
		// the middle, until settle moves it to the Kit's value, so that a drag going on does not jump.
		out = e.setLock(Vol, NoLock);
		CHECK(out.forward.size() == 1 && out.forward[0].value == NoLock);
		CHECK(lastPanel(out, ints("lock", {Vol, NoLock, 0})) && lastPanel(out, named("lockvalue", Vol, "Kit 100")));
		out = e.settle();
		CHECK(out.panel.size() == 2 && lastPanel(out, ints("lock", {Vol, 100, 0})));
		CHECK(e.settle().panel.empty());

		// A drag through the middle that goes on: nothing left to settle.
		e.setLock(Vol, 62);
		e.setLock(Vol, NoLock);
		out = e.setLock(Vol, 64);
		CHECK(lastPanel(out, ints("lock", {Vol, 64, 1})));
		CHECK(e.settle().panel.empty());

		// Another step selected before settling: its dials show its own Locks, and nothing is left to settle.
		e.setLock(Vol, NoLock);
		e.gridMouse(true);
		e.gridClick(gridX(1), gridY(0));
		e.gridMouse(false);
		CHECK(e.settle().panel.empty());
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	redrawAll();
	panelInOrder();
	kitNames();
	userBankNames();
	kitValuesOnLocks();
	stripClicksAndDrags();
	oneClickAsTheLcdSendsIt();
	gridSelectsAndPaints();
	gridSelectsWithoutTakingOff();
	selectionTitles();
	mutesAndLength();
	pagesWithinTheLength();
	playedStep();
	columnNumbers();
	statusLine();
	editsFromElsewhere();
	locksFromThePanel();
	conditionsFromThePanel();
	ratchetsFromThePanel();
	microFromThePanel();
	loadsAPattern();
	copiesAPattern();
	doublesThePattern();
	clearsThePattern();
	chains();
	gridOnlyWhileTheWindowShows();
	chainKeptInTheSet();
	patternStates();
	fillEdit();
	scaleFromTheWindow();
	swingFromTheWindow();
	accentFromTheWindow();
	trigRowMenu();
	patterns();
	forwardedEditsKeepTheSequencerEqual();
	return mdseqTest::finish("mdseqEditorTest");
}
