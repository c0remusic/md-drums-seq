// Push 2's pads, encoders and buttons as the device's (issue 33): the layout of Push's Drum Rack, the gestures on the
// steps, Tracks and Patterns, the Locks of the step held on the encoders, the buttons, and the pads lit.

#include "mdseqPush.h"
#include "mdseqTestSupport.h"

#include <cstdio>
#include <string>
#include <vector>

namespace
{
	using namespace mdseq;
	using push::Button;
	using push::Colour;

	// The colour last sent to a pad, as the adapter keeps lighting it: -1 for one never sent.
	struct Pads
	{
		int colour[push::Rows][push::Columns];

		Pads()
		{
			for(auto& row : colour)
				for(int& c : row)
					c = -1;
		}

		void take(const PushOutput& _out)
		{
			for(const push::Pad& p : _out.pads)
				colour[p.row][p.column] = p.colour;
		}

		int at(const int _column, const int _row) const { return colour[_row][_column]; }
	};

	int slot(const Colour _c) { return push::slotOf(_c); }

	// Whether the output lights the button (_on) or puts it out.
	bool lit(const PushOutput& _out, const Button _button, const bool _on)
	{
		for(const push::Light& l : _out.buttons)
			if(l.button == _button)
				return l.on == _on;
		return false;
	}

	bool forwards(const PushOutput& _out, const Input::Kind _kind)
	{
		for(const Input& in : _out.editor.forward)
			if(in.kind == _kind)
				return true;
		return false;
	}

	// The pads: steps 1-32 on the four top rows, left to right then down; the Tracks bottom left, Track 1 at the
	// bottom left corner as on Push's Drum Rack; the Patterns bottom right, 01 at the top left as in the strip.
	void layout()
	{
		CHECK(push::stepAt(0, 0) == 0 && push::stepAt(7, 0) == 7 && push::stepAt(0, 1) == 8 && push::stepAt(7, 3) == 31);
		CHECK(push::stepAt(0, 4) == -1);
		CHECK(push::trackAt(0, 7) == 0 && push::trackAt(3, 7) == 3 && push::trackAt(0, 6) == 4 && push::trackAt(3, 4) == 15);
		CHECK(push::trackAt(4, 7) == -1 && push::trackAt(0, 3) == -1);
		CHECK(push::patternAt(4, 4) == 0 && push::patternAt(7, 4) == 3 && push::patternAt(4, 5) == 4 &&
			push::patternAt(7, 7) == 15);
		CHECK(push::patternAt(3, 4) == -1);

		// The controls grabbed, by their names on Push 2's control surface (as Tuple's dump of them lists them).
		CHECK(std::string(push::controlName(Button::Accent)) == "Accent_Button");
		CHECK(std::string(push::controlName(Button::LockPage3)) == "Track_Select_Button2");
		CHECK(std::string(push::encoderName(7)) == "Track_Control_7" && std::string(push::encoderName(8)).empty());

		// A palette slot set by Push 2's SysEx: each channel's low seven bits then its top bit, white at 0; then the
		// reapply. Off stays Push's black, slot 0; the others are rewritten from slot 101.
		CHECK(push::paletteEntry(101, Rgb{0xff, 0xa5, 0x2e}) == (std::vector<uint8_t>{0xf0, 0x00, 0x21, 0x1d, 0x01,
			0x01, 0x03, 101, 0x7f, 0x01, 0x25, 0x01, 0x2e, 0x00, 0x00, 0x00, 0xf7}));
		CHECK(push::reapplyPalette() == (std::vector<uint8_t>{0xf0, 0x00, 0x21, 0x1d, 0x01, 0x01, 0x05, 0xf7}));
		CHECK(push::slotOf(Colour::Off) == 0 && push::slotOf(Colour::Beat) == 101);
		// Push keeps its own palette, the strip's before v12: an orange Hit, a red Accent.
		CHECK(push::colourOf(Colour::HitHigh) == (Rgb{0xff, 0xa5, 0x2e}));
		CHECK(push::colourOf(Colour::AccentHit) == (Rgb{0xff, 0x6b, 0x5b}));
	}

	// Lit once active, every pad; nothing while not.
	void lighting()
	{
		Editor editor;
		PushMode mode(editor);
		CHECK(mode.refresh().pads.empty());	// not active: Push is Live's
		Pads pads;
		pads.take(mode.setActive(true));
		int lit = 0;
		for(int r = 0; r < push::Rows; ++r)
			for(int c = 0; c < push::Columns; ++c)
				lit += pads.at(c, r) >= 0 ? 1 : 0;
		CHECK(lit == 64);
		// An empty Pattern of 16 steps: the beats marked, steps 17-32 off; Track 1 shown, the others play; Pattern 01
		// shown, the others empty.
		CHECK(pads.at(0, 0) == slot(Colour::Beat) && pads.at(4, 0) == slot(Colour::Beat));
		CHECK(pads.at(1, 0) == slot(Colour::Off));
		CHECK(pads.at(0, 2) == slot(Colour::Off));	// step 17, past the length
		CHECK(pads.at(0, 7) == slot(Colour::TrackShown) && pads.at(1, 7) == slot(Colour::TrackPlays));
		CHECK(pads.at(4, 4) == slot(Colour::PatternShown) && pads.at(5, 4) == slot(Colour::PatternEmpty));
		// Nothing new: nothing sent; relit, everything again.
		CHECK(mode.refresh().pads.empty());
		CHECK(mode.relight().pads.size() == 64 && mode.relight().buttons.size() == push::ButtonCount);
		CHECK(push::ObserverCount == 19 && push::buttonObserver(Button::PageLeft) == 1 &&
			push::encoderObserver(0) == 11 && push::encoderObserver(7) == 18);
		CHECK(mode.setActive(false).pads.empty() && !mode.active());
	}

	// A step pad: pressed empty, it puts a Hit at 127 and selects it; pressed on a Hit and let go soon, it takes the
	// Hit off; held long, or with a Lock turned, it keeps it.
	void steps()
	{
		Editor editor;
		PushMode mode(editor);
		mode.setActive(true);
		PushOutput out = mode.pad(2, 0, 90, 1000.0);
		CHECK(forwards(out, Input::Kind::SetHit));
		CHECK(editor.pattern().steps[0][2].hit && editor.pattern().steps[0][2].velocity == 127);
		CHECK(editor.selectedTrack() == 0 && editor.selectedStep() == 2);
		Pads pads;
		pads.take(out);
		CHECK(pads.at(2, 0) == slot(Colour::Selected));	// held: the step selected's colour
		CHECK(mode.pad(2, 0, 0, 1050.0).editor.forward.empty());	// let go: it stays
		CHECK(editor.pattern().steps[0][2].hit);

		// Pressed on the Hit and let go soon: off.
		mode.pad(2, 0, 90, 2000.0);
		CHECK(forwards(mode.pad(2, 0, 0, 2100.0), Input::Kind::SetHit));
		CHECK(!editor.pattern().steps[0][2].hit);

		// Held past HoldMs: kept, and selected.
		mode.pad(5, 1, 90, 3000.0);	// step 14: a Hit
		mode.pad(5, 1, 0, 3050.0);
		mode.pad(5, 1, 90, 4000.0);
		CHECK(mode.pad(5, 1, 0, 4000.0 + push::HoldMs + 1.0).editor.forward.empty());
		CHECK(editor.pattern().steps[0][13].hit && editor.selectedStep() == 13);

		// Past the Pattern's length: nothing.
		CHECK(mode.pad(0, 3, 90, 5000.0).editor.forward.empty() && !editor.pattern().steps[0][24].hit);

		// A pad let go is the pad pressed, whatever the page meanwhile: the Hit it pressed taken off, the step no longer
		// held (its encoders free).
		mode.button(Button::Double, true);
		mode.button(Button::Double, false);	// 32 steps
		mode.pad(5, 1, 90, 5500.0);	// step 14's Hit
		mode.button(Button::PageRight, true);
		mode.button(Button::PageRight, false);
		CHECK(forwards(mode.pad(5, 1, 0, 5600.0), Input::Kind::SetHit));
		CHECK(!editor.pattern().steps[0][13].hit && editor.pattern().steps[0][29].hit);
		CHECK(mode.encoder(0, 1).editor.forward.empty());
		mode.button(Button::PageLeft, true);
		mode.button(Button::PageLeft, false);
		mode.pad(5, 1, 90, 5700.0);	// step 14's Hit back
		mode.pad(5, 1, 0, 5710.0);

		// Delete held: a Hit taken off at once, Locks with it.
		mode.button(Button::Delete, true);
		CHECK(forwards(mode.pad(5, 1, 90, 6000.0), Input::Kind::SetHit));
		CHECK(!editor.pattern().steps[0][13].hit);
		mode.pad(5, 1, 0, 6010.0);
		mode.button(Button::Delete, false);

		// Accent held: the step toggles in the Accent row, every Track's, no Hit put.
		mode.button(Button::Accent, true);
		CHECK(forwards(mode.pad(1, 0, 90, 7000.0), Input::Kind::SetAccentStep));
		CHECK(editor.pattern().inAccentRow(1) && !editor.pattern().steps[0][1].hit);
		mode.pad(1, 0, 0, 7010.0);
		mode.button(Button::Accent, false);
	}

	// The encoders: the Locks of the step held, the page chosen below the display, one value a click, 63 skipped (it
	// cannot travel to MD Drums); nothing without a step held.
	void locks()
	{
		Editor editor;
		PushMode mode(editor);
		mode.setActive(true);
		mode.pad(0, 0, 90, 0.0);	// step 1: a Hit, held
		CHECK(forwards(mode.encoder(2, 1), Input::Kind::SetLock));	// SYN3, from 64 (the Kit unknown) to 65
		CHECK(editor.pattern().steps[0][0].locks[2] == 65);
		mode.encoder(2, -2);	// 65 to 63: past it, 62
		CHECK(editor.pattern().steps[0][0].locks[2] == 62);
		mode.button(Button::LockPage3, true);
		mode.button(Button::LockPage3, false);
		mode.encoder(1, -100);	// ROUTING's second, VOL: down to 0
		CHECK(editor.pattern().steps[0][0].locks[17] == 0);
		// Turned while held: kept when let go, even soon.
		CHECK(mode.pad(0, 0, 0, 10.0).editor.forward.empty() && editor.pattern().steps[0][0].hit);
		// Nothing held: nothing.
		CHECK(mode.encoder(0, 5).editor.forward.empty());
	}

	// A Track pad plays its Track at the pad's velocity through the device's MIDI out and shows it; let go, its
	// note-off. Mute held: it mutes the Track instead.
	void tracks()
	{
		Editor editor;
		PushMode mode(editor);
		Pads pads;
		pads.take(mode.setActive(true));
		PushOutput out = mode.pad(1, 7, 100, 0.0);	// Track 2
		pads.take(out);
		CHECK(out.notes.size() == 1 && out.notes[0] == (MidiBytes{0x91, FirstHitNote + 1, 100}));
		CHECK(editor.shownTrack() == 1 && pads.at(1, 7) == slot(Colour::TrackShown));
		// The strip's Track tab follows, as a tab clicked would show it.
		bool tab = false;
		for(const PanelValue& v : out.editor.panel)
			tab = tab || (v.key == "trackindex" && v.ints == std::vector<int>{1});
		CHECK(tab);
		out = mode.pad(1, 7, 0, 50.0);
		CHECK(out.notes.size() == 1 && out.notes[0] == (MidiBytes{0x81, FirstHitNote + 1, 0}));
		mode.button(Button::Mute, true);
		out = mode.pad(2, 6, 100, 100.0);	// Track 7
		pads.take(out);
		CHECK(out.notes.empty() && forwards(out, Input::Kind::SetMute) && editor.muted(6));
		mode.pad(2, 6, 0, 110.0);
		CHECK(!lit(mode.button(Button::Mute, false), Button::Mute, true));	// held with a pad: not latched
		CHECK(pads.at(2, 6) == slot(Colour::TrackMuted));

		// Mute tapped without a Track pad: latched, its button lit; each Track pad then mutes or unmutes its Track,
		// without a note, until Mute is tapped again.
		CHECK(lit(mode.button(Button::Mute, true), Button::Mute, true));
		CHECK(!lit(mode.button(Button::Mute, false), Button::Mute, false));	// let go: still lit
		out = mode.pad(2, 6, 100, 120.0);	// Track 7 back
		CHECK(out.notes.empty() && forwards(out, Input::Kind::SetMute) && !editor.muted(6));
		mode.pad(2, 6, 0, 130.0);
		out = mode.pad(3, 7, 100, 140.0);	// Track 4 muted
		CHECK(out.notes.empty() && editor.muted(3) && editor.shownTrack() == 1);
		mode.pad(3, 7, 0, 150.0);
		mode.button(Button::Mute, true);
		CHECK(lit(mode.button(Button::Mute, false), Button::Mute, false));
		CHECK(mode.pad(1, 7, 100, 160.0).notes.size() == 1);
		mode.pad(1, 7, 0, 170.0);

		// Let go of the mode with a Track sounding: its note-off. Taken again, the Mute is no longer latched.
		mode.pad(0, 7, 100, 200.0);
		mode.button(Button::Mute, true);
		mode.button(Button::Mute, false);
		out = mode.setActive(false);
		CHECK(out.notes.size() == 1 && out.notes[0] == (MidiBytes{0x80, FirstHitNote, 0}));
		CHECK(lit(mode.setActive(true), Button::Mute, false) && mode.pad(1, 7, 100, 300.0).notes.size() == 1);
	}

	// A Pattern pad chooses it, or adds it to the chain in Chain mode; Delete held clears it; Duplicate held, the first
	// Pattern pressed is copied over the second.
	void patterns()
	{
		Editor editor;
		PushMode mode(editor);
		mode.setActive(true);
		mode.pad(0, 0, 90, 0.0);	// a Hit in Pattern 01
		mode.pad(0, 0, 0, 10.0);
		mode.button(Button::Duplicate, true);
		mode.pad(4, 4, 90, 100.0);	// source: 01
		mode.pad(4, 4, 0, 110.0);
		CHECK(forwards(mode.pad(6, 4, 90, 200.0), Input::Kind::CopyPattern));	// target: 03
		mode.pad(6, 4, 0, 210.0);
		mode.button(Button::Duplicate, false);
		CHECK(editor.bank()[2].steps[0][0].hit);

		mode.button(Button::Delete, true);
		CHECK(forwards(mode.pad(6, 4, 90, 300.0), Input::Kind::ClearPattern));
		mode.pad(6, 4, 0, 310.0);
		mode.button(Button::Delete, false);
		CHECK(!editor.bank()[2].steps[0][0].hit && editor.bank()[0].steps[0][0].hit);

		CHECK(forwards(mode.pad(5, 4, 90, 400.0), Input::Kind::SetPattern));	// stopped: shown at once
		CHECK(editor.shown() == 1);
	}

	// Page shows steps 33-64, Double doubles the Pattern shown, and the step played lights its pad.
	void buttons()
	{
		Editor editor;
		PushMode mode(editor);
		Pads pads;
		pads.take(mode.setActive(true));
		pads.take(mode.pad(3, 0, 90, 0.0));	// step 4
		pads.take(mode.pad(3, 0, 0, 10.0));
		PushOutput out = mode.button(Button::Double, true);
		pads.take(out);
		CHECK(forwards(out, Input::Kind::DoubleLength));
		pads.take(mode.button(Button::Double, false));
		CHECK(editor.pattern().length == 32);
		pads.take(mode.button(Button::PageRight, true));
		pads.take(mode.button(Button::PageRight, false));
		CHECK(pads.at(3, 0) == slot(Colour::Off));	// step 36: past the length
		out = mode.button(Button::PageLeft, true);
		pads.take(out);
		CHECK(out.buttons.size() == 2);	// the two pages' lights trade places
		pads.take(mode.button(Button::PageLeft, false));
		CHECK(pads.at(3, 0) == slot(Colour::HitHigh) && pads.at(3, 2) == slot(Colour::HitHigh));	// steps 4 and 20

		editor.played(19);
		pads.take(mode.refresh());
		CHECK(pads.at(3, 2) == slot(Colour::Played));
		CHECK(pads.at(0, 7) == slot(Colour::TrackHit));	// Track 1 sounds at the step played
	}
}

namespace
{
	// What live.observer reports, as the adapter hands it on: the matrix's velocity, column and row; a button's
	// value; an encoder's clicks in two's complement over seven bits. Too few values, or an observer out of range, do
	// nothing.
	void observers()
	{
		Editor editor;
		PushMode mode(editor);
		mode.setActive(true);
		const long press[] = {90, 2, 0};
		CHECK(forwards(mode.observed(push::MatrixObserver, press, 3, 1000.0), Input::Kind::SetHit));
		CHECK(editor.pattern().steps[0][2].hit && editor.selectedStep() == 2);
		CHECK(mode.observed(push::MatrixObserver, press, 2, 1000.0).editor.forward.empty());

		// Turned while the step is held: down one click (127), stepping over NoLock; up two.
		const long down[] = {127};
		const long up[] = {2};
		CHECK(forwards(mode.observed(push::encoderObserver(0), down, 1, 1010.0), Input::Kind::SetLock));
		CHECK(editor.lockOrKit(0) == NoLock - 1);
		mode.observed(push::encoderObserver(0), up, 1, 1020.0);
		CHECK(editor.lockOrKit(0) == NoLock + 1);
		const long release[] = {0, 2, 0};
		mode.observed(push::MatrixObserver, release, 3, 1030.0);

		const long pressed[] = {1};
		CHECK(forwards(mode.observed(push::buttonObserver(Button::Double), pressed, 1, 1040.0),
			Input::Kind::DoubleLength));
		const PushOutput& none = mode.observed(push::ObserverCount, pressed, 1, 1050.0);
		CHECK(none.editor.forward.empty() && none.pads.empty() && none.buttons.empty());
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	layout();
	lighting();
	steps();
	locks();
	tracks();
	patterns();
	buttons();
	observers();
	return mdseqTest::finish("mdseqPushTest");
}
