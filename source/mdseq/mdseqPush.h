#pragma once

// Push 2 as the device's (issue 33): its pads laid out as Push's own Drum Rack, the 32 steps of the Track the strip
// shows on the four top rows, the 16 Tracks bottom left (Track 1 at the bottom left corner, as a Drum Rack's first
// pad), the 16 Patterns bottom right (01 at the top left, as in the strip); the Locks of the step held on the eight
// encoders, a page of eight at a time; a few buttons. The adapter grabs the controls through the LOM while the device
// and its track have the focus, hands their messages over, lights what this module says, and sends the notes it
// plays. Everything is edited through the Editor, as the window's clicks are. Main thread; it may allocate.

#include "mdseqEditor.h"
#include "mdseqKit.h"
#include "mdseqView.h"

#include <array>
#include <cstdint>
#include <vector>

namespace mdseq
{
	namespace push
	{
		constexpr int Columns = 8;
		constexpr int Rows = 8;			// row 0 at the top, as Push's matrix numbers them
		constexpr int StepRows = 4;		// the steps' rows, at the top
		constexpr int StepPads = StepRows * Columns;

		// A Hit pressed and let go sooner, without a Lock turned meanwhile, is taken off; held longer, it stays.
		constexpr double HoldMs = 400.0;

		// What a pad stands for: the step on the page shown (0-31), the Track (0-15), the Pattern (0-15); -1 for none.
		int stepAt(int _column, int _row);
		int trackAt(int _column, int _row);
		int patternAt(int _column, int _row);

		enum class Button : uint8_t
		{
			PageLeft,	// steps 1-32
			PageRight,	// steps 33-64
			Delete,		// held: a step's Hit, or a Pattern, cleared
			Duplicate,	// held: the first Pattern pressed copied over the second
			Double,		// the Pattern shown twice as long
			Mute,		// held, or tapped to latch it until tapped again: a Track pad mutes or unmutes its Track
			Accent,		// held: a step pad toggles the step in the Accent row
			LockPage1,	// the encoders on SYN1-8
			LockPage2,	// on the EFX page
			LockPage3	// on the ROUTING page
		};
		constexpr int ButtonCount = static_cast<int>(Button::LockPage3) + 1;
		// The control's name on Push 2's control surface, which the adapter grabs.
		const char* controlName(Button _button);
		// The eight encoders' names, Track_Control_0 to 7.
		const char* encoderName(int _index);
		constexpr int EncoderCount = 8;

		// The live.observer objects of the patch that report what Push sends: the pad matrix's, then one per button, then
		// one per encoder.
		constexpr int ObserverCount = 1 + ButtonCount + EncoderCount;
		constexpr int MatrixObserver = 0;
		constexpr int buttonObserver(const Button _button) { return 1 + static_cast<int>(_button); }
		constexpr int encoderObserver(const int _index) { return 1 + ButtonCount + _index; }

		// The colours the pads and the lit buttons take, each a slot of Push 2's palette that the adapter rewrites by
		// SysEx (slotOf) to its colour (colourOf) once it has grabbed the pads.
		enum class Colour : uint8_t
		{
			Off,
			Beat,			// an empty step on a beat
			HitLow,			// a Hit, by its velocity: 1-42, 43-95, 96-127
			HitMid,
			HitHigh,
			AccentEmpty,	// a step of the Accent row without a Hit
			AccentHit,		// with one
			Played,			// the step played
			Selected,		// the step held
			TrackPlays,
			TrackMuted,
			TrackShown,		// the Track the steps show
			TrackHit,		// a Track that sounds at the step played
			PatternEmpty,
			PatternUsed,
			PatternShown,
			PatternQueued,
			ButtonOn		// a button held, or the Lock page or the page of steps shown
		};
		constexpr int ColourCount = static_cast<int>(Colour::ButtonOn) + 1;
		uint8_t slotOf(Colour _colour);		// 0 for Off, Push's black; the others from 1
		Rgb colourOf(Colour _colour);

		// Push 2's SysEx that sets palette slot _slot to _rgb (its white channel at 0), and the one that has it take the
		// slots set.
		std::vector<uint8_t> paletteEntry(uint8_t _slot, Rgb _rgb);
		std::vector<uint8_t> reapplyPalette();

		struct Pad
		{
			int column;
			int row;
			uint8_t colour;		// a palette slot
		};

		struct Light
		{
			Button button;
			bool on;
		};
	}

	struct PushOutput
	{
		std::vector<push::Pad> pads;		// the pads whose colour changed since the last output
		std::vector<push::Light> buttons;	// the buttons whose light changed
		EditorOutput editor;				// what the Editor's calls gave, in order, for the patch and the Sequencer
		std::vector<MidiBytes> notes;		// a Track played from its pad, for the device's MIDI out

		void clear();
	};

	class PushMode
	{
	public:
		explicit PushMode(Editor& _editor);

		// The adapter has grabbed Push's controls: every pad and button lit; or given them back: nothing more sent, and
		// the next activation lights everything again. A Track still sounding from its pad is let go.
		const PushOutput& setActive(bool _active);
		bool active() const { return m_active; }

		// A pad pressed (_velocity 1-127) or let go (0), at _ms on any clock that runs.
		const PushOutput& pad(int _column, int _row, int _velocity, double _ms);
		const PushOutput& button(push::Button _button, bool _down);
		// An encoder turned by _delta clicks (Push 2 sends them relative, faster turns giving more).
		const PushOutput& encoder(int _index, int _delta);
		// What live.observer _observer (push::MatrixObserver, buttonObserver, encoderObserver) reports, as its values
		// come: the matrix's velocity, column and row, a button's value (down above 0), an encoder's clicks in two's
		// complement over seven bits. Nothing for an observer out of range or too few values.
		const PushOutput& observed(int _observer, const long* _values, size_t _count, double _ms);
		// What changed elsewhere (a click in the window, the step played, a Pattern switched, the queued one's blink):
		// the pads and lights that differ from those last sent.
		const PushOutput& refresh();
		// Every pad and light again: Push takes a grab a moment after it is asked, and drops what was sent before.
		const PushOutput& relight();

	private:
		void stepPad(int _column, int _row, bool _down, double _ms);
		void trackPad(int _track, int _velocity);
		void patternPad(int _pattern, bool _down);
		void take(const EditorOutput& _out);
		void light();
		push::Colour stepColour(int _step) const;
		push::Colour trackColour(int _track) const;
		push::Colour patternColour(int _pattern) const;
		bool held(push::Button _button) const { return m_held[static_cast<size_t>(_button)]; }

		Editor& m_editor;
		bool m_active = false;
		int m_half = 0;			// the steps shown: 1-32 or 33-64
		int m_lockPage = 0;		// the encoders' page of eight Locks
		std::array<bool, push::ButtonCount> m_held{};

		// The step pad held: the pad, its Track and step as pressed (a page turned or a Track shown meanwhile changes
		// neither), when, whether pressing it put its Hit, whether a Lock was turned
		int m_heldPad = -1;
		int m_heldTrack = 0;
		int m_heldStep = -1;
		double m_heldSince = 0.0;
		bool m_heldPut = false;
		bool m_heldTurned = false;

		int m_duplicateSource = -1;		// Duplicate held: the Pattern pressed first
		bool m_muteLatched = false;		// Mute tapped: the Track pads mute until it is tapped again
		bool m_muteUsed = false;		// a Track pad pressed while Mute is held, which then latches nothing
		std::array<bool, TrackCount> m_sounding{};	// Tracks played from their pads, not let go yet

		std::array<int, push::Rows * push::Columns> m_padsSent{};	// -1 before the first
		std::array<int, push::ButtonCount> m_lightsSent{};
		PushOutput m_out;
	};
}
