#include "mdseqSequencer.h"

#include "mdseqCondition.h"
#include "mdseqEdit.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <type_traits>

namespace mdseq
{
	namespace
	{
		// A ratchet's notes, in that order of their ids: its 24 Locks, the Accent, the Slide's clocks, its 24 targets,
		// its clocks past 127, and the Hit.
		constexpr int AccentParam = AccentNote - FirstLockNote;
		constexpr int SlideParam = SlideNote - FirstLockNote;
		constexpr int FirstTargetParam = FirstSlideTargetNote - FirstLockNote;
		constexpr int LongSlideParam = LongSlideNote - FirstLockNote;
		constexpr int NotesPerRatchet = LongSlideParam + 2;
		constexpr int SlideNotes = LongSlideParam - AccentParam;	// on a step's last ratchet only

		// Worst case of the planned events: step n still sounding, n+1 planned, n+2 being planned, each with 16 Tracks
		// of 8 ratchets of a Hit, 24 Locks and an Accent, and a Slide on the last, a note-off for each; and step n-1,
		// whose last note-offs a swing of 80 % and a micro-timing of +23 put 2.56 steps after its place, past the start
		// of n+1.
		constexpr size_t StepCapacity = TrackCount * (MaxRatchets * (LockCount + 2) + SlideNotes) * 2;
		constexpr size_t QueueCapacity = 4 * StepCapacity;

		// Events stand in units of a 42nd of a tick, from the song's start: at every scale a step (60 to 960 ticks)
		// and its 840ths (ratchets' shares, micro-timing's 24ths) are whole numbers of them, so that events compare
		// exactly, and a Pattern that follows another at the end of its Pass starts on a whole unit.
		constexpr int64_t UnitsPerTick = 42;
		constexpr int64_t UnitsPerClock = static_cast<int64_t>(TicksPerClock) * UnitsPerTick;	// 24 to a quarter note
		// The longest Slide, a Pattern's length at the slowest scale and two steps more of swing and micro-timing, fits
		// note 114's velocity.
		static_assert((MaxSteps + 2) * ScaleTicks[0] / TicksPerClock < 127.0 * 127.0, "a Slide's clocks fit two notes");

		// How far from where playing on would bring it a look may find Live and still be playing on (see jumped).
		constexpr double JumpSlackTicks = TicksPerStep / 4.0;
		static_assert(2.0 * JumpSlackTicks <= ScaleTicks[ScaleCount - 1], "a seek of half a step is a jump at 2x");
	}

	uint8_t ratchetVelocity(const uint8_t _velocity, const int _curve, const int _index, const int _count)
	{
		if(_count <= 1 || _curve == 0)
			return _velocity;
		const int span = _count - 1;
		const int away = _curve < 0 ? _index : span - _index;
		const int whole = MaxCurve * span;
		const int scaled = _velocity * (whole - std::abs(_curve) * away);
		return static_cast<uint8_t>(std::max((2 * scaled + whole) / (2 * whole), 1));
	}

	static_assert(MaxSteps <= 127 && PatternCount <= 127 && ChainLength <= 127,
		"a Playhead keeps each of its values in a signed byte");

	Playhead playhead(const Wake& _wake)
	{
		return {_wake.step, _wake.pattern, _wake.queued, _wake.chain};
	}

	uint32_t packPlayhead(const Playhead& _playhead)
	{
		const auto byte = [](const int _value) { return static_cast<uint32_t>(_value) & 0xffu; };
		return byte(_playhead.step) | (byte(_playhead.pattern) << 8) | (byte(_playhead.queued) << 16) |
			(byte(_playhead.chain) << 24);
	}

	Playhead unpackPlayhead(const uint32_t _word)
	{
		const auto value = [_word](const int _shift)
		{
			const auto b = static_cast<int>((_word >> _shift) & 0xffu);
			return b < 128 ? b : b - 256;
		};
		return {value(0), value(8), value(16), value(24)};
	}

	Sequencer::Sequencer(const uint64_t _seedSource) : m_bank(PatternCount), m_seedSource(_seedSource)
	{
		m_queue.reserve(QueueCapacity);
		m_staged.reserve(StepCapacity);
		m_out.reserve(QueueCapacity + TrackCount * NotesPerRatchet);
		applyScale();
	}

	Wake Sequencer::wake(const Transport& _now)
	{
		m_out.clear();
		m_started = m_jumped = m_stopped = false;

		if(!_now.playing)
		{
			if(m_playing)
			{
				silence(_now.ticks);
				m_queue.clear();
				m_sent = 0;
				m_playing = false;
				m_stopped = true;
				// A Pattern waiting for the end of the Pass is the one played next. A chain's is the place after the
				// Pass played: the planning may have begun the next Pass, and queued the place after that.
				if(m_chainLength > 0)
				{
					m_chainPos = playedAt(m_lastTicks).chain;
					m_chainNext = (m_chainPos + 1) % m_chainLength;
					m_queued = m_chain[m_chainNext];
				}
				if(m_queued >= 0)
					setCurrent(m_queued);
			}
			return result(_now.ticks);
		}

		// A new scale changes which step Live's position is: planned afresh from there, as after a jump.
		if(!m_playing)
			restart(_now, Restart::Start);
		else if(jumped(_now))
			restart(_now, Restart::Jump);
		else if(m_rescaled)
			restart(_now, Restart::Rescale);
		m_rescaled = false;

		while(m_nextToPlan < m_planLimit && tickOfStep(m_nextToPlan - 1) <= _now.ticks)
			planNext();

		emitDue(_now.ticks);
		m_lastTicks = _now.ticks;
		m_lastMs = _now.ms;
		return result(_now.ticks);
	}

	bool Sequencer::input(const Input& _in)
	{
		// Read when a step is planned, like a Condition's draw.
		if(_in.kind == Input::Kind::SetFill)
		{
			const bool fill = _in.value != 0;
			if(fill == m_fill)
				return false;
			m_fill = fill;
			return true;
		}
		if(_in.kind == Input::Kind::ChainEntry)
		{
			if(_in.step < 0 || _in.step >= ChainLength || _in.value < 0 || _in.value >= PatternCount ||
				m_chain[_in.step] == _in.value)
				return false;
			m_chain[_in.step] = static_cast<int8_t>(_in.value);
			return true;
		}
		if(_in.kind == Input::Kind::SetChain)
			return setChain(_in.value);
		if(_in.kind == Input::Kind::SetPattern)
		{
			// Chosen by hand: a chain playing ends.
			const bool ended = m_chainLength > 0;
			m_chainLength = 0;
			return choose(_in.value) || ended;
		}

		if(_in.pattern < 0 || _in.pattern >= PatternCount)
			return false;
		if(_in.kind == Input::Kind::CopyPattern || _in.kind == Input::Kind::ClearPattern)
		{
			// Into the Pattern the edit names, as a Pattern the set gives back: played from the step planned ahead if it
			// plays, at its scale.
			const bool copy = _in.kind == Input::Kind::CopyPattern;
			if(copy && (_in.value < 0 || _in.value >= PatternCount))
				return false;
			const Pattern& from = copy ? m_bank[_in.value] : m_empty;
			if(m_bank[_in.pattern] == from)
				return false;
			load(_in.pattern, from);
			return true;
		}
		if(!applyEdit(m_bank[_in.pattern], m_mutes, _in))
			return false;
		// Nothing is planned from a Pattern not playing; a Mute is every Pattern's.
		if(_in.pattern != m_current || _in.kind == Input::Kind::SetMute)
			return true;
		if(_in.kind == Input::Kind::SetScale)
		{
			rescale();
			return true;
		}

		const bool stepEdit = _in.kind != Input::Kind::SetLength && _in.kind != Input::Kind::DoubleLength;
		if(stepEdit && position(m_nextToPlan - 1) == _in.step)
			planAheadAgain();
		return true;
	}

	// load runs on the scheduler thread: a Pattern that held a container would allocate there.
	static_assert(std::is_trivially_copyable_v<Pattern>, "a Pattern copies without allocating");

	void Sequencer::load(const int _index, const Pattern& _pattern)
	{
		if(_index < 0 || _index >= PatternCount)
			return;
		const bool rescaled = _pattern.scale != m_bank[_index].scale;
		m_bank[_index] = _pattern;
		if(_index != m_current)
			return;
		if(rescaled)
			rescale();
		else
			planAheadAgain();
	}

	void Sequencer::prepareRender(const uint64_t _startSeed, const int64_t _steps)
	{
		m_givenSeed = _startSeed;
		m_planLimit = _steps;
	}

	bool Sequencer::choose(const int _pattern)
	{
		if(_pattern < 0 || _pattern >= PatternCount)
			return false;
		if(!m_playing)
		{
			if(_pattern == m_current && m_queued < 0)
				return false;
			setCurrent(_pattern);
			return true;
		}

		// During a Pass's last step, once the next Pass's first step is planned from the Pattern queued, the planning
		// holds that one as current: the Pattern Live plays is the step played's. Choosing it takes the switch back,
		// unless a Track has begun to sound that step: its next Pass is planned again on its own grid, anchored where
		// the switch stood, and counted on from the Pass played.
		const StepPlanned& played = playedAt(m_lastTicks);
		if(played.pattern != m_current && _pattern == played.pattern && tracksBegun() == 0)
		{
			dropPlannedAhead(0);
			setCurrent(played.pattern);
			replanAhead(played.position, played.pass);
			return true;
		}

		// Choosing the Pattern playing takes back the one queued.
		const int queued = _pattern == m_current ? -1 : _pattern;
		if(queued == m_queued)
			return false;
		m_queued = queued;

		// Chosen during a Pass's last step, after the next Pass's first step was planned: that step is planned again,
		// from the Pattern chosen, unless a Track has begun to sound it.
		if(m_queued < 0 || position(m_nextToPlan - 1) != 0 || tracksBegun() != 0)
			return true;
		dropPlannedAhead(0);
		replanAhead(m_positionBeforePlanned, m_passBeforePlanned);
		return true;
	}

	bool Sequencer::setChain(const int _length)
	{
		const int length = _length >= 2 && _length <= ChainLength ? _length : 0;
		if(length == 0 && m_chainLength == 0)
			return false;
		m_chainLength = length;
		m_chainPos = -1;
		m_chainNext = length > 0 ? 0 : -1;
		// Stopped, Live's start finds its place. Playing, it starts at the end of the Pass, at once if its first
		// Pattern is the one planned: its next Pattern queued as a Pattern chosen is, the step planned ahead planned
		// again if it begins the next Pass.
		if(length == 0 || !m_playing)
			return true;
		chainPass();
		choose(m_chain[m_chainNext]);
		return true;
	}

	void Sequencer::chainPass()
	{
		if(m_chainLength == 0)
			return;
		if(m_chainNext >= 0 && m_chain[m_chainNext] == m_current)
			m_chainPos = m_chainNext;
		else if(m_chainPos >= 0 && m_chain[m_chainPos] != m_current)
			m_chainPos = -1;
		m_chainNext = (m_chainPos + 1) % m_chainLength;
	}

	int Sequencer::chainQueued() const
	{
		if(m_chainNext < 0)
			return -1;
		const int next = m_chain[m_chainNext];
		return next == m_current ? -1 : next;
	}

	void Sequencer::setCurrent(const int _pattern)
	{
		m_current = _pattern;
		m_queued = -1;
		applyScale();
	}

	void Sequencer::dropPlannedAhead(const uint16_t _keep)
	{
		// The step planned ahead's events, but for the Tracks kept, and the Conditions' memory from before it.
		const int64_t planned = m_nextToPlan - 1;
		const auto dropped = [&](const Planned& _p)
		{
			return _p.step == planned && _p.pattern == m_current && !(_keep & 1u << _p.track);
		};
		const auto unsent = m_queue.begin() + static_cast<std::ptrdiff_t>(m_sent);
		m_queue.erase(std::remove_if(unsent, m_queue.end(), dropped), m_queue.end());
		m_memory = m_memoryBeforePlanned;
	}

	void Sequencer::replanAhead(const int _position, const uint32_t _pass)
	{
		// The planning goes back to the step played, at its position and Pass, and plans the next step again.
		m_lastPlannedPosition = _position;
		m_lastPlannedPass = _pass;
		m_planned[1] = m_planned[0];
		--m_nextToPlan;
		planNext();
	}

	void Sequencer::switchTo(const int _pattern, const double _anchor)
	{
		setCurrent(_pattern);
		m_anchor = _anchor;
		m_nextToPlan = 0;
		m_lastPlannedPosition = -1;	// its first Pass
	}

	void Sequencer::applyScale()
	{
		const double ticks = ScaleTicks[m_bank[m_current].scale];
		m_ticksPerStep = ticks;
		m_stepUnits = static_cast<int64_t>(ticks) * UnitsPerTick;
		m_subUnits = m_stepUnits / SubticksPerStep;
	}

	void Sequencer::rescale()
	{
		applyScale();
		// The Pattern queued, current for the planning since the next Pass's first step was planned from it: that step
		// alone stands on its scale, anchored where the Pass Live plays ends whatever its length. It is planned again,
		// and the step Live plays keeps its own grid.
		if(m_playing && playedAt(m_lastTicks).pattern != m_current)
		{
			planAheadAgain();
			return;
		}
		// The steps planned stand in the old length: the next wake plans afresh where Live is.
		m_rescaled = m_playing;
	}

	uint16_t Sequencer::tracksBegun() const
	{
		uint16_t begun = 0;
		for(int t = 0; t < TrackCount; ++t)
			if(m_firstOfPlanned[t] != NoUnit && tickOf(m_firstOfPlanned[t]) <= m_lastTicks)
				begun = static_cast<uint16_t>(begun | 1u << t);
		return begun;
	}

	void Sequencer::planAheadAgain()
	{
		// The step planned ahead is planned again, so that an edit just before a step is heard on it, not a Pass later;
		// the Conditions draw again from the memory they had before it, so NEI and PRE stay right. A Track that has
		// begun to sound it (micro-timing moves a step's Hits up to 23/24 of a step earlier) keeps what it plays: no
		// Hit is played twice, nor late.
		if(!m_playing)
			return;
		const uint16_t begun = tracksBegun();
		dropPlannedAhead(begun);
		planStep(m_nextToPlan - 1, m_drawOfPlanned, std::nextafter(m_lastTicks, Never), begun);
	}

	double Sequencer::tickOfStep(const int64_t _step) const
	{
		return m_anchor + static_cast<double>(_step) * m_ticksPerStep;
	}

	int64_t Sequencer::stepAt(const double _ticks) const
	{
		return static_cast<int64_t>(std::floor((_ticks - m_anchor) / m_ticksPerStep));
	}

	int Sequencer::position(const int64_t _step) const
	{
		const int64_t length = m_bank[m_current].length;
		return static_cast<int>(((_step % length) + length) % length);
	}

	bool Sequencer::jumped(const Transport& _now) const
	{
		// Where Live would be had it played on since the last look, at the tempo of this one: over the millisecond
		// between two looks, a tempo change shifts it by a fraction of a tick.
		const double playedOn = m_lastTicks + (_now.ms - m_lastMs) * _now.ticksPerMs;

		// The slack absorbs the looks' jitter, a vector of 1.45 ms: 12 ticks at Live's top tempo. Past it, a move is a
		// jump. Playing on through one sends the events it skips late, at once (forward), or keeps silent while Live
		// plays a stretch again (back), for good when a loop brace is no longer than the slack, since Live never comes
		// back to where the Sequencer waits; planning again ends the notes sounding, drops the Hits of the step begun
		// before the grace and, backwards, starts a new Pass. So the slack is a 64th, 30 ticks, at every scale: half
		// the shortest step, 2x's 32nd. A loop brace of a 32nd, which Live's grid offers, is then a jump whatever the
		// scale, and so is a seek of half a step at 2x. Half a 16th at the slower scales let that loop brace play on,
		// for good; half a step at every scale was a beat at 1/8x.
		return std::fabs(_now.ticks - playedOn) > JumpSlackTicks;
	}

	void Sequencer::restart(const Transport& _now, const Restart _why)
	{
		// A jump ends the notes sounding, and so does a new scale, whose note-offs stood on the old grid; a start has
		// none.
		m_started = _why == Restart::Start;
		m_jumped = _why == Restart::Jump;
		m_jumpedFrom = m_lastTicks;
		silence(_now.ticks);
		m_queue.clear();
		m_sent = 0;

		if(_why == Restart::Start)
		{
			// Anchored at the song's start (Pattern semantics).
			m_anchor = 0.0;
			m_lastPlannedPosition = -1;
			m_lastPlannedPass = 0;
			m_planned[0] = m_planned[1] = {-Never, m_current, 0, 1, -1};
			// A start draws afresh: its own seed, and Conditions that remember nothing.
			m_startSeed = rendering() ? m_givenSeed : passSeed(m_seedSource, ++m_starts);
			m_memory = {};
			// A chain goes on from the first place holding the Pattern played, from the one Live stopped before (the
			// first if none holds it).
			if(m_chainLength > 0 && (m_chainNext < 0 || m_chain[m_chainNext] != m_current))
			{
				const int from = std::max(m_chainNext, 0);
				m_chainNext = -1;
				for(int i = 0; i < m_chainLength && m_chainNext < 0; ++i)
					if(m_chain[(from + i) % m_chainLength] == m_current)
						m_chainNext = (from + i) % m_chainLength;
			}
		}

		// The step Live lands in goes on in the Pass played: a Pattern queued waits for that Pass's end.
		bool within = false;
		if(_why != Restart::Start)
		{
			// The Pass of the step played, as it was planned, whatever the grid now: the step planned ahead may stand
			// at the same position (a Pattern of one step), and a new scale moves the positions. Every step planned
			// has a Pass from 1, so the one taken back below never wraps: plan counts a new Pass from the one before.
			const StepPlanned& played = playedAt(m_lastTicks);
			m_lastPlannedPass = played.pass;
			if(played.pattern != m_current)
			{
				// During a Pass's last step, the next Pass's first is planned from the Pattern queued, which the
				// planning holds as current already. Live has not reached it: the switch is undone, so that the step
				// played stands on its own Pattern's grid, and the Pattern queued follows again at the end of the Pass
				// Live plays now (a Pattern chosen since then instead). The switch's tick is a step 0 of that grid too,
				// so the anchor stays; the draws of the step never played are forgotten.
				const int queued = m_queued >= 0 ? m_queued : m_current;
				setCurrent(played.pattern);
				m_queued = queued == m_current ? -1 : queued;
				m_memory = m_memoryBeforePlanned;
			}
			// A new scale moves Live's step on the grid, not its Pass: the step it is in now plays on in the Pass
			// played, wherever the new grid puts it. For a jump, the step played, on the grid the Pattern has now (a new
			// scale may come with the jump), decides whether it starts a Pass: landing on it or before it does, but a
			// jump forward within it plays on in its Pass.
			const int64_t was = stepAt(m_lastTicks);
			within = _why == Restart::Rescale || (_now.ticks > m_lastTicks && stepAt(_now.ticks) == was);
			m_lastPlannedPosition = position(_why == Restart::Rescale ? stepAt(_now.ticks) : was);
			if(within)
				--m_lastPlannedPass;
			// The chain goes back to the Pass played too: planned again, it keeps its place; a new Pass takes the next.
			if(m_chainLength > 0)
			{
				m_chainPos = played.chain;
				m_chainNext = within ? played.chain : (played.chain + 1) % m_chainLength;
				m_queued = chainQueued();
			}
		}

		m_playing = true;
		// The events of the step reached that fall within the grace before the position go out late rather than not at
		// all: the look came after the move. A new scale is no move of Live's, and the last look sent what was due up
		// to it: as after an edit, nothing at or before that look goes out again, which would strike a Hit twice where
		// the two grids meet.
		const double grace = rendering() ? Never : GraceMs * _now.ticksPerMs;
		m_earliest = _why == Restart::Rescale ? std::nextafter(m_lastTicks, Never) : _now.ticks - grace;
		m_nextToPlan = stepAt(_now.ticks);
		if(within)
			plan(m_nextToPlan++);
		else
			planNext();
	}

	void Sequencer::planNext()
	{
		// A Pattern queued follows where the Pass of the one playing ends: on its own grid from there, its Passes
		// counted from 1 (Pattern semantics).
		if(m_queued >= 0 && position(m_nextToPlan) == 0)
			switchTo(m_queued, tickOfStep(m_nextToPlan));
		plan(m_nextToPlan++);
	}

	void Sequencer::plan(const int64_t _step)
	{
		const int pos = position(_step);
		m_positionBeforePlanned = m_lastPlannedPosition;
		m_passBeforePlanned = m_lastPlannedPass;
		const bool begins = m_lastPlannedPosition < 0 || pos <= m_lastPlannedPosition;
		const uint32_t pass = m_lastPlannedPosition < 0 ? 1 : begins ? m_lastPlannedPass + 1 : m_lastPlannedPass;
		m_lastPlannedPosition = pos;
		m_lastPlannedPass = pass;
		// A chain queues its next Pattern as the Pass begins, so that it follows at the Pass's end whatever its
		// length: a Pass of one step is one step.
		if(begins && m_chainLength > 0)
		{
			chainPass();
			m_queued = chainQueued();
		}
		m_planned[0] = m_planned[1];
		m_planned[1] = {tickOfStep(_step), m_current, pos, pass, m_chainLength > 0 ? m_chainPos : -1};

		m_memoryBeforePlanned = m_memory;
		m_drawOfPlanned = {pass, passSeed(m_startSeed, pass), m_fill};
		planStep(_step, m_drawOfPlanned, m_earliest, 0);
	}

	void Sequencer::planStep(const int64_t _step, const Draw& _draw, const double _earliest, const uint16_t _keep)
	{
		const Pattern& pattern = m_bank[m_current];
		const int pos = position(_step);
		const int64_t grid = gridOf(_step);
		const auto earliest = std::isinf(_earliest) ? (std::numeric_limits<int64_t>::min)()
			: static_cast<int64_t>(std::ceil(_earliest * UnitsPerTick));

		// The Tracks that play the step, drawn from 1 to 16 (NEI reads the Track before).
		std::array<Playing, TrackCount> playing;
		int count = 0;
		for(int t = 0; t < TrackCount; ++t)
		{
			// A Track kept as it was planned gives NEI and PRE the result it plays with.
			if(_keep & 1u << t)
			{
				m_memory.last[t] = m_lastOfPlanned[t];
				continue;
			}
			const Step& step = pattern.steps[t][pos];
			// The Condition is drawn whatever happens next, so that its memory follows every step, muted or not.
			const bool plays = step.hit && evaluate(step.condition, t, pos, _draw, m_memory);
			m_lastOfPlanned[t] = m_memory.last[t];
			m_firstOfPlanned[t] = NoUnit;
			if(!plays)
				continue;

			// Swing and micro-timing move the whole group of ratchets, the swing first, as on the machine. On a step
			// that begins at or after the earliest tick (one a start, a jump or an edit reaches), the Hits it would
			// have sent before go at once; on a step begun before it (a jump inside it), the ratchets before it are
			// dropped.
			const int64_t share = SubticksPerStep / step.ratchets * m_subUnits;
			int64_t begin = groupBegin(_step, t);
			int first = 0;
			if(grid >= earliest)
				begin = std::max(begin, earliest);
			else
				while(first < step.ratchets && begin + share * first < earliest)
					++first;
			if(first == step.ratchets)
				continue;
			m_firstOfPlanned[t] = begin + share * first;
			playing[count++] = {t, begin, share, first, step.ratchets, begin + share * first};
		}

		// The events in their order, without a sort: from one unit where a ratchet begins or ends to the next, the
		// note-offs of the ratchets ending, Track by Track, then the notes of those beginning. A step holds up to 7488
		// events and the queue three steps of them: an insertion each took 9.6 ms in the heaviest wake of 6400, a sort
		// 0.38 ms a step. Each note of the step has its id in a block of them, by Track, ratchet and parameter.
		m_staged.clear();
		const uint64_t notes = m_nextNote;
		m_nextNote += TrackCount * MaxRatchets * NotesPerRatchet;
		for(;;)
		{
			int64_t k = NoUnit;
			for(int i = 0; i < count; ++i)
				k = std::min(k, playing[i].next);
			if(k == NoUnit)
				break;
			for(int i = 0; i < count; ++i)
			{
				const Playing& p = playing[i];
				const auto r = static_cast<int>((k - p.begin) / p.share);
				if(p.next == k && r > p.first)
					stageRatchet(_step, k, p.track, r - 1, notes, false);
			}
			for(int i = 0; i < count; ++i)
			{
				Playing& p = playing[i];
				if(p.next != k)
					continue;
				const auto r = static_cast<int>((k - p.begin) / p.share);
				if(r < p.count)
					stageRatchet(_step, k, p.track, r, notes, true);
				p.next = r < p.count ? k + p.share : NoUnit;
			}
		}
		assert(std::is_sorted(m_staged.begin(), m_staged.end(), before));

		// The events sent leave the queue once a step, not at each wake. Then one merge for the step, from the back,
		// so that the queue's events stay before the staged ones they equal, as an insertion after them would leave
		// them.
		m_queue.erase(m_queue.begin(), m_queue.begin() + static_cast<std::ptrdiff_t>(m_sent));
		m_sent = 0;
		const size_t queued = m_queue.size();
		assert(queued + m_staged.size() <= m_queue.capacity());
		m_queue.resize(queued + m_staged.size());
		auto q = m_queue.begin() + static_cast<std::ptrdiff_t>(queued);
		auto s = m_staged.end();
		auto out = m_queue.end();
		while(s != m_staged.begin())
		{
			if(q != m_queue.begin() && before(*(s - 1), *(q - 1)))
				*--out = *--q;
			else
				*--out = *--s;
		}
	}

	void Sequencer::stageRatchet(const int64_t _step, const int64_t _at, const int _track, const int _ratchet,
		const uint64_t _notes, const bool _begins)
	{
		// A ratchet is the Hit with each of its Locks, on a step of the Accent row the Accent, and on a step of the Slide
		// row the Slide with the last ratchet, a note each that lasts the ratchet's share of the step (MD Drums' ADR
		// 0003: a Lock, the Accent and the Slide go with the Hit just before them). It begins with the Hit, then the
		// others; it ends with the others, then the Hit, in rankOf's order.
		const Pattern& played = m_bank[m_current];
		const int at = position(_step);
		const Step& step = played.steps[_track][at];
		const auto track = static_cast<uint8_t>(_track);
		const auto pattern = static_cast<uint8_t>(m_current);
		const uint64_t notes = _notes + static_cast<uint64_t>((_track * MaxRatchets + _ratchet) * NotesPerRatchet);
		const uint64_t hitNote = notes + NotesPerRatchet - 1;
		const auto stage = [&](const Kind _kind, const uint8_t _param, const uint8_t _velocity, const uint64_t _note)
		{
			assert(m_staged.size() < m_staged.capacity());
			m_staged.push_back({_at, _step, rankOf(_kind, track, _param), _kind, track, _param, _velocity, pattern,
				_note});
		};
		const auto note = [&](const int _param, const uint8_t _velocity)
		{
			const auto param = static_cast<uint8_t>(_param);
			stage(_begins ? Kind::Lock : Kind::Off, param, _begins ? _velocity : 0, notes + param);
		};

		if(_begins)
			stage(Kind::Hit, HitNote, ratchetVelocity(step.velocity, step.curve, _ratchet, step.ratchets), hitNote);
		for(int p = 0; p < LockCount; ++p)
			if(step.locks[p] != NoLock)
				note(p, lockVelocity(step.locks[p]));
		if(played.inAccentRow(at))
			note(AccentParam, lockVelocity(played.accent));
		if(played.inSlideRow(at) && _ratchet == step.ratchets - 1)
		{
			// The Track's next Hit in the Pattern, whatever its Condition, around its end; the Hit itself when it is the
			// only one. The clocks from this ratchet to it as both play, at least one, past 127 in a second note.
			int distance = 1;
			while(distance < played.length && !played.steps[_track][position(_step + distance)].hit)
				++distance;
			const Step& next = played.steps[_track][position(_step + distance)];
			const int64_t from = groupBegin(_step, _track) + SubticksPerStep / step.ratchets * m_subUnits * _ratchet;
			const int64_t clocks = std::max<int64_t>(
				(groupBegin(_step + distance, _track) - from + UnitsPerClock / 2) / UnitsPerClock, 1);
			const auto longer = static_cast<int>((clocks - 1) / 127);
			note(SlideParam, static_cast<uint8_t>(clocks - 127 * longer));
			for(int p = 0; p < LockCount; ++p)
				if(step.locks[p] != NoLock && next.locks[p] != NoLock)
					note(FirstTargetParam + p, lockVelocity(next.locks[p]));
			if(longer > 0)
				note(LongSlideParam, static_cast<uint8_t>(longer));
		}
		if(!_begins)
			stage(Kind::Off, HitNote, 0, hitNote);
	}

	int64_t Sequencer::gridOf(const int64_t _step) const
	{
		return std::llround(m_anchor * UnitsPerTick) + _step * m_stepUnits;
	}

	int64_t Sequencer::groupBegin(const int64_t _step, const int _track) const
	{
		const Pattern& pattern = m_bank[m_current];
		const int pos = position(_step);
		const int64_t swing = pattern.swings(pos) ? swingSubticks(pattern.swing) : 0;
		return gridOf(_step) + (pattern.steps[_track][pos].micro * SubticksPerMicro + swing) * m_subUnits;
	}

	double Sequencer::tickOf(const int64_t _at) const
	{
		return static_cast<double>(_at) / static_cast<double>(UnitsPerTick);
	}

	uint16_t Sequencer::rankOf(const Kind _kind, const uint8_t _track, const uint8_t _param)
	{
		// At one unit: note-offs first, then Track by Track each Hit followed by its Locks: Live brings a device's
		// notes to channel 1, where MD Drums binds a Lock to the last Hit before it at that sample (its ADR 0003 as
		// amended on 2026-10-11). Note-offs keep their parameter's order, the Hit's last.
		const unsigned on = _kind == Kind::Off ? 0u : 1u;
		const unsigned lock = _kind == Kind::Lock ? 1u : 0u;
		return static_cast<uint16_t>(on << 13 | static_cast<unsigned>(_track) << 9 | lock << 8 | _param);
	}

	uint64_t& Sequencer::sounding(const uint8_t _track, const uint8_t _param)
	{
		return _param == HitNote ? m_sounding[_track] : m_lockSounding[_track][_param];
	}

	uint8_t Sequencer::key(const uint8_t _track, const uint8_t _param)
	{
		return static_cast<uint8_t>(_param == HitNote ? FirstHitNote + _track : FirstLockNote + _param);
	}

	void Sequencer::emitDue(const double _now)
	{
		for(; m_sent < m_queue.size() && tickOf(m_queue[m_sent].at) <= _now; ++m_sent)
			emit(m_queue[m_sent]);
	}

	void Sequencer::emit(const Planned& _event)
	{
		const uint8_t channel = _event.track;
		const uint8_t note = key(_event.track, _event.param);
		uint64_t& playing = sounding(_event.track, _event.param);

		if(_event.kind == Kind::Off)
		{
			// A note dropped by a Mute, or cut short by a later one, has no note-off of its own to send.
			if(playing != _event.note)
				return;
			out(tickOf(_event.at), noteStatus(NoteOff, channel), note, 0);
			playing = 0;
			return;
		}

		if(m_mutes & (1u << _event.track))
			return;

		// The same note again before its note-off: end it first, as a clip would.
		const double tick = tickOf(_event.at);
		if(playing)
			out(tick, noteStatus(NoteOff, channel), note, 0);
		out(tick, noteStatus(NoteOn, channel), note, _event.velocity);
		// Played live, a Lock ends at once, its planned note-off dropped: MD Drums reads a Lock at its note-on, and Live
		// keeps the notes a Max device holds in a set of 128, which a note-on overflowing it makes Live quit (issue
		// 24). Printed, it keeps its length, which a clip's note needs.
		if(_event.kind == Kind::Lock && !rendering())
		{
			out(tick, noteStatus(NoteOff, channel), note, 0);
			playing = 0;
			return;
		}
		playing = _event.note;
	}

	void Sequencer::silence(const double _now)
	{
		for(uint8_t t = 0; t < TrackCount; ++t)
		{
			for(uint8_t p = 0; p < ParamCount; ++p)
			{
				if(!m_lockSounding[t][p])
					continue;
				out(_now, noteStatus(NoteOff, t), key(t, p), 0);
				m_lockSounding[t][p] = 0;
			}
			if(!m_sounding[t])
				continue;
			out(_now, noteStatus(NoteOff, t), key(t, HitNote), 0);
			m_sounding[t] = 0;
		}
	}

	void Sequencer::out(const double _tick, const uint8_t _status, const uint8_t _data1, const uint8_t _data2)
	{
		assert(m_out.size() < m_out.capacity());
		m_out.push_back({_tick, {_status, _data1, _data2}});
	}

	const Sequencer::StepPlanned& Sequencer::playedAt(const double _ticks) const
	{
		// The last planned that has begun: the step planned ahead, once Live reaches it.
		return m_planned[1].tick <= _ticks ? m_planned[1] : m_planned[0];
	}

	Wake Sequencer::result(const double _ticks) const
	{
		Wake wake{m_out.data(), m_out.data() + m_out.size(), Never, -1, 0, m_current, m_queued};
		wake.started = m_started;
		wake.jumped = m_jumped;
		wake.jumpedFrom = m_jumpedFrom;
		wake.stopped = m_stopped;
		if(!m_playing)
		{
			// The place a start goes on from, when it holds the Pattern played.
			if(m_chainLength > 0 && m_chainNext >= 0 && m_chain[m_chainNext] == m_current)
				wake.chain = m_chainNext;
			return wake;
		}

		const StepPlanned& played = playedAt(_ticks);
		wake.step = played.position;
		wake.pass = played.pass;
		wake.pattern = played.pattern;
		wake.chain = m_chainLength > 0 ? played.chain : -1;
		// The Pass's last step plays, and the next Pass's first is planned from the Pattern queued, which the planning
		// no longer holds as queued: it still follows at the end of this Pass.
		if(m_planned[1].tick > _ticks && m_planned[1].pattern != played.pattern)
			wake.queued = m_planned[1].pattern;

		if(m_nextToPlan < m_planLimit)
			wake.next = tickOfStep(m_nextToPlan - 1);
		if(m_sent < m_queue.size())
			wake.next = std::min(wake.next, tickOf(m_queue[m_sent].at));
		return wake;
	}
}
