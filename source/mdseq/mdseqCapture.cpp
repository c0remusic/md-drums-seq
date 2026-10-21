#include "mdseqCapture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <utility>

namespace mdseq
{
	namespace
	{
		uint64_t ticksBits(const double _ticks)
		{
			uint64_t bits = 0;
			std::memcpy(&bits, &_ticks, sizeof(bits));
			return bits;
		}

		double bitsTicks(const uint64_t _bits)
		{
			double ticks = 0.0;
			std::memcpy(&ticks, &_bits, sizeof(ticks));
			return ticks;
		}

		uint32_t packBytes(const MidiBytes& _bytes)
		{
			return static_cast<uint32_t>(_bytes[0]) | static_cast<uint32_t>(_bytes[1]) << 8 |
				static_cast<uint32_t>(_bytes[2]) << 16;
		}

		MidiBytes unpackBytes(const uint32_t _word)
		{
			return {static_cast<uint8_t>(_word), static_cast<uint8_t>(_word >> 8), static_cast<uint8_t>(_word >> 16)};
		}

		bool isHit(const int _key)
		{
			return _key >= FirstHitNote && _key < FirstHitNote + TrackCount;
		}

		// Whole bars, at least one.
		int barsOf(const double _ticks)
		{
			return std::max(1, static_cast<int>(std::ceil(_ticks / BarTicks - 1e-9)));
		}
	}

	Journal::Journal(const size_t _capacity) : m_capacity(std::max<size_t>(_capacity, 1)),
		m_slots(static_cast<Slot*>(::operator new(sizeof(Slot) * m_capacity)))
	{
	}

	void Journal::start(const double _ticks)
	{
		// Take time from the bar line at or before the start, so that a take's clip keeps the song's bars.
		m_segment = _ticks;
		m_offset = _ticks - std::floor(_ticks / BarTicks) * BarTicks;
		m_running = true;
		m_takeStart.store(m_written.load(std::memory_order_relaxed), std::memory_order_release);
		m_length.store(m_offset, std::memory_order_release);
	}

	void Journal::jump(const double _from, const double _to)
	{
		if(!m_running)
			return;
		m_offset = takeTime(_from);
		m_segment = _to;
	}

	void Journal::played(const double _ticks)
	{
		if(m_running)
			m_length.store(std::max(m_length.load(std::memory_order_relaxed), takeTime(_ticks)), std::memory_order_release);
	}

	void Journal::note(const double _ticks, const MidiBytes& _bytes)
	{
		if(!m_running)
			return;
		const uint64_t n = m_written.load(std::memory_order_relaxed);
		Slot* const at = m_slots.get() + n % m_capacity;
		Slot& slot = n < m_capacity ? *new(at) Slot : *at;
		slot.ticks.store(ticksBits(takeTime(_ticks)), std::memory_order_relaxed);
		slot.bytes.store(packBytes(_bytes), std::memory_order_relaxed);
		m_written.store(n + 1, std::memory_order_release);
	}

	void Journal::stop(const double _ticks)
	{
		if(!m_running)
			return;
		played(_ticks);
		m_running = false;
	}

	Take Journal::take() const
	{
		Take take;
		// Copied while the scheduler thread may write on: entries it overwrote meanwhile are dropped, and a take that
		// began meanwhile is read again.
		for(int attempt = 0; attempt < 3; ++attempt)
		{
			take = Take();
			const uint64_t begin = m_takeStart.load(std::memory_order_acquire);
			const uint64_t end = m_written.load(std::memory_order_acquire);
			uint64_t from = begin;
			if(end - from > m_capacity)
			{
				from = end - m_capacity;
				take.cut = true;
			}
			take.entries.reserve(static_cast<size_t>(end - from));
			for(uint64_t i = from; i < end; ++i)
			{
				const Slot& slot = m_slots.get()[i % m_capacity];
				take.entries.push_back({bitsTicks(slot.ticks.load(std::memory_order_relaxed)),
					unpackBytes(slot.bytes.load(std::memory_order_relaxed))});
			}
			const uint64_t written = m_written.load(std::memory_order_acquire);
			if(written > m_capacity && written - m_capacity > from)
			{
				const auto lost = static_cast<size_t>(std::min<uint64_t>(written - m_capacity - from,
					take.entries.size()));
				take.entries.erase(take.entries.begin(), take.entries.begin() + static_cast<std::ptrdiff_t>(lost));
				take.cut = true;
			}
			take.length = m_length.load(std::memory_order_acquire);
			if(m_takeStart.load(std::memory_order_acquire) == begin)
				break;
		}
		return take;
	}

	Printed takeClips(const Take& _take)
	{
		struct Open
		{
			double start;
			int velocity;
			bool on;
		};
		struct Note
		{
			int track;
			int key;
			double start;
			double end;
			int velocity;
		};
		std::array<std::array<Open, 128>, TrackCount> open{};
		std::vector<Note> notes;
		const auto close = [&](const int _track, const int _key, const double _end)
		{
			Open& o = open[static_cast<size_t>(_track)][static_cast<size_t>(_key)];
			if(!o.on)
				return;
			o.on = false;
			notes.push_back({_track, _key, o.start, std::max(_end, o.start), o.velocity});
		};
		for(const JournalEntry& e : _take.entries)
		{
			const int track = trackOfStatus(e.bytes[0]);
			const int key = e.bytes[1] & 0x7f;
			const int kind = e.bytes[0] & 0xf0;
			if(kind != NoteOn && kind != NoteOff)
				continue;
			close(track, key, e.ticks);
			if(isNoteOn(e.bytes[0], e.bytes[2]))
				open[static_cast<size_t>(track)][static_cast<size_t>(key)] = {e.ticks, e.bytes[2], true};
		}
		for(int t = 0; t < TrackCount; ++t)
			for(int k = 0; k < 128; ++k)
				close(t, k, _take.length);

		// Each Track's Hits by their start, for the notes that go with them.
		std::array<std::vector<std::pair<double, double>>, TrackCount> hits;
		for(const Note& n : notes)
			if(isHit(n.key))
				hits[static_cast<size_t>(n.track)].push_back({n.start, n.end});
		for(auto& h : hits)
			std::sort(h.begin(), h.end());

		Printed printed;
		const int bars = barsOf(_take.length);
		printed.length = bars * BarTicks / TicksPerQuarter;
		for(const Note& n : notes)
		{
			double end = n.end;
			// A note the device ended at once lasts as the Hit of its Track that began with it, a step without one.
			if(end == n.start && !isHit(n.key))
			{
				const auto& h = hits[static_cast<size_t>(n.track)];
				const auto it = std::lower_bound(h.begin(), h.end(), std::make_pair(n.start, -1e300));
				end = it != h.end() && it->first == n.start ? it->second : n.start + TicksPerStep;
			}
			const double start = std::max(n.start, 0.0);
			if(end <= start)
				continue;
			printed.tracks[static_cast<size_t>(n.track)].push_back({n.key, start / TicksPerQuarter,
				(end - start) / TicksPerQuarter, n.velocity});
		}
		sortNotes(printed);
		return printed;
	}

	bool Journal::hasNotes() const
	{
		return m_written.load(std::memory_order_acquire) > m_takeStart.load(std::memory_order_acquire);
	}

	double Journal::length() const
	{
		return m_length.load(std::memory_order_acquire);
	}

	std::string takeText(const bool _notes, const double _length)
	{
		if(!_notes)
			return "No take";
		const int bars = barsOf(_length);
		char text[32];
		std::snprintf(text, sizeof(text), "Take %d bar%s", bars, bars > 1 ? "s" : "");
		return text;
	}
}
