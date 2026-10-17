// A print through the Printer onto a set in memory, as the adapter runs it: the first step, then the second each time
// it asks, Live's main thread taking a turn in between. Each Track's clip goes on the first MIDI track sent to the
// device's track on its channel, a new "MD nn" track after the device's for the others (which moves the tracks after
// it), the selected scene unless a clip is in the way of a track found, then a new scene below it. A created track's
// channels come a few turns late, or never; a type or a clip refused are told in the status line, and a track moved
// between the two steps, or one Live did not create, stops the print before any clip, while a name Live renumbers
// itself does not; a created track put back where it was gets its channel, and one left without it is told.
// Also the channel names and the notes as add_new_notes takes them.

#include "mdseqCondition.h"
#include "mdseqPrinter.h"
#include "mdseqTestSupport.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace
{
	using namespace mdseq;

	// A set in memory, as the LOM shows it to a print: tracks with their output and their clips by scene, Live's
	// habit of listing a track's output channels only some turns of its main thread after its output's type is set,
	// and the names Live numbers itself: a track named "#-MIDI", as Live's default "2-MIDI" is, shows its number in
	// the set in place of the '#', which changes when a track is created or taken out before it.
	class FakeSet : public LiveSet
	{
	public:
		struct Clip
		{
			double length = 0.0;
			std::string name;
			std::vector<PrintedNote> notes;
		};

		struct LiveTrack
		{
			std::string name;
			bool midi = true;
			std::string outputType;
			std::string outputChannel;
			bool device = false;			// holds the device, and MD Drums with its sixteen channels
			std::map<int, Clip> clips;		// by scene
			int unlisted = 0;				// the turns before Live lists the channels of the output's type
		};

		std::vector<LiveTrack> tracks;
		int selectedScene = 0;
		int scenes = 8;
		std::string instrument = "MD Drums";	// the plug-in's name, as Live names its channels
		int listedAfter = 1;		// never in the call that sets the type: once it has returned
		bool refuseTracks = false;
		int tracksAllowed = -1;		// the tracks Live creates before it refuses the next, -1 for any number
		bool refuseTypes = false;
		std::vector<std::string> refuseClipsOn;	// tracks, by name, where Live makes no clip
		int reads = 0;
		int writes = 0;				// each an entry in Live's undo history

		// A turn of Live's main thread between two steps: it lists what the calls before it set.
		void turn()
		{
			for(LiveTrack& t : tracks)
				t.unlisted = std::max(0, t.unlisted - 1);
		}

		// The name of the track at _index as the set shows it.
		std::string shown(const size_t _index) const
		{
			std::string name = tracks[_index].name;
			const size_t mark = name.find('#');
			if(mark != std::string::npos)
				name.replace(mark, 1, std::to_string(_index + 1));
			return name;
		}

		std::vector<std::string> names() const
		{
			std::vector<std::string> list;
			for(size_t i = 0; i < tracks.size(); ++i)
				list.push_back(shown(i));
			return list;
		}

		// The track that shows _name.
		const LiveTrack& named(const std::string& _name) const
		{
			static const LiveTrack none;
			for(size_t i = 0; i < tracks.size(); ++i)
				if(shown(i) == _name)
					return tracks[i];
			return none;
		}

		// The clip in a track's slot of that scene, or null.
		const Clip* clip(const std::string& _track, const int _scene) const
		{
			const auto c = named(_track).clips.find(_scene);
			return c == named(_track).clips.end() ? nullptr : &c->second;
		}

		int clipCount() const
		{
			int count = 0;
			for(const LiveTrack& t : tracks)
				count += static_cast<int>(t.clips.size());
			return count;
		}

		SetView read() override
		{
			++reads;
			SetView view;
			for(size_t i = 0; i < tracks.size(); ++i)
			{
				const LiveTrack& t = tracks[i];
				SetTrack s;
				s.name = shown(i);
				s.midi = t.midi;
				s.outputType = t.outputType;
				s.outputChannel = t.outputChannel;
				view.tracks.push_back(s);
				if(t.device)
					view.deviceTrack = static_cast<int>(i);
			}
			view.selectedScene = selectedScene;
			return view;
		}

		bool hasClip(const int _index, const int _scene) override
		{
			++reads;
			return valid(_index) && track(_index).clips.count(_scene) > 0;
		}

		bool createMidiTrack(const int _index, const std::string& _name) override
		{
			if(refuseTracks || tracksAllowed == 0 || _index < 0 || _index > static_cast<int>(tracks.size()))
				return false;
			if(tracksAllowed > 0)
				--tracksAllowed;
			// A MIDI track the LOM creates plays to "No Output".
			LiveTrack t;
			t.name = _name;
			t.outputType = "No Output";
			tracks.insert(tracks.begin() + _index, t);
			++writes;
			return true;
		}

		bool setOutputType(const int _index, const std::string& _type) override
		{
			if(refuseTypes || !valid(_index))
				return false;
			// A MIDI track's output lists the other tracks.
			for(int i = 0; i < static_cast<int>(tracks.size()); ++i)
			{
				if(i == _index || shown(static_cast<size_t>(i)) != _type)
					continue;
				LiveTrack& t = track(_index);
				t.outputType = _type;
				t.outputChannel = "Track In";
				t.unlisted = listedAfter;
				++writes;
				return true;
			}
			return false;
		}

		bool setOutputChannel(const int _index, const int _track) override
		{
			if(!valid(_index))
				return false;
			for(const std::string& channel : channels(track(_index)))
			{
				if(channelTrack(channel) != _track)
					continue;
				track(_index).outputChannel = channel;
				++writes;
				return true;
			}
			return false;
		}

		std::string trackName(const int _index) override
		{
			++reads;
			return valid(_index) ? shown(static_cast<size_t>(_index)) : std::string();
		}

		void createScene(const int _index) override
		{
			++scenes;
			for(LiveTrack& t : tracks)
			{
				std::map<int, Clip> moved;
				for(const auto& [scene, clip] : t.clips)
					moved[scene >= _index ? scene + 1 : scene] = clip;
				t.clips = moved;
			}
			++writes;
		}

		bool createClip(const int _index, const int _scene, const double _length, const std::string& _name,
			const std::vector<PrintedNote>& _notes) override
		{
			if(!valid(_index) || _scene < 0 || _scene >= scenes || track(_index).clips.count(_scene))
				return false;
			if(std::find(refuseClipsOn.begin(), refuseClipsOn.end(), shown(static_cast<size_t>(_index))) !=
				refuseClipsOn.end())
				return false;
			track(_index).clips[_scene] = {_length, _name, _notes};
			++writes;
			return true;
		}

	private:
		bool valid(const int _index) const
		{
			return _index >= 0 && _index < static_cast<int>(tracks.size());
		}

		LiveTrack& track(const int _index)
		{
			return tracks[static_cast<size_t>(_index)];
		}

		// What a track's output lists as channels: "Track In", and once Live has listed them, MD Drums' sixteen when
		// the output goes to the device's track.
		std::vector<std::string> channels(const LiveTrack& _t) const
		{
			std::vector<std::string> list{"Track In"};
			if(_t.unlisted > 0)
				return list;
			for(size_t d = 0; d < tracks.size(); ++d)
				if(tracks[d].device && shown(d) == _t.outputType)
					for(int n = 1; n <= TrackCount; ++n)
						list.push_back(std::to_string(n) + "-" + instrument);
			return list;
		}	};

	FakeSet::LiveTrack midiTo(const std::string& _name, const std::string& _type, const std::string& _channel)
	{
		FakeSet::LiveTrack t;
		t.name = _name;
		t.outputType = _type;
		t.outputChannel = _channel;
		return t;
	}

	FakeSet::LiveTrack deviceTrack(const std::string& _name)
	{
		FakeSet::LiveTrack t = midiTo(_name, "Main", "");
		t.device = true;
		return t;
	}

	FakeSet aSet()
	{
		FakeSet set;
		set.tracks = {
			midiTo("KICK", "Main", ""),					// 0: elsewhere
			deviceTrack("Drums"),						// 1: the device's track
			midiTo("Hats", "Drums", "7-MD Drums"),		// 2: Track 7's
			midiTo("Busy", "Drums", "Track In"),		// 3: the device's track, not a Track's channel
			midiTo("Hats 2", "Drums", "7-MD Drums"),	// 4: Track 7's too, second
			midiTo("Other", "KICK", "3-MD Drums"),		// 5: another track's channel
		};
		FakeSet::LiveTrack audio = midiTo("Audio", "Drums", "3-MD Drums");	// 6: not MIDI
		audio.midi = false;
		set.tracks.push_back(audio);
		set.selectedScene = 3;
		return set;
	}

	// A Hit on the first step of each Track given (0-15).
	Pattern hitsOn(std::initializer_list<int> _tracks)
	{
		Pattern p;
		for(const int t : _tracks)
			p.steps[static_cast<size_t>(t)][0].hit = true;
		return p;
	}

	// What the window showed of a print, and the second steps it took.
	struct Shown
	{
		std::vector<std::string> statuses;
		int steps = 0;

		std::string last() const { return statuses.empty() ? std::string() : statuses.back(); }
	};

	// Shows a report's status and, while it asks for it, runs the second step after a turn of Live's main thread, as
	// the adapter's clock does.
	Shown follow(Printer& _printer, FakeSet& _set, Printer::Report _report)
	{
		Shown shown;
		for(;;)
		{
			if(!_report.status.empty())
				shown.statuses.push_back(_report.status);
			if(!_report.again)
				return shown;
			_set.turn();
			++shown.steps;
			_report = _printer.resume();
		}
	}

	Shown printOnce(FakeSet& _set, const Pattern& _pattern, const int _index = 0, const int _passes = 4)
	{
		Printer printer(_set);
		const Shown shown = follow(printer, _set, printer.print(_pattern, _index, _passes, 1, false));
		CHECK(!printer.printing());
		return shown;
	}

	const char* const Mute = "Mute the device or the clips, or each Hit plays twice.";
	const char* const TwoOnSeven = " Track 07: two tracks on 7-MD Drums, printed on Hats.";

	// A clip's name as a print gives it: "P01 ×4" for the first Pattern over four Passes.
	std::string times(const char* _pattern, const int _passes)
	{
		return std::string(_pattern) + " \xC3\x97" + std::to_string(_passes);
	}

	// A print's status line once made: "Printed P01 ×4 in " and _where, then the advice to mute.
	std::string printed(const char* _pattern, const int _passes, const char* _where)
	{
		return "Printed " + times(_pattern, _passes) + " in " + _where + ". " + Mute;
	}

	void channels()
	{
		CHECK(channelTrack("3-MD Drums") == 2);
		CHECK(channelTrack("1-MD Drums") == 0);
		CHECK(channelTrack("16-MD Drums") == 15);
		CHECK(channelTrack("17-MD Drums") == -1);
		CHECK(channelTrack("0-MD Drums") == -1);
		CHECK(channelTrack("Track In") == -1);
		CHECK(channelTrack("3-") == -1);
		CHECK(channelTrack("") == -1);
		// The number alone counts: MD Drums is MoDule from 0.1.2, and a set made before keeps "MD Drums".
		CHECK(channelTrack("3-MoDule") == 2 && channelTrack("16-MoDule") == 15);
	}

	// A print with the plug-in named MoDule: the tracks found and created by their channels' numbers alike.
	void instrumentRenamed()
	{
		FakeSet set = aSet();
		set.instrument = "MoDule";
		for(FakeSet::LiveTrack& t : set.tracks)
			if(t.outputChannel == "7-MD Drums")
				t.outputChannel = "7-MoDule";
		printOnce(set, hitsOn({0, 6}));
		CHECK(set.named("MD 01").outputChannel == "1-MoDule" && set.clip("MD 01", 3));
		CHECK(set.clip("Hats", 3) && set.named("Hats 2").clips.empty());
	}

	void notes()
	{
		CHECK(notesJson({{36, 0.0, 0.25, 110}, {81, 1.5, 0.125, 1}}) ==
			"{\"notes\":[{\"pitch\":36,\"start_time\":0,\"duration\":0.25,\"velocity\":110},"
			"{\"pitch\":81,\"start_time\":1.5,\"duration\":0.125,\"velocity\":1}]}");
		CHECK(notesJson({}) == "{\"notes\":[]}");
	}

	void tracksFoundAndCreated()
	{
		FakeSet set = aSet();
		const Pattern pattern = hitsOn({0, 2, 6});
		const Shown shown = printOnce(set, pattern);

		// Tracks 1 and 3 have no track: created right after the device's, in Track order, sent to its channel.
		CHECK((set.names() == std::vector<std::string>{"KICK", "Drums", "MD 01", "MD 03", "Hats", "Busy", "Hats 2",
			"Other", "Audio"}));
		CHECK(set.named("MD 01").outputType == "Drums" && set.named("MD 01").outputChannel == "1-MD Drums");
		CHECK(set.named("MD 03").outputType == "Drums" && set.named("MD 03").outputChannel == "3-MD Drums");
		// Track 7 on the first track sent to its channel, moved by the two created before it: its clip found it there.
		CHECK(set.clipCount() == 3);
		const Printed played = printPattern(pattern, 4, 1, false);
		CHECK(set.clip("MD 01", 3) && set.clip("MD 01", 3)->notes == played.tracks[0]);
		CHECK(set.clip("MD 03", 3) && set.clip("MD 03", 3)->notes == played.tracks[2]);
		CHECK(set.clip("Hats", 3) && set.clip("Hats", 3)->notes == played.tracks[6]);
		CHECK(set.named("Hats 2").clips.empty() && set.named("Other").clips.empty());
		CHECK(set.scenes == 8);

		// The channels are listed by the first second step.
		CHECK(shown.steps == 1);
		const std::string done = printed("P01", 4, "scene 4: 3 clips, 2 tracks added") + TwoOnSeven;
		CHECK((shown.statuses == std::vector<std::string>{"Printing...", done}));

		// A Track without notes warns of nothing.
		set = aSet();
		CHECK(printOnce(set, hitsOn({0})).last() == printed("P01", 4, "scene 4: 1 clip, 1 track added"));
	}

	void trackBeforeTheDevice()
	{
		FakeSet set;
		set.tracks = {midiTo("Kick line", "Drums", "1-MD Drums"), deviceTrack("Drums")};
		printOnce(set, hitsOn({0, 1}));
		// Before the device's track: the created one does not move it.
		CHECK((set.names() == std::vector<std::string>{"Kick line", "Drums", "MD 02"}));
		CHECK(set.clip("Kick line", 0) && set.clip("MD 02", 0));
		CHECK(set.named("MD 02").outputChannel == "2-MD Drums");
	}

	void clipContents()
	{
		// Track 1's Hit and its VOL Lock, over two Passes of the second Pattern: one clip, eight beats long.
		Pattern pattern;
		pattern.steps[0][0].hit = true;
		pattern.steps[0][0].velocity = 110;
		pattern.steps[0][0].locks[17] = 0;
		FakeSet set;
		set.tracks = {deviceTrack("Drums"), midiTo("Kick", "Drums", "1-MD Drums")};
		const Shown shown = printOnce(set, pattern, 1, 2);
		const FakeSet::Clip* clip = set.clip("Kick", 0);
		CHECK(clip && clip->name == times("P02", 2));
		CHECK(clip && clip->length == 8.0);
		CHECK(clip && (clip->notes == std::vector<PrintedNote>{{36, 0.0, 0.25, 110}, {64 + 17, 0.0, 0.25, 1},
			{36, 4.0, 0.25, 110}, {64 + 17, 4.0, 0.25, 1}}));
		CHECK(shown.last() == printed("P02", 2, "scene 1: 1 clip"));

		// The last Pattern, the last Track on a track created for it, and at most 16 Passes.
		set.tracks = {deviceTrack("Drums")};
		CHECK(printOnce(set, hitsOn({15}), 15, 20).last() == printed("P16", 16, "scene 1: 1 clip, 1 track added"));
		CHECK(set.named("MD 16").outputChannel == "16-MD Drums");
		clip = set.clip("MD 16", 0);
		CHECK(clip && clip->name == times("P16", 16) && clip->length == 64.0);

		// And at least one: the adapter hands over the Passes as the patch sends them, a missing number as 0.
		set.tracks = {deviceTrack("Drums"), midiTo("Kick", "Drums", "1-MD Drums")};
		CHECK(printOnce(set, hitsOn({0}), 0, 0).last() == printed("P01", 1, "scene 1: 1 clip"));
		clip = set.clip("Kick", 0);
		CHECK(clip && clip->name == times("P01", 1) && clip->length == 4.0);
	}

	void scenes()
	{
		// Track 7's track has a clip in the selected scene: a new scene below it, the clip in the way kept.
		FakeSet set = aSet();
		set.tracks[2].clips[3] = {4.0, "old", {}};
		const Shown shown = printOnce(set, hitsOn({0, 6}), 1, 1);
		CHECK(set.scenes == 9);
		CHECK(set.named("Hats").clips.size() == 2 && set.clip("Hats", 3) && set.clip("Hats", 3)->name == "old");
		CHECK(set.clip("Hats", 4) && set.clip("Hats", 4)->name == times("P02", 1) && set.clip("MD 01", 4));
		CHECK(shown.last() == printed("P02", 1, "scene 5 (new): 2 clips, 1 track added") + TwoOnSeven);

		// A clip on a track the print does not use is not in the way: the second track on a channel, or a Track's
		// track when that Track has no notes.
		set = aSet();
		set.tracks[2].clips[3] = {4.0, "old", {}};
		set.tracks[4].clips[3] = {4.0, "old", {}};
		CHECK(printOnce(set, hitsOn({0})).last() == printed("P01", 4, "scene 4: 1 clip, 1 track added"));
		CHECK(set.scenes == 8 && set.clip("MD 01", 3));
		set = aSet();
		set.tracks[4].clips[3] = {4.0, "old", {}};
		printOnce(set, hitsOn({6}));
		CHECK(set.scenes == 8 && set.clip("Hats", 3));

		// A second print finds the first's clips in the way.
		set = aSet();
		printOnce(set, hitsOn({0, 6}));
		CHECK(printOnce(set, hitsOn({0, 6})).last() == printed("P01", 4, "scene 5 (new): 2 clips") + TwoOnSeven);
		CHECK(set.named("MD 01").clips.size() == 2 && set.named("Hats").clips.size() == 2);
	}

	void channelsListedLate()
	{
		// Live holds the created track's channels back for three second steps, then lists them: done at the fourth.
		FakeSet set = aSet();
		set.listedAfter = 4;
		Printer printer(set);
		CHECK(printer.print(hitsOn({0, 6}), 0, 4, 1, false).again);
		for(int step = 0; step < 3; ++step)
		{
			set.turn();
			const Printer::Report report = printer.resume();
			CHECK(report.again && report.status.empty());
		}
		// Nothing written meanwhile but the created track and its output's type.
		CHECK(set.clipCount() == 0 && set.named("MD 01").outputChannel == "Track In");
		const Shown shown = follow(printer, set, {{}, true});
		CHECK(shown.steps == 1);
		CHECK(set.named("MD 01").outputChannel == "1-MD Drums");
		CHECK(set.clipCount() == 2);
		CHECK(shown.last() == printed("P01", 4, "scene 4: 2 clips, 1 track added") + TwoOnSeven);

		set = aSet();
		set.listedAfter = 8;
		CHECK(printOnce(set, hitsOn({0, 2})).steps == 8);
	}

	void channelsNeverListed()
	{
		// After every attempt, a warning; the clips are written all the same, the created track's on it.
		FakeSet set = aSet();
		set.listedAfter = 1000000;
		const Shown shown = printOnce(set, hitsOn({0, 6}));
		CHECK(shown.steps == Printer::Attempts);
		CHECK(set.named("MD 01").outputChannel == "Track In");
		CHECK(set.clip("Hats", 3) && set.clip("MD 01", 3));
		CHECK(shown.last() == printed("P01", 4, "scene 4: 2 clips, 1 track added") + TwoOnSeven +
			" MD 01 could not be sent to Drums's channel.");
	}

	void typeRefused()
	{
		// A created track Live does not send to the device's track: a warning, and no wait for its channel.
		FakeSet set = aSet();
		set.refuseTypes = true;
		const Shown shown = printOnce(set, hitsOn({0}));
		CHECK(shown.steps == 1);
		CHECK(set.named("MD 01").outputType == "No Output" && set.clip("MD 01", 3));
		CHECK(shown.last() ==
			printed("P01", 4, "scene 4: 1 clip, 1 track added") + " MD 01 could not be sent to Drums.");
	}

	// The user takes a track out of the set, by the name it shows, as a drag or a delete does: the tracks after it move
	// up.
	FakeSet::LiveTrack takeOut(FakeSet& _set, const std::string& _name)
	{
		const std::vector<std::string> names = _set.names();
		const auto t = _set.tracks.begin() + (std::find(names.begin(), names.end(), _name) - names.begin());
		const FakeSet::LiveTrack taken = *t;
		_set.tracks.erase(t);
		return taken;
	}

	// Tracks moved between the two steps put other tracks at the indices the print planned: no clip at all, and no
	// channel set on a track the print did not create.
	void trackMoved()
	{
		// The user moves the created track to the end of the set before its channel is set: Hats, Track 7's, is now
		// where it was, and would have taken Track 1's clip.
		FakeSet set = aSet();
		Printer printer(set);
		CHECK(printer.print(hitsOn({0}), 0, 4, 1, false).again);
		set.tracks.push_back(takeOut(set, "MD 01"));
		Shown shown = follow(printer, set, {{}, true});
		CHECK(shown.steps == 1);
		CHECK(set.named("MD 01").outputChannel == "Track In" && set.named("Hats").outputChannel == "7-MD Drums");
		CHECK(set.clipCount() == 0 && !set.clip("Hats", 3) && set.scenes == 8);
		CHECK(shown.last() == "Not printed: tracks moved while printing: MD 01. 1 track added.");
		CHECK(!printer.printing());

		// A track found shifts: KICK, before the device, deleted before the second step. Busy is now where Hats was.
		set = aSet();
		CHECK(printer.print(hitsOn({6}), 0, 4, 1, false).again);
		takeOut(set, "KICK");
		shown = follow(printer, set, {{}, true});
		CHECK(set.clipCount() == 0 && set.named("Busy").clips.empty());
		CHECK(shown.last() == std::string("Not printed: tracks moved while printing: Hats.") + TwoOnSeven);

		// The same while a created track waits for its channel: it is not sent to the channel of the track that took
		// its place, and neither Track's clip goes to the track now at its index.
		set = aSet();
		set.listedAfter = 3;
		CHECK(printer.print(hitsOn({0, 6}), 0, 4, 1, false).again);
		set.turn();
		CHECK(printer.resume().again);
		takeOut(set, "KICK");
		shown = follow(printer, set, {{}, true});
		CHECK(set.clipCount() == 0 && set.named("Hats").outputChannel == "7-MD Drums");
		CHECK(set.named("MD 01").outputChannel == "Track In");
		CHECK(shown.last() ==
			std::string("Not printed: tracks moved while printing: MD 01, Hats. 1 track added.") + TwoOnSeven);

		// A created track sent to its channel, then moved while another waits for its own.
		set = aSet();
		CHECK(printer.print(hitsOn({0, 2}), 0, 4, 1, false).again);
		set.tracks[3].unlisted = 3;	// MD 03's channels come later than MD 01's
		set.turn();
		CHECK(printer.resume().again);
		CHECK(set.named("MD 01").outputChannel == "1-MD Drums");
		set.tracks.push_back(takeOut(set, "MD 01"));
		shown = follow(printer, set, {{}, true});
		CHECK(set.clipCount() == 0 && set.named("Hats").outputChannel == "7-MD Drums");
		CHECK(shown.last() == "Not printed: tracks moved while printing: MD 01, MD 03. 2 tracks added.");

		// A created track moved away while another waits for its channels, then put back (Live's undo): it gets its
		// channel once back, and the print goes on. Left on "Track In", the next print would not find it and would add
		// a second "MD 03".
		set = aSet();
		CHECK(printer.print(hitsOn({0, 2}), 0, 4, 1, false).again);
		set.tracks[2].unlisted = 5;	// MD 01's channels come late
		set.tracks.push_back(takeOut(set, "MD 03"));
		set.turn();
		CHECK(printer.resume().again);
		set.tracks.insert(set.tracks.begin() + 3, takeOut(set, "MD 03"));
		shown = follow(printer, set, {{}, true});
		CHECK(set.named("MD 01").outputChannel == "1-MD Drums" && set.named("MD 03").outputChannel == "3-MD Drums");
		CHECK(shown.last() == printed("P01", 4, "scene 4: 2 clips, 2 tracks added"));
		CHECK(printOnce(set, hitsOn({0, 2})).last() == printed("P01", 4, "scene 5 (new): 2 clips"));
		const std::vector<std::string> names = set.names();
		CHECK(std::count(names.begin(), names.end(), "MD 03") == 1);
	}

	// A track found after the device's, with a name Live numbers itself: the track created before it renumbers it,
	// "2-MIDI" becoming "3-MIDI", and it is still the track planned, which plays its Track. Moved before the device's
	// track while the print waits, it is not, and neither is the track now at the created one's index.
	void autoNumbered()
	{
		FakeSet set;
		set.tracks = {deviceTrack("Drums"), midiTo("#-MIDI", "Drums", "1-MD Drums")};
		CHECK((set.names() == std::vector<std::string>{"Drums", "2-MIDI"}));
		Shown shown = printOnce(set, hitsOn({0, 1}));
		CHECK((set.names() == std::vector<std::string>{"Drums", "MD 02", "3-MIDI"}));
		CHECK(set.clip("3-MIDI", 0) && set.clip("MD 02", 0) && set.clipCount() == 2);
		CHECK(shown.last() == printed("P01", 4, "scene 1: 2 clips, 1 track added"));

		set.tracks = {deviceTrack("Drums"), midiTo("#-MIDI", "Drums", "1-MD Drums")};
		Printer printer(set);
		CHECK(printer.print(hitsOn({0, 1}), 0, 4, 1, false).again);
		set.tracks.insert(set.tracks.begin(), takeOut(set, "3-MIDI"));
		shown = follow(printer, set, {{}, true});
		CHECK((set.names() == std::vector<std::string>{"1-MIDI", "Drums", "MD 02"}));
		CHECK(set.clipCount() == 0);
		CHECK(shown.last() == "Not printed: tracks moved while printing: 2-MIDI, MD 02. 1 track added.");
	}

	void clipsRefused()
	{
		// Live makes none of the clips: not printed, and the Tracks named, never "Nothing to print".
		FakeSet set = aSet();
		set.refuseClipsOn = {"MD 01", "MD 03"};
		Shown shown = printOnce(set, hitsOn({0, 2}));
		CHECK(set.clipCount() == 0);
		CHECK(shown.last() == "Not printed: Live made no clip in scene 4 for Track 01, Track 03. 2 tracks added.");

		set = aSet();
		set.refuseClipsOn = {"Hats"};
		shown = printOnce(set, hitsOn({6}));
		CHECK(shown.last() == std::string("Not printed: Live made no clip in scene 4 for Track 07.") + TwoOnSeven);

		// Some clips made: printed, the others named.
		set = aSet();
		set.refuseClipsOn = {"Hats"};
		shown = printOnce(set, hitsOn({0, 6}));
		CHECK(shown.last() ==
			printed("P01", 4, "scene 4: 1 clip, 1 track added") + TwoOnSeven + " no clip for Track 07.");
	}

	void notPrinted()
	{
		// No Hit: nothing read, nothing written.
		FakeSet set = aSet();
		Shown shown = printOnce(set, Pattern(), 2);
		CHECK((shown.statuses == std::vector<std::string>{"Nothing to print: P03 has no Hit"}));
		CHECK(shown.steps == 0 && set.reads == 0 && set.writes == 0);

		// A FILL step prints with the Fill held only.
		Pattern fill = hitsOn({6});
		fill.steps[6][0].condition = static_cast<uint8_t>(conditionIndex("FILL"));
		Printer printer(set);
		CHECK(follow(printer, set, printer.print(fill, 0, 4, 1, false)).last() == "Nothing to print: P01 has no Hit");
		CHECK(follow(printer, set, printer.print(fill, 0, 4, 1, true)).last().rfind("Printed P01", 0) == 0);

		// The device in a rack: its parent is no track.
		set = aSet();
		set.tracks[1].device = false;
		shown = printOnce(set, hitsOn({0}));
		CHECK(shown.last() == "Not printed: the device must sit on a track, not in a rack");
		CHECK(shown.steps == 0 && set.writes == 0);

		// Live creates no track.
		set = aSet();
		set.refuseTracks = true;
		shown = printOnce(set, hitsOn({0, 6}));
		CHECK(shown.last() == "Not printed: Live did not create MD 01");
		CHECK(shown.steps == 0 && set.writes == 0 && set.clipCount() == 0);

		// Live creates the first track only: no clip, but the track made gets its channel and is told, so that the
		// next print finds it rather than adding a second "MD 01".
		set = aSet();
		set.tracksAllowed = 1;
		shown = printOnce(set, hitsOn({0, 2, 6}));
		CHECK(shown.last() == std::string("Not printed: Live did not create MD 03. 1 track added.") + TwoOnSeven);
		CHECK(set.named("MD 01").outputChannel == "1-MD Drums" && set.named("MD 03").name.empty());
		CHECK(set.clipCount() == 0);
		set.tracksAllowed = -1;
		CHECK(printOnce(set, hitsOn({0, 2, 6})).last() ==
			printed("P01", 4, "scene 4: 3 clips, 1 track added") + TwoOnSeven);
		CHECK((set.names() == std::vector<std::string>{"KICK", "Drums", "MD 03", "MD 01", "Hats", "Busy", "Hats 2",
			"Other", "Audio"}));
		CHECK(set.clip("MD 01", 3) && set.clip("MD 03", 3) && set.clip("Hats", 3));

		// The same with a track made, then moved before it gets its channel: the status says it was left without.
		set = aSet();
		set.tracksAllowed = 2;
		Printer refused(set);
		CHECK(refused.print(hitsOn({0, 2, 4}), 0, 4, 1, false).again);
		set.tracks.push_back(takeOut(set, "MD 03"));
		shown = follow(refused, set, {{}, true});
		CHECK(set.named("MD 01").outputChannel == "1-MD Drums" && set.named("MD 03").outputChannel == "Track In");
		CHECK(shown.last() ==
			"Not printed: Live did not create MD 05. 2 tracks added. MD 03 could not be sent to Drums's channel.");
		CHECK(set.clipCount() == 0);
	}

	void oneAtATime()
	{
		// A print asked while one is under way does nothing; the next one after it prints.
		FakeSet set = aSet();
		Printer printer(set);
		CHECK(printer.resume().status.empty());
		CHECK(printer.print(hitsOn({0}), 0, 4, 1, false).again && printer.printing());
		const int reads = set.reads;
		const int writes = set.writes;
		const Printer::Report ignored = printer.print(hitsOn({6}), 0, 4, 1, false);
		CHECK(ignored.status.empty() && !ignored.again && set.reads == reads && set.writes == writes);
		CHECK(follow(printer, set, {{}, true}).last().rfind("Printed P01", 0) == 0);
		CHECK(!printer.printing() && set.clipCount() == 1);
		CHECK(follow(printer, set, printer.print(hitsOn({6}), 0, 4, 1, false)).last().rfind("Printed P01", 0) == 0);
		CHECK(set.clipCount() == 2);
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	channels();
	instrumentRenamed();
	notes();
	tracksFoundAndCreated();
	trackBeforeTheDevice();
	clipContents();
	scenes();
	channelsListedLate();
	channelsNeverListed();
	typeRefused();
	trackMoved();
	autoNumbered();
	clipsRefused();
	notPrinted();
	oneAtATime();
	return mdseqTest::finish("mdseqPrinterTest");
}
