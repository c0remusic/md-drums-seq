#pragma once

// Printing (issue 07): N Passes of a Pattern as the notes of a clip per Track, the very stream the Sequencer plays
// ("flux joué = flux imprimé"): it is rendered by a Sequencer of its own, from the start seed the device last drew, so
// a print replays the Passes heard. Main thread; it allocates.

#include "mdseqTypes.h"

#include <array>
#include <cstdint>
#include <vector>

namespace mdseq
{
	constexpr int MaxPasses = 16;	// a print holds 1 to 16 Passes

	// A clip's note, in beats from the clip's start.
	struct PrintedNote
	{
		int pitch;
		double start;
		double duration;
		int velocity;

		bool operator==(const PrintedNote& _other) const
		{
			return pitch == _other.pitch && start == _other.start && duration == _other.duration &&
				velocity == _other.velocity;
		}
	};

	struct Printed
	{
		double length = 0.0;	// the clip's length in beats: _passes Passes
		std::array<std::vector<PrintedNote>, TrackCount> tracks;	// Hits and Locks, in the order played

		size_t trackCount() const;	// the Tracks with at least one note: those that get a clip
	};

	// _passes Passes of _pattern from _startSeed, the Fill held or not, Mutes ignored. A Hit the first step plays early
	// lands at the clip's end (looping, it sounds before each repeat, as the device plays it); a note that runs past
	// the end stops there.
	Printed printPattern(const Pattern& _pattern, int _passes, uint64_t _startSeed, bool _fill);
}
