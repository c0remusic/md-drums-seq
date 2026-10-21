#pragma once

// The device's interface state on Max's main thread: its own copy of the bank of Patterns (edited with the same
// applyEdit as the Sequencer's, so the two stay equal), the Pattern shown (the one played), the one queued, the Track
// and page the strip shows, the half of the Pattern the edit window shows, the step selected, the step played. Every
// call returns what the patch has to change: lcd operations for the strip and the grid, the messages for the step
// panel, the status line and the other controls, worded and numbered as the patch shows them, and the edits to
// forward to the Sequencer.
// Main thread only; it may allocate.

#include "mdseqKit.h"
#include "mdseqProtocol.h"
#include "mdseqTypes.h"
#include "mdseqView.h"

#include <array>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

namespace mdseq
{
	// One message to the patch, ready to send: the key its route sends on, one of mdseqProtocol.h's panel keys, then
	// its integers, then its texts, each one symbol. The adapter only turns each into a Max message, in the order given.
	struct PanelValue
	{
		std::string key;
		std::vector<int> ints;
		std::vector<std::string> texts;

		bool operator==(const PanelValue& _other) const
		{
			return key == _other.key && ints == _other.ints && texts == _other.texts;
		}
	};

	struct EditorOutput
	{
		std::vector<DrawOp> strip;
		std::vector<DrawOp> grid;
		std::vector<PanelValue> panel;
		std::vector<Input> forward;

		void clear();
	};

	class Editor
	{
	public:
		Editor();

		// Everything drawn again, with the step panel: on load and when a window opens.
		const EditorOutput& redrawAll();

		// The lcd reports the button going down and up apart from the positions, and sends the position once more
		// after the button comes up: a position counts only while the button is down. A click in the strip toggles the
		// Hit under it; dragging gives the cells crossed the same value.
		const EditorOutput& stripMouse(bool _down);
		const EditorOutput& stripClick(int _x, int _y);

		// A click in the grid selects the step for the panel and toggles its Hit; dragging paints as in the strip.
		const EditorOutput& gridMouse(bool _down);
		const EditorOutput& gridClick(int _x, int _y);

		const EditorOutput& showTrack(int _track);
		const EditorOutput& showPage(int _page);
		const EditorOutput& showHalf(int _half);
		// The window's half follows the step played.
		const EditorOutput& setFollow(bool _follow);
		// The edit window shown or closed: closed, nothing is drawn in its grid, drawn whole again when it shows. An
		// Editor starts with it shown; the external's starts with it closed, as the device loads.
		const EditorOutput& setWindow(bool _shown);

		const EditorOutput& setMute(int _track, bool _muted);
		// The step panel, on the step selected.
		const EditorOutput& setVelocity(int _velocity);
		const EditorOutput& setHit(bool _hit);
		// A Lock on the step selected, NoLock to give the parameter back to the Kit. A step without a Hit takes none:
		// the panel shows the Kit again. A dial without a Lock sits on the Kit's value, but NoLock leaves it on its
		// middle, where it sent NoLock from, until settle: moving it then would jump a drag that goes on through it.
		const EditorOutput& setLock(int _param, int _value);
		// The dials left on their middle by NoLock and not turned since, moved to the Kit's value. The adapter calls it
		// every Kit poll.
		const EditorOutput& settle();
		// The queued Pattern's button outlined, then its own look, in turn; the adapter calls it every Kit poll.
		const EditorOutput& blink();
		// The Condition of the step selected, its index in the menu; a step without a Hit takes none.
		const EditorOutput& setCondition(int _index);
		// The ratchets of the step selected, 1-8, and their velocity curve, -100 to +100; a step without a Hit takes
		// neither.
		const EditorOutput& setRatchets(int _count);
		const EditorOutput& setCurve(int _curve);
		// The micro-timing of the step selected, -23 to +23 24ths of a step; a step without a Hit takes none.
		const EditorOutput& setMicro(int _micro);
		// The Fill held or let go: not an edit, forwarded as it is.
		const EditorOutput& setFill(bool _held);
		const EditorOutput& setLength(int _length);
		// The Pattern's scale, by index: 1/8x to 2x.
		const EditorOutput& setScale(int _scale);
		// The trig row under the grid, which the menu at its left chooses: the swing row (its steps play late by the
		// Pattern's swing), the Accent row (every Hit of its steps plays at 128 + 2 x the Pattern's Accent; the strip
		// marks them, whichever row is shown) or the Slide row. A click in it toggles its step for every Track; dragging
		// paints as in the grid.
		enum class TrigRow
		{
			Swing,
			Accent,
			Slide
		};
		static constexpr int TrigRowCount = 3;
		const EditorOutput& showTrigRow(int _row);
		TrigRow trigRow() const { return m_trigRow; }
		// The Pattern's swing, 50-80 %.
		const EditorOutput& setSwing(int _percent);
		// The Pattern's Accent, 0-127.
		const EditorOutput& setAccent(int _amount);

		// The step the Sequencer plays, -1 when stopped.
		const EditorOutput& played(int _step);
		// The Pattern the Sequencer plays, the one it has queued and the place of its Pass in the chain (-1 for none): a
		// switch shows the new Pattern.
		const EditorOutput& follow(int _pattern, int _queued, int _chain = -1);
		// The Pattern to play: queued for the end of the Pass while playing, shown at once while stopped. Forwarded.
		const EditorOutput& setPattern(int _pattern);
		// The Pattern shown copied over Pattern _target, forwarded as an edit of _target (Live's undo gives it back,
		// through its Blob); the Copy to menu back on its label, whatever was chosen.
		const EditorOutput& copyPattern(int _target);
		// The Pattern shown twice as long, up to 64 steps, its steps again after them on every Track.
		const EditorOutput& doubleLength();
		// The Pattern shown back to an empty one, its length, scale, swing and Accent too (Live's undo gives it back,
		// through its Blob).
		const EditorOutput& clearPattern();
		// Fill edit: a Hit the mouse puts in the grid or the strip takes the Condition FILL, and the Hits the Fill
		// does not concern (neither FILL nor !FILL) are drawn faded.
		const EditorOutput& setFillEdit(bool _on);

		// A chain of Patterns, one Pass each, round and round. Chain mode on starts a new one, to which each Pattern
		// clicked is appended (chainAdd, up to ChainLength) instead of chosen; off, two or more are forwarded to the
		// Sequencer, which plays them (its first shown at once while stopped), one is chosen. A Pattern chosen by hand
		// (setPattern) ends it, in the Sequencer too.
		const EditorOutput& setChainMode(bool _on);
		const EditorOutput& chainAdd(int _pattern);
		// The chain as the set kept it ("chainstore playing length patterns...", which the Editor sends each time a
		// chain starts or ends): restored, playing or not, from its first Pattern once Live plays. Fewer than two
		// words (the Blob's initial value), or words this build cannot read, restore nothing.
		const EditorOutput& restoreChain(const std::vector<int>& _words);

		// An edit from elsewhere than the interface (a demonstration): applied, redrawn, forwarded.
		const EditorOutput& apply(const Input& _in);

		// A whole Pattern of the bank, the one the set kept: redrawn if it is the one shown, nothing forwarded (the
		// external hands the Sequencer the same Pattern).
		const EditorOutput& load(int _index, const Pattern& _pattern);

		// The Kit MD Drums tells, none while it tells nothing: each Track's Machine on its strip tab (its short name) and
		// its row in the window, the panel's title, the Machine's names for SYN1-8 and the Kit's values on the Lock dials.
		const EditorOutput& setKit(const std::optional<Kit>& _kit);

		// The step selected moved to _step of _track, drawn and told as a click selecting it would (Push's pads).
		const EditorOutput& selectStep(int _track, int _step);
		// Pattern _source copied over Pattern _target, and Pattern _pattern back to an empty one, as copyPattern and
		// clearPattern do for the Pattern shown (Push's Duplicate and Delete).
		const EditorOutput& copyPattern(int _target, int _source);
		const EditorOutput& clearPattern(int _pattern);

		const Pattern& pattern() const { return m_bank[m_shown]; }
		int shownTrack() const { return m_track; }	// the Track the strip shows
		int playedStep() const { return m_played; }
		bool chainMode() const { return m_chainMode; }
		bool muted(int _track) const { return (m_mutes >> _track & 1u) != 0; }
		PatternState patternState(int _pattern) const;
		// The step selected's value of parameter _param: its Lock, else the Kit's, -1 while MD Drums does not tell it.
		int lockOrKit(int _param) const;
		const Pattern& pattern(const int _index) const { return m_bank[_index]; }
		const std::vector<Pattern>& bank() const { return m_bank; }	// the PatternStore writes from it
		int shown() const { return m_shown; }
		int queued() const { return m_queued; }
		int selectedTrack() const { return m_selectedTrack; }
		int selectedStep() const { return m_selectedStep; }
		int half() const { return m_half; }
		bool fill() const { return m_fill; }	// held: a print plays the FILL steps

	private:
		bool edit(const Input& _in);
		void paint(int _track, int _step);
		void paintTrig(int _step);
		Input::Kind trigKind() const;		// the edit of a step of the trig row shown
		bool inTrigRow(int _step) const;	// whether the step is in the trig row shown
		void select(int _track, int _step);
		void redrawCell(int _track, int _step);
		void redrawGridColumn(int _step);
		void redrawStripStep(int _step);
		CellLook lookOf(int _step) const;	// a beat, the step played, past the length
		bool faded(const Step& _step) const;
		// The Pattern shown drawn again whole into the output, with the step panel, but not the Kit's names, which a
		// Pattern switch keeps.
		void drawPattern();
		void redrawStrip();
		void redrawGrid();
		void lengthChanged();
		int pageCount() const;		// the strip's pages within the Pattern's length, 1-4
		void keepPageInLength();
		void pagesPanel();
		void drawStripCell(int _cell);
		void drawGridCell(int _row, int _column);
		void drawTrigRow();
		void drawTrigCell(int _column);
		const Step& stepSelected() const;
		void panelInts(PanelKey _key, std::initializer_list<int> _ints);
		void panelText(PanelKey _key, const char* _text);
		void kitPanel();
		void trackPanel();
		void selectionPanel();
		void lengthPanel();
		void swingPanel();
		void accentPanel();
		void trigRowPanel();
		void halfPanel();
		void statusPanel();
		void patternPanel();
		void patternStatePanel(int _pattern);
		void choose(int _pattern);	// setPattern's choice, without ending a chain
		void forwardChain();		// the chain's Patterns, then how many play: none while it does not
		void chainPanel();
		void chainStore();			// the chain for its Blob in the set
		bool used(int _pattern) const;	// holds a Hit
		void mutePanel(int _track, bool _muted);
		void lockPanel(int _param);
		void lockPanel(int _param, int _position);
		int kitValue(int _param) const;	// the step selected's Track's, -1 while MD Drums does not tell it
		bool unused(int _param) const;	// a SYN its Machine does not use
		void conditionPanel();
		void microPanel();
		void ratchetsPanel();

		std::vector<Pattern> m_bank = std::vector<Pattern>(PatternCount);
		int m_shown = 0;		// the Pattern shown and edited: the one played
		int m_queued = -1;		// the one chosen to follow it
		bool m_queuedOutlined = true;	// the queued one's blink: outlined, or its own look
		uint16_t m_mutes = 0;
		bool m_fill = false;
		bool m_fillEdit = false;
		TrigRow m_trigRow = TrigRow::Swing;
		bool m_chainMode = false;		// building a chain: Patterns clicked are appended
		bool m_chainActive = false;		// a chain plays
		std::array<int, ChainLength> m_chain{};
		int m_chainLength = 0;
		int m_chainPos = -1;			// the place playing, as the Sequencer reports it; -1 while its first is queued

		int m_track = 0;	// the Track the strip shows
		int m_page = 0;		// the strip's page of 16 steps
		int m_half = 0;		// the window's half of 32 steps
		bool m_follow = true;
		bool m_window = true;	// the edit window shows: its grid is drawn
		int m_selectedTrack = 0;
		int m_selectedStep = 0;
		int m_played = -1;

		std::optional<Kit> m_kit;
		uint32_t m_unsettled = 0;	// a bit per Lock dial left on its middle by NoLock (setLock)

		// The value a drag gives, -1 before its first cell, PaintOnDrag while it started on a Hit not selected (which
		// a drag takes off, and a click only selects).
		static constexpr int PaintOnDrag = 2;
		int m_paintValue = -1;
		grid::Cell m_gestureStart{-1, -1};	// the row and the step a grid gesture started on
		bool m_stripDown = false;
		bool m_gridDown = false;
		int m_lastStripCell = -1;
		grid::Cell m_lastGridCell{-1, -1};

		EditorOutput m_out;
	};
}
