#include "mdseqPrint.h"

#include "mdseqSequencer.h"

#include <algorithm>
#include <memory>

namespace mdseq
{
	size_t Printed::trackCount() const
	{
		return static_cast<size_t>(std::count_if(tracks.begin(), tracks.end(), [](const std::vector<PrintedNote>& _t)
		{
			return !_t.empty();
		}));
	}

	Printed printPattern(const Pattern& _pattern, const int _passes, const uint64_t _startSeed, const bool _fill)
	{
		Printed printed;
		const int passes = std::clamp(_passes, 1, MaxPasses);
		auto seq = std::make_unique<Sequencer>();
		seq->load(0, _pattern);
		if(_fill)
			seq->input({Input::Kind::SetFill, 0, 0, 1});
		seq->prepareRender(_startSeed, static_cast<int64_t>(passes) * _pattern.length);

		const double clipTicks = static_cast<double>(passes) * _pattern.length * seq->ticksPerStep();
		printed.length = clipTicks / TicksPerQuarter;

		// Notes open per Track and key, until their note-off.
		struct Open
		{
			double tick;
			int velocity;
			bool on;
		};
		std::array<std::array<Open, 128>, TrackCount> open{};
		const auto close = [&](const int _track, const int _key, const double _tick)
		{
			Open& o = open[_track][_key];
			if(!o.on)
				return;
			o.on = false;
			double start = o.tick;
			double end = _tick;
			// An early Hit of the first step: at the clip's end, so that it sounds before each repeat. A Hit of the last
			// step that swing and micro-timing put past the clip's end: at its start, as the repeat plays it.
			if(start < 0.0)
			{
				start += clipTicks;
				end += clipTicks;
			}
			else if(start >= clipTicks)
			{
				start -= clipTicks;
				end -= clipTicks;
			}
			end = std::min(end, clipTicks);
			if(end <= start)
				return;
			printed.tracks[_track].push_back({_key, start / TicksPerQuarter, (end - start) / TicksPerQuarter,
				o.velocity});
		};

		// In virtual time where a millisecond is worth a tick, so that every wake reads as playing on; from tick 0,
		// where a render's infinite grace plays step 0's early Hits before it, as on a repeat, not at it.
		const auto at = [](const double _ticks) { return Transport{_ticks, true, _ticks, 1.0}; };
		Wake w = seq->wake(at(0.0));
		for(;;)
		{
			for(const MidiEvent* e = w.begin; e != w.end; ++e)
			{
				const int track = trackOfStatus(e->bytes[0]);
				const int key = e->bytes[1] & 0x7f;
				if(isNoteOn(e->bytes[0], e->bytes[2]))
				{
					close(track, key, e->tick);
					open[track][key] = {e->tick, e->bytes[2], true};
				}
				else
					close(track, key, e->tick);
			}
			if(w.next == Never)
				break;
			w = seq->wake(at(w.next));
		}

		// Notes by start within each clip, a Hit (36-51) before its Locks (64-87).
		for(std::vector<PrintedNote>& notes : printed.tracks)
			std::sort(notes.begin(), notes.end(), [](const PrintedNote& _a, const PrintedNote& _b)
			{
				return _a.start != _b.start ? _a.start < _b.start : _a.pitch < _b.pitch;
			});
		return printed;
	}
}
