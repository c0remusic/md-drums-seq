#pragma once

// The sequencer's core: it plans each step one step ahead and hands out the MIDI events that are due, in Live's ticks.
// Build it on the main thread; from then on, call it from Max's scheduler thread only (Live's audio thread in a Max
// for Live device): wake() and input() never allocate, lock or block (ADR 0002).

#include "mdseqCondition.h"
#include "mdseqTypes.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace mdseq
{
	constexpr double Never = std::numeric_limits<double>::infinity();

	// How late a look may see a start or a jump and still send, late, the Hits of the step it lands on that fell before
	// the position: the adapter sees the transport a vector after it moved.
	constexpr double GraceMs = 20.0;

	// The velocity of ratchet _index (from 0) of _count on a step of _velocity whose curve is _curve (-100 to +100):
	// the loudest has the step's velocity, the first when falling and the last when rising; the one at the other end
	// loses the curve's share of it, the ones between in proportion. Rounded, and never below 1.
	uint8_t ratchetVelocity(uint8_t _velocity, int _curve, int _index, int _count);

	struct Wake
	{
		// The events due, in the order to send them; valid until the next call to the Sequencer.
		const MidiEvent* begin;
		const MidiEvent* end;
		// The tick of the next useful wake, Never while stopped. Waking earlier does no harm, later sends late.
		double next;
		// The step played, from 0, and the Pass it belongs to, from 1; -1 and 0 while stopped.
		int step;
		uint32_t pass;
		// The Pattern played (the one chosen while stopped), and the one that follows at the end of its Pass, -1 for
		// none.
		int pattern;
		int queued;
	};

	// Where the Sequencer plays, as the main thread shows it (Editor::follow and Editor::played): a Wake's step,
	// Pattern played and Pattern queued. The defaults are a Sequencer's stopped on the first Pattern, none queued.
	struct Playhead
	{
		int step = -1;
		int pattern = 0;
		int queued = -1;
	};

	Playhead playhead(const Wake& _wake);

	// A Playhead as one word, a signed byte for each value (every step and Pattern fits, -1 too): the adapter stores it
	// on the scheduler thread and loads it on the main thread, each in one lock-free access, so that the three values
	// the main thread reads always come from the same wake.
	uint32_t packPlayhead(const Playhead& _playhead);
	Playhead unpackPlayhead(uint32_t _word);

	class Sequencer
	{
	public:
		// _seedSource feeds the seed each transport start draws its Conditions from: entropy in the device, a constant
		// in tests.
		explicit Sequencer(uint64_t _seedSource = 0);

		// Everything due up to where Live is, from a look at its transport as the adapter reads it: from one look to
		// the next the Sequencer tells a start, a stop and a jump (a seek, or the loop brace sending Live back). A
		// transport that stops, or jumps, also sends the note-off of every note sounding.
		Wake wake(const Transport& _now);

		// An edit of a Pattern of the bank, a Mute, the Fill, or the Pattern to play. Step n+1 is planned while step n
		// plays: an edit of it plans it again, so that it is heard on this Pass, but for the Tracks it has begun to
		// sound on (micro-timing moves them earlier), which play as they were planned. A Mute cuts at once, events
		// already planned included. A Pattern chosen while playing follows at the end of the Pass played, anchored
		// there and counting its Passes from 1; chosen while stopped, it is the one played. Returns false when nothing
		// changed.
		bool input(const Input& _in);

		// A whole Pattern of the bank, the one the set kept (loaded, or given back by Live's undo): if it is the one
		// playing, played from the step planned ahead, as an edit of it would be. Never allocates.
		void load(int _index, const Pattern& _pattern);

		const Pattern& pattern() const { return m_bank[m_current]; }
		const Pattern& pattern(const int _index) const { return m_bank[_index]; }
		// The Pattern planned from, and the one queued for the planning: during a Pass's last step, once the next Pass's
		// first step is planned from the Pattern queued, it is the current one already. A Wake tells what Live plays.
		int current() const { return m_current; }
		int queued() const { return m_queued; }
		// Printing (mdseqPrint): the next start draws from _startSeed instead of the seed source, and only _steps steps
		// are planned from it; its grace is infinite, so that a start at tick 0 plays step 0's early Hits before it.
		void prepareRender(uint64_t _startSeed, int64_t _steps);

		// A step's length now, in ticks: the scale of the Pattern played.
		double ticksPerStep() const { return m_ticksPerStep; }
		// The seed of the last transport start, from which every Pass's draws follow (printing replays them).
		uint64_t startSeed() const { return m_startSeed; }

	private:
		// At the same unit of time: note-offs first, then Track by Track the Hit and its Locks (see rankOf).
		enum class Kind : uint8_t
		{
			Off,
			Lock,
			Hit
		};

		// What a note plays: the Track's Hit, or one of its Locks.
		static constexpr uint8_t HitNote = 0xff;

		struct Planned
		{
			int64_t at;		// in units from the song's start (UnitsPerTick)
			int64_t step;	// the step it belongs to, counted from its Pattern's anchor
			uint16_t rank;	// the order among the events of one unit (rankOf)
			Kind kind;
			uint8_t track;
			uint8_t param;	// a Lock's parameter, HitNote for the Hit; a note-off carries its note's
			uint8_t velocity;
			uint8_t pattern;	// the Pattern it was planned from
			uint64_t note;	// which note it is, so that a note-off ends only its own note
		};

		// A Track that plays the step being planned: where its group of ratchets begins, their share of the step, the
		// first that plays and how many there are, and the next unit where one begins or ends.
		struct Playing
		{
			int track;
			int64_t begin;
			int64_t share;
			int first;
			int count;
			int64_t next;
		};

		// A step planned: where it begins, from which Pattern, its position and Pass; what the step played is.
		struct StepPlanned
		{
			double tick;
			int pattern;
			int position;
			uint32_t pass;
		};

		// Parenthesised: the Max SDK's headers define a max macro.
		static constexpr int64_t NoUnit = (std::numeric_limits<int64_t>::max)();

		int64_t stepAt(double _ticks) const;
		double tickOf(int64_t _at) const;
		static uint16_t rankOf(Kind _kind, uint8_t _track, uint8_t _param);
		int position(int64_t _step) const;
		uint16_t tracksBegun() const;

		bool choose(int _pattern);
		void switchTo(int _pattern, double _anchor);
		void applyScale();
		// Whether Live, playing since the last look, has moved other than by playing on.
		bool jumped(const Transport& _now) const;
		// Why the steps are planned afresh from where Live is: it started, it jumped, or the scale played changed.
		enum class Restart : uint8_t
		{
			Start,
			Jump,
			Rescale
		};
		void restart(const Transport& _now, Restart _why);
		void setCurrent(int _pattern);	// the Pattern planned from, none queued, its scale applied
		// Takes back the step planned ahead (but for the Tracks in _keep), then plans it again from the step played at
		// _position in Pass _pass.
		void dropPlannedAhead(uint16_t _keep);
		void replanAhead(int _position, uint32_t _pass);
		void planAheadAgain();
		double tickOfStep(int64_t _step) const;	// stepAt's inverse
		bool rendering() const { return m_planLimit != NoUnit; }
		void rescale();
		void planNext();
		void plan(int64_t _step);
		// Plans _step's events, but for the Tracks in _keep, which keep the events they were planned with; Hits before
		// _earliest are sent at it on a step that begins at or after it, and dropped on a step begun before it.
		void planStep(int64_t _step, const Draw& _draw, double _earliest, uint16_t _keep);
		void stageRatchet(int64_t _step, int64_t _at, int _track, int _ratchet, uint64_t _notes, bool _begins);
		static bool before(const Planned& _a, const Planned& _b)
		{
			return _a.at != _b.at ? _a.at < _b.at : _a.rank < _b.rank;
		}
		void emitDue(double _now);
		void emit(const Planned& _event);
		void silence(double _now);
		void out(double _tick, uint8_t _status, uint8_t _data1, uint8_t _data2);
		// The step played when Live is at _ticks.
		const StepPlanned& playedAt(double _ticks) const;
		Wake result(double _ticks) const;

		// The bank, on the heap: 16 Patterns are half a megabyte.
		std::vector<Pattern> m_bank;
		Pattern m_empty{};	// what a Clear loads, built on the main thread with the Sequencer
		int m_current = 0;				// the Pattern planned from
		int m_queued = -1;				// the one that follows at the end of its Pass
		double m_anchor = 0.0;			// the tick of the current Pattern's step 0: the song's start, then each switch
		uint16_t m_mutes = 0;

		bool m_playing = false;
		int64_t m_nextToPlan = 0;		// step k is planned once the transport reaches step k-1
		int m_lastPlannedPosition = -1;
		uint32_t m_lastPlannedPass = 0;
		int m_positionBeforePlanned = -1;	// the two above before the step planned ahead, to plan it again
		uint32_t m_passBeforePlanned = 0;
		StepPlanned m_planned[2]{};		// the step planned before the one planned ahead, and that one
		double m_earliest = 0.0;		// events planned before this tick are dropped (start, jump)
		double m_ticksPerStep = 0.0;	// the scale of the Pattern played, from applyScale
		int64_t m_stepUnits = 0;		// a step, and a 840th of it, in units
		int64_t m_subUnits = 0;
		bool m_rescaled = false;		// the scale changed while playing: the next wake goes where Live is
		// The last look while playing: where Live was, and when, from which the next look tells a jump.
		double m_lastTicks = 0.0;
		double m_lastMs = 0.0;

		std::vector<Planned> m_queue;	// sorted, capacity reserved for the worst case
		std::vector<Planned> m_staged;	// the step being planned, in order, then merged into the queue
		uint64_t m_nextNote = 1;
		// The note each Track's Hit and each of its Locks sounds, 0 for none.
		std::array<uint64_t, TrackCount> m_sounding{};
		std::array<std::array<uint64_t, LockCount>, TrackCount> m_lockSounding{};
		uint64_t& sounding(uint8_t _track, uint8_t _param);
		static uint8_t key(uint8_t _track, uint8_t _param);
		std::vector<MidiEvent> m_out;

		// Conditions: the seed of this start, PRE and NEI's memory, and what the step planned ahead was drawn with, to
		// draw it again when it is edited before it begins.
		uint64_t m_seedSource;
		// Printing (rendering()): the start seed is given, the start's grace is infinite, and planning stops at
		// m_planLimit.
		uint64_t m_givenSeed = 0;
		int64_t m_planLimit = NoUnit;
		uint32_t m_starts = 0;
		uint64_t m_startSeed = 0;
		bool m_fill = false;
		ConditionMemory m_memory;
		ConditionMemory m_memoryBeforePlanned;
		Draw m_drawOfPlanned{1, 0, false};
		// Per Track, for the step planned ahead: the unit of its first event (NoUnit for none), and its memory once its
		// Condition was drawn, which a Track kept when the step is planned again gives NEI and PRE.
		std::array<int64_t, TrackCount> m_firstOfPlanned{};
		std::array<int8_t, TrackCount> m_lastOfPlanned{};
	};
}
