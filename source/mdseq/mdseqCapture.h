#pragma once

// Capturing a performance (issue 23): the device journals every note it sends while the transport runs, in a ring of
// fixed size that the scheduler thread writes without allocating, and a capture turns the last take into a clip per
// Track on the main thread, placed as a print's (mdseqPrinter.h).

#include "mdseqKit.h"
#include "mdseqPrint.h"
#include "mdseqTypes.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace mdseq
{
	constexpr double BarTicks = 4.0 * TicksPerQuarter;	// a bar of 4/4

	// A note as the device sent it, at its take time: the ticks played since the bar line before the take's start, the
	// stretches between Live's jumps joined end to end.
	struct JournalEntry
	{
		double ticks;
		MidiBytes bytes;
	};

	struct Take
	{
		std::vector<JournalEntry> entries;	// in the order sent
		double length = 0.0;				// in ticks of take time: up to the stop, or to the last wake while playing
		bool cut = false;					// the ring had overwritten its beginning
	};

	class Journal
	{
	public:
		static constexpr size_t DefaultCapacity = size_t{1} << 20;	// about a million notes, 16 MB

		explicit Journal(size_t _capacity = DefaultCapacity);

		// Scheduler thread. A take begins where the transport starts; a jump joins what follows to what came before;
		// played moves the take's end with the transport; a stop ends it. The notes between, at the song's ticks.
		void start(double _ticks);
		void jump(double _from, double _to);
		void played(double _ticks);
		void note(double _ticks, const MidiBytes& _bytes);
		void stop(double _ticks);

		// Main thread: the last take, or the one playing, as much of it as the ring holds; and without copying it,
		// whether it holds a note and how long it is.
		Take take() const;
		bool hasNotes() const;
		double length() const;

	private:
		// One entry in two words that both threads touch atomically: the ticks' bits, then the three bytes.
		struct Slot
		{
			std::atomic<uint64_t> ticks{0};
			std::atomic<uint32_t> bytes{0};
		};
		static_assert(std::is_trivially_destructible_v<Slot>, "the ring frees its storage without destroying slots");
		struct FreeStorage
		{
			void operator()(Slot* _slots) const { ::operator delete(_slots); }
		};

		double takeTime(double _ticks) const { return m_offset + (_ticks - m_segment); }

		size_t m_capacity;
		// Storage only: each slot is built as the ring's first lap writes it, and read only once written. Built whole,
		// a ring of a million touched 16 MB per device as it loaded, most of it never played.
		std::unique_ptr<Slot, FreeStorage> m_slots;
		std::atomic<uint64_t> m_written{0};		// entries ever written; the ring holds the last m_capacity
		std::atomic<uint64_t> m_takeStart{0};	// the entry the take begins at
		std::atomic<double> m_length{0.0};
		// Scheduler thread only: whether a take runs, where its current stretch began in the song and in take time.
		bool m_running = false;
		double m_segment = 0.0;
		double m_offset = 0.0;
	};

	// A take's notes as clips, one per Track (its channel), in beats from the take's first bar line, the clip as long
	// as the whole bars the take covers. A Lock, the Accent or the Slide, which the device ends at once when it plays
	// live (issue 24), lasts as the Hit it goes with, as printed. A note still sounding at the take's end ends there;
	// one sent before the first bar line starts on it.
	Printed takeClips(const Take& _take);

	// The strip's words for a take of _length ticks: "Take 12 bars", "No take" when it holds no note.
	std::string takeText(bool _notes, double _length);
}
