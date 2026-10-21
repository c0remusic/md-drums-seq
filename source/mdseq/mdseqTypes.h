#pragma once

// The sequencer's values, shared by the core, its tests and the Max external. No Max header here (ADR 0002).

#include <array>
#include <cstdint>

namespace mdseq
{
	constexpr int TrackCount = 16;
	constexpr int MaxSteps = 64;
	constexpr int DefaultLength = 16;
	constexpr int PatternCount = 16;	// the device's bank
	constexpr int ChainLength = 16;		// the Patterns of a chain, at most

	// The Pattern's scale, the Machinedrum MKII's: 1/8x, 1/4x, 1/2x, 3/4x, 1x, 3/2x, 2x, by index.
	constexpr int ScaleCount = 7;
	constexpr int DefaultScale = 4;	// 1x

	// Live's song position in ticks, as Max's ITM counts them: 480 per quarter note; a 16th note, a step at 1x, is 120.
	constexpr double TicksPerQuarter = 480.0;
	constexpr double TicksPerStep = TicksPerQuarter / 4.0;

	// A step's length in ticks at each scale: a half note at 1/8x, a quarter, an 8th, an 8th-note triplet at 3/4x, a
	// 16th at 1x, a 16th-note triplet at 3/2x, a 32nd at 2x.
	constexpr double ScaleTicks[ScaleCount] = {960.0, 480.0, 240.0, 160.0, 120.0, 80.0, 60.0};

	// MD Drums plays Tracks 1-16 on notes 36-51, on any channel; the sequencer sends each Track on its own channel,
	// which its Locks will need (MD Drums' ADR 0003).
	constexpr uint8_t FirstHitNote = 36;
	constexpr uint8_t DefaultVelocity = 127;

	// The MIDI the sequencer sends: a note-on or a note-off on the channel of its Track.
	constexpr uint8_t NoteOn = 0x90;
	constexpr uint8_t NoteOff = 0x80;
	constexpr uint8_t noteStatus(const uint8_t _kind, const int _track) { return static_cast<uint8_t>(_kind | _track); }
	constexpr int trackOfStatus(const uint8_t _status) { return _status & 0x0f; }
	constexpr bool isNoteOn(const uint8_t _status, const uint8_t _velocity)
	{
		return (_status & 0xf0) == NoteOn && _velocity > 0;
	}

	// Locks: notes 64-87 for MD Drums' 24 Track parameters (SYN1-8, AMD AMF EQF EQG FLTF FLTW FLTQ SRR, DIST VOL PAN
	// DEL REV, LFOS LFOD LFOM), on their Track's channel, the value in the velocity: 1-63 give 0-62, 64-127 themselves,
	// so 63 cannot travel (MD Drums' ADR 0003). The sequencer uses 63 to mean "no Lock": the Kit's value.
	constexpr int LockCount = 24;
	constexpr int SynCount = 8;	// the first Locks, SYN1-8, which each Machine names its own way
	constexpr uint8_t FirstLockNote = 64;
	constexpr uint8_t NoLock = 63;

	// The velocity that carries a Lock's value.
	constexpr uint8_t lockVelocity(const uint8_t _value) { return _value < NoLock ? _value + 1 : _value; }

	// Accent, the Machinedrum's: every Hit of a step in the Pattern's Accent row plays at 128 + 2 x the Pattern's amount
	// (0-127). Note 88, after the Locks, tells MD Drums, its velocity the amount on the Locks' scale (MD Drums' ADR
	// 0003 as amended on 2026-10-17); it goes with its Hit as a Lock does.
	constexpr uint8_t AccentNote = FirstLockNote + LockCount;
	constexpr int MaxAccent = 127;

	// Slide, the Machinedrum's: on a step of the Pattern's Slide row, each Track's Hit has its Locks glide to those of
	// the Track's next Hit in the Pattern, whatever that Hit's Condition (a Lock that Hit lacks glides to the Kit's
	// value), over the clocks between the two Hits as they play, swing and micro-timing included. With the Hit's last
	// ratchet, after the Accent, as Locks go: note 89, its velocity the clocks (24 to a quarter note); notes 90-113, the
	// next Hit's Locks in the Locks' order and scale, for the Locks the step has; note 114 when the clocks pass 127, each
	// of its velocity's units 127 clocks more (issue 26, decided on 2026-10-19 on MD Drums' proposal).
	constexpr uint8_t SlideNote = AccentNote + 1;
	constexpr uint8_t FirstSlideTargetNote = SlideNote + 1;
	constexpr uint8_t LongSlideNote = FirstSlideTargetNote + LockCount;
	constexpr double TicksPerClock = TicksPerQuarter / 24.0;

	// Ratchets: 1 to 8 Hits shared out evenly over the step, their velocities along a curve from -100 % (falling from
	// the step's velocity) to +100 % (rising to it).
	constexpr int MaxRatchets = 8;
	constexpr int MaxCurve = 100;

	// Micro-timing: a step's Hits move by -23 to +23 24ths of a step, as on Elektron's machines (384 to a bar).
	constexpr int MaxMicro = 23;
	constexpr int MicrosPerStep = 24;

	// The core places its events in subticks, 840 to a step: every ratchet's share (1 to 8 to a step) and every
	// micro-timing offset (35 subticks a 24th) is a whole number of them, so that events compare exactly.
	constexpr int64_t SubticksPerStep = 840;
	constexpr int64_t SubticksPerMicro = SubticksPerStep / MicrosPerStep;

	// Swing, the Machinedrum's: the Pattern's amount, 50 % (straight) to 80 %, delays the steps of its swing row by
	// (2 x amount - 1) of a step, so that a pair of steps splits amount : 100 - amount. The row starts on every second
	// step, the off-beat 16ths at 1x, as the machine's does.
	constexpr int MinSwing = 50;
	constexpr int MaxSwing = 80;
	constexpr uint64_t DefaultSwingSteps = 0xaaaaaaaaaaaaaaaaull;

	// A swung step's delay in subticks, rounded: 504 at 80 %, 269 at 66 %.
	constexpr int64_t swingSubticks(const int _percent)
	{
		const int p = _percent < MinSwing ? MinSwing : _percent > MaxSwing ? MaxSwing : _percent;
		return ((p - MinSwing) * 2 * SubticksPerStep + 50) / 100;
	}

	struct Step
	{
		bool hit = false;
		uint8_t velocity = DefaultVelocity;
		// An index into the Conditions' list (mdseqCondition.h), 0 for none; a step without a Hit holds none.
		uint8_t condition = 0;
		// The Hits the step plays, 1-MaxRatchets, and their velocity curve; a step without a Hit holds 1 and 0.
		uint8_t ratchets = 1;
		int8_t curve = 0;
		// Its micro-timing in 24ths of a step, -MaxMicro to +MaxMicro, the ratchets moving with it; 0 without a Hit.
		int8_t micro = 0;
		// One per parameter, NoLock where the Kit plays; a step without a Hit holds none.
		std::array<uint8_t, LockCount> locks = []
		{
			std::array<uint8_t, LockCount> none{};
			none.fill(NoLock);
			return none;
		}();

		bool locked() const
		{
			for(const uint8_t l : locks)
				if(l != NoLock)
					return true;
			return false;
		}

		bool operator==(const Step& _other) const
		{
			return hit == _other.hit && velocity == _other.velocity && condition == _other.condition &&
				ratchets == _other.ratchets && curve == _other.curve && micro == _other.micro && locks == _other.locks;
		}
	};

	struct Pattern
	{
		std::array<std::array<Step, MaxSteps>, TrackCount> steps{};
		int length = DefaultLength;
		uint8_t scale = DefaultScale;
		uint8_t swing = MinSwing;
		uint64_t swingSteps = DefaultSwingSteps;	// bit s: step s swings
		uint8_t accent = 0;							// 0-127, the Accent's amount
		uint64_t accentSteps = 0;					// bit s: step s is accented, every Track's Hit there
		uint64_t slideSteps = 0;					// bit s: every Track's Locks glide from step s to its next Hit

		// Whether the step is in the swing row, and whether it plays late: in the row, at more than 50 %.
		bool inSwingRow(const int _step) const { return (swingSteps >> _step & 1u) != 0; }
		bool swings(const int _step) const { return swing > MinSwing && inSwingRow(_step); }
		bool inAccentRow(const int _step) const { return (accentSteps >> _step & 1u) != 0; }
		bool inSlideRow(const int _step) const { return (slideSteps >> _step & 1u) != 0; }

		bool operator==(const Pattern& _other) const
		{
			return length == _other.length && scale == _other.scale && swing == _other.swing &&
				swingSteps == _other.swingSteps && accent == _other.accent && accentSteps == _other.accentSteps &&
				slideSteps == _other.slideSteps && steps == _other.steps;
		}
	};

	// What the adapter reads from Live's transport at each look, as it reads it: the Sequencer tells a start, a stop
	// and a jump from one look to the next.
	struct Transport
	{
		double ticks = 0.0;
		bool playing = false;
		// When the adapter looked, in milliseconds, on a clock that runs whether the transport plays or not.
		double ms = 0.0;
		// What a millisecond is worth in ticks at the tempo of the look.
		double ticksPerMs = 0.0;
	};

	struct MidiEvent
	{
		double tick;	// the tick the event was planned for, which a late wake has already passed
		uint8_t bytes[3];
	};

	// An edit of a Pattern of the bank, or a Mute; Patterns, Tracks, steps and parameters from 0.
	struct Input
	{
		enum class Kind : uint8_t
		{
			SetHit,			// value 0 or 1; taking a Hit off takes its Condition, Locks, ratchets and micro-timing off
			SetVelocity,	// value 1-127
			SetMute,		// value 0 or 1; step unused
			SetLength,		// value 1-64; track and step unused
			SetLock,		// param 0-23, value 0-127, NoLock (63) to take the Lock off; only on a step with a Hit
			SetCondition,	// value: the Condition's index, 0 for none; only on a step with a Hit
			SetFill,		// value 0 or 1, held or let go; not an edit of the Pattern
			SetRatchets,	// value 1-8; only on a step with a Hit
			SetCurve,		// value -100 to +100, the ratchets' velocity curve; only on a step with a Hit
			SetMicro,		// value -23 to +23, in 24ths of a step; only on a step with a Hit
			SetScale,		// value 0-6, the scale's index; track and step unused
			SetPattern,		// value 0-15: the Pattern to play, at the end of the Pass while playing; not an edit
			SetSwing,		// value 50-80, the swing amount in percent; track and step unused
			SetSwingStep,	// value 0 or 1: whether the step swings, for every Track; track unused
			CopyPattern,	// value 0-15: the Pattern copied over the Pattern the edit names; track and step unused
			DoubleLength,	// the length doubled, up to 64, and steps 1-L again after them; track, step, value unused
			ClearPattern,	// the Pattern back to an empty one, length, scale, swing and accent too; track, step, value unused
			SetAccent,		// value 0-127, the Accent's amount; track and step unused
			SetAccentStep,	// value 0 or 1: whether the step is accented, for every Track; track unused
			SetSlideStep,	// value 0 or 1: whether the step slides, for every Track; track unused
			ChainEntry,		// step 0-15, value 0-15: the chain's Pattern at that place; not an edit
			SetChain		// value: how many of the chain's Patterns play, round and round, from 2; 0 for no chain. A
							// Pattern chosen (SetPattern) ends it. Not an edit
		};
		static constexpr int KindCount = static_cast<int>(Kind::SetChain) + 1;

		Kind kind;
		int track;
		int step;
		int value;
		int param = 0;
		int pattern = 0;	// the Pattern edited: an edit names it, so that none lands on another during a switch
	};
}
