// Conditions as the ticket "Sémantique du Pattern" settled them: the 65 of the menu with their labels, A:B counted in
// Passes, X% drawn without state from the Pass's seed (about its share, the same every time for the same Pass, Track
// and step), PRE reading the Track's last Condition and NEI Track N−1's, neither updating it, nor a step without a
// Condition; FILL and 1ST.

#include "mdseqCondition.h"
#include "mdseqTestSupport.h"

#include <cstdio>
#include <cstring>

namespace
{
	using namespace mdseq;

	void menu()
	{
		CHECK(std::strcmp(conditionLabel(0), "None") == 0);
		CHECK(std::strcmp(conditionLabel(1), "1%") == 0);
		CHECK(std::strcmp(conditionLabel(11), "50%") == 0);
		CHECK(std::strcmp(conditionLabel(21), "99%") == 0);
		CHECK(std::strcmp(conditionLabel(22), "1:2") == 0);
		CHECK(std::strcmp(conditionLabel(56), "8:8") == 0);
		CHECK(std::strcmp(conditionLabel(57), "FILL") == 0);
		CHECK(std::strcmp(conditionLabel(64), "!1ST") == 0);
		CHECK(std::strcmp(conditionLabel(65), "") == 0);
		CHECK(condition(conditionIndex("3:4")).kind == ConditionInfo::Kind::Ratio);
		CHECK(condition(conditionIndex("3:4")).a == 3 && condition(conditionIndex("3:4")).b == 4);
		CHECK(condition(-1).kind == ConditionInfo::Kind::None);
	}

	bool plays(const char* _label, const uint32_t _pass, const bool _fill = false)
	{
		ConditionMemory memory;
		return evaluate(conditionIndex(_label), 0, 0, {_pass, passSeed(7, _pass), _fill}, memory);
	}

	void passCounts()
	{
		for(uint32_t pass = 1; pass <= 8; ++pass)
		{
			CHECK(plays("1:2", pass) == (pass % 2 == 1));
			CHECK(plays("2:2", pass) == (pass % 2 == 0));
			CHECK(plays("3:4", pass) == (pass == 3 || pass == 7));
			CHECK(plays("1ST", pass) == (pass == 1));
			CHECK(plays("!1ST", pass) == (pass != 1));
		}
		CHECK(plays("FILL", 1, true) && !plays("FILL", 1, false));
		CHECK(!plays("!FILL", 1, true) && plays("!FILL", 1, false));
		CHECK(plays("None", 5));
	}

	void chances()
	{
		// About its share over many Passes and steps, and the same draw for the same Pass, Track and step.
		const int half = conditionIndex("50%");
		const int rare = conditionIndex("1%");
		int played = 0;
		int rarePlayed = 0;
		for(uint32_t pass = 1; pass <= 625; ++pass)
		{
			for(int step = 0; step < 16; ++step)
			{
				ConditionMemory memory;
				const Draw d{pass, passSeed(1234, pass), false};
				played += evaluate(half, 4, step, d, memory) ? 1 : 0;
				rarePlayed += evaluate(rare, 4, step, d, memory) ? 1 : 0;
			}
		}
		CHECK(played > 4700 && played < 5300);	// 10 000 draws, 50 %
		CHECK(rarePlayed > 50 && rarePlayed < 160);	// 1 %
		CHECK(draw(passSeed(1234, 9), 3, 5) == draw(passSeed(1234, 9), 3, 5));

		// Another start seed draws otherwise.
		int differ = 0;
		for(int step = 0; step < 64; ++step)
			differ += draw(passSeed(1, 1), 0, step) != draw(passSeed(2, 1), 0, step) ? 1 : 0;
		CHECK(differ > 40);
	}

	void previousAndNeighbour()
	{
		const Draw d{1, passSeed(3, 1), true};
		ConditionMemory memory;

		// Nothing evaluated yet: PRE does not play, !PRE does; Track 1 has no neighbour.
		CHECK(!evaluate(conditionIndex("PRE"), 2, 0, d, memory));
		CHECK(evaluate(conditionIndex("!PRE"), 2, 0, d, memory));
		CHECK(!evaluate(conditionIndex("NEI"), 0, 0, d, memory));
		CHECK(evaluate(conditionIndex("!NEI"), 0, 0, d, memory));

		// FILL plays (held) on Track 3: PRE on Track 3 and NEI on Track 4 follow it.
		CHECK(evaluate(conditionIndex("FILL"), 2, 1, d, memory));
		CHECK(evaluate(conditionIndex("PRE"), 2, 2, d, memory));
		CHECK(!evaluate(conditionIndex("!PRE"), 2, 3, d, memory));
		CHECK(evaluate(conditionIndex("NEI"), 3, 1, d, memory));

		// !FILL does not play: it becomes what PRE reads. PRE, NEI and steps without a Condition leave it.
		CHECK(!evaluate(conditionIndex("!FILL"), 2, 4, d, memory));
		CHECK(evaluate(conditionIndex("None"), 2, 5, d, memory));
		CHECK(evaluate(conditionIndex("!PRE"), 2, 6, d, memory));
		CHECK(!evaluate(conditionIndex("PRE"), 2, 7, d, memory));
		CHECK(!evaluate(conditionIndex("NEI"), 3, 2, d, memory));
		CHECK(memory.last[3] == -1);	// Track 4 evaluated only NEI
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	menu();
	passCounts();
	chances();
	previousAndNeighbour();
	return mdseqTest::finish("mdseqConditionTest");
}
