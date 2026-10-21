// Printing: N Passes of a Pattern as a clip per Track, in beats. Each Hit and its Locks are notes a step (or a
// ratchet's share) long; an early Hit of the first step lands at the clip's end and a late one stops there; the same
// start seed draws the same Conditions, so a print replays the Passes heard; the Fill decides FILL steps; the scale
// sets the length of a step.

#include "mdseqCondition.h"
#include "mdseqPrint.h"
#include "mdseqTestSupport.h"

#include <cmath>
#include <cstdio>

namespace
{
	using namespace mdseq;

	Step hit(const int _velocity = DefaultVelocity)
	{
		Step s;
		s.hit = true;
		s.velocity = static_cast<uint8_t>(_velocity);
		return s;
	}

	bool near(const double _a, const double _b)
	{
		return std::fabs(_a - _b) < 1e-9;
	}

	void hitsAndLocks()
	{
		Pattern p;
		p.steps[0][0] = hit(110);
		p.steps[2][4] = hit(90);
		p.steps[2][4].locks[17] = 0;	// VOL
		const Printed printed = printPattern(p, 2, 1, false);

		// 16 steps of a 16th: 4 beats a Pass, 8 for two.
		CHECK(near(printed.length, 8.0));
		CHECK(printed.trackCount() == 2);
		CHECK((printed.tracks[0] == std::vector<PrintedNote>{{36, 0.0, 0.25, 110}, {36, 4.0, 0.25, 110}}));
		// Track 3's Hit then its Lock, on its own clip, at step 5 of each Pass.
		CHECK((printed.tracks[2] == std::vector<PrintedNote>{{38, 1.0, 0.25, 90}, {64 + 17, 1.0, 0.25, 1},
			{38, 5.0, 0.25, 90}, {64 + 17, 5.0, 0.25, 1}}));
	}

	void ratchetsAndEdges()
	{
		Pattern p;
		p.length = 4;
		p.steps[0][1] = hit(100);
		p.steps[0][1].ratchets = 2;
		p.steps[0][1].curve = -50;
		// Early on the first step: at the clip's end; late on the last: cut at the end.
		p.steps[1][0] = hit();
		p.steps[1][0].micro = -12;
		p.steps[2][3] = hit();
		p.steps[2][3].micro = 12;
		const Printed printed = printPattern(p, 1, 1, false);

		CHECK(near(printed.length, 1.0));
		CHECK((printed.tracks[0] == std::vector<PrintedNote>{{36, 0.25, 0.125, 100}, {36, 0.375, 0.125, 50}}));
		CHECK(printed.tracks[1].size() == 1 && near(printed.tracks[1][0].start, 1.0 - 0.125) &&
			near(printed.tracks[1][0].duration, 0.125));
		CHECK(printed.tracks[2].size() == 1 && near(printed.tracks[2][0].start, 0.75 + 0.125) &&
			near(printed.tracks[2][0].duration, 0.125));
	}

	// Swing moves the steps of its row in the clip as the Sequencer plays them; the last step, late past the clip's
	// end by swing and micro-timing, starts the clip, as each repeat plays it.
	void swing()
	{
		Pattern p;
		p.length = 4;
		p.swing = 80;
		p.steps[0][1] = hit();	// 0.6 of a step late: 0.25 + 0.15 beats
		p.steps[1][2] = hit();	// on the beat: step 3 is not in the row
		p.steps[2][3] = hit();
		p.steps[2][3].micro = MaxMicro;	// 0.6 + 23/24 of a step late: 1.15625 + 0.1, past 1 beat
		const Printed printed = printPattern(p, 1, 1, false);

		CHECK(near(printed.length, 1.0));
		CHECK(printed.tracks[0].size() == 1 && near(printed.tracks[0][0].start, 0.4) &&
			near(printed.tracks[0][0].duration, 0.25));
		CHECK(printed.tracks[1].size() == 1 && near(printed.tracks[1][0].start, 0.5));
		const double late = 0.75 + 0.25 * (0.6 + 23.0 / 24.0);
		CHECK(printed.tracks[2].size() == 1 && near(printed.tracks[2][0].start, late - 1.0) &&
			near(printed.tracks[2][0].duration, 0.25));
	}

	// Accent (issue 26): note 88 after the Hit of a step in the Accent row, its velocity the amount on the Locks' scale,
	// as long as the Hit in the clip: printed, it keeps a length, which a clip's note needs.
	void accent()
	{
		Pattern p;
		p.length = 4;
		p.accent = 100;
		p.accentSteps = 1ull << 1;
		p.steps[0][1] = hit();
		p.steps[0][2] = hit();
		const Printed printed = printPattern(p, 1, 1, false);
		const std::vector<PrintedNote>& notes = printed.tracks[0];
		CHECK(notes.size() == 3);
		CHECK(notes[0].pitch == FirstHitNote && near(notes[0].start, 0.25) && near(notes[0].duration, 0.25));
		CHECK(notes[1].pitch == AccentNote && notes[1].velocity == 100 && near(notes[1].start, 0.25) &&
			near(notes[1].duration, 0.25));
		CHECK(notes[2].pitch == FirstHitNote && near(notes[2].start, 0.5));
	}

	// Slide (issue 26): notes 89 (the clocks to the next Hit) and 90-113 (its Locks) after the Hit of a step in the
	// Slide row, as long as the Hit in the clip, so that the clip plays the Slide again.
	void slide()
	{
		Pattern p;
		p.length = 4;
		p.slideSteps = 1ull << 1;
		p.steps[0][1] = hit();
		p.steps[0][1].locks[3] = 20;
		p.steps[0][3] = hit();
		p.steps[0][3].locks[3] = 90;
		const Printed printed = printPattern(p, 1, 1, false);
		const std::vector<PrintedNote>& notes = printed.tracks[0];
		CHECK(notes.size() == 6);	// the next Hit's Lock last
		CHECK(notes[1].pitch == FirstLockNote + 3 && notes[1].velocity == 21);
		CHECK(notes[2].pitch == SlideNote && notes[2].velocity == 12 && near(notes[2].start, 0.25) &&
			near(notes[2].duration, 0.25));
		CHECK(notes[3].pitch == FirstSlideTargetNote + 3 && notes[3].velocity == 90 && near(notes[3].duration, 0.25));
		CHECK(notes[4].pitch == FirstHitNote && near(notes[4].start, 0.75));
	}

	void conditionsAndFill()
	{
		// A:B over the Passes printed, counted from 1.
		Pattern p;
		p.steps[0][0] = hit();
		p.steps[0][0].condition = static_cast<uint8_t>(conditionIndex("1:2"));
		Printed printed = printPattern(p, 4, 1, false);
		CHECK(printed.tracks[0].size() == 2 && near(printed.tracks[0][0].start, 0.0) &&
			near(printed.tracks[0][1].start, 8.0));

		// The same seed draws the same chances; another seed, others.
		Pattern chances;
		for(int s = 0; s < 16; ++s)
		{
			chances.steps[0][s] = hit();
			chances.steps[0][s].condition = static_cast<uint8_t>(conditionIndex("50%"));
		}
		const Printed a = printPattern(chances, 8, 42, false);
		CHECK(a.tracks[0] == printPattern(chances, 8, 42, false).tracks[0]);
		CHECK(a.tracks[0] != printPattern(chances, 8, 43, false).tracks[0]);
		CHECK(a.tracks[0].size() > 20 && a.tracks[0].size() < 108);

		// FILL steps play with the Fill held only.
		Pattern fill;
		fill.steps[4][0] = hit();
		fill.steps[4][0].condition = static_cast<uint8_t>(conditionIndex("FILL"));
		CHECK(printPattern(fill, 1, 1, false).trackCount() == 0);
		CHECK(printPattern(fill, 1, 1, true).tracks[4].size() == 1);
	}

	void scaleAndEmpty()
	{
		Pattern p;
		p.scale = 6;	// 2x: a step is a 32nd
		p.length = 8;
		p.steps[0][2] = hit(100);
		const Printed printed = printPattern(p, 2, 1, false);
		CHECK(near(printed.length, 2.0));
		CHECK((printed.tracks[0] == std::vector<PrintedNote>{{36, 0.25, 0.125, 100}, {36, 1.25, 0.125, 100}}));

		CHECK(printPattern(Pattern{}, 4, 1, false).trackCount() == 0);
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	hitsAndLocks();
	ratchetsAndEdges();
	swing();
	accent();
	slide();
	conditionsAndFill();
	scaleAndEmpty();
	return mdseqTest::finish("mdseqPrintTest");
}
