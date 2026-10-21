// Capturing a performance (issue 23): the journal keeps the notes sent at their take time (from the bar line before
// the start, Live's jumps joined end to end), a ring that keeps the latest; a take becomes a clip per Track whose Locks
// last as their Hit; and a Pass played live and captured is the Pass printed from the same seed.

#include "mdseqCapture.h"
#include "mdseqCondition.h"
#include "mdseqSequencer.h"
#include "mdseqTestSupport.h"

#include <cmath>
#include <cstdio>
#include <memory>

namespace
{
	using namespace mdseq;

	bool near(const double _a, const double _b)
	{
		return std::fabs(_a - _b) < 1e-9;
	}

	MidiBytes on(const int _track, const int _key, const int _velocity = 100)
	{
		return {static_cast<uint8_t>(NoteOn | _track), static_cast<uint8_t>(_key), static_cast<uint8_t>(_velocity)};
	}

	MidiBytes off(const int _track, const int _key)
	{
		return {static_cast<uint8_t>(NoteOff | _track), static_cast<uint8_t>(_key), 0};
	}

	void journal()
	{
		Journal j(16);
		j.note(10.0, on(0, 36));	// stopped: not kept
		CHECK(j.take().entries.empty() && !j.hasNotes() && takeText(j.hasNotes(), j.length()) == "No take");

		// Started at 500, inside the first bar: take time counts from its bar line.
		j.start(500.0);
		j.note(500.0, on(0, 36));
		j.note(620.0, off(0, 36));
		// Live jumps from 1000 to 4000: what follows goes on from 1000.
		j.played(1000.0);
		j.jump(1000.0, 4000.0);
		j.note(4100.0, on(1, 37));
		j.stop(4200.0);
		j.note(4300.0, on(1, 38));	// after the stop: not kept
		Take t = j.take();
		CHECK(t.entries.size() == 3 && !t.cut);
		CHECK(near(t.entries[0].ticks, 500.0) && near(t.entries[1].ticks, 620.0) && near(t.entries[2].ticks, 1100.0));
		CHECK(near(t.length, 1200.0));
		CHECK(j.hasNotes() && takeText(j.hasNotes(), j.length()) == "Take 1 bar");

		// A new take forgets the last.
		j.start(1920.0);
		j.note(1920.0, on(2, 38));
		j.played(1920.0 + 2 * BarTicks + 10.0);
		t = j.take();
		CHECK(t.entries.size() == 1 && near(t.entries[0].ticks, 0.0));
		CHECK(takeText(j.hasNotes(), j.length()) == "Take 3 bars");	// a take still playing, to its last wake

		// The ring keeps the latest: its beginning cut.
		Journal small(4);
		small.start(0.0);
		for(int i = 0; i < 6; ++i)
			small.note(i * 10.0, on(0, 36 + i));
		t = small.take();
		CHECK(t.cut && t.entries.size() == 4 && t.entries[0].bytes[1] == 38 && t.entries[3].bytes[1] == 41);
	}

	void clips()
	{
		Take t;
		// Track 2: a Hit a step long with a Lock the device ended at once; a Hit still sounding at the end; a note sent
		// before the first bar line (an early Hit at the start).
		t.entries = {
			{-15.0, on(3, 39)},
			{0.0, on(1, 37, 90)}, {0.0, on(1, 64 + 17, 31)}, {0.0, off(1, 64 + 17)},
			{120.0, off(1, 37)},
			{240.0, on(1, 37, 80)},
			{105.0, off(3, 39)},
		};
		t.length = 300.0;
		const Printed p = takeClips(t);
		CHECK(near(p.length, 4.0));	// one whole bar
		CHECK(p.trackCount() == 2);
		const std::vector<PrintedNote>& two = p.tracks[1];
		CHECK(two.size() == 3);
		CHECK(two[0] == (PrintedNote{37, 0.0, 0.25, 90}));
		CHECK(two[1] == (PrintedNote{64 + 17, 0.0, 0.25, 31}));	// as long as its Hit
		CHECK(two[2].pitch == 37 && near(two[2].start, 0.5) && near(two[2].duration, 60.0 / TicksPerQuarter));
		CHECK(p.tracks[3].size() == 1 && near(p.tracks[3][0].start, 0.0) && near(p.tracks[3][0].duration, 105.0 / 480.0));
	}

	// The Sequencer tells the journal what the transport did: a start, a jump from the last look, a stop.
	void wakes()
	{
		Sequencer seq(1);
		CHECK(seq.wake(Transport{0.0, true, 0.0, 1.0}).started);
		const Wake on = seq.wake(Transport{100.0, true, 100.0, 1.0});
		CHECK(!on.started && !on.jumped && !on.stopped);
		const Wake jump = seq.wake(Transport{1000.0, true, 101.0, 1.0});
		CHECK(jump.jumped && !jump.started && near(jump.jumpedFrom, 100.0));
		CHECK(!seq.wake(Transport{1001.0, true, 102.0, 1.0}).jumped);
		const Wake stop = seq.wake(Transport{1002.0, false, 103.0, 1.0});
		CHECK(stop.stopped && !seq.wake(Transport{1002.0, false, 104.0, 1.0}).stopped);
	}

	// A Pass played live, journaled as the device does, is the Pass printed from the start seed it drew.
	void capturedIsPrinted()
	{
		Pattern pattern;
		for(int s = 0; s < pattern.length; s += 2)
		{
			Step& step = pattern.steps[0][s];
			step.hit = true;
			step.velocity = static_cast<uint8_t>(60 + s);
			step.locks[3] = static_cast<uint8_t>(10 + s);
		}
		Step& ratchet = pattern.steps[4][3];
		ratchet.hit = true;
		ratchet.ratchets = 3;
		ratchet.micro = 5;
		Step& chance = pattern.steps[9][6];
		chance.hit = true;
		chance.condition = static_cast<uint8_t>(conditionIndex("50%"));
		pattern.accentSteps = 1ull << 4;
		pattern.accent = 40;
		pattern.slideSteps = 1ull << 2;
		pattern.swing = 60;

		auto seq = std::make_unique<Sequencer>(7);
		seq->load(0, pattern);
		Journal j;
		const auto look = [&](const double _ticks, const bool _playing)
		{
			const Wake w = seq->wake(Transport{_ticks, _playing, _ticks, 1.0});
			if(w.started)
				j.start(_ticks);
			if(w.jumped)
				j.jump(w.jumpedFrom, _ticks);
			for(const MidiEvent* e = w.begin; e != w.end; ++e)
				j.note(e->tick, {e->bytes[0], e->bytes[1], e->bytes[2]});
			if(_playing)
				j.played(_ticks);
			if(w.stopped)
				j.stop(_ticks);
			return w;
		};
		Wake w = look(0.0, true);
		const uint64_t seed = seq->startSeed();
		while(w.next < BarTicks)
			w = look(w.next, true);
		look(BarTicks, false);

		const Printed captured = takeClips(j.take());
		const Printed printed = printPattern(pattern, 1, seed, false);
		CHECK(near(captured.length, printed.length));
		for(int t = 0; t < TrackCount; ++t)
		{
			const auto& a = captured.tracks[static_cast<size_t>(t)];
			const auto& b = printed.tracks[static_cast<size_t>(t)];
			bool same = a.size() == b.size();
			for(size_t i = 0; same && i < a.size(); ++i)
				same = a[i].pitch == b[i].pitch && near(a[i].start, b[i].start) && near(a[i].duration, b[i].duration) &&
					a[i].velocity == b[i].velocity;
			if(!same)
				std::printf("  Track %d: %zu notes captured, %zu printed\n", t + 1, a.size(), b.size());
			CHECK(same);
		}
		CHECK(captured.tracks[0].size() == 8 + 8 + 1 + 2);	// 8 Hits, their Locks, an Accent, a Slide and its target
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	journal();
	clips();
	wakes();
	capturedIsPrinted();
	return mdseqTest::finish("mdseqCaptureTest");
}
