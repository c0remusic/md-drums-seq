#pragma once

// The Kit as MD Drums tells it (ADR 0003): each Track's Machine, the names the Machine gives SYN1-8 and the Kit's
// values of the Track's 24 parameters, read from the file MD Drums writes for the device's id, which the device
// gives it in a hello: four notes in the MIDI it already sends. Main thread; it may allocate.

#include "mdseqTypes.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace mdseq
{
	struct KitTrack
	{
		int machine = -1;			// MD Drums' Machine id, 0-255; -1 while MD Drums does not say
		std::string machineName;	// as the Machinedrum shows it, "TRX-BD"
		std::array<std::string, SynCount> synNames;	// "PTCH", "DEC"... "-" for one the Machine does not use
		std::array<int, LockCount> values;			// the Kit's, in the Locks' order; -1 while MD Drums does not say

		KitTrack() { values.fill(-1); }
		bool operator==(const KitTrack& _other) const
		{
			return machine == _other.machine && machineName == _other.machineName && synNames == _other.synNames &&
				values == _other.values;
		}
	};

	struct Kit
	{
		std::string name;
		std::array<KitTrack, TrackCount> tracks;

		bool operator==(const Kit& _other) const { return name == _other.name && tracks == _other.tracks; }
		bool operator!=(const Kit& _other) const { return !(*this == _other); }
	};

	// The file's text read, none for a file of another version or no file of this format. Lines it does not know, or
	// does not understand, are skipped.
	std::optional<Kit> parseKit(std::string_view _text);

	// The hello, as MIDI bytes for the device's MIDI out: note-ons 124 to 127 on channel 1, each with velocity 1 plus six
	// bits of the id (the highest first), each followed by its note-off.
	constexpr int HelloFirstNote = 124;
	constexpr int HelloNotes = 4;
	using MidiBytes = std::array<uint8_t, 3>;
	std::array<MidiBytes, 2 * HelloNotes> hello(uint32_t _id);

	// MD Drums' file as the adapter found it at a poll: read, absent, or there but not opening for the moment. Windows
	// refuses to open a file that a rename is replacing, as MD Drums does each time it writes one (measured: 923 opens
	// refused in 10 s against a file replaced 567 times).
	struct KitFile
	{
		enum class State
		{
			Absent,
			Busy,
			Read
		};
		State state = State::Absent;
		std::string text;	// what was read

		static KitFile none() { return {}; }
		static KitFile busy() { return {State::Busy, {}}; }
		static KitFile read(std::string _text) { return {State::Read, std::move(_text)}; }
	};

	// What MD Drums answered. The adapter reads the file every PollMs on the main thread and hands it over; the link
	// says whether the Kit read differs from the last. A busy file keeps the Kit.
	class KitLink
	{
	public:
		static constexpr double PollMs = 500.0;

		// _random: any 64 bits, from which the link draws its id, 24 bits, never 0.
		explicit KitLink(uint64_t _random);

		uint32_t id() const { return m_id; }
		// "<id>.kit", the id in six lowercase hexadecimal digits: the file's name in <temp>/md-drums-seq.
		std::string fileName() const;

		// Whether kit() is not what the last poll gave.
		bool poll(const KitFile& _file);

		// The Kit of the last file read, none while MD Drums has written no file this build reads.
		const std::optional<Kit>& kit() const { return m_kit; }

	private:
		uint32_t m_id;
		std::optional<std::string> m_text;
		std::optional<Kit> m_kit;
	};

	// When to say hello: at the first tick; while MD Drums' file is absent, 1, 2, 4 and 8 seconds later, then every ten
	// seconds (KitLink::PollMs ticks); every ten seconds while it is there, read or busy; and from a second again once
	// it has gone. Ten seconds keeps the hellos rare in the MIDI the device sends while stopped, under the 15 s after
	// which MD Drums forgets a device, deleting its file: MD Drums takes the hellos it has heard before it forgets
	// (Controller::followKitFiles), so a stop of its own message thread loses none. It ticks on Max's scheduler thread,
	// apart from the file's reading on the main thread: in Live, Max's main thread stops for seconds at a time
	// (measured on 2026-10-17: 3.7 s and 6.4 s, and 13 s between two hellos). Allocates nothing.
	class HelloPace
	{
	public:
		// _file: what the last poll found of MD Drums' file.
		bool tick(KitFile::State _file);

	private:
		int m_sinceHello = -1;	// ticks since the last hello, -1 before the first
		int m_wait = 1;			// ticks between two unanswered hellos, doubled after each up to ten seconds
	};
}
