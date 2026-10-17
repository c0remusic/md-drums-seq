#pragma once

// Printing onto the set (issues 07 and 22): the Printer renders N Passes of a Pattern and writes a clip per Track into
// Live through LiveSet, the few things a print reads and writes there. Each Track's clip goes on the MIDI track whose
// output goes to the device's track on channel "N-<instrument>", created as "MD nn" right after the device's track
// when there is none; to the selected scene when the slots of the tracks found are free there, else to a new scene
// below it. Live fills a created track's list of output channels only after the call that set its output's type
// returns (measured 2026-10-14: right after, "Track In" alone), so a print takes two steps, the second called again
// until the channels are listed. Main thread; it allocates.

#include "mdseqPrint.h"
#include "mdseqTypes.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mdseq
{
	// A track of the set as printing reads it.
	struct SetTrack
	{
		std::string name;
		bool midi = false;			// a MIDI track: the only kind a print uses
		std::string outputType;		// its output's routing type, by display name (a track's name)
		std::string outputChannel;	// and channel ("3-MD Drums")
	};

	struct SetView
	{
		std::vector<SetTrack> tracks;	// in the set's order
		int deviceTrack = -1;			// the track that holds the device, -1 when it is not straight on one (a rack)
		int selectedScene = 0;
	};

	// The set as a print reaches it: the adapter's is the LOM, the tests' a set in memory. Tracks and scenes are
	// numbered as the set orders them now, a creation moving those after it. Every write is an entry in Live's undo
	// history.
	class LiveSet
	{
	public:
		virtual ~LiveSet() = default;

		virtual SetView read() = 0;
		virtual bool hasClip(int _index, int _scene) = 0;

		// A MIDI track inserted at _index, named _name; false when Live made none.
		virtual bool createMidiTrack(int _index, const std::string& _name) = 0;
		// The output's type set to the entry of that display name; false when the track lists none, or Live kept
		// another.
		virtual bool setOutputType(int _index, const std::string& _type) = 0;
		// The output's channel set to the one that names _track (0-15, see channelTrack); false while the set does
		// not list it yet, or when Live kept another.
		virtual bool setOutputChannel(int _index, int _track) = 0;
		virtual std::string trackName(int _index) = 0;

		virtual void createScene(int _index) = 0;
		// A clip of _length beats in an empty slot, named _name, holding _notes; false when Live made none.
		virtual bool createClip(int _index, int _scene, double _length, const std::string& _name,
			const std::vector<PrintedNote>& _notes) = 0;
	};

	// The Track (0-15) a channel's display name names ("3-MD Drums" is 2), -1 for any other ("Track In").
	int channelTrack(const std::string& _channel);

	// The notes of a Track as the dictionary Clip.add_new_notes takes, in JSON.
	std::string notesJson(const std::vector<PrintedNote>& _notes);

	// A print from its start to its status line. The adapter calls print, then resume each time a Report asks for it,
	// RetryMs later on the main thread, and shows each status it is given; it keeps nothing else of the print.
	class Printer
	{
	public:
		// The second step comes this long after the first, then again until Live lists the channels: 2 s at most.
		static constexpr double RetryMs = 50.0;
		static constexpr int Attempts = 40;

		struct Report
		{
			std::string status;		// the window's status line from now on, empty to leave it as it is
			bool again = false;		// call resume RetryMs from now
		};

		explicit Printer(LiveSet& _set);

		// The first step: _passes Passes of _pattern (Pattern _index of the bank) from _seed, the Fill held or not,
		// rendered and placed in the set read now; the tracks missing created and sent to the device's track. Nothing
		// while a print is under way. A track Live does not create ends the print without a clip, the tracks made
		// before it still getting their channel.
		Report print(const Pattern& _pattern, int _index, int _passes, uint64_t _seed, bool _fill);

		// The second step: the created tracks' channels, tried again until the set lists them, then the scene, the
		// clips and the status line. No clip at all when a track the plan names is no longer at its index (moved,
		// or one inserted or deleted before it): the track there now would get it. A created track is known there
		// by its name, a track found by its output to the device's Track channel. Nothing when no print is under
		// way.
		Report resume();

		bool printing() const { return m_printing; }

	private:
		// A track the print creates: the Track (0-15) it plays and its index once created.
		struct Created
		{
			int track;
			int index;
		};

		void plan(const SetView& _view);
		// Whether the track planned for _track (0-15) is still at its index in _view.
		bool inPlace(const SetView& _view, int _track) const;
		Report stop(const std::string& _why);
		std::string notPrintedStatus(const std::string& _why) const;
		std::string printedStatus(int _clips, const std::vector<int>& _refused) const;
		// "2 tracks added" for the tracks created, empty when none were; the warnings, each a sentence after _status.
		std::string tracksAdded() const;
		void appendWarnings(std::string& _status) const;

		LiveSet& m_set;
		bool m_printing = false;
		Printed m_printed;
		int m_pattern = 0;
		int m_passes = 0;
		std::string m_deviceName;
		std::vector<Created> m_created;				// in Track order, right after the device's track
		std::array<int, TrackCount> m_trackOf{};	// each Track's clip's track once the tracks are created, or -1
		std::array<std::string, TrackCount> m_nameOf;	// and that track's name when the print planned it
		bool m_newScene = false;					// a scene inserted below the selected one first
		int m_scene = 0;							// the scene the clips go to, once it exists
		std::vector<Created> m_unrouted;			// created tracks still without their channel
		int m_attempts = 0;
		std::string m_refusal;						// why the print writes no clip, found in the first step
		std::vector<std::string> m_warnings;
	};
}
