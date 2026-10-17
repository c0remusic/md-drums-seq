#pragma once

// Conditions, as Elektron's machines play them (ticket "Sémantique du Pattern"): one per step, deciding each time the
// step comes round whether its Hit plays. A step holds its Condition as an index into one list, the panel's menu:
// none, X% (Elektron's 21 values), A:B (B from 2 to 8, A from 1 to B), then FILL, !FILL, PRE, !PRE, NEI, !NEI, 1ST,
// !1ST.

#include "mdseqTypes.h"

#include <array>
#include <cstdint>

namespace mdseq
{
	constexpr int ConditionCount = 65;

	struct ConditionInfo
	{
		enum class Kind : uint8_t
		{
			None,
			Percent,	// a: the chance, 1-99
			Ratio,		// a: the Pass played in each cycle of b
			Fill,
			NotFill,
			Pre,
			NotPre,
			Nei,
			NotNei,
			First,
			NotFirst
		};

		Kind kind;
		int a;
		int b;
	};

	// The Condition at a menu index, None out of range; and its label, as the cells and the menu show it.
	ConditionInfo condition(int _index);
	const char* conditionLabel(int _index);
	// The menu index of the Condition a label names, -1 for none.
	int conditionIndex(const char* _label);

	// The draws, from seeds rather than a generator's sequence, so that a Pass, a Track and a step always draw the
	// same, whatever is edited elsewhere: a Pass's seed from the transport start's, a step's draw from the Pass's.
	uint64_t passSeed(uint64_t _startSeed, uint32_t _pass);
	int draw(uint64_t _passSeed, int _track, int _step);	// 0-99

	// What PRE and NEI read: each Track's last result of a Condition evaluated on it (PRE, NEI and steps without a
	// Condition leave it), -1 before the first.
	struct ConditionMemory
	{
		std::array<int8_t, TrackCount> last = []
		{
			std::array<int8_t, TrackCount> none{};
			none.fill(-1);
			return none;
		}();
	};

	struct Draw
	{
		uint32_t pass;		// from 1
		uint64_t passSeed;
		bool fill;
	};

	// Whether the step's Hit plays this time; updates the Track's memory as the rules say. Tracks are evaluated from 1
	// to 16 at each step, so NEI reads Track N−1's result at the same step; Track 1 has no neighbour: NEI never plays,
	// !NEI always.
	bool evaluate(int _conditionIndex, int _track, int _step, const Draw& _draw, ConditionMemory& _memory);
}
