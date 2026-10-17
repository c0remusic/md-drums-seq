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
		int machine = -1;			// MD Drums' Machine id, 0-191; -1 while MD Drums does not say
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

	// When to say hello and what MD Drums answered. The adapter reads the file every PollMs and hands its text over;
	// the link says when a hello is due (at the first look, then every second while there is no file, every five seconds
	// while there is one) and whether the Kit read differs from the last.
	class KitLink
	{
	public:
		static constexpr double PollMs = 500.0;

		// _random: any 64 bits, from which the link draws its id, 24 bits, never 0.
		explicit KitLink(uint64_t _random);

		uint32_t id() const { return m_id; }
		// "<id>.kit", the id in six lowercase hexadecimal digits: the file's name in <temp>/md-drums-seq.
		std::string fileName() const;

		struct Poll
		{
			bool hello = false;			// send one now
			bool kitChanged = false;	// kit() is not what the last poll gave
		};
		Poll poll(const std::optional<std::string>& _fileText);

		// The Kit of the last poll, none while MD Drums has written no file this build reads.
		const std::optional<Kit>& kit() const { return m_kit; }

	private:
		uint32_t m_id;
		int m_sinceHello = -1;	// polls since the last hello, -1 before the first
		std::optional<std::string> m_text;
		std::optional<Kit> m_kit;
	};
}
