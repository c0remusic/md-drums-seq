#include "mdseqPrinter.h"

#include <algorithm>
#include <cstdio>

namespace mdseq
{
	namespace
	{
		std::string trackLabel(const int _track)
		{
			char text[16];
			std::snprintf(text, sizeof(text), "Track %02d", _track + 1);
			return text;
		}

		// A created track's name: "MD 03" for Track 3 (index 2).
		std::string createdTrackName(const int _track)
		{
			char text[16];
			std::snprintf(text, sizeof(text), "MD %02d", _track + 1);
			return text;
		}

		// A clip's name: "P01 ×4" for the first Pattern over four Passes.
		std::string printedClipName(const int _pattern, const int _passes)
		{
			char text[32];
			std::snprintf(text, sizeof(text), "P%02d \xC3\x97%d", _pattern + 1, _passes);
			return text;
		}

		// After a print or a capture made: the clips and the device both play MD Drums (issue 23).
		constexpr const char* Twice = "Set the device to Clips, or each Hit plays twice.";
	}

	int channelTrack(const std::string& _channel)
	{
		size_t digits = 0;
		while(digits < _channel.size() && _channel[digits] >= '0' && _channel[digits] <= '9')
			++digits;
		if(digits == 0 || digits > 2 || digits + 1 >= _channel.size() || _channel[digits] != '-')
			return -1;
		const int n = std::stoi(_channel.substr(0, digits));
		return n >= 1 && n <= TrackCount ? n - 1 : -1;
	}

	std::string notesJson(const std::vector<PrintedNote>& _notes)
	{
		std::string json = "{\"notes\":[";
		char note[160];
		for(size_t i = 0; i < _notes.size(); ++i)
		{
			const PrintedNote& n = _notes[i];
			std::snprintf(note, sizeof(note),
				"%s{\"pitch\":%d,\"start_time\":%.17g,\"duration\":%.17g,\"velocity\":%d}", i ? "," : "", n.pitch,
				n.start, n.duration, n.velocity);
			json += note;
		}
		return json + "]}";
	}

	Printer::Printer(LiveSet& _set) : m_set(_set)
	{
		m_trackOf.fill(-1);
	}

	Printer::Report Printer::print(const Pattern& _pattern, const int _index, const int _passes, const uint64_t _seed,
		const bool _fill)
	{
		if(m_printing)
			return {};
		static constexpr Words Print{"Printing...", "printing", "Printed", "Not printed"};
		const int passes = std::clamp(_passes, 1, MaxPasses);
		m_printed = printPattern(_pattern, passes, _seed, _fill);
		if(m_printed.trackCount() == 0)
		{
			char text[64];
			std::snprintf(text, sizeof(text), "Nothing to print: P%02d has no Hit", _index + 1);
			return {text};
		}
		m_clipName = printedClipName(_index, passes);
		m_loop = true;
		m_words = &Print;
		return begin();
	}

	Printer::Report Printer::capture(const Take& _take)
	{
		if(m_printing)
			return {};
		static constexpr Words Capture{"Capturing...", "capturing", "Captured", "Not captured"};
		if(_take.entries.empty())
			return {"Nothing to capture: no note played since the transport last started"};
		Printed clips = takeClips(_take);
		if(clips.trackCount() == 0)
			return {"Nothing to capture: the take has no note"};
		m_printed = std::move(clips);
		m_clipName = "Take " + std::to_string(m_takes + 1);
		m_loop = false;
		m_words = &Capture;
		const Report report = begin();
		if(report.again)
			++m_takes;
		return report;
	}

	Printer::Report Printer::begin()
	{
		const SetView view = m_set.read();
		if(view.deviceTrack < 0 || view.deviceTrack >= static_cast<int>(view.tracks.size()))
			return {std::string(m_words->notDone) + ": the device must sit on a track, not in a rack"};
		m_warnings.clear();
		m_unrouted.clear();
		m_attempts = 0;
		plan(view);

		// The track created at its index is sent to the device's track now; its channel waits for the second step.
		m_refusal.clear();
		for(size_t k = 0; k < m_created.size(); ++k)
		{
			const Created& c = m_created[k];
			const std::string name = createdTrackName(c.track);
			if(!m_set.createMidiTrack(c.index, name))
			{
				// No clip is written then. The tracks made before it still get their channel, so that the next print
				// finds them rather than adding others beside them.
				m_refusal = "Live did not create " + name;
				m_created.resize(k);
				break;
			}
			if(m_set.setOutputType(c.index, m_deviceName))
				m_unrouted.push_back(c);
			else
				m_warnings.push_back(name + " could not be sent to " + m_deviceName);
		}
		if(!m_refusal.empty() && m_created.empty())
			return {std::string(m_words->notDone) + ": " + m_refusal};
		m_printing = true;
		return {m_words->doing, true};
	}

	Printer::Report Printer::resume()
	{
		if(!m_printing)
			return {};
		std::vector<Created> still;
		bool waiting = false;	// a created track at its index whose channels Live has not listed yet
		for(const Created& c : m_unrouted)
		{
			// Only the track created gets its channel. The user may have moved tracks meanwhile: the track at its index
			// is then another, which keeps its channel, and the created one is tried again next time, should it come
			// back. Only a track at its index is worth waiting for.
			if(m_set.trackName(c.index) != createdTrackName(c.track))
				still.push_back(c);
			else if(!m_set.setOutputChannel(c.index, c.track))
			{
				still.push_back(c);
				waiting = true;
			}
		}
		m_unrouted = still;
		if(waiting && ++m_attempts < Attempts)
			return {{}, true};

		// A created track left without its channel stays on "Track In", where the next print would not find it: the
		// status names it, here or among the tracks moved.
		const auto unrouted = [this](const Created& _c)
		{
			m_warnings.push_back(createdTrackName(_c.track) + " could not be sent to " + m_deviceName + "'s channel");
		};
		if(!m_refusal.empty())
		{
			for(const Created& c : m_unrouted)
				unrouted(c);
			m_unrouted.clear();
			return stop(m_refusal);
		}

		// A clip goes to the index planned only while the track planned is still there: a track moved, inserted or
		// deleted since the set was read puts another track at that index, which must not get the clip.
		std::vector<int> planned;
		for(const int index : m_trackOf)
			if(index >= 0)
				planned.push_back(index);
		const SetView view = m_set.readTracks(planned);
		std::string moved;
		for(int track = 0; track < TrackCount; ++track)
			if(m_trackOf[static_cast<size_t>(track)] >= 0 && !inPlace(view, track))
				moved += (moved.empty() ? "" : ", ") + m_nameOf[static_cast<size_t>(track)];
		for(const Created& c : m_unrouted)
			if(inPlace(view, c.track))
				unrouted(c);
		m_unrouted.clear();
		if(!moved.empty())
			return stop(std::string("tracks moved while ") + m_words->during + ": " + moved);

		if(m_newScene)
			m_set.createScene(m_scene);
		int clips = 0;
		std::vector<int> refused;
		for(int track = 0; track < TrackCount; ++track)
		{
			const int index = m_trackOf[static_cast<size_t>(track)];
			if(index < 0)
				continue;
			const std::vector<PrintedNote>& notes = m_printed.tracks[static_cast<size_t>(track)];
			if(m_set.createClip(index, m_scene, m_printed.length, m_clipName, notes, m_loop))
				++clips;
			else
				refused.push_back(track);
		}
		m_printing = false;
		const std::string status = printedStatus(clips, refused);
		m_printed = Printed();
		return {status};
	}

	void Printer::plan(const SetView& _view)
	{
		const int device = _view.deviceTrack;
		m_deviceName = _view.tracks[static_cast<size_t>(device)].name;

		// Each Track's track: the first MIDI track of the set sent to the device's track on its channel.
		std::array<int, TrackCount> found;
		found.fill(-1);
		for(int i = 0; i < static_cast<int>(_view.tracks.size()); ++i)
		{
			const SetTrack& t = _view.tracks[static_cast<size_t>(i)];
			if(i == device || !t.midi || t.outputType != m_deviceName)
				continue;
			const int track = channelTrack(t.outputChannel);
			if(track < 0)
				continue;
			if(found[static_cast<size_t>(track)] < 0)
				found[static_cast<size_t>(track)] = i;
			else if(!m_printed.tracks[static_cast<size_t>(track)].empty())
				m_warnings.push_back(trackLabel(track) + ": two tracks on " + t.outputChannel + ", printed on " +
					_view.tracks[static_cast<size_t>(found[static_cast<size_t>(track)])].name);
		}

		// The Tracks without one get a track right after the device's, in Track order, which moves the tracks after it.
		m_created.clear();
		for(int track = 0; track < TrackCount; ++track)
			if(!m_printed.tracks[static_cast<size_t>(track)].empty() && found[static_cast<size_t>(track)] < 0)
				m_created.push_back({track, device + 1 + static_cast<int>(m_created.size())});
		const int createdCount = static_cast<int>(m_created.size());

		m_trackOf.fill(-1);
		for(const Created& c : m_created)
		{
			m_trackOf[static_cast<size_t>(c.track)] = c.index;
			m_nameOf[static_cast<size_t>(c.track)] = createdTrackName(c.track);
		}
		bool slotTaken = false;
		for(int track = 0; track < TrackCount; ++track)
		{
			const int existing = found[static_cast<size_t>(track)];
			if(existing < 0 || m_printed.tracks[static_cast<size_t>(track)].empty())
				continue;
			m_trackOf[static_cast<size_t>(track)] = existing + (existing > device ? createdCount : 0);
			// Its name as read, which a status names it by: Live renumbers the names it gives itself once a track is
			// created before them, so resume knows it by its output (inPlace).
			m_nameOf[static_cast<size_t>(track)] = _view.tracks[static_cast<size_t>(existing)].name;
			// Read before anything is created, at the index read.
			slotTaken = slotTaken || m_set.hasClip(existing, _view.selectedScene);
		}

		// Nothing is overwritten: a clip in the way puts the print in a new scene below the selected one.
		m_newScene = slotTaken;
		m_scene = _view.selectedScene + (slotTaken ? 1 : 0);
	}

	bool Printer::inPlace(const SetView& _view, const int _track) const
	{
		const int index = m_trackOf[static_cast<size_t>(_track)];
		if(index < 0 || index >= static_cast<int>(_view.tracks.size()))
			return false;
		const SetTrack& t = _view.tracks[static_cast<size_t>(index)];
		// A track created by its name, which Live does not renumber.
		for(const Created& c : m_created)
			if(c.track == _track)
				return t.name == createdTrackName(_track);
		// A track found by what found it, its output to the device's track on the Track's channel. Its name is no
		// sign: Live renumbers the names it gives itself, "2-MIDI" or any with a '#', when a track is created before
		// them. Another track there that plays this Track is as good a place for its clip.
		return index != _view.deviceTrack && t.midi && t.outputType == m_deviceName &&
			channelTrack(t.outputChannel) == _track;
	}

	Printer::Report Printer::stop(const std::string& _why)
	{
		m_printing = false;
		m_printed = Printed();
		return {notPrintedStatus(_why)};
	}

	std::string Printer::notPrintedStatus(const std::string& _why) const
	{
		// The tracks created stay in the set, each an entry in Live's undo history: the status line counts them.
		std::string status = std::string(m_words->notDone) + ": " + _why + ".";
		const std::string added = tracksAdded();
		if(!added.empty())
			status += " " + added + ".";
		appendWarnings(status);
		return status;
	}

	std::string Printer::tracksAdded() const
	{
		const size_t created = m_created.size();
		if(created == 0)
			return {};
		return std::to_string(created) + (created > 1 ? " tracks added" : " track added");
	}

	void Printer::appendWarnings(std::string& _status) const
	{
		for(const std::string& warning : m_warnings)
			_status += " " + warning + ".";
	}

	std::string Printer::printedStatus(const int _clips, const std::vector<int>& _refused) const
	{
		char text[160];
		std::string status;
		const char* sceneMark = m_newScene ? " (new)" : "";
		if(_clips == 0)
		{
			// Every Track printed had a track: none of them took its clip.
			std::snprintf(text, sizeof(text), "Live made no clip in scene %d%s for ", m_scene + 1, sceneMark);
			std::string why = text;
			for(size_t i = 0; i < _refused.size(); ++i)
				why += (i ? ", " : "") + trackLabel(_refused[i]);
			return notPrintedStatus(why);
		}

		std::snprintf(text, sizeof(text), "%s %s in scene %d%s: %d clip%s", m_words->done, m_clipName.c_str(),
			m_scene + 1, sceneMark, _clips, _clips > 1 ? "s" : "");
		status = text;
		const std::string added = tracksAdded();
		if(!added.empty())
			status += ", " + added;
		status += std::string(". ") + Twice;
		appendWarnings(status);
		for(const int track : _refused)
			status += " no clip for " + trackLabel(track) + ".";
		return status;
	}
}
