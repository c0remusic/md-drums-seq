// The Sequencer driven in virtual time, as the external drives it from Live's transport: Hits planned one step ahead
// and sent at their tick, note-offs a step later and before a Hit of the same tick, Tracks in order, a late wake
// catching up with the planned ticks, Stop and jumps ending the notes sounding, Mutes cut at once without leaving a
// note on, an edit of the step planned and not begun heard on it, Passes counted, ratchets sharing their step with
// their Locks and their velocity curve, micro-timing moving a step's Hits (and the edits of a step some Tracks have
// begun to sound), and no allocation while playing, nor a wake longer than a millisecond, with every Track on 8
// ratchets of 24 Locks. Live's transport as the external reads it, raw: a start and a stop told from the looks, a
// loop brace longer than the slack taken for a jump at every scale and a tempo change not, the grace of a start seen
// late, and a jump's slack, a 64th at every scale. The Pass kept across a new scale and a seek inside the step played,
// and a Pattern queued that follows at the end of the Pass Live plays, though Live jumps or the scale changes during
// its last step. The Playhead a wake hands the main thread, packed into one word.

#include "mdseqSequencer.h"
#include "mdseqTestSupport.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <new>
#include <set>
#include <string>
#include <vector>

namespace
{
	thread_local bool g_countAllocations = false;
	thread_local size_t g_allocations = 0;
}

void* operator new(const std::size_t _size)
{
	if(g_countAllocations)
		++g_allocations;
	if(void* const result = std::malloc(_size ? _size : 1))
		return result;
	throw std::bad_alloc();
}

void* operator new[](const std::size_t _size) { return ::operator new(_size); }
void operator delete(void* _p) noexcept { std::free(_p); }
void operator delete[](void* _p) noexcept { std::free(_p); }
void operator delete(void* _p, std::size_t) noexcept { std::free(_p); }
void operator delete[](void* _p, std::size_t) noexcept { std::free(_p); }

namespace
{
	using namespace mdseq;

	struct Event
	{
		double tick;
		int status;
		int data1;
		int data2;

		bool operator==(const Event& _other) const
		{
			return tick == _other.tick && status == _other.status && data1 == _other.data1 && data2 == _other.data2;
		}
	};

	using Events = std::vector<Event>;

	// Track 1-16 as the user counts them.
	Event on(const double _tick, const int _track, const int _velocity = DefaultVelocity)
	{
		return {_tick, 0x90 | (_track - 1), FirstHitNote + _track - 1, _velocity};
	}

	Event off(const double _tick, const int _track)
	{
		return {_tick, 0x80 | (_track - 1), FirstHitNote + _track - 1, 0};
	}

	void append(Events& _events, const Wake& _wake)
	{
		for(const MidiEvent* e = _wake.begin; e != _wake.end; ++e)
			_events.push_back({e->tick, e->bytes[0], e->bytes[1], e->bytes[2]});
	}

	void print(const char* _title, const Events& _events)
	{
		std::printf("  %s:\n", _title);
		for(const Event& e : _events)
			std::printf("    %8.2f  %02X %d %d\n", e.tick, e.status, e.data1, e.data2);
	}

	void checkEvents(const Events& _got, const Events& _expected, const int _line)
	{
		if(_got == _expected)
			return;
		std::printf("FAIL line %d: events differ\n", _line);
		print("got", _got);
		print("expected", _expected);
		++mdseqTest::g_failures;
	}

#define CHECK_EVENTS(_got, _expected) checkEvents((_got), (_expected), __LINE__)

	// A Sequencer looking at Live's transport in virtual time, at 125 BPM: a millisecond is worth a tick (a quarter,
	// 480 ticks, lasts 480 ms), so that the clock keeps pace with the position while Live plays on and stands while it
	// jumps. It looks where a test says, not every millisecond.
	class Played : public Sequencer
	{
	public:
		using Sequencer::Sequencer;

		// Live plays on to _ticks, or starts there, and the external looks.
		Wake at(const double _ticks)
		{
			move(_ticks);
			m_live.playing = true;
			return wake(m_live);
		}

		// Live's position moves to _ticks in no time: a seek, or the loop brace.
		Wake jumpTo(const double _ticks)
		{
			m_live.ticks = _ticks;
			return wake(m_live);
		}

		// Live plays on to _ticks and stops there.
		Wake stopAt(const double _ticks)
		{
			move(_ticks);
			m_live.playing = false;
			return wake(m_live);
		}

	private:
		void move(const double _ticks)
		{
			if(m_live.playing)
				m_live.ms += (_ticks - m_live.ticks) / m_live.ticksPerMs;
			m_live.ticks = _ticks;
		}

		Transport m_live{0.0, false, 0.0, 1.0};
	};

	// Looks exactly when the Sequencer asks, from _from (a start, or playing on) until _until.
	Events play(Played& _seq, const double _from, const double _until)
	{
		Events events;
		Wake w = _seq.at(_from);
		append(events, w);
		while(w.next < _until)
		{
			w = _seq.at(w.next);
			append(events, w);
		}
		return events;
	}

	void setHit(Sequencer& _seq, const int _track, const int _step, const int _velocity = DefaultVelocity)
	{
		_seq.input({Input::Kind::SetHit, _track - 1, _step, 1});
		_seq.input({Input::Kind::SetVelocity, _track - 1, _step, _velocity});
	}

	void silentPattern()
	{
		Played seq;
		const Wake w = seq.at(0.0);
		CHECK(w.begin == w.end);
		CHECK(w.next == TicksPerStep);	// steps 0 and 1 planned at the start, step 2 at step 1
		CHECK(w.step == 0);
		CHECK(w.pass == 1);
		CHECK(play(seq, 120.0, 4000.0).empty());
	}

	void oneHit()
	{
		Played seq;
		setHit(seq, 1, 0);
		CHECK_EVENTS(play(seq, 0.0, 2100.0), (Events{on(0, 1), off(120, 1), on(1920, 1), off(2040, 1)}));
	}

	void sameNoteNextStep()
	{
		Played seq;
		setHit(seq, 3, 0, 90);
		setHit(seq, 3, 1, 110);
		CHECK_EVENTS(play(seq, 0.0, 300.0), (Events{on(0, 3, 90), off(120, 3), on(120, 3, 110), off(240, 3)}));
	}

	void tracksInOrder()
	{
		Played seq;
		setHit(seq, 16, 0);
		setHit(seq, 1, 0);
		setHit(seq, 2, 1);
		CHECK_EVENTS(play(seq, 0.0, 300.0),
			(Events{on(0, 1), on(0, 16), off(120, 1), off(120, 16), on(120, 2), off(240, 2)}));
	}

	void lateWake()
	{
		Played seq;
		for(int s = 0; s < 16; ++s)
			setHit(seq, 1, s);

		Events events;
		append(events, seq.at(0.0));
		const Wake late = seq.at(500.0);
		append(events, late);
		CHECK_EVENTS(events, (Events{on(0, 1), off(120, 1), on(120, 1), off(240, 1), on(240, 1), off(360, 1),
			on(360, 1), off(480, 1), on(480, 1)}));
		CHECK(late.next == 600.0);
		CHECK(late.step == 4);
	}

	void stopAndStartAgain()
	{
		Played seq;
		setHit(seq, 1, 0);
		CHECK_EVENTS(play(seq, 0.0, 60.0), (Events{on(0, 1)}));

		Events events;
		const Wake stopped = seq.stopAt(60.0);
		append(events, stopped);
		CHECK_EVENTS(events, (Events{off(60, 1)}));
		CHECK(stopped.next == Never);
		CHECK(stopped.step == -1);
		CHECK(stopped.pass == 0);

		// Nothing left planned: the note-off of 120 never comes.
		const Wake still = seq.stopAt(200.0);
		CHECK(still.begin == still.end);
		CHECK_EVENTS(play(seq, 0.0, 130.0), (Events{on(0, 1), off(120, 1)}));
	}

	void jumps()
	{
		Played seq;
		setHit(seq, 1, 0);
		setHit(seq, 1, 8);
		CHECK_EVENTS(play(seq, 0.0, 100.0), (Events{on(0, 1)}));

		// Forward into the same Pass: the note sounding ends, step 8 plays from where the transport landed.
		Events events;
		Wake w = seq.jumpTo(960.0);
		append(events, w);
		CHECK_EVENTS(events, (Events{off(960, 1), on(960, 1)}));
		CHECK(w.step == 8);
		CHECK(w.pass == 1);

		// Back to the start: a new Pass.
		events.clear();
		w = seq.jumpTo(0.0);
		append(events, w);
		CHECK_EVENTS(events, (Events{off(0, 1), on(0, 1)}));
		CHECK(w.pass == 2);
	}

	// The tests below give the Sequencer Live's transport raw, as the external reads it: the ticks, playing or not,
	// the time of the look in ms, and what a ms is worth in ticks (1 at 125 BPM).

	void startAndStop()
	{
		// Stopped, nothing plays, wherever the position.
		Sequencer seq;
		setHit(seq, 1, 4);
		Wake w = seq.wake({0.0, false, 0.0, 1.0});
		CHECK(w.begin == w.end && w.next == Never && w.step == -1);
		w = seq.wake({480.0, false, 30.0, 1.0});
		CHECK(w.begin == w.end && w.next == Never);

		// The first look that sees it playing is a start, there, whenever the look before.
		Events events;
		w = seq.wake({480.0, true, 31.0, 1.0});
		append(events, w);
		CHECK_EVENTS(events, (Events{on(480, 1)}));
		CHECK(w.step == 4 && w.pass == 1 && w.next == 600.0);
		const uint64_t firstSeed = seq.startSeed();

		// The first that sees it stopped is a stop: the note sounding ends there, and nothing more is planned.
		events.clear();
		w = seq.wake({530.0, false, 81.0, 1.0});
		append(events, w);
		CHECK_EVENTS(events, (Events{off(530, 1)}));
		CHECK(w.next == Never && w.step == -1 && w.pass == 0);

		// Moved while stopped, then started: a start, never a jump back, whose Passes count from 1 again, with a seed
		// of its own.
		seq.wake({0.0, false, 500.0, 1.0});
		events.clear();
		w = seq.wake({0.0, true, 501.0, 1.0});
		append(events, w);
		CHECK(events.empty());
		CHECK(w.step == 0 && w.pass == 1);
		CHECK(seq.startSeed() != firstSeed);
	}

	void loopBack()
	{
		// Live loops the first bar, the external looking every millisecond: at 1920 the position goes back to 0 while
		// the clock runs on. Taken for a jump: step 15's note ends there, and step 0 plays again, on a new Pass. Played
		// on, the Sequencer would wait for Live to reach 1920, which it never does.
		Sequencer seq;
		setHit(seq, 1, 0);
		setHit(seq, 1, 15);
		Events events;
		for(int ms = 0; ms <= 2000; ++ms)
		{
			const double now = static_cast<double>(ms);
			const Wake w = seq.wake({std::fmod(now, 1920.0), true, now, 1.0});
			append(events, w);
			if(ms == 1920)
				CHECK(w.step == 0 && w.pass == 2);
		}
		CHECK_EVENTS(events, (Events{on(0, 1), off(120, 1), on(1800, 1), off(0, 1), on(0, 1)}));
	}

	void loopBackAtSlowScales()
	{
		// A loop brace no longer than half a step, looked at every millisecond, down to a 32nd at 1x and slower: each
		// wrap is a jump too, and step 0's Hit plays again on a new Pass. Played on, step 0's note would never end and
		// nothing would play again, the Sequencer waiting for Live to reach step 1.
		const auto looped = [](const int _scale, const double _loop)
		{
			Sequencer seq;
			seq.input({Input::Kind::SetScale, 0, 0, _scale});
			setHit(seq, 1, 0);
			Events events;
			for(int ms = 0; ms <= static_cast<int>(2 * _loop) + 10; ++ms)
			{
				const double now = static_cast<double>(ms);
				const Wake w = seq.wake({std::fmod(now, _loop), true, now, 1.0});
				append(events, w);
				if(ms == static_cast<int>(_loop))
					CHECK(w.step == 0 && w.pass == 2);
			}
			return events;
		};
		const Events twoWraps{on(0, 1), off(0, 1), on(0, 1), off(0, 1), on(0, 1)};
		CHECK_EVENTS(looped(0, 480.0), twoWraps);	// 1/8x, a step of 960 ticks: a beat
		CHECK_EVENTS(looped(0, 240.0), twoWraps);	// an 8th
		CHECK_EVENTS(looped(1, 240.0), twoWraps);	// 1/4x, an 8th
		CHECK_EVENTS(looped(2, 120.0), twoWraps);	// 1/2x, a 16th
		CHECK_EVENTS(looped(3, 80.0), twoWraps);	// 3/4x, a 24th
		CHECK_EVENTS(looped(4, 60.0), twoWraps);	// 1x, a 32nd
		CHECK_EVENTS(looped(3, 60.0), twoWraps);	// 3/4x, a 32nd
		CHECK_EVENTS(looped(0, 60.0), twoWraps);	// 1/8x, a 32nd
	}

	void tempoChange()
	{
		// Looks 20 ms apart (the external looks every millisecond), and Live triples its tempo (125 to 375 BPM) half
		// way between those at 200 and 220 ms: that look finds the position 40 ticks on, where its tempo makes 60, and
		// every later one 60 ticks on. Each look measures the time since the last at its own tempo, so nothing is taken
		// for a jump: no note ends early, every Hit keeps its tick. At the start's tempo, every look after would be 40
		// ticks off, past the slack.
		Sequencer seq;
		for(int s = 0; s < DefaultLength; ++s)
			setHit(seq, 1, s);
		Events events;
		for(int ms = 0; ms <= 400; ms += 20)
		{
			const bool before = ms <= 200;
			const double ticks = before ? ms : 240.0 + (ms - 220) * 3.0;
			append(events, seq.wake({ticks, true, static_cast<double>(ms), before ? 1.0 : 3.0}));
		}
		CHECK_EVENTS(events, (Events{on(0, 1), off(120, 1), on(120, 1), off(240, 1), on(240, 1), off(360, 1),
			on(360, 1), off(480, 1), on(480, 1), off(600, 1), on(600, 1), off(720, 1), on(720, 1)}));
	}

	void graceOfALateLook()
	{
		// Live started at tick 0, but the first look sees it 15 ms later, 15 ticks on: within the grace of 20 ms, step
		// 0's Hit still goes out, late.
		Sequencer late;
		setHit(late, 1, 0);
		Events events;
		append(events, late.wake({15.0, true, 15.0, 1.0}));
		CHECK_EVENTS(events, (Events{on(0, 1)}));
		CHECK(GraceMs == 20.0);

		// Seen 25 ms later, past the grace: the Hit before the position is dropped, and so is its note-off.
		Played later;
		setHit(later, 1, 0);
		CHECK_EVENTS(play(later, 25.0, 300.0), Events{});

		// The grace is a time: at 250 BPM a millisecond is worth 2 ticks, and 25 ticks on are 12.5 ms, within it.
		Sequencer faster;
		setHit(faster, 1, 0);
		events.clear();
		append(events, faster.wake({25.0, true, 12.5, 2.0}));
		CHECK_EVENTS(events, (Events{on(0, 1)}));
	}

	void jumpSlack()
	{
		// Hits on every step at _scale, Live started at 0 and seen at _from, then a millisecond later at _to.
		const auto moved = [](const int _scale, const double _from, const double _to)
		{
			Sequencer seq;
			seq.input({Input::Kind::SetScale, 0, 0, _scale});
			for(int s = 0; s < DefaultLength; ++s)
				setHit(seq, 1, s);
			seq.wake({0.0, true, 0.0, 1.0});
			seq.wake({_from, true, _from, 1.0});
			Events events;
			append(events, seq.wake({_to, true, _from + 1.0, 1.0}));
			return events;
		};

		// The slack is 30 ticks, a 64th, at every scale. At 1x Live found 29 ticks past where playing on would bring it
		// is still playing on, and the Hit passed goes out at once; 31 ticks past is a jump, which ends the note
		// sounding and plays the step reached, its Hit within the grace going out late.
		CHECK_EVENTS(moved(4, 100.0, 130.0), (Events{off(120, 1), on(120, 1)}));
		CHECK_EVENTS(moved(4, 100.0, 132.0), (Events{off(132, 1), on(120, 1)}));
		CHECK_EVENTS(moved(4, 100.0, 170.0), (Events{off(170, 1)}));

		// At 2x a step is 60 ticks, its half the slack. A seek from 100 to 150, 49 ticks past and 30 into step 2, is a
		// jump: step 1's note ends, and step 2's Hit, whose start Live skipped, is not played. Half a 16th of slack
		// took it for playing on, and sent that Hit 30 ticks late. 24 ticks past is still playing on.
		CHECK_EVENTS(moved(6, 100.0, 150.0), (Events{off(150, 1)}));
		CHECK_EVENTS(moved(6, 100.0, 125.0), (Events{off(120, 1), on(120, 1)}));

		// At 1/8x a step is 960 ticks, and the slack still 30: the looks' jitter does not grow with the step. 29 ticks
		// past plays on (step 0's note sounds on), 31 is a jump. A seek of 400 ticks across step 1's start at 960 is a
		// jump too: step 1's Hit, begun before the grace, is dropped rather than sent 340 ticks late.
		CHECK_EVENTS(moved(0, 100.0, 130.0), Events{});
		CHECK_EVENTS(moved(0, 100.0, 132.0), (Events{off(132, 1)}));
		CHECK_EVENTS(moved(0, 900.0, 1300.0), (Events{off(1300, 1)}));
	}

	void mutes()
	{
		Played seq;
		setHit(seq, 1, 0);
		setHit(seq, 2, 0);
		CHECK(seq.input({Input::Kind::SetMute, 0, 0, 1}));
		CHECK(!seq.input({Input::Kind::SetMute, 0, 0, 1}));
		CHECK_EVENTS(play(seq, 0.0, 60.0), (Events{on(0, 2)}));

		// Muted while sounding: its note-off still goes, so that nothing stays on.
		CHECK(seq.input({Input::Kind::SetMute, 1, 0, 1}));
		CHECK_EVENTS(play(seq, 60.0, 2000.0), (Events{off(120, 2)}));
	}

	void editsFromTheNextPlannedStep()
	{
		Played seq;
		CHECK(play(seq, 0.0, 60.0).empty());

		// At tick 60 step 1 is already planned, not begun: its Track is planned again, and the new Hit plays on this
		// Pass. Step 2 is not planned yet. Step 0 has begun: a Hit put there is not played late, but on the next Pass.
		setHit(seq, 1, 1);
		setHit(seq, 2, 2);
		setHit(seq, 3, 0);
		CHECK_EVENTS(play(seq, 60.0, 2100.0),
			(Events{on(120, 1), off(240, 1), on(240, 2), off(360, 2), on(1920, 3), off(2040, 3), on(2040, 1)}));
	}

	void hitTakenOffBeforeItsStep()
	{
		Played seq;
		setHit(seq, 1, 1);
		setHit(seq, 2, 1);
		CHECK(play(seq, 0.0, 60.0).empty());

		// Step 1 is planned with both Hits; Track 1's goes before it sounds, Track 2's stays.
		seq.input({Input::Kind::SetHit, 0, 1, 0});
		CHECK_EVENTS(play(seq, 60.0, 300.0), (Events{on(120, 2), off(240, 2)}));
	}

	void passes()
	{
		Played seq;
		seq.at(0.0);
		Wake w{};
		for(double t = 120.0; t < 1920.0; t += 120.0)
			w = seq.at(t);
		CHECK(w.step == 15);
		CHECK(w.pass == 1);
		w = seq.at(1920.0);
		CHECK(w.step == 0);
		CHECK(w.pass == 2);
	}

	void inputs()
	{
		Sequencer seq;
		CHECK(seq.input({Input::Kind::SetHit, 0, 0, 1}));
		CHECK(!seq.input({Input::Kind::SetHit, 0, 0, 1}));
		CHECK(!seq.input({Input::Kind::SetHit, 16, 0, 1}));
		CHECK(!seq.input({Input::Kind::SetHit, 0, 64, 1}));
		CHECK(seq.input({Input::Kind::SetVelocity, 0, 0, 90}));
		CHECK(seq.input({Input::Kind::SetVelocity, 0, 0, 200}));
		CHECK(seq.pattern().steps[0][0].velocity == 127);
		CHECK(seq.input({Input::Kind::SetVelocity, 0, 0, 0}));
		CHECK(seq.pattern().steps[0][0].velocity == 1);
	}

	Event lockOn(const double _tick, const int _track, const int _param, const int _velocity)
	{
		return {_tick, 0x90 | (_track - 1), FirstLockNote + _param, _velocity};
	}

	Event lockOff(const double _tick, const int _track, const int _param)
	{
		return {_tick, 0x80 | (_track - 1), FirstLockNote + _param, 0};
	}

	void setLock(Sequencer& _seq, const int _track, const int _step, const int _param, const int _value)
	{
		Input in{Input::Kind::SetLock, _track - 1, _step, _value};
		in.param = _param;
		_seq.input(in);
	}

	constexpr int Syn1 = 0;
	constexpr int Vol = 17;

	void locks()
	{
		// MD Drums' velocity scale (ADR 0003): 0-62 travel as 1-63, 64-127 as themselves.
		CHECK(lockVelocity(0) == 1);
		CHECK(lockVelocity(62) == 63);
		CHECK(lockVelocity(64) == 64);
		CHECK(lockVelocity(127) == 127);

		Played seq;
		setLock(seq, 3, 0, Vol, 20);	// no Hit there: refused
		CHECK(seq.pattern().steps[2][0].locks[Vol] == NoLock);
		setHit(seq, 3, 0);
		setLock(seq, 3, 0, Vol, 0);
		setLock(seq, 3, 0, Syn1, 100);
		setHit(seq, 3, 1);
		setLock(seq, 3, 1, Vol, 62);

		// On the Track's channel, the Hit a step long then its Locks, each ended at once: MD Drums reads a Lock at its
		// note-on, and Live holds at most 128 notes from a device (issue 24). Note-offs first at the same tick. Live
		// brings a device's notes to channel 1, where MD Drums binds a Lock to the last Hit before it (ADR 0003 as
		// amended).
		CHECK_EVENTS(play(seq, 0.0, 300.0), (Events{
			on(0, 3), lockOn(0, 3, Syn1, 100), lockOff(0, 3, Syn1), lockOn(0, 3, Vol, 1), lockOff(0, 3, Vol),
			off(120, 3), on(120, 3), lockOn(120, 3, Vol, 63), lockOff(120, 3, Vol),
			off(240, 3)}));

		// Two Tracks at one tick: each Hit is followed by its own Locks, never by the other Track's.
		Played two;
		setHit(two, 5, 0);
		setLock(two, 5, 0, Vol, 30);
		setHit(two, 2, 0);
		setLock(two, 2, 0, Syn1, 90);
		CHECK_EVENTS(play(two, 0.0, 60.0), (Events{on(0, 2), lockOn(0, 2, Syn1, 90), lockOff(0, 2, Syn1), on(0, 5),
			lockOn(0, 5, Vol, 31), lockOff(0, 5, Vol)}));

		// Taking the Hit off takes its Locks off; giving it back plays it without them.
		seq.input({Input::Kind::SetHit, 2, 1, 0});
		seq.input({Input::Kind::SetHit, 2, 1, 1});
		CHECK(!seq.pattern().steps[2][1].locked());
		setLock(seq, 3, 0, Syn1, NoLock);
		CHECK(seq.pattern().steps[2][0].locks[Syn1] == NoLock);
		CHECK(seq.pattern().steps[2][0].locks[Vol] == 0);

		// A Mute silences the Locks with the Hit.
		Played muted;
		setHit(muted, 1, 0);
		setLock(muted, 1, 0, Vol, 90);
		muted.input({Input::Kind::SetMute, 0, 0, 1});
		CHECK(play(muted, 0.0, 300.0).empty());

		// Stopping ends the Hit sounding; its Locks have ended already.
		Played stopped;
		setHit(stopped, 2, 0);
		setLock(stopped, 2, 0, Vol, 90);
		play(stopped, 0.0, 60.0);
		Events events;
		append(events, stopped.stopAt(60.0));
		CHECK_EVENTS(events, (Events{off(60, 2)}));
	}

	void setCondition(Sequencer& _seq, const int _track, const int _step, const char* _label, const int _pattern = 0)
	{
		Input in{Input::Kind::SetCondition, _track - 1, _step, conditionIndex(_label)};
		in.pattern = _pattern;
		_seq.input(in);
	}

	// The Hits a Track sounded, as the steps they were planned on (16 per Pass).
	std::vector<int> stepsPlayed(const Events& _events, const int _track)
	{
		std::vector<int> steps;
		for(const Event& e : _events)
			if(e.status == (0x90 | (_track - 1)) && e.data1 == FirstHitNote + _track - 1)
				steps.push_back(static_cast<int>(e.tick / TicksPerStep));
		return steps;
	}

	void conditions()
	{
		// A:B counts Passes from the start.
		Played ratio;
		setHit(ratio, 1, 0);
		setCondition(ratio, 1, 0, "1:2");
		CHECK(stepsPlayed(play(ratio, 0.0, 4 * 1920.0), 1) == (std::vector<int>{0, 32}));

		// FILL is read when the step is planned.
		Played fill;
		setHit(fill, 2, 0);
		setCondition(fill, 2, 0, "FILL");
		fill.input({Input::Kind::SetFill, 0, 0, 1});
		CHECK(stepsPlayed(play(fill, 0.0, 1000.0), 2) == (std::vector<int>{0}));
		fill.input({Input::Kind::SetFill, 0, 0, 0});
		CHECK(stepsPlayed(play(fill, 1000.0, 3 * 1920.0), 2).empty());

		// The same seed source draws the same; another draws otherwise; each start draws a new seed.
		const auto chances = [](const uint64_t _source)
		{
			Played seq(_source);
			for(int s = 0; s < 16; ++s)
			{
				setHit(seq, 1, s);
				setCondition(seq, 1, s, "50%");
			}
			return stepsPlayed(play(seq, 0.0, 8 * 1920.0), 1);
		};
		CHECK(chances(5) == chances(5));
		CHECK(chances(5) != chances(6));
		Played starts(5);
		play(starts, 0.0, 100.0);
		const uint64_t first = starts.startSeed();
		starts.stopAt(200.0);
		play(starts, 0.0, 100.0);
		CHECK(starts.startSeed() != first);

		// NEI follows its neighbour Pass after Pass, and still does when the step planned ahead is edited before it
		// begins: it is drawn again from the memory it had.
		Played pair(9);
		setHit(pair, 1, 1);
		setCondition(pair, 1, 1, "50%");
		setHit(pair, 2, 1);
		setCondition(pair, 2, 1, "NEI");
		Events events;
		Wake w = pair.at(0.0);
		append(events, w);
		while(w.next < 20 * 1920.0)
		{
			// At each Pass's step 0 (step 1 planned, not begun), touch Track 1's step 1.
			if(static_cast<int64_t>(w.next) % 1920 == 0)
				pair.input({Input::Kind::SetVelocity, 0, 1, 90 + static_cast<int>(w.next / 1920) % 20});
			w = pair.at(w.next);
			append(events, w);
		}
		const std::vector<int> track1 = stepsPlayed(events, 1);
		const std::vector<int> track2 = stepsPlayed(events, 2);
		CHECK(track1 == track2);
		CHECK(track1.size() > 3 && track1.size() < 17);	// about half of 20 Passes

		// Drawn again, the step planned ahead starts from the memory it had: its FILL, taken off before it begins,
		// leaves nothing for the PRE after it.
		Played pre(3);
		setHit(pre, 1, 1);
		setCondition(pre, 1, 1, "FILL");
		setHit(pre, 1, 2);
		setCondition(pre, 1, 2, "PRE");
		pre.input({Input::Kind::SetFill, 0, 0, 1});
		play(pre, 0.0, 60.0);
		setCondition(pre, 1, 1, "None");
		CHECK(stepsPlayed(play(pre, 60.0, 1900.0), 1) == (std::vector<int>{1}));
	}

	// A jump forward that lands in the step played is no new Pass: the Pattern did not come round. Track 1's step 1
	// plays on Pass 1 of 2 (1:2), and still does after a seek inside step 0 at 1x and at 2x. A jump back inside the
	// step played starts a new Pass, as Live plays it again.
	void forwardWithinTheStep()
	{
		const auto seek = [](const int _scale, const double _to)
		{
			Sequencer seq;
			seq.input({Input::Kind::SetScale, 0, 0, _scale});
			setHit(seq, 1, 1);
			setCondition(seq, 1, 1, "1:2");
			seq.wake({0.0, true, 0.0, 1.0});
			seq.wake({5.0, true, 5.0, 1.0});
			const Wake w = seq.wake({_to, true, 6.0, 1.0});
			CHECK(w.step == 0 && w.pass == 1);
			Events events;
			for(double t = _to + 1.0; t <= 350.0; t += 1.0)
				append(events, seq.wake({t, true, t - _to + 6.0, 1.0}));
			return events;
		};
		CHECK_EVENTS(seek(4, 100.0), (Events{on(120, 1), off(240, 1)}));
		CHECK_EVENTS(seek(6, 50.0), (Events{on(60, 1), off(120, 1)}));

		// From 350 back to 250, both in step 2 at 1x: a new Pass.
		Sequencer back;
		for(double t = 0.0; t <= 350.0; t += 1.0)
			back.wake({t, true, t, 1.0});
		const Wake w = back.wake({250.0, true, 351.0, 1.0});
		CHECK(w.step == 2 && w.pass == 2);

		// A Pattern of one step, whose step planned ahead stands at the same position as the one played: forward
		// within the step, the same Pass; back within it, the next.
		Sequencer one;
		one.input({Input::Kind::SetLength, 0, 0, 1});
		for(double t = 0.0; t <= 5.0; t += 1.0)
			one.wake({t, true, t, 1.0});
		const Wake forward = one.wake({100.0, true, 6.0, 1.0});
		CHECK(forward.step == 0 && forward.pass == 1);
		one.wake({101.0, true, 7.0, 1.0});
		const Wake backward = one.wake({50.0, true, 8.0, 1.0});
		CHECK(backward.step == 0 && backward.pass == 2);
	}

	void setRatchets(Sequencer& _seq, const int _track, const int _step, const int _count, const int _curve = 0)
	{
		_seq.input({Input::Kind::SetRatchets, _track - 1, _step, _count});
		_seq.input({Input::Kind::SetCurve, _track - 1, _step, _curve});
	}

	void ratchetCurve()
	{
		// The step's velocity at the loud end, the curve's share of it off at the other end, in proportion between.
		CHECK(ratchetVelocity(100, 0, 2, 3) == 100);
		CHECK(ratchetVelocity(100, -50, 0, 3) == 100);
		CHECK(ratchetVelocity(100, -50, 1, 3) == 75);
		CHECK(ratchetVelocity(100, -50, 2, 3) == 50);
		CHECK(ratchetVelocity(100, 50, 0, 3) == 50);
		CHECK(ratchetVelocity(100, 50, 1, 3) == 75);
		CHECK(ratchetVelocity(100, 50, 2, 3) == 100);
		CHECK(ratchetVelocity(99, -33, 1, 2) == 66);	// 66.33
		CHECK(ratchetVelocity(101, -50, 1, 2) == 51);	// 50.5, rounded up
		CHECK(ratchetVelocity(127, 100, 7, 8) == 127);
		CHECK(ratchetVelocity(127, -100, 7, 8) == 1);	// never 0, which would be a note-off
		CHECK(ratchetVelocity(127, 100, 0, 8) == 1);
		CHECK(ratchetVelocity(90, -100, 0, 1) == 90);	// a single Hit has no curve
	}

	void ratchets()
	{
		// Three ratchets share the step, each a third of it long, the note-off before the next note-on, falling.
		Played seq;
		setHit(seq, 2, 0, 100);
		setRatchets(seq, 2, 0, 3, -50);
		CHECK_EVENTS(play(seq, 0.0, 200.0),
			(Events{on(0, 2, 100), off(40, 2), on(40, 2, 75), off(80, 2), on(80, 2, 50), off(120, 2)}));

		// Seven do not divide the step's 120 ticks: their ticks are fractions, each note-off on the next note-on's
		// tick.
		Played seven;
		setHit(seven, 1, 0);
		setRatchets(seven, 1, 0, 7);
		const Events events = play(seven, 0.0, 200.0);
		CHECK(events.size() == 14);
		for(size_t i = 0; i + 1 < events.size(); i += 2)
		{
			const int r = static_cast<int>(i / 2);
			CHECK(events[i].status == 0x90 && std::fabs(events[i].tick - 120.0 * r / 7) < 1e-9);
			CHECK(events[i + 1].status == 0x80);
			if(i + 2 < events.size())
				CHECK(events[i + 1].tick == events[i + 2].tick);
		}
		CHECK(events.back().tick == 120.0);

		// Two Tracks whose ratchets meet: every note-off at that tick first, then Track by Track the notes; a Track of
		// 3 and one of 2 meet only at the step's ends.
		Played pair;
		setHit(pair, 4, 0);
		setRatchets(pair, 4, 0, 2);
		setHit(pair, 1, 0);
		setRatchets(pair, 1, 0, 2);
		setHit(pair, 2, 0);
		setRatchets(pair, 2, 0, 3);
		CHECK_EVENTS(play(pair, 0.0, 100.0), (Events{on(0, 1), on(0, 2), on(0, 4), off(40, 2), on(40, 2),
			off(60, 1), off(60, 4), on(60, 1), on(60, 4), off(80, 2), on(80, 2)}));

		// Each ratchet is the Hit, a share of the step long, followed by its Locks.
		Played locked;
		setHit(locked, 3, 0);
		setLock(locked, 3, 0, Vol, 0);
		setRatchets(locked, 3, 0, 2);
		CHECK_EVENTS(play(locked, 0.0, 100.0), (Events{on(0, 3), lockOn(0, 3, Vol, 1), lockOff(0, 3, Vol),
			off(60, 3), on(60, 3), lockOn(60, 3, Vol, 1), lockOff(60, 3, Vol)}));

		// One draw decides for the whole group: every step played sounds its 4 Hits.
		Played chance(11);
		for(int s = 0; s < 16; ++s)
		{
			setHit(chance, 1, s);
			setRatchets(chance, 1, s, 4);
			setCondition(chance, 1, s, "50%");
		}
		std::map<int, int> hits;
		for(const int s : stepsPlayed(play(chance, 0.0, 8 * 1920.0), 1))
			++hits[s];
		CHECK(hits.size() > 20 && hits.size() < 108);	// about half of 128 steps
		const auto groupOfFour = [](const std::pair<const int, int>& _h) { return _h.second == 4; };
		CHECK(std::all_of(hits.begin(), hits.end(), groupOfFour));

		// Muted, none of them sounds.
		Played muted;
		setHit(muted, 1, 0);
		setRatchets(muted, 1, 0, 4);
		muted.input({Input::Kind::SetMute, 0, 0, 1});
		CHECK(play(muted, 0.0, 300.0).empty());

		// Edited on the step planned and not begun, they are heard on this Pass.
		Played edited;
		setHit(edited, 1, 1);
		CHECK(play(edited, 0.0, 60.0).empty());
		setRatchets(edited, 1, 1, 2);
		CHECK_EVENTS(play(edited, 60.0, 300.0), (Events{on(120, 1), off(180, 1), on(180, 1), off(240, 1)}));

		// A start inside the step plays the ratchets from there on.
		Played inside;
		setHit(inside, 1, 0);
		setRatchets(inside, 1, 0, 4);
		CHECK_EVENTS(play(inside, 60.0, 130.0), (Events{on(60, 1), off(90, 1), on(90, 1), off(120, 1)}));

		// Taking the Hit off takes the ratchets off; they need a Hit.
		edited.input({Input::Kind::SetHit, 0, 1, 0});
		CHECK(edited.pattern().steps[0][1].ratchets == 1);
		CHECK(!edited.input({Input::Kind::SetRatchets, 0, 1, 3}));
		CHECK(!edited.input({Input::Kind::SetCurve, 0, 1, 40}));
	}

	Event accentOn(const double _tick, const int _track, const int _velocity)
	{
		return {_tick, 0x90 | (_track - 1), AccentNote, _velocity};
	}

	Event accentOff(const double _tick, const int _track)
	{
		return {_tick, 0x80 | (_track - 1), AccentNote, 0};
	}

	// Accent (issue 26): every Hit of a step in the Pattern's Accent row is followed, after its Locks, by note 88, whose
	// velocity carries the Pattern's amount on the Locks' scale; played live it ends at once, as a Lock does.
	void accents()
	{
		Played seq;
		setHit(seq, 2, 0);
		setLock(seq, 2, 0, Vol, 30);
		setHit(seq, 5, 0);
		setHit(seq, 5, 1);
		CHECK(seq.input({Input::Kind::SetAccentStep, 0, 0, 1}));
		CHECK(seq.input({Input::Kind::SetAccent, 0, 0, 40}));
		CHECK(seq.pattern().accent == 40 && seq.pattern().inAccentRow(0) && !seq.pattern().inAccentRow(1));
		CHECK_EVENTS(play(seq, 0.0, 200.0), (Events{
			on(0, 2), lockOn(0, 2, Vol, 31), lockOff(0, 2, Vol), accentOn(0, 2, 41), accentOff(0, 2),
			on(0, 5), accentOn(0, 5, 41), accentOff(0, 5),
			off(120, 2), off(120, 5), on(120, 5)}));

		// The amount from 0 to 127, at 0 by default: an accent of 0 still travels, at velocity 1.
		CHECK(Pattern{}.accent == 0 && Pattern{}.accentSteps == 0);
		CHECK(seq.input({Input::Kind::SetAccent, 0, 0, 200}));
		CHECK(seq.pattern().accent == 127);
		Played none;
		setHit(none, 1, 0);
		none.input({Input::Kind::SetAccentStep, 0, 0, 1});
		CHECK_EVENTS(play(none, 0.0, 60.0), (Events{on(0, 1), accentOn(0, 1, 1), accentOff(0, 1)}));
		CHECK(!none.input({Input::Kind::SetAccentStep, 0, MaxSteps, 1}));

		// Each ratchet is accented.
		Played ratchets;
		setHit(ratchets, 3, 0);
		setRatchets(ratchets, 3, 0, 2);
		ratchets.input({Input::Kind::SetAccentStep, 0, 0, 1});
		ratchets.input({Input::Kind::SetAccent, 0, 0, 100});
		CHECK_EVENTS(play(ratchets, 0.0, 100.0), (Events{on(0, 3), accentOn(0, 3, 100), accentOff(0, 3),
			off(60, 3), on(60, 3), accentOn(60, 3, 100), accentOff(60, 3)}));

		// x2 copies the Accent row with the steps; Clear takes the amount and the row off.
		Played doubled;
		doubled.input({Input::Kind::SetLength, 0, 0, 16});
		doubled.input({Input::Kind::SetAccentStep, 0, 3, 1});
		doubled.input({Input::Kind::SetAccent, 0, 0, 70});
		CHECK(doubled.input({Input::Kind::SetSlideStep, 0, 5, 1}));
		CHECK(doubled.input({Input::Kind::DoubleLength, 0, 0, 0}));
		CHECK(doubled.pattern().inAccentRow(3) && doubled.pattern().inAccentRow(19) && !doubled.pattern().inAccentRow(4));
		CHECK(doubled.pattern().inSlideRow(5) && doubled.pattern().inSlideRow(21) && !doubled.pattern().inSlideRow(3));
		CHECK(doubled.input({Input::Kind::ClearPattern, 0, 0, 0}));
		CHECK(doubled.pattern().accent == 0 && doubled.pattern().accentSteps == 0 && doubled.pattern().slideSteps == 0);

		// A muted Track's accent is silent with it.
		Played muted;
		setHit(muted, 1, 0);
		muted.input({Input::Kind::SetAccentStep, 0, 0, 1});
		muted.input({Input::Kind::SetMute, 0, 0, 1});
		CHECK(play(muted, 0.0, 60.0).empty());
	}

	void setMicro(Sequencer& _seq, const int _track, const int _step, const int _micro)
	{
		_seq.input({Input::Kind::SetMicro, _track - 1, _step, _micro});
	}

	void microTiming()
	{
		// A 24th of a step is 5 ticks: late by 6, a Hit and its note-off move 30 ticks; early by 6, 30 ticks before.
		Played seq;
		setHit(seq, 1, 0);
		setMicro(seq, 1, 0, 6);
		setHit(seq, 2, 1);
		setMicro(seq, 2, 1, -6);
		CHECK_EVENTS(play(seq, 0.0, 300.0), (Events{on(30, 1), on(90, 2), off(150, 1), off(210, 2)}));
		CHECK(!seq.input({Input::Kind::SetMicro, 2, 0, 5}));	// no Hit there
		CHECK(seq.input({Input::Kind::SetMicro, 0, 0, 40}));
		CHECK(seq.pattern().steps[0][0].micro == MaxMicro);

		// The ratchets move with it.
		Played group;
		setHit(group, 1, 0);
		setRatchets(group, 1, 0, 2);
		setMicro(group, 1, 0, 3);
		CHECK_EVENTS(play(group, 0.0, 200.0), (Events{on(15, 1), off(75, 1), on(75, 1), off(135, 1)}));

		// Early on the first step: 15 ticks before the start, within its grace, so sent at once; then early on every
		// later Pass.
		Played early;
		setHit(early, 1, 0);
		setMicro(early, 1, 0, -3);
		CHECK_EVENTS(play(early, 0.0, 2000.0), (Events{on(-15, 1), off(105, 1), on(1905, 1)}));

		// A late Hit meets the next step's early one: the early one ends it, as a clip would.
		Played meet;
		setHit(meet, 1, 0);
		setMicro(meet, 1, 0, 12);
		setHit(meet, 1, 1);
		setMicro(meet, 1, 1, -11);
		CHECK_EVENTS(play(meet, 0.0, 300.0), (Events{on(60, 1), off(65, 1), on(65, 1), off(185, 1)}));

		// Taking the Hit off takes its micro-timing off.
		meet.input({Input::Kind::SetHit, 0, 1, 0});
		CHECK(meet.pattern().steps[0][1].micro == 0);
	}

	// Swing, the Machinedrum's: the steps of the swing row, every second one until it is edited, play late by
	// (2 x swing - 1) of a step, every Track's; micro-timing adds to it and the ratchets move with it.
	void swing()
	{
		// 50 %, straight: nothing moves.
		Played straight;
		setHit(straight, 1, 0);
		setHit(straight, 1, 1);
		CHECK(straight.pattern().swing == MinSwing && straight.pattern().swingSteps == DefaultSwingSteps);
		CHECK_EVENTS(play(straight, 0.0, 300.0), (Events{on(0, 1), off(120, 1), on(120, 1), off(240, 1)}));

		// 80 %: step 2 of each pair 0.6 of a step late, 72 ticks, for every Track; step 1 stays.
		Played late;
		setHit(late, 1, 0);
		setHit(late, 1, 1);
		setHit(late, 2, 1);
		CHECK(late.input({Input::Kind::SetSwing, 0, 0, 80}));
		CHECK_EVENTS(play(late, 0.0, 400.0),
			(Events{on(0, 1), off(120, 1), on(192, 1), on(192, 2), off(312, 1), off(312, 2)}));
		CHECK(swingSubticks(80) == 504 && swingSubticks(66) == 269 && swingSubticks(50) == 0);

		// 65 %: 0.3 of a step, 36 ticks; with micro-timing +6 (30 ticks) on top, and two ratchets moving together.
		Played both;
		both.input({Input::Kind::SetSwing, 0, 0, 65});
		setHit(both, 1, 1);
		setMicro(both, 1, 1, 6);
		setRatchets(both, 1, 1, 2);
		CHECK_EVENTS(play(both, 0.0, 400.0), (Events{on(186, 1), off(246, 1), on(246, 1), off(306, 1)}));

		// The row edited: step 2 no longer swings, step 3 does.
		Played row;
		row.input({Input::Kind::SetSwing, 0, 0, 80});
		setHit(row, 1, 1);
		setHit(row, 1, 2);
		CHECK(row.input({Input::Kind::SetSwingStep, 0, 1, 0}));
		CHECK(row.input({Input::Kind::SetSwingStep, 0, 2, 1}));
		CHECK(!row.input({Input::Kind::SetSwingStep, 0, 2, 1}));
		CHECK(!row.input({Input::Kind::SetSwingStep, 0, MaxSteps, 1}));
		CHECK_EVENTS(play(row, 0.0, 500.0), (Events{on(120, 1), off(240, 1), on(312, 1), off(432, 1)}));

		// At 2x a step is 60 ticks: 80 % puts step 2 36 ticks late.
		Played fast;
		fast.input({Input::Kind::SetScale, 0, 0, 6});
		fast.input({Input::Kind::SetSwing, 0, 0, 80});
		setHit(fast, 1, 1);
		CHECK_EVENTS(play(fast, 0.0, 200.0), (Events{on(96, 1), off(156, 1)}));

		// The amount is clamped to 50-80 %, and an edit that changes nothing is none.
		Played clamped;
		CHECK(clamped.input({Input::Kind::SetSwing, 0, 0, 95}));
		CHECK(clamped.pattern().swing == MaxSwing);
		CHECK(!clamped.input({Input::Kind::SetSwing, 0, 0, 80}));
		CHECK(clamped.input({Input::Kind::SetSwing, 0, 0, 10}));
		CHECK(clamped.pattern().swing == MinSwing);

		// The last step of a Pass, late by swing and micro-timing past the Pass's end, sounds after the next Pass's
		// first step: Pattern of 2 steps, step 2 at 80 % and +23 24ths, 307 ticks, after step 1's 240.
		Played wrap;
		wrap.input({Input::Kind::SetLength, 0, 0, 2});
		wrap.input({Input::Kind::SetSwing, 0, 0, 80});
		setHit(wrap, 1, 0);
		setHit(wrap, 2, 1);
		setMicro(wrap, 2, 1, MaxMicro);
		CHECK_EVENTS(play(wrap, 0.0, 320.0), (Events{on(0, 1), off(120, 1), on(240, 1), on(307, 2)}));
	}

	void editsOfAStepBegun()
	{
		// At tick 60, step 1 is planned: Track 1, early by 20, has sounded it at 20; Track 2, on the grid, has not.
		// Edits of step 1 are heard on Track 2 on this Pass, never on Track 1, which is not played twice.
		Played seq;
		setHit(seq, 1, 1);
		setMicro(seq, 1, 1, -20);
		setHit(seq, 2, 1);
		CHECK_EVENTS(play(seq, 0.0, 60.0), (Events{on(20, 1)}));
		seq.input({Input::Kind::SetVelocity, 0, 1, 50});
		seq.input({Input::Kind::SetVelocity, 1, 1, 70});
		CHECK_EVENTS(play(seq, 60.0, 300.0), (Events{on(120, 2, 70), off(140, 1), off(240, 2)}));

		// Moved before the position by an edit, a Hit not yet sounded goes at once rather than a Pass later. (The
		// external wakes the Sequencer every millisecond: it knows the position an edit lands at.)
		Played moved;
		setHit(moved, 2, 1);
		CHECK(play(moved, 0.0, 60.0).empty());
		CHECK(moved.at(60.0).begin == moved.at(60.0).end);
		setMicro(moved, 2, 1, -23);
		const Events events = play(moved, 60.0, 300.0);
		CHECK(events.size() == 2 && events[0].status == 0x91 && events[0].tick > 60.0 && events[0].tick < 61.0);

		// A Track kept as it began gives NEI the result it plays with, even when its Condition changes after it began.
		Played pair(9);
		setHit(pair, 1, 1);
		setMicro(pair, 1, 1, -20);
		setCondition(pair, 1, 1, "50%");
		setHit(pair, 2, 1);
		setCondition(pair, 2, 1, "NEI");
		Events all;
		Wake w = pair.at(0.0);
		append(all, w);
		int pass = 0;
		while(w.next < 30 * 1920.0)
		{
			const double now = w.next;
			w = pair.at(now);
			append(all, w);
			// Just after Track 1 sounds step 1 (at 20 of each Pass), its Condition changes.
			if(static_cast<int64_t>(now) % 1920 == 20)
				setCondition(pair, 1, 1, ++pass % 2 ? "25%" : "50%");
		}
		std::vector<int> passes1;
		std::vector<int> passes2;
		for(const Event& e : all)
		{
			if(e.status == 0x90)
				passes1.push_back(static_cast<int>(e.tick / 1920.0));
			if(e.status == 0x91)
				passes2.push_back(static_cast<int>(e.tick / 1920.0));
		}
		CHECK(passes1 == passes2);
		CHECK(passes1.size() > 3 && passes1.size() < 27);
	}

	// Slide (issue 26): with the last Hit of a step in the Slide row, after its Locks and Accent, note 89 carries the
	// clocks (24 to a quarter note) to the Track's next Hit as both play, notes 90-113 that Hit's Locks for the Locks the
	// step has, note 114 the clocks past 127; played live, each ends at once, as a Lock does. As Lock parameters:
	constexpr int Slide = SlideNote - FirstLockNote;
	constexpr int Target = FirstSlideTargetNote - FirstLockNote;
	constexpr int Longer = LongSlideNote - FirstLockNote;
	constexpr int Syn4 = 3;

	void slides()
	{
		// Four steps to the next Hit at 1x: 24 clocks. SYN1 glides to the next Hit's; VOL, which that Hit does not lock,
		// to the Kit's value, MD Drums' part; SYN4, locked on the next Hit only, does not glide.
		Played seq;
		setHit(seq, 2, 0);
		setLock(seq, 2, 0, Syn1, 10);
		setLock(seq, 2, 0, Vol, 20);
		setHit(seq, 2, 4);
		setLock(seq, 2, 4, Syn1, 100);
		setLock(seq, 2, 4, Syn4, 50);
		CHECK(seq.input({Input::Kind::SetSlideStep, 0, 0, 1}));
		CHECK_EVENTS(play(seq, 0.0, 700.0), (Events{
			on(0, 2), lockOn(0, 2, Syn1, 11), lockOff(0, 2, Syn1), lockOn(0, 2, Vol, 21), lockOff(0, 2, Vol),
			lockOn(0, 2, Slide, 24), lockOff(0, 2, Slide), lockOn(0, 2, Target + Syn1, 100), lockOff(0, 2, Target + Syn1),
			off(120, 2),
			on(480, 2), lockOn(480, 2, Syn1, 100), lockOff(480, 2, Syn1), lockOn(480, 2, Syn4, 51), lockOff(480, 2, Syn4),
			off(600, 2)}));

		// With the last ratchet, from where it plays to where the next Hit plays: 60 to 270 ticks, 10.5 clocks, 11.
		Played ratchets;
		setHit(ratchets, 1, 0);
		setRatchets(ratchets, 1, 0, 2);
		setHit(ratchets, 1, 2);
		setMicro(ratchets, 1, 2, 6);
		ratchets.input({Input::Kind::SetSlideStep, 0, 0, 1});
		CHECK_EVENTS(play(ratchets, 0.0, 200.0), (Events{on(0, 1), off(60, 1), on(60, 1), lockOn(60, 1, Slide, 11),
			lockOff(60, 1, Slide), off(120, 1)}));

		// The next step swung at 75 %, half a step late: 9 clocks.
		Played swung;
		setHit(swung, 1, 0);
		setHit(swung, 1, 1);
		swung.input({Input::Kind::SetSwing, 0, 0, 75});
		swung.input({Input::Kind::SetSlideStep, 0, 0, 1});
		CHECK_EVENTS(play(swung, 0.0, 10.0), (Events{on(0, 1), lockOn(0, 1, Slide, 9), lockOff(0, 1, Slide)}));

		// The next Hit whatever its Condition: a FILL step that does not play still ends the Slide's clocks.
		Played fill;
		setHit(fill, 1, 0);
		setHit(fill, 1, 2);
		setCondition(fill, 1, 2, "FILL");
		fill.input({Input::Kind::SetSlideStep, 0, 0, 1});
		CHECK_EVENTS(play(fill, 0.0, 10.0), (Events{on(0, 1), lockOn(0, 1, Slide, 12), lockOff(0, 1, Slide)}));

		// The only Hit glides to itself, a Pattern later: 16 steps, 96 clocks.
		Played alone;
		setHit(alone, 1, 2);
		setLock(alone, 1, 2, Vol, 30);
		alone.input({Input::Kind::SetSlideStep, 0, 2, 1});
		CHECK_EVENTS(play(alone, 0.0, 300.0), (Events{on(240, 1), lockOn(240, 1, Vol, 31), lockOff(240, 1, Vol),
			lockOn(240, 1, Slide, 96), lockOff(240, 1, Slide), lockOn(240, 1, Target + Vol, 31),
			lockOff(240, 1, Target + Vol)}));

		// Past 127 clocks, note 114 counts 127 more each: three steps at 1/8x are 144 clocks, 17 + 127.
		Played slow;
		slow.input({Input::Kind::SetScale, 0, 0, 0});
		setHit(slow, 1, 0);
		setHit(slow, 1, 3);
		slow.input({Input::Kind::SetSlideStep, 0, 0, 1});
		CHECK_EVENTS(play(slow, 0.0, 10.0), (Events{on(0, 1), lockOn(0, 1, Slide, 17), lockOff(0, 1, Slide),
			lockOn(0, 1, Longer, 1), lockOff(0, 1, Longer)}));

		// A muted Track's Slide is silent with it; a step outside the row sends none.
		Played muted;
		setHit(muted, 1, 0);
		muted.input({Input::Kind::SetSlideStep, 0, 0, 1});
		muted.input({Input::Kind::SetMute, 0, 0, 1});
		CHECK(play(muted, 0.0, 60.0).empty());
	}

	// Every Track on every step: 8 ratchets of a Hit, 24 Locks, the Accent and the Slide, the most a step can hold,
	// early or late.
	void heaviestPattern(Sequencer& _seq)
	{
		_seq.input({Input::Kind::SetAccent, 0, 0, 90});
		for(int s = 0; s < DefaultLength; ++s)
		{
			_seq.input({Input::Kind::SetAccentStep, 0, s, 1});
			_seq.input({Input::Kind::SetSlideStep, 0, s, 1});
		}
		for(int t = 1; t <= TrackCount; ++t)
		{
			for(int s = 0; s < DefaultLength; ++s)
			{
				setHit(_seq, t, s);
				setRatchets(_seq, t, s, MaxRatchets, -50);
				setMicro(_seq, t, s, (t * 7 + s * 5) % (2 * MaxMicro + 1) - MaxMicro);
				for(int p = 0; p < LockCount; ++p)
					setLock(_seq, t, s, p, (t * 7 + s * 3 + p) % 128);
			}
		}
	}

	struct HeavyRun
	{
		size_t events = 0;
		size_t allocations = 0;
		double start = 0.0;		// the transport start, which plans two steps
		double median = 0.0;
		double longest = 0.0;	// every later wake plans at most one step
		double replan = 0.0;	// an edit of the step planned ahead, which plans it again
	};

	HeavyRun heavyRun()
	{
		Played seq;
		heaviestPattern(seq);
		std::vector<double> wakes;
		wakes.reserve(4096);
		HeavyRun run;

		const auto ms = [](const std::chrono::steady_clock::time_point _begin)
		{
			return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - _begin).count();
		};
		g_allocations = 0;
		g_countAllocations = true;
		auto begin = std::chrono::steady_clock::now();
		Wake w = seq.at(0.0);
		run.start = ms(begin);
		run.events = static_cast<size_t>(w.end - w.begin);
		double now = 0.0;
		while(w.next < 4 * 1920.0)
		{
			now = w.next;
			begin = std::chrono::steady_clock::now();
			w = seq.at(now);
			wakes.push_back(ms(begin));
			run.events += static_cast<size_t>(w.end - w.begin);
		}
		const int planned = (static_cast<int>(now / TicksPerStep) + 1) % DefaultLength;
		begin = std::chrono::steady_clock::now();
		seq.input({Input::Kind::SetVelocity, 15, planned, 50});
		run.replan = ms(begin);
		seq.stopAt(now + 10.0);
		g_countAllocations = false;
		run.allocations = g_allocations;

		std::sort(wakes.begin(), wakes.end());
		run.median = wakes[wakes.size() / 2];
		run.longest = wakes.back();
		return run;
	}

	void heaviestWakes()
	{
		// The best of three runs for each figure: what the Sequencer costs, not what the machine took from the test
		// meanwhile (a run just after a build saw a wake of 3.1 ms, its neighbours 0.3 ms).
		HeavyRun best = heavyRun();
		for(int i = 0; i < 2; ++i)
		{
			const HeavyRun run = heavyRun();
			CHECK(run.events == best.events);
			best.allocations = std::max(best.allocations, run.allocations);
			best.start = std::min(best.start, run.start);
			best.median = std::min(best.median, run.median);
			best.longest = std::min(best.longest, run.longest);
			best.replan = std::min(best.replan, run.replan);
		}
		std::printf("  heaviest Pattern: %zu events; start %.3f ms; wakes median %.3f ms, longest %.3f ms; a step"
			" planned again %.3f ms\n", best.events, best.start, best.median, best.longest, best.replan);
		CHECK(best.allocations == 0);

		// The same Pattern swung at 80 % with every step +23 24ths late keeps a step more in the queue (a step's last
		// note-offs 2.56 steps after its place): still no allocation.
		{
			Played swung;
			heaviestPattern(swung);
			swung.input({Input::Kind::SetSwing, 0, 0, MaxSwing});
			for(int t = 1; t <= TrackCount; ++t)
				for(int s = 0; s < DefaultLength; ++s)
					setMicro(swung, t, s, MaxMicro);
			g_allocations = 0;
			g_countAllocations = true;
			Wake w = swung.at(0.0);
			while(w.next < 4 * 1920.0)
				w = swung.at(w.next);
			swung.stopAt(w.next + 10.0);
			g_countAllocations = false;
			CHECK(g_allocations == 0);
		}
		// Live's audio thread runs Max's scheduler in a device: a 64-sample vector at 44.1 kHz lasts 1.45 ms. Timed in
		// an optimised build only, as the gate builds it.
#ifdef NDEBUG
		CHECK(best.start < 1.0);
		CHECK(best.longest < 1.0);
		CHECK(best.replan < 1.0);
#endif
	}

	// Live keeps the notes a Max device holds in a set of 128 and quits when a note-on overflows it (issue 24: the
	// crash dump of 2026-10-13 throws boost::container::bad_alloc from that set, reached from Live's MIDI handler under
	// MaxPlug). Played live, the heaviest Pattern never holds more than a note per Track, its Hit, and the Lock between
	// its note-on and its note-off, whatever its Locks, ratchets, micro-timing and swing (399 notes before).
	void heldNotes()
	{
		Played seq;
		heaviestPattern(seq);
		seq.input({Input::Kind::SetSwing, 0, 0, MaxSwing});
		std::set<std::pair<int, int>> held;
		size_t most = 0;
		size_t locks = 0;
		for(const Event& e : play(seq, 0.0, 4 * 1920.0))
		{
			const std::pair<int, int> note{e.status & 0x0f, e.data1};
			if((e.status & 0xf0) == 0x90)
			{
				held.insert(note);
				locks += e.data1 >= FirstLockNote ? 1 : 0;
			}
			else
				held.erase(note);
			most = std::max(most, held.size());
		}
		std::printf("  heaviest Pattern played live: %zu Locks, at most %zu notes held\n", locks, most);
		CHECK(locks > 10000);
		CHECK(most <= static_cast<size_t>(TrackCount) + 1);
	}

	void scales()
	{
		// At 2x a step is a 32nd, 60 ticks; at 1/2x an 8th, 240, its ratchets sharing it; 3/4x an 8th-note triplet.
		Played fast;
		CHECK(fast.input({Input::Kind::SetScale, 0, 0, 6}));
		CHECK(fast.ticksPerStep() == 60.0);
		setHit(fast, 1, 1);
		CHECK_EVENTS(play(fast, 0.0, 150.0), (Events{on(60, 1), off(120, 1)}));

		Played slow;
		slow.input({Input::Kind::SetScale, 0, 0, 2});
		setHit(slow, 1, 1);
		setRatchets(slow, 1, 1, 2);
		CHECK_EVENTS(play(slow, 0.0, 600.0), (Events{on(240, 1), off(360, 1), on(360, 1), off(480, 1)}));
		CHECK(ScaleTicks[3] == 160.0);
		CHECK(!slow.input({Input::Kind::SetScale, 0, 0, ScaleCount}));

		// Changed while playing: the note sounding ends, and the next wake plays the new steps where Live is (tick 150
		// is in step 2 of 60 ticks, which began before the last look at 149: its Hit is not played late; step 3
		// follows at 180).
		Played changed;
		for(int s = 0; s < DefaultLength; ++s)
			setHit(changed, 1, s);
		Events events;
		for(double t = 0.0; t < 150.0; t += 1.0)
			append(events, changed.at(t));
		CHECK_EVENTS(events, (Events{on(0, 1), off(120, 1), on(120, 1)}));
		changed.input({Input::Kind::SetScale, 0, 0, 6});
		CHECK_EVENTS(play(changed, 150.0, 250.0), (Events{off(150, 1), on(180, 1), off(240, 1), on(240, 1)}));

		// Changed just after a Hit, where the two grids meet: 1x's step 1 and 2x's step 2 both begin at 120. The Hit
		// sounded at 120 is ended by the new scale, never struck again, and the first Pass goes on.
		Sequencer meet;
		for(int s = 0; s < DefaultLength; ++s)
			setHit(meet, 1, s);
		events.clear();
		for(int ms = 0; ms <= 200; ++ms)
		{
			if(ms == 126)
				meet.input({Input::Kind::SetScale, 0, 0, 6});
			const Wake w = meet.wake({static_cast<double>(ms), true, static_cast<double>(ms), 1.0});
			if(ms >= 100)
				append(events, w);
			if(ms == 126)
				CHECK(w.step == 2 && w.pass == 1);
		}
		CHECK_EVENTS(events, (Events{off(120, 1), on(120, 1), off(126, 1), on(180, 1)}));

		// Changed just before a step of the new grid begins that the next look passes: looks at half ticks, 2x set
		// between 179.5 and 180.5, where 2x's step 3 begins at 180. The last look sent nothing of it: its Hit goes out
		// late, at once.
		Sequencer between;
		for(int s = 0; s < DefaultLength; ++s)
			setHit(between, 1, s);
		events.clear();
		for(int ms = 0; ms <= 200; ++ms)
		{
			if(ms == 180)
				between.input({Input::Kind::SetScale, 0, 0, 6});
			const Wake w = between.wake({ms + 0.5, true, static_cast<double>(ms), 1.0});
			if(ms >= 100)
				append(events, w);
		}
		CHECK_EVENTS(events, (Events{off(120, 1), on(120, 1), off(180.5, 1), on(180, 1)}));

		// A new scale and a jump in the same look: a jump's grace, so that the Hit of the step Live jumped to, 10 ms
		// before the look, still goes out.
		Sequencer both;
		for(int s = 0; s < DefaultLength; ++s)
			setHit(both, 1, s);
		for(int ms = 0; ms <= 500; ++ms)
			both.wake({static_cast<double>(ms), true, static_cast<double>(ms), 1.0});
		both.input({Input::Kind::SetScale, 0, 0, 6});
		events.clear();
		append(events, both.wake({10.0, true, 511.0, 1.0}));
		CHECK_EVENTS(events, (Events{off(10, 1), on(0, 1)}));

		// The jump forward of that look, read on the new grid, stays in the Pass played, the second.
		Sequencer jumpedTo;
		for(int ms = 0; ms <= 2521; ++ms)
			jumpedTo.wake({static_cast<double>(ms), true, static_cast<double>(ms), 1.0});
		jumpedTo.input({Input::Kind::SetScale, 0, 0, 6});
		CHECK(jumpedTo.wake({2575.0, true, 2522.0, 1.0}).pass == 2);

		// A new scale is no jump: the Pass played carries on, whenever the change comes in it and whatever the scale,
		// though the step played stands at another position on the new grid, or one never planned, the Pattern having
		// grown. Every tick of Pass 2 at 1x, the look after the change crossing the new grid's first step included;
		// then after a start that follows ten Passes.
		int lost = 0;
		for(int scale = 0; scale < ScaleCount; ++scale)
		{
			if(scale == DefaultScale)
				continue;
			for(int at = 1920; at < 3840; ++at)
			{
				Played seq;
				play(seq, 0.0, at);
				seq.at(at);
				seq.input({Input::Kind::SetScale, 0, 0, scale});
				if(seq.at(at + 1.0).pass != 2)
					++lost;
			}
		}
		CHECK(lost == 0);
		Played grown;
		play(grown, 0.0, 3000.0);
		grown.input({Input::Kind::SetLength, 0, 0, 32});
		grown.input({Input::Kind::SetScale, 0, 0, 6});
		const Wake past = grown.at(3001.0);
		CHECK(past.step == 18 && past.pass == 2);
		Played again;
		play(again, 0.0, 10 * 1920.0 + 10.0);
		again.stopAt(10 * 1920.0 + 20.0);
		play(again, 0.0, 400.0);
		again.input({Input::Kind::SetScale, 0, 0, 6});
		const Wake first = again.at(401.0);
		CHECK(first.step == 6 && first.pass == 1);

		// A Pattern loaded with another scale plays at it.
		Played loaded;
		Pattern p;
		p.scale = 6;
		p.steps[0][1].hit = true;
		loaded.load(0, p);
		CHECK_EVENTS(play(loaded, 0.0, 150.0), (Events{on(60, 1), off(120, 1)}));
	}

	// Pattern 0: 4 steps of Track 1. Pattern 1: 2 steps of Track 2 at 2x (60 ticks a step).
	void twoPatterns(Sequencer& _seq)
	{
		_seq.input({Input::Kind::SetLength, 0, 0, 4});
		for(int s = 0; s < 4; ++s)
			setHit(_seq, 1, s);
		Input in{Input::Kind::SetLength, 0, 0, 2};
		in.pattern = 1;
		_seq.input(in);
		in = {Input::Kind::SetScale, 0, 0, 6};
		in.pattern = 1;
		_seq.input(in);
		for(int s = 0; s < 2; ++s)
		{
			in = {Input::Kind::SetHit, 1, s, 1};
			in.pattern = 1;
			_seq.input(in);
		}
	}

	void bank()
	{
		// Chosen while playing, a Pattern follows where the Pass ends (tick 480), on its own grid from there, its
		// Passes counted from 1.
		Played seq;
		twoPatterns(seq);
		CHECK_EVENTS(play(seq, 0.0, 60.0), (Events{on(0, 1)}));
		CHECK(seq.input({Input::Kind::SetPattern, 0, 0, 1}));
		Wake w = seq.at(60.0);
		CHECK(w.pattern == 0 && w.queued == 1);
		CHECK_EVENTS(play(seq, 60.0, 610.0), (Events{off(120, 1), on(120, 1), off(240, 1), on(240, 1), off(360, 1),
			on(360, 1), off(480, 1), on(480, 2), off(540, 2), on(540, 2), off(600, 2), on(600, 2)}));
		w = seq.at(610.0);
		CHECK(w.pattern == 1 && w.queued == -1 && w.step == 0 && w.pass == 2);
		CHECK(seq.current() == 1 && seq.ticksPerStep() == 60.0);

		// Chosen during the Pass's last step, after the next Pass's first step was planned: still at the Pass's end.
		Played late;
		twoPatterns(late);
		play(late, 0.0, 380.0);
		late.at(400.0);
		late.input({Input::Kind::SetPattern, 0, 0, 1});
		const Wake lw = late.at(410.0);
		CHECK(lw.step == 3 && lw.pattern == 0 && lw.queued == 1);
		CHECK_EVENTS(play(late, 410.0, 500.0), (Events{off(480, 1), on(480, 2)}));

		// Taken back during that step: the next Pass's first step is Pattern 0's again, nothing is queued, and the
		// Passes count on: Track 1's step 0, on 1:2, is silent on the second and plays on the third.
		Played taken;
		twoPatterns(taken);
		setCondition(taken, 1, 0, "1:2");
		play(taken, 0.0, 60.0);
		taken.input({Input::Kind::SetPattern, 0, 0, 1});
		play(taken, 60.0, 380.0);
		CHECK(taken.at(400.0).queued == 1);
		taken.input({Input::Kind::SetPattern, 0, 0, 0});
		Wake tw = taken.at(410.0);
		CHECK(tw.step == 3 && tw.pattern == 0 && tw.queued == -1);
		CHECK_EVENTS(play(taken, 410.0, 500.0), (Events{off(480, 1)}));
		tw = taken.at(500.0);
		CHECK(tw.step == 0 && tw.pass == 2 && tw.pattern == 0 && tw.queued == -1);
		CHECK_EVENTS(play(taken, 500.0, 980.0), (Events{on(600, 1), off(720, 1), on(720, 1), off(840, 1), on(840, 1),
			off(960, 1), on(960, 1)}));

		// Taken back once Pattern 1's first Hit, 23 24ths early, has begun: Pattern 1 plays its Pass, that Hit ending
		// as planned, and Pattern 0 follows.
		Played sounding;
		twoPatterns(sounding);
		Input early{Input::Kind::SetMicro, 1, 0, -MaxMicro};
		early.pattern = 1;
		sounding.input(early);
		play(sounding, 0.0, 60.0);
		sounding.input({Input::Kind::SetPattern, 0, 0, 1});
		play(sounding, 60.0, 430.0);
		sounding.input({Input::Kind::SetPattern, 0, 0, 0});
		CHECK_EVENTS(play(sounding, 430.0, 700.0),
			(Events{off(480, 1), off(482.5, 2), on(540, 2), off(600, 2), on(600, 1)}));

		// Choosing the Pattern playing takes the choice back.
		Played back;
		twoPatterns(back);
		play(back, 0.0, 60.0);
		back.input({Input::Kind::SetPattern, 0, 0, 1});
		CHECK(back.input({Input::Kind::SetPattern, 0, 0, 0}));
		CHECK(back.queued() == -1);
		CHECK_EVENTS(play(back, 60.0, 500.0), (Events{off(120, 1), on(120, 1), off(240, 1), on(240, 1), off(360, 1),
			on(360, 1), off(480, 1), on(480, 1)}));

		// Stopped with a Pattern queued, it is the one played next; chosen while stopped, at once.
		back.input({Input::Kind::SetPattern, 0, 0, 1});
		back.stopAt(500.0);
		CHECK(back.current() == 1 && back.queued() == -1);
		CHECK(back.input({Input::Kind::SetPattern, 0, 0, 0}));
		CHECK(back.current() == 0);
		CHECK(!back.input({Input::Kind::SetPattern, 0, 0, PatternCount}));

		// An edit of a Pattern not playing changes nothing heard; the Pattern it names keeps it.
		Played named;
		twoPatterns(named);
		play(named, 0.0, 60.0);
		Input in{Input::Kind::SetHit, 4, 1, 1};
		in.pattern = 1;
		CHECK(named.input(in));
		CHECK(named.pattern(1).steps[4][1].hit && !named.pattern().steps[4][1].hit);
		CHECK_EVENTS(play(named, 60.0, 130.0), (Events{off(120, 1), on(120, 1)}));

		// A Pattern loaded into the bank while another plays waits its turn.
		Pattern p;
		p.steps[9][0].hit = true;
		named.load(5, p);
		CHECK(named.pattern(5).steps[9][0].hit && named.current() == 0);
	}

	// During Pattern 0's last step, the next Pass's first step is planned from Pattern 1, queued. Live sent elsewhere
	// before it reaches that step undoes the switch: Pattern 0 plays where Live lands, in the Pass that follows from
	// it, and Pattern 1 begins at the end of that Pass, on its first (its 1ST step on Track 2 plays). A scale given to
	// Pattern 1 then moves only its first step; one given to Pattern 0 earlier in the Pass keeps the Pass to its end.
	void switchNotReached()
	{
		const auto patterns = [](Sequencer& _seq, const int _length1, const int _scale1)
		{
			twoPatterns(_seq);
			Input in{Input::Kind::SetLength, 0, 0, _length1};
			in.pattern = 1;
			_seq.input(in);
			in = {Input::Kind::SetScale, 0, 0, _scale1};
			in.pattern = 1;
			_seq.input(in);
			setCondition(_seq, 2, 0, "1ST", 1);
		};

		// A loop brace over Pattern 0's Pass, 480 ticks, looked at every millisecond, Pattern 1 chosen on the fourth
		// Pass: at the wrap, Pattern 1 begins at its step 0, on its first Pass, whatever its length and scale.
		const auto looped = [&](const int _length1, const int _scale1)
		{
			Sequencer seq;
			patterns(seq, _length1, _scale1);
			Events events;
			for(int ms = 0; ms <= 2050; ++ms)
			{
				if(ms == 1500)
					seq.input({Input::Kind::SetPattern, 0, 0, 1});
				const Wake w = seq.wake({std::fmod(ms, 480.0), true, static_cast<double>(ms), 1.0});
				if(ms >= 1910)
					append(events, w);
				if(ms == 1910)
					CHECK(w.step == 3 && w.pass == 4 && w.pattern == 0 && w.queued == 1);
				if(ms == 1920)
					CHECK(w.step == 0 && w.pass == 1 && w.pattern == 1 && w.queued == -1);
			}
			return events;
		};
		CHECK_EVENTS(looped(2, 6), (Events{off(0, 1), on(0, 2), off(60, 2), on(60, 2), off(120, 2)}));
		CHECK_EVENTS(looped(3, DefaultScale), (Events{off(0, 1), on(0, 2), off(120, 2), on(120, 2)}));

		// A seek forward inside the last step: it plays on in its Pass, Pattern 1 still queued, which begins at 480.
		Played inside;
		patterns(inside, 2, 6);
		play(inside, 0.0, 60.0);
		inside.input({Input::Kind::SetPattern, 0, 0, 1});
		play(inside, 60.0, 370.0);
		Events events;
		Wake w = inside.jumpTo(450.0);
		append(events, w);
		CHECK(w.step == 3 && w.pass == 1 && w.pattern == 0 && w.queued == 1);
		w = inside.at(490.0);
		append(events, w);
		CHECK(w.step == 0 && w.pass == 1 && w.pattern == 1 && w.queued == -1);
		CHECK_EVENTS(events, (Events{off(450, 1), on(480, 2)}));

		// Back to the middle of the Pass: Pattern 0's next Pass, to its end at 480. The 1ST drawn for Pattern 1's first
		// step, which never played, is forgotten: Track 2's PRE on Pattern 0's step 2 has nothing before it, as on the
		// first Pass, and does not play.
		Played back;
		patterns(back, 2, 6);
		setHit(back, 2, 2);
		setCondition(back, 2, 2, "PRE");
		play(back, 0.0, 60.0);
		back.input({Input::Kind::SetPattern, 0, 0, 1});
		play(back, 60.0, 370.0);
		events.clear();
		w = back.jumpTo(130.0);
		append(events, w);
		CHECK(w.step == 1 && w.pass == 2 && w.pattern == 0 && w.queued == 1);
		CHECK_EVENTS(events, (Events{off(130, 1), on(120, 1)}));
		CHECK_EVENTS(play(back, 130.0, 500.0),
			(Events{off(240, 1), on(240, 1), off(360, 1), on(360, 1), off(480, 1), on(480, 2)}));

		// Pattern 0 chosen again once Pattern 1's first Hit, 23 24ths early, has begun: the switch stands, until Live
		// goes back to the middle of the Pass. Undone there, the last choice holds: Pattern 0 plays on, nothing queued.
		Played begun;
		patterns(begun, 2, 6);
		Input early{Input::Kind::SetMicro, 1, 0, -MaxMicro};
		early.pattern = 1;
		begun.input(early);
		play(begun, 0.0, 60.0);
		begun.input({Input::Kind::SetPattern, 0, 0, 1});
		play(begun, 60.0, 370.0);
		CHECK_EVENTS(play(begun, 370.0, 430.0), (Events{on(422.5, 2)}));
		begun.input({Input::Kind::SetPattern, 0, 0, 0});
		events.clear();
		w = begun.jumpTo(130.0);
		append(events, w);
		CHECK(w.step == 1 && w.pass == 2 && w.pattern == 0 && w.queued == -1);
		CHECK_EVENTS(events, (Events{off(130, 1), off(130, 2), on(120, 1)}));
		CHECK_EVENTS(play(begun, 130.0, 500.0),
			(Events{off(240, 1), on(240, 1), off(360, 1), on(360, 1), off(480, 1), on(480, 1)}));

		// A seek forward inside a Pass's first step, Pattern 1 chosen during it: that Pass goes on, and Pattern 0's
		// step 1 plays; Pattern 1 waits for the Pass's end.
		Played first;
		patterns(first, 2, 6);
		play(first, 0.0, 490.0);
		first.input({Input::Kind::SetPattern, 0, 0, 1});
		events.clear();
		w = first.jumpTo(550.0);
		append(events, w);
		CHECK(w.step == 0 && w.pass == 2 && w.pattern == 0 && w.queued == 1);
		append(events, first.at(610.0));
		CHECK_EVENTS(events, (Events{off(550, 1), on(600, 1)}));

		// Pattern 1 given 1x during the last step: its first step, at 480, plays at it; nothing of Pattern 0's step
		// ends early.
		Played rescaled;
		patterns(rescaled, 2, 6);
		play(rescaled, 0.0, 60.0);
		rescaled.input({Input::Kind::SetPattern, 0, 0, 1});
		play(rescaled, 60.0, 370.0);
		Input scale{Input::Kind::SetScale, 0, 0, DefaultScale};
		scale.pattern = 1;
		CHECK(rescaled.input(scale));
		CHECK_EVENTS(play(rescaled, 370.0, 610.0), (Events{off(480, 1), on(480, 2), off(600, 2), on(600, 2)}));

		// Pattern 0 given 2x at 245, Pattern 1 queued: the new grid puts Live on a first step, at 240, but the Pass
		// played goes on to its end on that grid, at 480, where Pattern 1 begins.
		Sequencer regrid;
		patterns(regrid, 2, 6);
		events.clear();
		for(int ms = 0; ms <= 500; ++ms)
		{
			if(ms == 60)
				regrid.input({Input::Kind::SetPattern, 0, 0, 1});
			if(ms == 245)
				regrid.input({Input::Kind::SetScale, 0, 0, 6});
			w = regrid.wake({static_cast<double>(ms), true, static_cast<double>(ms), 1.0});
			if(ms >= 240)
				append(events, w);
			if(ms == 245)
				CHECK(w.step == 0 && w.pass == 1 && w.pattern == 0 && w.queued == 1);
		}
		CHECK_EVENTS(events, (Events{off(240, 1), on(240, 1), off(245, 1), on(300, 1), off(360, 1), on(360, 1),
			off(420, 1), on(420, 1), off(480, 1), on(480, 2)}));
	}

	// The Playhead the adapter hands the main thread in one word: every step, Pattern played and Pattern queued a wake
	// can give comes back whole, so no two share a word; a stopped Sequencer's is the one the adapter starts with, and
	// a wake's word holds that wake's step and Patterns.
	// Pattern _pattern, _length steps long, whose Track _pattern + 1 has a Hit on step 0.
	void hitOnFirstStep(Sequencer& _seq, const int _pattern, const int _length)
	{
		Input in{Input::Kind::SetLength, 0, 0, _length};
		in.pattern = _pattern;
		_seq.input(in);
		in = {Input::Kind::SetHit, _pattern, 0, 1};
		in.pattern = _pattern;
		_seq.input(in);
	}

	void setChain(Sequencer& _seq, const std::vector<int>& _patterns)
	{
		for(size_t i = 0; i < _patterns.size(); ++i)
			_seq.input({Input::Kind::ChainEntry, 0, static_cast<int>(i), _patterns[i]});
		_seq.input({Input::Kind::SetChain, 0, 0, static_cast<int>(_patterns.size())});
	}

	// A chain plays one Pass of each of its Patterns, round and round, its next one queued as each Pass is planned:
	// a Pattern of one step plays one step, whatever the main thread sees of it. The same Pattern twice plays two
	// Passes. Stopped, Live starts again from the place after the Pass played, though the next Pass was planned; a
	// chain set while playing starts at the end of the Pass; a Pattern chosen ends it.
	void chains()
	{
		Played seq;
		hitOnFirstStep(seq, 0, 1);
		hitOnFirstStep(seq, 2, 2);
		setChain(seq, {0, 2, 2});
		Wake w = seq.stopAt(0.0);
		CHECK(w.chain == 0 && w.pattern == 0);
		CHECK_EVENTS(play(seq, 0.0, 10.0), (Events{on(0, 1)}));
		w = seq.at(10.0);
		CHECK(w.chain == 0 && w.pattern == 0 && w.queued == 2);
		CHECK_EVENTS(play(seq, 10.0, 130.0), (Events{off(120, 1), on(120, 3)}));
		w = seq.at(130.0);
		CHECK(w.chain == 1 && w.pattern == 2 && w.queued == -1);
		CHECK_EVENTS(play(seq, 130.0, 370.0), (Events{off(240, 3), on(360, 3)}));
		w = seq.at(370.0);
		CHECK(w.chain == 2 && w.pattern == 2 && w.queued == 0);

		// Stopped during the Pass's last step, once the next Pass was planned from Pattern 0: Live starts again from
		// there.
		CHECK_EVENTS(play(seq, 370.0, 485.0), (Events{off(480, 3)}));
		w = seq.at(485.0);
		CHECK(w.step == 1 && w.chain == 2 && w.queued == 0);
		w = seq.stopAt(490.0);
		CHECK(w.chain == 0 && w.pattern == 0 && w.queued == -1);
		CHECK_EVENTS(play(seq, 0.0, 130.0), (Events{on(0, 1), off(120, 1), on(120, 3)}));

		// A Pattern chosen ends the chain: the one it queued gives way.
		CHECK(seq.input({Input::Kind::SetPattern, 0, 0, 0}));
		w = seq.at(140.0);
		CHECK(w.chain == -1 && w.pattern == 2 && w.queued == 0);

		// Set while playing on a Pattern it does not hold: none plays until the Pass's end, then its first.
		Played late;
		hitOnFirstStep(late, 0, 1);
		hitOnFirstStep(late, 1, 2);
		hitOnFirstStep(late, 2, 2);
		late.input({Input::Kind::SetPattern, 0, 0, 2});
		play(late, 0.0, 10.0);
		setChain(late, {1, 0});
		w = late.at(20.0);
		CHECK(w.chain == -1 && w.pattern == 2 && w.queued == 1);
		CHECK_EVENTS(play(late, 20.0, 490.0), (Events{off(120, 3), on(240, 2), off(360, 2), on(480, 1)}));
		CHECK(late.at(490.0).chain == 1);
		// Set again empty: the Pattern it queued still follows.
		CHECK(late.input({Input::Kind::SetChain, 0, 0, 0}));
		w = late.at(500.0);
		CHECK(w.chain == -1 && w.queued == 1);
		CHECK(!late.input({Input::Kind::SetChain, 0, 0, 1}));	// one Pattern is no chain
	}

	void playheads()
	{
		bool whole = true;
		for(int step = -1; step < MaxSteps; ++step)
		{
			for(int pattern = 0; pattern < PatternCount; ++pattern)
			{
				for(int queued = -1; queued < PatternCount; ++queued)
				{
					const int chain = (step + pattern + queued + 2) % (ChainLength + 1) - 1;
					const Playhead back = unpackPlayhead(packPlayhead({step, pattern, queued, chain}));
					whole = whole && back.step == step && back.pattern == pattern && back.queued == queued &&
						back.chain == chain;
				}
			}
		}
		CHECK(whole);

		Played seq;
		twoPatterns(seq);
		CHECK(packPlayhead(playhead(seq.stopAt(0.0))) == packPlayhead({}));
		play(seq, 0.0, 60.0);
		seq.input({Input::Kind::SetPattern, 0, 0, 1});
		const Playhead queued = unpackPlayhead(packPlayhead(playhead(seq.at(250.0))));
		CHECK(queued.step == 2 && queued.pattern == 0 && queued.queued == 1);
		// The Pass's last step, the next Pass's first planned from Pattern 1: still queued, until Live reaches it.
		play(seq, 250.0, 400.0);
		const Playhead last = unpackPlayhead(packPlayhead(playhead(seq.at(400.0))));
		CHECK(last.step == 3 && last.pattern == 0 && last.queued == 1);
		play(seq, 400.0, 610.0);
		const Playhead switched = unpackPlayhead(packPlayhead(playhead(seq.at(610.0))));
		CHECK(switched.step == 0 && switched.pattern == 1 && switched.queued == -1);
		const Playhead stopped = unpackPlayhead(packPlayhead(playhead(seq.stopAt(620.0))));
		CHECK(stopped.step == -1 && stopped.pattern == 1 && stopped.queued == -1);
	}

	void loads()
	{
		// A Pattern loaded while playing (the set's, or Live's undo) is heard from the step planned ahead, without an
		// allocation: here step 1, planned at tick 0, not begun at 60.
		Played seq;
		setHit(seq, 2, 1);
		CHECK(play(seq, 0.0, 60.0).empty());
		Pattern loaded;
		loaded.steps[0][1].hit = true;
		loaded.steps[0][1].velocity = 77;
		g_allocations = 0;
		g_countAllocations = true;
		seq.load(0, loaded);
		g_countAllocations = false;
		CHECK(g_allocations == 0);
		CHECK_EVENTS(play(seq, 60.0, 300.0), (Events{on(120, 1, 77), off(240, 1)}));
		CHECK(!seq.pattern().steps[1][1].hit);

		// Stopped, it is simply the Pattern played next.
		Played stopped;
		stopped.load(0, loaded);
		CHECK_EVENTS(play(stopped, 0.0, 300.0), (Events{on(120, 1, 77), off(240, 1)}));
	}

	void noAllocationWhilePlaying()
	{
		Played seq;
		for(int t = 1; t <= TrackCount; ++t)
			for(int s = 0; s < DefaultLength; ++s)
				setHit(seq, t, s, 64 + s);

		g_allocations = 0;
		g_countAllocations = true;
		Wake w = seq.at(0.0);
		size_t events = static_cast<size_t>(w.end - w.begin);
		while(w.next < 10 * 1920.0)
		{
			w = seq.at(w.next);
			events += static_cast<size_t>(w.end - w.begin);
		}
		seq.jumpTo(5000.0);
		seq.input({Input::Kind::SetHit, 0, 3, 0});
		seq.input({Input::Kind::SetPattern, 0, 0, 3});
		w = seq.at(5100.0);
		while(w.next < 8000.0)
			w = seq.at(w.next);
		CHECK(seq.current() == 3);
		seq.stopAt(8000.0);
		g_countAllocations = false;

		CHECK(g_allocations == 0);
		// Ten Passes of 256 Hits and their note-offs, but those of the last step, which fall on the tick not reached.
		CHECK(events == 10 * 16 * 16 * 2 - 16);
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);

	silentPattern();
	oneHit();
	sameNoteNextStep();
	tracksInOrder();
	lateWake();
	stopAndStartAgain();
	jumps();
	startAndStop();
	loopBack();
	loopBackAtSlowScales();
	tempoChange();
	graceOfALateLook();
	jumpSlack();
	mutes();
	editsFromTheNextPlannedStep();
	hitTakenOffBeforeItsStep();
	passes();
	inputs();
	locks();
	accents();
	conditions();
	forwardWithinTheStep();
	ratchetCurve();
	ratchets();
	microTiming();
	swing();
	slides();
	editsOfAStepBegun();
	loads();
	scales();
	bank();
	switchNotReached();
	chains();
	playheads();
	noAllocationWhilePlaying();
	heaviestWakes();
	heldNotes();

	return mdseqTest::finish("mdseqSequencerTest");
}
