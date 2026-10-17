// mdseq: the Max external that runs the sequencer's core in a Max for Live device (ADR 0002). It is the adapter: the
// core's modules decide, and this file carries their decisions across Max's threads and into atoms.
//
// Two copies of the bank, one per thread, kept equal by the same applyEdit: the Sequencer plays on Max's scheduler
// thread (Live's audio thread in a device), where it never allocates, locks or blocks; the Editor, the PatternStore,
// the Printer and the LOM run on the main thread. The external looks at Live's transport every millisecond and hands
// the Sequencer what it reads, from which the Sequencer tells starts, stops and jumps; it wakes on an ITM timer at the
// tick the Sequencer asks for, sending the MIDI due out of its left outlet as bytes for midiout. Interface messages run
// the Editor on the main thread (one that arrives on the scheduler thread, a mapped parameter, is deferred there
// first); the edits it forwards move to the scheduler thread through schedule_delay(), and a Pattern the set restores
// goes over as a pointer that comes back to the main thread to be freed. The other way, two atomic words: the
// Playhead of the last wake (the step played, the Pattern played and the one queued), which a qelem shows, and the
// seed of the last transport start, which a print renders from.
//
// Inlet: the messages of mdseqProtocol.h, registered from its table. The Editor's go to mdseq::toEditor; the
// external's own are handled here: restore p words... (Pattern p as its pattr Blob keeps it: mdseq::PatternStore tells
// an echo of a write from a Pattern to load, and words this build cannot read, which load as an empty Pattern and
// are told in the Max console), passes n and print (the Pattern shown into Session clips, n Passes of it
// from the last start's seed, through mdseq::Printer, whose second step a clock and a qelem call back on the main
// thread), lomid and lomreply (live.path's and live.object's answers to the lom message being sent, which arrive
// before it returns).
// Outlets, left to right: MIDI bytes; strip lcd; grid lcd; panel. The panel outlet sends the panel keys of
// mdseqProtocol.h: the Editor's values, as it words them; printstatus text, what a print did; lom path|id|obj ..., a
// message to live.path or live.object; store p words..., Pattern p for its pattr Blob, half a second after the last
// edit, so that one gesture makes one entry in Live's undo history, and only when the Blob does not hold those words
// already (mdseq::PatternStore decides).
//
// Every half second a clock and a qelem read, on the main thread, the file MD Drums writes for the device's id
// (mdseq::KitLink, ADR 0003), whose Kit names the Tracks; the hello that gives MD Drums the id goes out of the MIDI
// outlet from the scheduler thread, through schedule_delay(), between two wakes.
//
// A development build (MDSEQ_DEV) also logs to %TEMP%\mdseq-log.txt, starts with a demonstration Pattern at the first
// redraw and sends the lines of %TEMP%\mdseq-lomprobe.txt to the LOM (see loadDemo and lomProbe).

#include "ext.h"
#include "ext_dictobj.h"
#include "ext_obex.h"
#include "ext_itm.h"
#include "ext_time.h"

#include "mdseqEdit.h"
#include "mdseqEditor.h"
#include "mdseqKit.h"
#include "mdseqPrinter.h"
#include "mdseqProtocol.h"
#include "mdseqSequencer.h"
#include "mdseqStore.h"

#include <array>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <new>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "mdseqUpdates.h"
#include "updates.h"

namespace
{
	constexpr double HeartbeatMs = 1.0;
	// How long after the last edit the Pattern goes to the set: a drag or a dial turned is one write, so one entry in
	// Live's undo history.
	constexpr double StoreDelayMs = 500.0;

	class LomSet;

	// A newer release, which a thread of its own asks GitHub for (updates.h) and hands the main thread through a qelem
	// that the device may free first: the thread sets it under the lock, and only while the device lives.
	struct UpdateLine
	{
		std::mutex lock;
		bool alive = true;
		t_qelem* qelem = nullptr;
		std::string notice;		// the strip's line, empty for none
	};

	struct MdSeq
	{
		t_object object;
		void* midiOutlet;
		void* stripOutlet;
		void* gridOutlet;
		void* panelOutlet;
		t_object* timer;
		t_clock* heartbeat;
		t_qelem* playedQelem;
		t_clock* storeClock;			// armed on each edit, fires on the scheduler thread
		t_qelem* storeQelem;			// writes the Blobs on the main thread
		mdseq::PatternStore* store;		// main thread: what to write, and what a Blob's value received is
		t_itm* itm;
		mdseq::Sequencer* sequencer;	// scheduler thread
		mdseq::Editor* editor;			// main thread
		double armedTick;
		// Written on the scheduler thread and read on the main thread, each in one access without a lock. object_alloc
		// constructs nothing: create constructs them in place, destroy destroys them.
		std::atomic<uint32_t> playhead;		// mdseq::packPlayhead of the last wake, which the played qelem shows
		std::atomic<uint64_t> startSeed;	// the last transport start's, 0 before any
		long printPasses;				// the Passes asked for a print, which the Printer keeps to 1-16
		LomSet* lomSet;					// the set as printing reaches it, main thread only
		mdseq::Printer* printer;		// main thread only
		t_clock* printClock;			// a print's second step, soon: fires on the scheduler thread
		t_qelem* printQelem;			// and runs it on the main thread
		mdseq::KitLink* kitLink;		// main thread: the hello's id, and the Kit MD Drums tells (ADR 0003)
		uint32_t helloId;				// its id, which the scheduler thread's hello reads; constant once created
		t_clock* kitClock;				// every KitLink::PollMs, on the scheduler thread
		t_qelem* kitQelem;				// and the file read on the main thread
		std::shared_ptr<UpdateLine>* update;	// main thread; shared with the thread that looks for a release
		bool updatesEnabled;			// main thread: the settings' "updates on|off" as the window's switch shows it

		// Diagnostics, written on the scheduler thread and logged from the main thread (see logState)
		long heartbeats;
		long timerWakes;
		long eventsSent;
		long editsApplied;
		bool lastLoggedPlaying;
		bool demoLoaded;				// development builds: the demonstration Pattern was applied
		// The LOM's answers to the message being sent ("lomid ..." and "lomreply ..."), main thread only.
		std::vector<std::vector<t_atom>>* lomInbox;
	};

	// Live's audio thread may not wait: wherever the device is built, both words are atomic without a lock.
	static_assert(std::atomic<uint32_t>::is_always_lock_free && std::atomic<uint64_t>::is_always_lock_free,
		"the scheduler thread hands the main thread its Playhead and seed without a lock");

	t_class* s_class = nullptr;

	t_symbol* sym(const char* _name) { return gensym(_name); }

	// ---- Diagnostics: one line per event in %TEMP%\mdseq-log.txt, written on the main thread only ----
	//
	// Nobody reads Max's console from outside Live; this file can be read while Live runs. Development builds only
	// (MDSEQ_DEV): a release writes nothing, and counts nothing for it on the scheduler thread.
#ifdef MDSEQ_DEV
	constexpr bool Dev = true;
#else
	constexpr bool Dev = false;
#endif

	// A development build's file in %TEMP%: the log, the demonstration Pattern, the LOM probe.
	FILE* openTempFile([[maybe_unused]] const char* _name, [[maybe_unused]] const char* _mode)
	{
		char path[512];
		const char* folder = getenv("TEMP");
		snprintf(path, sizeof(path), "%s\\%s", folder ? folder : ".", _name);
		return fopen(path, _mode);
	}

	// 64 bits of the system's entropy: the Conditions' seed source, and a print's seed before any start.
	uint64_t randomSeed()
	{
		std::random_device entropy;
		return (static_cast<uint64_t>(entropy()) << 32) | entropy();
	}

	void logLine([[maybe_unused]] MdSeq* _x, [[maybe_unused]] const char* _format, ...)
	{
#ifndef MDSEQ_DEV
		return;
#else
		FILE* file = openTempFile("mdseq-log.txt", "a");
		if(!file)
			return;
		double now = 0.0;
		clock_getftime(&now);
		fprintf(file, "%p %10.1f ", static_cast<void*>(_x), now);
		va_list args;
		va_start(args, _format);
		vfprintf(file, _format, args);
		va_end(args);
		fputc('\n', file);
		fclose(file);
#endif
	}

	void logItm(MdSeq* _x, const char* _label, t_itm* _itm)
	{
		if(!_itm)
		{
			logLine(_x, "%s: none", _label);
			return;
		}
		const t_symbol* name = itm_getname(_itm);
		logLine(_x, "%s: %p name '%s' state %ld ticks %.2f tempo %.2f", _label, static_cast<void*>(_itm),
			name ? name->s_name : "", itm_getstate(_itm), itm_getticks(_itm), itm_gettempo(_itm));
	}

	void logState(MdSeq* _x)
	{
		logLine(_x, "heartbeats %ld timer wakes %ld events %ld edits %ld played %d", _x->heartbeats, _x->timerWakes,
			_x->eventsSent, _x->editsApplied,
			mdseq::unpackPlayhead(_x->playhead.load(std::memory_order_relaxed)).step);
		logItm(_x, "used itm", _x->itm);
		logItm(_x, "global itm", static_cast<t_itm*>(itm_getglobal()));
		logItm(_x, "timer itm", static_cast<t_itm*>(time_getitm(reinterpret_cast<t_timeobject*>(_x->timer))));
	}

	// ---- Scheduler thread ----

	void arm(MdSeq* _x, const double _next, const double _ticks)
	{
		if(_next == _x->armedTick)
			return;
		_x->armedTick = _next;
		time_stop(_x->timer);
		if(_next == mdseq::Never)
			return;

		t_atom delay;
		atom_setfloat(&delay, _next > _ticks ? _next - _ticks : 0.0);
		time_setvalue(_x->timer, nullptr, 1, &delay);
		time_schedule(_x->timer, nullptr);
	}

	void run(MdSeq* _x)
	{
		// The transport as read: the Sequencer tells a start, a stop and a jump from one look to the next.
		mdseq::Transport look;
		look.ticks = itm_getticks(_x->itm);
		look.playing = itm_getstate(_x->itm) != 0;
		clock_getftime(&look.ms);
		look.ticksPerMs = itm_mstoticks(_x->itm, 1.0);

		const mdseq::Wake wake = _x->sequencer->wake(look);
		// Relaxed, here and for the Playhead: each word holds all it says, and publishes nothing else.
		_x->startSeed.store(_x->sequencer->startSeed(), std::memory_order_relaxed);
		for(const mdseq::MidiEvent* e = wake.begin; e != wake.end; ++e)
		{
			outlet_int(_x->midiOutlet, e->bytes[0]);
			outlet_int(_x->midiOutlet, e->bytes[1]);
			outlet_int(_x->midiOutlet, e->bytes[2]);
		}

#ifdef MDSEQ_DEV
		_x->eventsSent += static_cast<long>(wake.end - wake.begin);
		++_x->heartbeats;
		// A line when the transport starts or stops, and once a second.
		if(_x->heartbeats % 1000 == 0 || look.playing != _x->lastLoggedPlaying)
		{
			_x->lastLoggedPlaying = look.playing;
			defer_low(_x, reinterpret_cast<method>(logState), nullptr, 0, nullptr);
		}
#endif

		// The step, the Pattern and the one queued in one word: the qelem reads the three of the same wake.
		const uint32_t playhead = mdseq::packPlayhead(mdseq::playhead(wake));
		if(playhead != _x->playhead.load(std::memory_order_relaxed))
		{
			_x->playhead.store(playhead, std::memory_order_relaxed);
			qelem_set(_x->playedQelem);
		}

		arm(_x, wake.next, look.ticks);
	}

	void heartbeat(MdSeq* _x)
	{
		run(_x);
		clock_fdelay(_x->heartbeat, HeartbeatMs);
	}

	void timer(MdSeq* _x)
	{
		_x->armedTick = mdseq::Never;
		if constexpr(Dev)
			++_x->timerWakes;
		run(_x);
	}

	// An edit the Editor made, now applied to the Sequencer: kind, track, step, value.
	void applyToSequencer(MdSeq* _x, t_symbol*, const short _argc, t_atom* _argv)
	{
		if(_argc < mdseq::InputWords)
			return;
		std::array<int, mdseq::InputWords> words{};
		for(int i = 0; i < mdseq::InputWords; ++i)
			words[i] = static_cast<int>(atom_getlong(_argv + i));
		const bool applied = _x->sequencer->input(mdseq::unpack(words));
		if constexpr(Dev)
			_x->editsApplied += applied ? 1 : 0;
	}

	void emit(MdSeq* _x, const mdseq::EditorOutput& _out);

	// A whole Pattern, decoded on the main thread, handed over as a pointer: the scheduler thread copies it into the
	// Sequencer's bank (no allocation: a Pattern is arrays) and gives the pointer back with defer_low, so that the
	// Pattern is allocated and freed on the main thread only. Max allocates a copy of schedule_delay's and defer_low's
	// atoms itself, one atom on Live's audio thread here: the defer_low handoff ADR 0002 accepts.
	// When the object is freed with handoffs pending (one per Pattern restored, up to 16 at a set's load), a
	// freeLoaded pending is harmless whether Max delivers it or drops it, the Pattern then leaking: it never touches
	// the object. A loadIntoSequencer pending, like an applyToSequencer, reads the freed object and its deleted
	// Sequencer if Max delivers it. The SDK does not say whether it does, and destroy cannot cancel either call: open
	// (ticket 14).
	void freeLoaded(MdSeq*, t_symbol*, const short _argc, t_atom* _argv)
	{
		if(_argc > 0)
			delete static_cast<mdseq::Pattern*>(atom_getobj(_argv));
	}

	// index, pointer
	void loadIntoSequencer(MdSeq* _x, t_symbol*, const short _argc, t_atom* _argv)
	{
		if(_argc < 2)
			return;
		const auto* pattern = static_cast<mdseq::Pattern*>(atom_getobj(_argv + 1));
		_x->sequencer->load(static_cast<int>(atom_getlong(_argv)), *pattern);
		defer_low(_x, reinterpret_cast<method>(freeLoaded), nullptr, 1, _argv + 1);
	}

	void storeSoon(MdSeq* _x)
	{
		qelem_set(_x->storeQelem);
	}

	// The hello's eight messages out of the MIDI outlet, between two wakes of the Sequencer: the device's id for MD
	// Drums, which writes its Kit into the file of that name.
	void sendHello(MdSeq* _x, t_symbol*, short, t_atom*)
	{
		for(const mdseq::MidiBytes& message : mdseq::hello(_x->helloId))
			for(const uint8_t byte : message)
				outlet_int(_x->midiOutlet, byte);
	}

	void kitSoon(MdSeq* _x)
	{
		qelem_set(_x->kitQelem);
		clock_fdelay(_x->kitClock, mdseq::KitLink::PollMs);
	}

	// ---- Main thread ----

	void storeOnMain(MdSeq* _x)
	{
		// Each Pattern due, to its own Blob: "store p words...".
		for(const mdseq::PatternStore::Write& w : _x->store->due(_x->editor->bank()))
		{
			std::vector<t_atom> atoms(w.words.size() + 1);
			atom_setlong(&atoms[0], w.pattern);
			for(size_t i = 0; i < w.words.size(); ++i)
				atom_setlong(&atoms[i + 1], static_cast<t_atom_long>(w.words[i]));
			const double begin = Dev ? systimer_gettime() : 0.0;
			outlet_anything(_x->panelOutlet, sym(mdseq::panelKey(mdseq::PanelKey::Store)),
				static_cast<short>(atoms.size()), atoms.data());
			if constexpr(Dev)
				logLine(_x, "store: Pattern %d, %zu words, %.2f ms into the pattr", w.pattern + 1, w.words.size(),
					systimer_gettime() - begin);
		}
	}

	void restore(MdSeq* _x, const long _argc, t_atom* _argv)
	{
		if(_argc < 1)
			return;
		const auto index = static_cast<int>(atom_getlong(_argv));
		std::vector<int64_t> words(static_cast<size_t>(_argc - 1));
		for(long i = 1; i < _argc; ++i)
			words[static_cast<size_t>(i - 1)] = static_cast<int64_t>(atom_getlong(_argv + i));
		auto pattern = std::make_unique<mdseq::Pattern>();
		switch(_x->store->received(index, words.data(), words.size(), *pattern))
		{
		case mdseq::PatternStore::Received::Echo:
			logLine(_x, "restore: Pattern %d, %zu words, the Pattern held already: ignored", index + 1, words.size());
			return;
		case mdseq::PatternStore::Received::Unusable:
			logLine(_x, "restore: Pattern %d is outside the bank: ignored", index + 1);
			return;
		case mdseq::PatternStore::Received::Refused:
			// The device shows and plays the empty Pattern the store gives: the user has to be told why.
			object_warn(reinterpret_cast<t_object*>(_x),
				"Pattern %d: the set holds words this version cannot read, shown empty until it is edited", index + 1);
			logLine(_x, "restore: Pattern %d, %zu words refused: emptied", index + 1, words.size());
			break;
		case mdseq::PatternStore::Received::Load:
			logLine(_x, "restore: Pattern %d, %zu words loaded", index + 1, words.size());
			break;
		}
		emit(_x, _x->editor->load(index, *pattern));
		t_atom a[2];
		atom_setlong(a, index);
		atom_setobj(a + 1, pattern.release());
		schedule_delay(_x, reinterpret_cast<method>(loadIntoSequencer), 0, nullptr, 2, a);
	}

	void sendOps(void* _outlet, const std::vector<mdseq::DrawOp>& _ops)
	{
		for(const mdseq::DrawOp& op : _ops)
		{
			if(op.kind == mdseq::DrawOp::Kind::Clear)
			{
				outlet_anything(_outlet, sym("clear"), 0, nullptr);
				continue;
			}
			if(op.kind == mdseq::DrawOp::Kind::Text)
			{
				// lcd draws text in its pen colour, from the pen position (the baseline's left end).
				t_atom a[3];
				atom_setlong(a, op.colour.r);
				atom_setlong(a + 1, op.colour.g);
				atom_setlong(a + 2, op.colour.b);
				outlet_anything(_outlet, sym("frgb"), 3, a);
				atom_setsym(a, sym("Arial"));
				atom_setlong(a + 1, 10);
				outlet_anything(_outlet, sym("font"), 2, a);
				atom_setsym(a, sym("bold"));
				outlet_anything(_outlet, sym("textface"), 1, a);
				atom_setlong(a, op.left);
				atom_setlong(a + 1, op.top);
				outlet_anything(_outlet, sym("moveto"), 2, a);
				atom_setsym(a, sym(op.text));
				outlet_anything(_outlet, sym("write"), 1, a);
				continue;
			}
			if(op.kind == mdseq::DrawOp::Kind::Triangle)
			{
				// paintpoly takes x/y pairs and paints in the pen colour.
				t_atom a[6];
				atom_setlong(a, op.colour.r);
				atom_setlong(a + 1, op.colour.g);
				atom_setlong(a + 2, op.colour.b);
				outlet_anything(_outlet, sym("frgb"), 3, a);
				atom_setlong(a, op.left);
				atom_setlong(a + 1, op.top);
				atom_setlong(a + 2, op.right);
				atom_setlong(a + 3, (op.top + op.bottom) / 2);
				atom_setlong(a + 4, op.left);
				atom_setlong(a + 5, op.bottom);
				outlet_anything(_outlet, sym("paintpoly"), 6, a);
				continue;
			}
			t_atom a[7];
			atom_setlong(a, op.left);
			atom_setlong(a + 1, op.top);
			atom_setlong(a + 2, op.right);
			atom_setlong(a + 3, op.bottom);
			atom_setlong(a + 4, op.colour.r);
			atom_setlong(a + 5, op.colour.g);
			atom_setlong(a + 6, op.colour.b);
			const char* selector = op.kind == mdseq::DrawOp::Kind::PaintRect ? "paintrect"
				: op.kind == mdseq::DrawOp::Kind::FrameRect ? "framerect"
				: "paintoval";
			outlet_anything(_outlet, sym(selector), 7, a);
		}
	}

	void sendText(void* _outlet, const char* _selector, const char* _text)
	{
		t_atom a;
		atom_setsym(&a, gensym(_text));
		outlet_anything(_outlet, sym(_selector), 1, &a);
	}

	// One message to the LOM ("path ...", "id n", "obj get|set|call ..."): its answers, which come back within the
	// call.
	const std::vector<std::vector<t_atom>>& lomAsk(MdSeq* _x, const long _argc, t_atom* _argv)
	{
		_x->lomInbox->clear();
		outlet_anything(_x->panelOutlet, sym(mdseq::panelKey(mdseq::PanelKey::Lom)), static_cast<short>(_argc), _argv);
		return *_x->lomInbox;
	}

	// A dictionary as JSON text.
	std::string dictionaryJson(t_dictionary* _d)
	{
		if(!_d)
			return {};
		t_object* writer = static_cast<t_object*>(object_new(gensym("nobox"), gensym("jsonwriter")));
		object_method(writer, gensym("writedictionary"), _d);
		t_handle json = nullptr;
		object_method(writer, gensym("getoutput"), &json);
		std::string text = json && *json ? *json : "";
		object_free(writer);
		return text;
	}

	std::string atomsText(const long _argc, t_atom* _argv)
	{
		long size = 0;
		char* text = nullptr;
		atom_gettext(_argc, _argv, &size, &text, OBEX_UTIL_ATOM_GETTEXT_DEFAULT);
		std::string s = text ? text : "";
		if(text)
			sysmem_freeptr(text);
		return s;
	}

	// A message to the LOM written as text ("path live_set tracks 3", "obj get name"); its answers.
	std::vector<std::vector<t_atom>> lomText(MdSeq* _x, const std::string& _text)
	{
		long argc = 0;
		t_atom* argv = nullptr;
		atom_setparse(&argc, &argv, _text.c_str());
		std::vector<std::vector<t_atom>> answers = lomAsk(_x, argc, argv);
		if(argv)
			sysmem_freeptr(argv);
		return answers;
	}

	// A dictionary entry's text, whether Max keeps it as a symbol or a string.
	std::string entryText(t_dictionary* _d, const char* _key)
	{
		t_symbol* s = nullptr;
		if(dictionary_getsym(_d, gensym(_key), &s) == MAX_ERR_NONE && s)
			return s->s_name;
		const char* text = nullptr;
		if(dictionary_getstring(_d, gensym(_key), &text) == MAX_ERR_NONE && text)
			return text;
		return {};
	}

	// ---- Printing (issue 22): the set as mdseq::Printer reaches it, through live.path and live.object ----
	//
	// Formats measured in Live 12 on 2026-10-14: a path answers "lomid id <n>"; "get" answers "lomreply <property>
	// <values>", a routing property "lomreply <property> dictionary <name>"; create_midi_track and create_clip answer
	// "id <n>"; add_new_notes takes "dictionary <name>"; a routing is set by the dictionary of one entry of its
	// available list.

	// What a message's first answer holds after its selector and property: "lomreply name MD 03" gives "MD 03".
	std::vector<t_atom> lomValue(MdSeq* _x, const std::string& _text)
	{
		const auto answers = lomText(_x, _text);
		if(answers.empty() || answers[0].size() < 2)
			return {};
		return std::vector<t_atom>(answers[0].begin() + 2, answers[0].end());
	}

	// The ids in atoms ("id 7 id 9"), in order.
	std::vector<long> lomIds(const std::vector<t_atom>& _atoms)
	{
		std::vector<long> ids;
		for(size_t i = 0; i + 1 < _atoms.size(); ++i)
			if(atom_gettype(&_atoms[i]) == A_SYM && atom_getsym(&_atoms[i]) == gensym("id"))
				ids.push_back(static_cast<long>(atom_getlong(&_atoms[i + 1])));
		return ids;
	}

	// The id a path or a call answers with, 0 for none.
	long lomId(const std::vector<std::vector<t_atom>>& _answers)
	{
		for(const auto& answer : _answers)
		{
			const std::vector<long> ids = lomIds(answer);
			if(!ids.empty())
				return ids[0];
		}
		return 0;
	}

	// One value as text: a name with spaces comes as one symbol.
	std::string valueText(const std::vector<t_atom>& _atoms)
	{
		if(_atoms.size() == 1 && atom_gettype(&_atoms[0]) == A_SYM)
			return atom_getsym(&_atoms[0])->s_name;
		std::vector<t_atom> copy = _atoms;
		return atomsText(static_cast<long>(copy.size()), copy.data());
	}

	// The dictionary an answer names, retained (dictobj_release it), or null.
	t_dictionary* answerDictionary(const std::vector<t_atom>& _atoms)
	{
		for(size_t i = 0; i + 1 < _atoms.size(); ++i)
			if(atom_gettype(&_atoms[i]) == A_SYM && atom_getsym(&_atoms[i]) == gensym("dictionary"))
				return dictobj_findregistered_retain(atom_getsym(&_atoms[i + 1]));
		return nullptr;
	}

	// The set as mdseq::Printer reads and writes it, which decides what to read and write and when: each call goes to
	// the object it names, then asks live.object. Main thread; each write is an entry in Live's undo history.
	class LomSet final : public mdseq::LiveSet
	{
	public:
		explicit LomSet(MdSeq* _x) : m_x(_x) {}

		// Every track's name, kind and MIDI output, the device's track (none when it is not straight on a track: in a
		// rack, its parent is a chain), and the selected scene.
		mdseq::SetView read() override
		{
			mdseq::SetView set;
			const long deviceTrack = lomId(lomText(m_x, "path this_device canonical_parent"));
			lomText(m_x, "path live_set");
			const std::vector<long> tracks = lomIds(lomValue(m_x, "obj get tracks"));
			const std::vector<long> scenes = lomIds(lomValue(m_x, "obj get scenes"));
			const long selected = lomId(lomText(m_x, "path live_set view selected_scene"));
			for(size_t s = 0; s < scenes.size(); ++s)
				if(scenes[s] == selected)
					set.selectedScene = static_cast<int>(s);
			for(size_t i = 0; i < tracks.size(); ++i)
			{
				if(tracks[i] == deviceTrack)
					set.deviceTrack = static_cast<int>(i);
				lomText(m_x, "id " + std::to_string(tracks[i]));
				mdseq::SetTrack t;
				t.name = valueText(lomValue(m_x, "obj get name"));
				const std::vector<t_atom> midi = lomValue(m_x, "obj get has_midi_input");
				t.midi = !midi.empty() && atom_getlong(&midi[0]) != 0;
				if(t.midi)
				{
					t.outputType = routingName("output_routing_type");
					t.outputChannel = routingName("output_routing_channel");
				}
				set.tracks.push_back(t);
			}
			return set;
		}

		bool hasClip(const int _index, const int _scene) override
		{
			lomText(m_x, slotPath(_index, _scene));
			const std::vector<t_atom> full = lomValue(m_x, "obj get has_clip");
			return !full.empty() && atom_getlong(&full[0]) != 0;
		}

		bool createMidiTrack(const int _index, const std::string& _name) override
		{
			lomText(m_x, "path live_set");
			// Without the new track's id, the track at _index is another one: it keeps its name.
			if(lomId(lomText(m_x, "obj call create_midi_track " + std::to_string(_index))) == 0)
				return false;
			lomText(m_x, trackPath(_index));
			setText("name", _name);
			return true;
		}

		bool setOutputType(const int _index, const std::string& _type) override
		{
			lomText(m_x, trackPath(_index));
			return setRouting("output_routing_type", [&](const std::string& _name) { return _name == _type; });
		}

		bool setOutputChannel(const int _index, const int _track) override
		{
			lomText(m_x, trackPath(_index));
			return setRouting("output_routing_channel",
				[&](const std::string& _name) { return mdseq::channelTrack(_name) == _track; });
		}

		std::string trackName(const int _index) override
		{
			lomText(m_x, trackPath(_index));
			return valueText(lomValue(m_x, "obj get name"));
		}

		void createScene(const int _index) override
		{
			lomText(m_x, "path live_set");
			lomText(m_x, "obj call create_scene " + std::to_string(_index));
		}

		bool createClip(const int _index, const int _scene, const double _length, const std::string& _name,
			const std::vector<mdseq::PrintedNote>& _notes) override
		{
			const std::string path = slotPath(_index, _scene);
			lomText(m_x, path);
			if(lomId(lomText(m_x, "obj call create_clip " + std::to_string(_length))) == 0)
				return false;
			lomText(m_x, path + " clip");
			setText("name", _name);
			withDictionary("call", "add_new_notes", mdseq::notesJson(_notes));
			return true;
		}

		// Sets a routing property of the object live.object holds to the first entry of its available list whose
		// display name _pick takes. True when Live took it.
		bool setRouting(const std::string& _property, const std::function<bool(const std::string&)>& _pick)
		{
			const std::string listName = "available_" + _property + "s";
			t_dictionary* all = answerDictionary(lomValue(m_x, "obj get " + listName));
			if(!all)
				return false;
			std::string entry;
			t_object* entries = nullptr;
			dictionary_getatomarray(all, gensym(listName.c_str()), &entries);
			long count = 0;
			t_atom* atoms = nullptr;
			if(entries)
				atomarray_getatoms(reinterpret_cast<t_atomarray*>(entries), &count, &atoms);
			std::string wanted;
			for(long e = 0; e < count && entry.empty(); ++e)
			{
				auto* d = static_cast<t_dictionary*>(atom_getobj(&atoms[e]));
				const std::string name = d ? entryText(d, "display_name") : std::string();
				if(d && _pick(name))
				{
					entry = dictionaryJson(d);
					wanted = name;
				}
			}
			dictobj_release(all);
			if(entry.empty())
			{
				logLine(m_x, "routing %s: no entry taken among %ld", _property.c_str(), count);
				return false;
			}
			withDictionary("set", _property, entry);
			const std::string now = routingName(_property);
			logLine(m_x, "routing %s: set %s, now %s", _property.c_str(), wanted.c_str(), now.c_str());
			return now == wanted;
		}

	private:
		static std::string trackPath(const int _index)
		{
			return "path live_set tracks " + std::to_string(_index);
		}

		static std::string slotPath(const int _index, const int _scene)
		{
			return trackPath(_index) + " clip_slots " + std::to_string(_scene);
		}

		// The display name of a routing property of the object live.object holds ("output_routing_channel").
		std::string routingName(const std::string& _property)
		{
			t_dictionary* d = answerDictionary(lomValue(m_x, "obj get " + _property));
			if(!d)
				return {};
			t_object* entry = nullptr;
			dictionary_getdictionary(d, gensym(_property.c_str()), &entry);
			std::string name =
				entry ? entryText(reinterpret_cast<t_dictionary*>(entry), "display_name") : std::string();
			dictobj_release(d);
			return name;
		}

		// Sends "obj <verb> <property|method> dictionary <name>" with a dictionary made from JSON; its answers.
		std::vector<std::vector<t_atom>> withDictionary(const char* _verb, const std::string& _name,
			const std::string& _json)
		{
			t_dictionary* d = nullptr;
			char error[256] = "";
			dictobj_dictionaryfromstring(&d, _json.c_str(), 1, error);
			if(!d)
				return {};
			t_symbol* registered = nullptr;
			d = dictobj_register(d, &registered);
			t_atom atoms[5];
			atom_setsym(&atoms[0], gensym("obj"));
			atom_setsym(&atoms[1], gensym(_verb));
			atom_setsym(&atoms[2], gensym(_name.c_str()));
			atom_setsym(&atoms[3], gensym("dictionary"));
			atom_setsym(&atoms[4], registered);
			std::vector<std::vector<t_atom>> answers = lomAsk(m_x, 5, atoms);
			dictobj_release(d);
			return answers;
		}

		// Sets a text property ("name") of the object live.object holds.
		void setText(const char* _property, const std::string& _text)
		{
			t_atom atoms[4];
			atom_setsym(&atoms[0], gensym("obj"));
			atom_setsym(&atoms[1], gensym("set"));
			atom_setsym(&atoms[2], gensym(_property));
			atom_setsym(&atoms[3], gensym(_text.c_str()));
			lomAsk(m_x, 4, atoms);
		}

		MdSeq* m_x;
	};

	// Shows what a step of a print says, and arms its next step when the Printer asks for one.
	void printReport(MdSeq* _x, const mdseq::Printer::Report& _report)
	{
		if(!_report.status.empty())
		{
			sendText(_x->panelOutlet, mdseq::panelKey(mdseq::PanelKey::PrintStatus), _report.status.c_str());
			logLine(_x, "print: %s", _report.status.c_str());
		}
		if(_report.again)
			clock_fdelay(_x->printClock, mdseq::Printer::RetryMs);
	}

	// Prints the Pattern shown: its notes over the Passes chosen, from the seed of the last transport start (what was
	// heard; a new one if the transport never started), the Fill as held, where mdseq::Printer puts them. Main thread.
	void print(MdSeq* _x)
	{
		const mdseq::Editor& e = *_x->editor;
		uint64_t seed = _x->startSeed.load(std::memory_order_relaxed);
		if(seed == 0)
		{
			seed = randomSeed();
		}
		printReport(_x, _x->printer->print(e.pattern(), e.shown(), static_cast<int>(_x->printPasses), seed, e.fill()));
	}

	void printSoon(MdSeq* _x)
	{
		qelem_set(_x->printQelem);
	}

	// A print's second step, from a later main-thread turn (see mdseq::Printer::resume).
	void finishPrint(MdSeq* _x)
	{
		printReport(_x, _x->printer->resume());
	}

	// A panel value as the Editor worded it: its key, its integers, then its texts, each one symbol.
	void sendPanel(MdSeq* _x, const mdseq::PanelValue& _v)
	{
		std::vector<t_atom> a(_v.ints.size() + _v.texts.size());
		for(size_t i = 0; i < _v.ints.size(); ++i)
			atom_setlong(&a[i], _v.ints[i]);
		for(size_t i = 0; i < _v.texts.size(); ++i)
			atom_setsym(&a[_v.ints.size() + i], gensym(_v.texts[i].c_str()));
		outlet_anything(_x->panelOutlet, sym(_v.key.c_str()), static_cast<short>(a.size()), a.data());
	}

	// ---- A newer release: looked for once a day, on a thread of its own (updates.h, mdseqUpdates.h) ----

	// What the device knows of a newer release and whether it looks, to the strip's line and the window's switch.
	void updatePanel(MdSeq* _x)
	{
		std::string notice;
		{
			std::lock_guard<std::mutex> guard((*_x->update)->lock);
			notice = (*_x->update)->notice;
		}
		sendPanel(_x, {mdseq::panelKey(mdseq::PanelKey::Updates), {_x->updatesEnabled ? 1 : 0}, {}});
		sendPanel(_x, {mdseq::panelKey(mdseq::PanelKey::Update), {}, {_x->updatesEnabled ? notice : std::string()}});
	}

	void updateOnMain(MdSeq* _x)
	{
		updatePanel(_x);
	}

	// GitHub asked at most once a day (the settings file keeps the day and the answer, shared by every device), on a
	// thread that holds the line alone, so that a device freed meanwhile is never touched. Nothing is downloaded.
	void lookForUpdate(MdSeq* _x)
	{
		std::shared_ptr<UpdateLine> line = *_x->update;
		std::thread([line]
		{
			mdseqUpdates::Settings settings = mdseqUpdates::readSettings();
			if(!settings.enabled)
				return;
			const std::string today = mdseqUpdates::today();
			if(mdseq::checkDue(settings.lastCheck, today))
			{
				if(const std::optional<std::string> answer = mdseqUpdates::fetchReleases())
				{
					const std::optional<mdseq::Version> release = mdseq::firstReleaseVersion(*answer);
					settings.lastCheck = today;
					settings.latest = release ? mdseq::versionText(*release) : std::string();
					mdseqUpdates::writeSettings(settings);
				}
			}
			const std::string notice = mdseq::updateNotice(mdseq::parseVersion(settings.latest), MDSEQ_VERSION);
			std::lock_guard<std::mutex> guard(line->lock);
			if(!line->alive)
				return;
			line->notice = notice;
			qelem_set(line->qelem);
		}).detach();
	}

	// The window's switch: kept in the settings for every set; on, a look at once (once a day still), off, no line.
	void setUpdates(MdSeq* _x, const bool _on)
	{
		mdseqUpdates::Settings settings = mdseqUpdates::readSettings();
		settings.enabled = _on;
		mdseqUpdates::writeSettings(settings);
		_x->updatesEnabled = _on;
		if(_on)
			lookForUpdate(_x);
		updatePanel(_x);
	}

	void emit(MdSeq* _x, const mdseq::EditorOutput& _out)
	{
		sendOps(_x->stripOutlet, _out.strip);
		sendOps(_x->gridOutlet, _out.grid);
		for(const mdseq::PanelValue& v : _out.panel)
			sendPanel(_x, v);
		// Every field of the edit, a Lock's parameter included: mdseqEditorTest feeds its Sequencer through the same
		// words.
		bool patternEdited = false;
		for(const mdseq::Input& in : _out.forward)
		{
			const std::array<int, mdseq::InputWords> words = mdseq::pack(in);
			t_atom a[mdseq::InputWords];
			for(int i = 0; i < mdseq::InputWords; ++i)
				atom_setlong(a + i, words[i]);
			schedule_delay(_x, reinterpret_cast<method>(applyToSequencer), 0, nullptr, mdseq::InputWords, a);
			if(_x->store->edited(in))
				patternEdited = true;
		}
		// The set keeps the Patterns: written once the edits stop.
		if(patternEdited)
			clock_fdelay(_x->storeClock, StoreDelayMs);
	}

	void playedOnMain(MdSeq* _x)
	{
		const mdseq::Playhead p = mdseq::unpackPlayhead(_x->playhead.load(std::memory_order_relaxed));
		emit(_x, _x->editor->follow(p.pattern, p.queued));
		emit(_x, _x->editor->played(p.step));
	}

	// MD Drums' folder for the devices' files: the system's temporary folder as std::filesystem gives it (GetTempPath
	// on Windows: TMP, then TEMP), as MD Drums takes it, so that both sides agree when TMP and TEMP differ. Empty when
	// the system gives none.
	std::filesystem::path kitFolder()
	{
		std::error_code error;
		const std::filesystem::path temp = std::filesystem::temp_directory_path(error);
		return error ? std::filesystem::path() : temp / "md-drums-seq";
	}

	// The file MD Drums writes for the device's id, or none. Read whole and closed at once: MD Drums renames its new
	// file over this one, which Windows refuses while it is open. A path, not a narrow string, for a profile whose
	// name is not ASCII.
	std::optional<std::string> readKitFile(const std::string& _name)
	{
		const std::filesystem::path folder = kitFolder();
		if(folder.empty())
			return std::nullopt;
		std::ifstream file(folder / _name, std::ios::binary);
		if(!file)
			return std::nullopt;
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	void kitOnMain(MdSeq* _x)
	{
		const mdseq::KitLink::Poll poll = _x->kitLink->poll(readKitFile(_x->kitLink->fileName()));
		if(poll.hello)
			schedule_delay(_x, reinterpret_cast<method>(sendHello), 0, nullptr, 0, nullptr);
		// The Lock dials given back to the Kit since the last poll, now on the Kit's value; the queued Pattern's button
		// blinks.
		emit(_x, _x->editor->settle());
		emit(_x, _x->editor->blink());
		if(!poll.kitChanged)
			return;
		const std::optional<mdseq::Kit>& kit = _x->kitLink->kit();
		logLine(_x, "kit %s: %s", _x->kitLink->fileName().c_str(), kit ? kit->name.c_str() : "none");
		emit(_x, _x->editor->setKit(kit));
	}

	long arg(const long _argc, t_atom* _argv, const long _index)
	{
		return _index < _argc ? static_cast<long>(atom_getlong(_argv + _index)) : 0;
	}

#ifdef MDSEQ_DEV
	void loadDemo(MdSeq* _x);
	void lomProbe(MdSeq* _x);
#endif

	void interfaceMessage(MdSeq* _x, t_symbol* _s, const long _argc, t_atom* _argv)
	{
		const auto a0 = static_cast<int>(arg(_argc, _argv, 0));
		const auto a1 = static_cast<int>(arg(_argc, _argv, 1));
		logLine(_x, "message %s %d %d (%ld atoms)", _s->s_name, a0, a1, _argc);

		// Max calls this method only for the selectors registered from the same table.
		const mdseq::MessageInfo* message = mdseq::findMessage(_s->s_name);
		if(!message)
		{
			object_error(reinterpret_cast<t_object*>(_x), "doesn't understand %s", _s->s_name);
			return;
		}

		switch(message->message)
		{
		case mdseq::Message::Restore:
			restore(_x, _argc, _argv);
			return;
		case mdseq::Message::Passes:
			_x->printPasses = a0 + 1;	// the menu's index, one Pass at 0
			return;
		case mdseq::Message::Print:
			print(_x);
			return;
		case mdseq::Message::Updates:
			setUpdates(_x, a0 != 0);
			return;
		case mdseq::Message::LomId:
		case mdseq::Message::LomReply:
		{
			// An answer of the LOM to the message lomAsk is sending: kept whole, the selector first.
			std::vector<t_atom> answer(static_cast<size_t>(_argc) + 1);
			atom_setsym(&answer[0], _s);
			for(long i = 0; i < _argc; ++i)
				answer[static_cast<size_t>(i) + 1] = _argv[i];
			_x->lomInbox->push_back(answer);
			return;
		}
		case mdseq::Message::Redraw:
			// The release line and its switch, which no Pattern holds, with everything else.
			updatePanel(_x);
#ifdef MDSEQ_DEV
			// live.thisdevice bangs once the set's values are in: the demonstration comes after them, so that a
			// Blob's initial value does not empty it.
			if(!_x->demoLoaded)
			{
				_x->demoLoaded = true;
				loadDemo(_x);
				lomProbe(_x);
			}
#endif
			break;
		default:
			break;
		}

		// The Editor's: its integers as the patch sends them, to the call mdseq::toEditor makes. Refused, it lacks
		// integers it needs: a patch that lost them, which the Max console says.
		std::vector<int> ints(static_cast<size_t>(_argc));
		for(long i = 0; i < _argc; ++i)
			ints[static_cast<size_t>(i)] = static_cast<int>(arg(_argc, _argv, i));
		const mdseq::EditorOutput* out = mdseq::toEditor(*_x->editor, message->message, ints);
		if(!out)
		{
			object_error(reinterpret_cast<t_object*>(_x), "%s needs %d integers or more, got %ld", _s->s_name,
				message->needs, _argc);
			return;
		}
		emit(_x, *out);
	}

	void interfaceFromAnyThread(MdSeq* _x, t_symbol* _s, const long _argc, t_atom* _argv)
	{
		// The Editor lives on the main thread; a message from the scheduler (a mapped parameter) goes there first.
		if(isr())
			defer_low(_x, reinterpret_cast<method>(interfaceMessage), _s, static_cast<short>(_argc), _argv);
		else
			interfaceMessage(_x, _s, _argc, _argv);
	}

	void assist(MdSeq*, void*, const long _io, const long _index, char* _text)
	{
		static const char* outlets[] = {"MIDI bytes, to midiout", "To the strip's lcd", "To the grid's lcd",
			"Panel values"};
		if(_io == ASSIST_INLET)
			snprintf(_text, 256, "Interface messages");
		else if(_index >= 0 && _index < 4)
			snprintf(_text, 256, "%s", outlets[_index]);
	}

#ifdef MDSEQ_DEV
	// Development: sets a routing property ("output_routing_type", "output_routing_channel") of the object live.object
	// holds to the entry named _wanted, as printing does, and logs whether Live took it.
	void routeProbe(MdSeq* _x, const std::string& _property, const std::string& _wanted)
	{
		const bool took = _x->lomSet->setRouting(_property, [&](const std::string& _name) { return _name == _wanted; });
		logLine(_x, "route %s %s: %s", _property.c_str(), _wanted.c_str(), took ? "took" : "not taken");
	}

	// The JSON text of the dictionary an answer names ("<property> dictionary <name>"), empty when it names none.
	std::string answerJson(const std::vector<t_atom>& _answer)
	{
		t_dictionary* d = answerDictionary(_answer);
		if(!d)
			return {};
		std::string text = dictionaryJson(d);
		dictobj_release(d);
		return text;
	}

	// Each line of %TEMP%\mdseq-lomprobe.txt, sent to the LOM as lomAsk sends it; the answers go to the log.
	void lomProbe(MdSeq* _x)
	{
		FILE* file = openTempFile("mdseq-lomprobe.txt", "r");
		if(!file)
			return;
		// Two forms besides a plain message: "dict <name> <json>" registers a dictionary under that name, for messages
		// that take "dictionary <name>"; "json <a> <b> <c> <json>" sends a, b, c, then the JSON text as one symbol.
		std::vector<t_dictionary*> dictionaries;
		char line[4096];
		while(fgets(line, sizeof(line), file))
		{
			line[strcspn(line, "\r\n")] = 0;
			if(!line[0])
				continue;
			logLine(_x, "lom> %s", line);
			// "passes <n>" then "print": a print, as the window's Print does
			if(strncmp(line, "passes ", 7) == 0)
			{
				_x->printPasses = atol(line + 7);
				continue;
			}
			if(strcmp(line, "print") == 0)
			{
				print(_x);
				continue;
			}
			if(strncmp(line, "dict ", 5) == 0)
			{
				char* name = line + 5;
				char* json = strchr(name, ' ');
				if(!json)
					continue;
				*json++ = 0;
				t_dictionary* d = nullptr;
				char error[256] = "";
				dictobj_dictionaryfromstring(&d, json, 1, error);
				if(!d)
				{
					logLine(_x, "lom< dict %s refused: %s", name, error);
					continue;
				}
				t_symbol* registered = gensym(name);
				d = dictobj_register(d, &registered);
				dictionaries.push_back(d);
				logLine(_x, "lom< dict registered as %s", registered->s_name);
				continue;
			}
			if(strncmp(line, "route ", 6) == 0)
			{
				// "route <property> <display name>": routeProbe on the object live.object holds
				char* property = line + 6;
				char* wanted = strchr(property, ' ');
				if(wanted)
				{
					*wanted++ = 0;
					routeProbe(_x, property, wanted);
				}
				continue;
			}
			long argc = 0;
			t_atom* argv = nullptr;
			if(strncmp(line, "json ", 5) == 0)
			{
				char* rest = line + 5;
				t_atom* atoms = reinterpret_cast<t_atom*>(sysmem_newptr(4 * sizeof(t_atom)));
				for(int i = 0; i < 3 && rest; ++i)
				{
					char* end = strchr(rest, ' ');
					if(end)
						*end++ = 0;
					atom_setsym(&atoms[i], gensym(rest));
					rest = end;
				}
				atom_setsym(&atoms[3], gensym(rest ? rest : ""));
				argc = 4;
				argv = atoms;
			}
			else
			{
				atom_setparse(&argc, &argv, line);
			}
			const auto& answers = lomAsk(_x, argc, argv);
			for(const std::vector<t_atom>& a : answers)
			{
				std::vector<t_atom> copy = a;
				logLine(_x, "lom< %s", atomsText(static_cast<long>(copy.size()), copy.data()).c_str());
				const std::string json = answerJson(a);
				if(!json.empty())
					logLine(_x, "lom< json %s", json.c_str());
			}
			if(answers.empty())
				logLine(_x, "lom< (nothing)");
			if(argv)
				sysmem_freeptr(argv);
		}
		fclose(file);
		for(t_dictionary* d : dictionaries)
			dictobj_release(d);
	}

	// A development build starts with a Pattern, so that it can be heard and measured without clicking: the one in
	// %TEMP%\mdseq-demo.txt when there is one, a line per edit (Tracks and steps from 1, parameters from 0):
	//   length 32
	//   hit <track> <step> [<velocity>]
	//   lock <track> <step> <param> <value>
	//   ratchets <track> <step> <count> [<curve>]
	//   micro <track> <step> <24ths>
	//   condition <track> <step> <index in the menu>
	//   dense (every step of every Track with a Hit, Locks and the rest: the heaviest Pattern to keep in the set)
	//   pattern <1-16> (the lines after it edit that Pattern; the device ends on the one of the last such line)
	//   scale <0-6> (1/8x to 2x)
	//   swing <50-80> (percent), swingstep <step> <0|1> (the swing row, for every Track)
	// else Track 1 on the beats with VOL (17) locked to 0 on 2 and 4, and Track 7 on the off-beats.
	void loadDemo(MdSeq* _x)
	{
		// On the main thread, with the transport perhaps running: through the Editor, which names the Pattern shown
		// and forwards to the Sequencer on its own thread, as the interface does.
		const auto apply = [_x](const mdseq::Input& _in)
		{
			emit(_x, _x->editor->apply(_in));
		};
		const auto hit = [&](const int _track, const int _step, const int _velocity)
		{
			apply({mdseq::Input::Kind::SetHit, _track - 1, _step - 1, 1});
			apply({mdseq::Input::Kind::SetVelocity, _track - 1, _step - 1, _velocity});
		};
		const auto lock = [&](const int _track, const int _step, const int _param, const int _value)
		{
			mdseq::Input in{mdseq::Input::Kind::SetLock, _track - 1, _step - 1, _value};
			in.param = _param;
			apply(in);
		};

		FILE* file = openTempFile("mdseq-demo.txt", "r");
		if(!file)
		{
			for(const int s : {1, 5, 9, 13})
				hit(1, s, 120);
			lock(1, 5, 17, 0);
			lock(1, 13, 17, 0);
			for(const int s : {3, 7, 11, 15})
				hit(7, s, 90);
			logLine(_x, "demo: built-in");
			return;
		}
		char line[256];
		int lines = 0;
		while(fgets(line, sizeof(line), file))
		{
			int a = 0, b = 0, c = 0, d = 0;
			if(sscanf(line, "length %d", &a) == 1)
				apply({mdseq::Input::Kind::SetLength, 0, 0, a});
			else if(const int n = sscanf(line, "hit %d %d %d", &a, &b, &c); n >= 2)
				hit(a, b, n == 3 ? c : mdseq::DefaultVelocity);
			else if(sscanf(line, "lock %d %d %d %d", &a, &b, &c, &d) == 4)
				lock(a, b, c, d);
			else if(strncmp(line, "dense", 5) == 0)
			{
				apply({mdseq::Input::Kind::SetLength, 0, 0, mdseq::MaxSteps});
				for(int t = 0; t < mdseq::TrackCount; ++t)
				{
					for(int s = 0; s < mdseq::MaxSteps; ++s)
					{
						apply({mdseq::Input::Kind::SetHit, t, s, 1});
						apply({mdseq::Input::Kind::SetVelocity, t, s, 1 + (t * 7 + s) % 127});
						apply({mdseq::Input::Kind::SetRatchets, t, s, 1 + s % 8});
						apply({mdseq::Input::Kind::SetMicro, t, s, s % 47 - 23});
						for(int p = 0; p < mdseq::LockCount; ++p)
						{
							mdseq::Input in{mdseq::Input::Kind::SetLock, t, s, (t + s + p) % 128};
							in.param = p;
							apply(in);
						}
					}
				}
			}
			else if(sscanf(line, "pattern %d", &a) == 1)
				emit(_x, _x->editor->setPattern(a - 1));
			else if(sscanf(line, "scale %d", &a) == 1)
				apply({mdseq::Input::Kind::SetScale, 0, 0, a});
			else if(sscanf(line, "swingstep %d %d", &a, &b) == 2)
				apply({mdseq::Input::Kind::SetSwingStep, 0, a - 1, b});
			else if(sscanf(line, "swing %d", &a) == 1)
				apply({mdseq::Input::Kind::SetSwing, 0, 0, a});
			else if(sscanf(line, "condition %d %d %d", &a, &b, &c) == 3)
				apply({mdseq::Input::Kind::SetCondition, a - 1, b - 1, c});
			else if(sscanf(line, "micro %d %d %d", &a, &b, &c) == 3)
				apply({mdseq::Input::Kind::SetMicro, a - 1, b - 1, c});
			else if(const int r = sscanf(line, "ratchets %d %d %d %d", &a, &b, &c, &d); r >= 3)
			{
				apply({mdseq::Input::Kind::SetRatchets, a - 1, b - 1, c});
				apply({mdseq::Input::Kind::SetCurve, a - 1, b - 1, r == 4 ? d : 0});
			}
			else
				continue;
			++lines;
		}
		fclose(file);
		logLine(_x, "demo: %d lines from %%TEMP%%\\mdseq-demo.txt", lines);
	}
#endif

	void* create(t_symbol*, long, t_atom*)
	{
		auto* x = static_cast<MdSeq*>(object_alloc(s_class));
		if(!x)
			return nullptr;

		// Outlets are created from right to left.
		x->panelOutlet = outlet_new(x, nullptr);
		x->gridOutlet = outlet_new(x, nullptr);
		x->stripOutlet = outlet_new(x, nullptr);
		x->midiOutlet = intout(x);

		// Each start of the transport draws its Conditions from this, so that two takes differ.
		x->sequencer = new mdseq::Sequencer(randomSeed());
		x->editor = new mdseq::Editor();
		x->timer = static_cast<t_object*>(
			time_new(reinterpret_cast<t_object*>(x), gensym("timer"), reinterpret_cast<method>(timer),
				TIME_FLAGS_TICKSONLY | TIME_FLAGS_USECLOCK));
		// Live's transport is the timer's ITM ("live"). itm_getglobal() is Max's own transport ("internal"), which
		// in a device never runs: read from it, the Sequencer never saw Live play (measured 2026-10-10).
		x->itm = static_cast<t_itm*>(time_getitm(reinterpret_cast<t_timeobject*>(x->timer)));
		itm_reference(x->itm);
		x->heartbeat = static_cast<t_clock*>(clock_new(x, reinterpret_cast<method>(heartbeat)));
		x->playedQelem = static_cast<t_qelem*>(qelem_new(x, reinterpret_cast<method>(playedOnMain)));
		x->storeClock = static_cast<t_clock*>(clock_new(x, reinterpret_cast<method>(storeSoon)));
		x->storeQelem = static_cast<t_qelem*>(qelem_new(x, reinterpret_cast<method>(storeOnMain)));

		x->armedTick = mdseq::Never;
		new(&x->playhead) std::atomic<uint32_t>(mdseq::packPlayhead({}));
		new(&x->startSeed) std::atomic<uint64_t>(0);
		x->printPasses = 4;
		x->lomSet = new LomSet(x);
		x->printer = new mdseq::Printer(*x->lomSet);
		x->printClock = static_cast<t_clock*>(clock_new(x, reinterpret_cast<method>(printSoon)));
		x->printQelem = static_cast<t_qelem*>(qelem_new(x, reinterpret_cast<method>(finishPrint)));
		x->store = new mdseq::PatternStore();
		x->kitLink = new mdseq::KitLink(randomSeed());
		x->helloId = x->kitLink->id();
		x->kitClock = static_cast<t_clock*>(clock_new(x, reinterpret_cast<method>(kitSoon)));
		x->kitQelem = static_cast<t_qelem*>(qelem_new(x, reinterpret_cast<method>(kitOnMain)));
		clock_fdelay(x->kitClock, mdseq::KitLink::PollMs);
		x->update = new std::shared_ptr<UpdateLine>(std::make_shared<UpdateLine>());
		(*x->update)->qelem = static_cast<t_qelem*>(qelem_new(x, reinterpret_cast<method>(updateOnMain)));
		x->updatesEnabled = mdseqUpdates::readSettings().enabled;
		if(x->updatesEnabled)
			lookForUpdate(x);

		x->demoLoaded = false;
		x->lomInbox = new std::vector<std::vector<t_atom>>();
		clock_fdelay(x->heartbeat, HeartbeatMs);
		object_post(reinterpret_cast<t_object*>(x), "md-drums-seq %s core ready", MDSEQ_VERSION);
		if constexpr(Dev)
		{
			logLine(x, "created; MD Drums' file for this device: %s",
				(kitFolder() / x->kitLink->fileName()).u8string().c_str());
			logState(x);
		}
		return x;
	}

	void destroy(MdSeq* _x)
	{
		clock_unset(_x->heartbeat);
		object_free(_x->heartbeat);
		time_stop(_x->timer);
		object_free(_x->timer);
		qelem_free(_x->playedQelem);
		clock_unset(_x->storeClock);
		object_free(_x->storeClock);
		qelem_free(_x->storeQelem);
		clock_unset(_x->printClock);
		object_free(_x->printClock);
		qelem_free(_x->printQelem);
		clock_unset(_x->kitClock);
		object_free(_x->kitClock);
		qelem_free(_x->kitQelem);
		// A look for a release still running finds the device gone and touches nothing.
		{
			std::lock_guard<std::mutex> guard((*_x->update)->lock);
			(*_x->update)->alive = false;
		}
		qelem_free((*_x->update)->qelem);
		delete _x->update;
		delete _x->kitLink;
		delete _x->printer;
		delete _x->lomSet;
		itm_dereference(_x->itm);
		delete _x->sequencer;
		delete _x->editor;
		delete _x->store;
		delete _x->lomInbox;
		// Nothing reads them now that the clocks, the timer and the qelems are gone.
		std::destroy_at(&_x->playhead);
		std::destroy_at(&_x->startSeed);
	}
}

void ext_main(void*)
{
	t_class* c = class_new(MDSEQ_CLASS_NAME, reinterpret_cast<method>(create), reinterpret_cast<method>(destroy),
		sizeof(MdSeq), nullptr, A_GIMME, 0);

	// Every message of the vocabulary tools/make_device.py builds the patch from, and none other.
	for(const mdseq::MessageInfo& m : mdseq::messages())
		class_addmethod(c, reinterpret_cast<method>(interfaceFromAnyThread), m.selector, A_GIMME, 0);
	class_addmethod(c, reinterpret_cast<method>(assist), "assist", A_CANT, 0);
	class_time_addattr(c, "timer", "Wake Timer", TIME_FLAGS_TICKSONLY | TIME_FLAGS_USECLOCK);

	class_register(CLASS_BOX, c);
	s_class = c;
}
