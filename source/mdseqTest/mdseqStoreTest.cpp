// A Pattern through the words the device keeps in its set: every field comes back, the heaviest Pattern stays within
// its 8194 words, every word stays below 2^31, the Blob's initial value reads as an empty Pattern, and words this
// format did not write are refused without touching the Pattern they were read into.
//
// Then the PatternStore, through its interface as the adapter drives it: an edit makes one write, whose echo is
// ignored; an edit undone before its write makes none; a Mute, the Fill and the choice of the Pattern played mark
// nothing; Live's undo giving a Blob other words loads them and drops the write pending, and a later edit writes
// again; the initial value loads an empty Pattern, and so do words this build cannot read, which the next edit
// writes over; and each of the 16 Patterns is written and given back on its own.

#include "mdseqCondition.h"
#include "mdseqEdit.h"
#include "mdseqStore.h"
#include "mdseqTestSupport.h"

#include <cstdio>
#include <random>

namespace
{
	using namespace mdseq;

	Pattern randomPattern(std::mt19937& _random, const int _hitsInTen)
	{
		Pattern p;
		p.length = 1 + static_cast<int>(_random() % MaxSteps);
		p.scale = static_cast<uint8_t>(_random() % ScaleCount);
		for(int t = 0; t < TrackCount; ++t)
		{
			for(int s = 0; s < MaxSteps; ++s)
			{
				if(static_cast<int>(_random() % 10) >= _hitsInTen)
					continue;
				Step& step = p.steps[t][s];
				step.hit = true;
				step.velocity = static_cast<uint8_t>(1 + _random() % 127);
				step.condition = static_cast<uint8_t>(_random() % ConditionCount);
				step.ratchets = static_cast<uint8_t>(1 + _random() % MaxRatchets);
				step.curve = static_cast<int8_t>(static_cast<int>(_random() % (2 * MaxCurve + 1)) - MaxCurve);
				step.micro = static_cast<int8_t>(static_cast<int>(_random() % (2 * MaxMicro + 1)) - MaxMicro);
				if(_random() % 2)
					for(uint8_t& lock : step.locks)
						lock = static_cast<uint8_t>(_random() % 3 ? NoLock : _random() % 128);
			}
		}
		// The swing last, so that the steps drawn stay those drawn before it came.
		p.swing = static_cast<uint8_t>(MinSwing + _random() % (MaxSwing - MinSwing + 1));
		if(_random() % 2)
			p.swingSteps = static_cast<uint64_t>(_random()) | static_cast<uint64_t>(_random()) << 32;
		return p;
	}

	bool roundTrip(const Pattern& _pattern)
	{
		const std::vector<int64_t> words = encode(_pattern);
		for(const int64_t w : words)
			if(w < 0 || w >= (int64_t{1} << 31))
				return false;
		Pattern back;
		return decode(words.data(), words.size(), back) && back == _pattern;
	}

	void emptyAndRandom()
	{
		const Pattern empty;
		CHECK(encode(empty).size() == 2);
		CHECK(roundTrip(empty));

		std::mt19937 random(14);
		int failures = 0;
		for(int i = 0; i < 200; ++i)
			if(!roundTrip(randomPattern(random, 1 + i % 10)))
				++failures;
		CHECK(failures == 0);
	}

	void heaviest()
	{
		Pattern p;
		p.length = MaxSteps;
		for(int t = 0; t < TrackCount; ++t)
		{
			for(int s = 0; s < MaxSteps; ++s)
			{
				Step& step = p.steps[t][s];
				step.hit = true;
				step.velocity = 127;
				step.condition = static_cast<uint8_t>(ConditionCount - 1);
				step.ratchets = MaxRatchets;
				step.curve = -MaxCurve;
				step.micro = MaxMicro;
				for(int k = 0; k < LockCount; ++k)
					step.locks[k] = static_cast<uint8_t>(k % 2 ? 127 : 0);
			}
		}
		CHECK(encode(p).size() == 8194);
		CHECK(roundTrip(p));
	}

	// The swing: a Pattern of the default swing writes the words it wrote before swing came; an amount goes in the
	// head, a row of its own in three words of 22 bits after it; what encode does not write is refused.
	void swingWords()
	{
		Pattern p;
		CHECK(encode(p) == (std::vector<int64_t>{StoreMagic, DefaultLength | DefaultScale << 8}));

		p.swing = 66;
		CHECK(encode(p) == (std::vector<int64_t>{StoreMagic, DefaultLength | DefaultScale << 8 | 16 << 12}));
		CHECK(roundTrip(p));

		// The first and the last step only: bits 0 and 63, in the first word and the third's bit 19.
		p.swingSteps = 1ull | 1ull << 63;
		std::vector<int64_t> words = encode(p);
		CHECK(words.size() == 5 && words[1] >> 17 == 1 && words[2] == 1 && words[3] == 0 && words[4] == 1 << 19);
		CHECK(roundTrip(p));
		p.swingSteps = 0;
		CHECK(roundTrip(p));
		p.swingSteps = ~0ull;
		CHECK(roundTrip(p));

		// A Pattern of a build before swing: its head has none of the swing's bits.
		Pattern old;
		const std::vector<int64_t> before{StoreMagic, 32 | 2 << 8, 3 | 5 << 4 | 100 << 10, 0 | MaxCurve << 3 | MaxMicro << 11};
		CHECK(decode(before.data(), before.size(), old));
		CHECK(old.length == 32 && old.scale == 2 && old.swing == MinSwing && old.swingSteps == DefaultSwingSteps);
		CHECK(old.steps[3][5].hit);

		Pattern target;
		target.length = 33;
		const auto refuses = [&](const std::vector<int64_t>& _words)
		{
			return !decode(_words.data(), _words.size(), target) && target.length == 33;
		};
		p.swingSteps = 1ull | 1ull << 63;
		const std::vector<int64_t> good = encode(p);
		words = good;
		words[1] |= 31 << 12;	// 81 %
		CHECK(refuses(words));
		words = good;
		words[1] |= 1 << 18;	// a bit the head does not use
		CHECK(refuses(words));
		words = good;
		words[4] |= 1 << 20;	// a step 65
		CHECK(refuses(words));
		words = good;
		words[3] = -1;
		CHECK(refuses(words));
		words = good;
		words.resize(4);	// the row cut short
		CHECK(refuses(words));
		// A row of its own equal to the default is not what encode writes.
		words = good;
		words[2] = static_cast<int64_t>(DefaultSwingSteps & 0x3fffff);
		words[3] = static_cast<int64_t>(DefaultSwingSteps >> 22 & 0x3fffff);
		words[4] = static_cast<int64_t>(DefaultSwingSteps >> 44);
		CHECK(refuses(words));
	}

	void refused()
	{
		std::mt19937 random(7);
		const Pattern original = randomPattern(random, 5);
		const std::vector<int64_t> good = encode(original);
		Pattern target;
		target.length = 33;

		// Each broken copy is refused, and leaves the Pattern read into as it was.
		const auto refuses = [&](std::vector<int64_t> _words)
		{
			const bool accepted = decode(_words.data(), _words.size(), target);
			return !accepted && target.length == 33 && !target.steps[0][0].hit;
		};
		auto words = good;
		words[0] = StoreMagic + 1;
		CHECK(refuses(words));
		words = good;
		words[1] = 0;	// length 0
		CHECK(refuses(words));
		words = good;
		words[1] = 16 | ScaleCount << 8;
		CHECK(refuses(words));
		words = good;
		words.pop_back();	// a step cut short
		CHECK(refuses(words));
		words = good;
		words.push_back(5);	// half a step left over
		CHECK(refuses(words));
		words = good;
		words[2] |= int64_t{1} << 25;	// a bit the format does not use
		CHECK(refuses(words));
		words = good;
		words[2] &= ~(int64_t{0x7f} << 10);	// velocity 0
		CHECK(refuses(words));
		words = good;
		words[3] = (words[3] & ~(int64_t{0x3f} << 11)) | int64_t{47} << 11;	// micro +24
		CHECK(refuses(words));
		CHECK(refuses({StoreMagic}));
		CHECK(refuses({}));
		CHECK(refuses({5}));

		// The Blob's initial value is no Pattern stored yet: an empty one, so that Live's undo back to it empties it.
		const int64_t initial = NoPatternStored;
		CHECK(decode(&initial, 1, target));
		CHECK(target == Pattern{});

		CHECK(decode(good.data(), good.size(), target));
		CHECK(target == original);
	}

	using Received = PatternStore::Received;

	const std::vector<int64_t> Initial{NoPatternStored};

	Input hit(const int _pattern, const int _track, const int _step, const int _on)
	{
		Input in{Input::Kind::SetHit, _track, _step, _on};
		in.pattern = _pattern;
		return in;
	}

	// The device around the store, as the adapter has it: the Editor's bank, edited with applyEdit, the store told
	// of every edit forwarded, and each Pattern a Blob gives back loaded into the bank.
	struct Device
	{
		PatternStore store;
		std::vector<Pattern> bank = std::vector<Pattern>(PatternCount);
		uint16_t mutes = 0;

		// The set's values when the device loads: each Blob's initial value.
		Device()
		{
			for(int p = 0; p < PatternCount; ++p)
				restore(p, Initial);
		}

		bool edit(const Input& _in)
		{
			if(_in.pattern >= 0 && _in.pattern < PatternCount)
				applyEdit(bank[static_cast<size_t>(_in.pattern)], mutes, _in);
			return store.edited(_in);
		}

		Received restore(const int _pattern, const std::vector<int64_t>& _words)
		{
			Pattern out;
			out.length = 33;	// what the store does not write stays visible
			const Received r = store.received(_pattern, _words.data(), _words.size(), out);
			if(r == Received::Load || r == Received::Refused)
				bank[static_cast<size_t>(_pattern)] = out;
			return r;
		}
	};

	// One write per Pattern edited, however many edits, with the Pattern's words; the pattr's echo of it is ignored.
	void editWrites()
	{
		Device d;
		CHECK(d.store.due(d.bank).empty());
		CHECK(d.edit(hit(0, 2, 4, 1)));
		const std::vector<PatternStore::Write> writes = d.store.due(d.bank);
		CHECK(writes.size() == 1);
		CHECK(!writes.empty() && writes[0].pattern == 0 && writes[0].words == encode(d.bank[0]));
		CHECK(d.store.due(d.bank).empty());

		Pattern untouched;
		untouched.length = 33;
		CHECK(d.store.received(0, writes[0].words.data(), writes[0].words.size(), untouched) == Received::Echo);
		CHECK(untouched.length == 33 && !untouched.steps[2][4].hit);
		CHECK(d.store.due(d.bank).empty());

		// A drag over three cells, then the velocity turned: one write.
		CHECK(d.edit(hit(0, 2, 5, 1)));
		CHECK(d.edit(hit(0, 2, 6, 1)));
		CHECK(d.edit(hit(0, 2, 7, 1)));
		Input velocity{Input::Kind::SetVelocity, 2, 7, 64};
		CHECK(d.edit(velocity));
		const std::vector<PatternStore::Write> drag = d.store.due(d.bank);
		CHECK(drag.size() == 1 && drag[0].words == encode(d.bank[0]));
	}

	// An edit undone before its write writes nothing, which would give Live an undo entry that changes nothing: on a
	// Pattern written, and on a Blob that still holds its initial value.
	void undoneBeforeWrite()
	{
		Device d;
		CHECK(d.edit(hit(0, 0, 0, 1)));
		CHECK(d.edit(hit(0, 0, 0, 0)));
		CHECK(d.store.due(d.bank).empty());

		CHECK(d.edit(hit(0, 1, 1, 1)));
		CHECK(d.store.due(d.bank).size() == 1);
		CHECK(d.edit(hit(0, 3, 3, 1)));
		CHECK(d.edit(hit(0, 3, 3, 0)));
		Input longer{Input::Kind::SetLength, 0, 0, 32};
		Input back{Input::Kind::SetLength, 0, 0, DefaultLength};
		CHECK(d.edit(longer));
		CHECK(d.edit(back));
		CHECK(d.store.due(d.bank).empty());
	}

	// A Mute, the Fill and the choice of the Pattern played are not in a Pattern, and an edit of a Pattern outside the
	// bank names none: none of them leaves a Pattern to write, even one that has changed meanwhile.
	void notPatternEdits()
	{
		Device d;
		d.bank[0].steps[0][0].hit = true;
		Input mute{Input::Kind::SetMute, 0, 0, 1};
		Input fill{Input::Kind::SetFill, 0, 0, 1};
		Input choice{Input::Kind::SetPattern, 0, 0, 3};
		CHECK(!d.edit(mute));
		CHECK(!d.edit(fill));
		CHECK(!d.edit(choice));
		CHECK(!d.store.edited(hit(PatternCount, 0, 0, 1)));
		CHECK(!d.store.edited(hit(-1, 0, 0, 1)));
		CHECK(d.store.due(d.bank).empty());

		CHECK(d.edit(hit(0, 1, 0, 1)));
		const std::vector<PatternStore::Write> writes = d.store.due(d.bank);
		CHECK(writes.size() == 1 && writes[0].pattern == 0 && writes[0].words == encode(d.bank[0]));
	}

	// Live's undo gives a Blob other words while an edit waits for its write: they load, and the write is dropped, so
	// that it does not undo the undo. A later edit writes again, back to the words the undo took away included, since
	// the Blob no longer holds them.
	void restored()
	{
		Device d;
		CHECK(d.edit(hit(0, 0, 0, 1)));
		const std::vector<PatternStore::Write> first = d.store.due(d.bank);
		CHECK(d.edit(hit(0, 0, 4, 1)));
		const std::vector<PatternStore::Write> second = d.store.due(d.bank);
		CHECK(first.size() == 1 && second.size() == 1);
		if(first.size() != 1 || second.size() != 1)
			return;

		CHECK(d.edit(hit(0, 0, 8, 1)));
		Pattern out;
		CHECK(d.store.received(0, first[0].words.data(), first[0].words.size(), out) == Received::Load);
		Pattern expected;
		CHECK(decode(first[0].words.data(), first[0].words.size(), expected) && out == expected);
		// Dropped whatever the bank holds: here it still holds the edit, which the adapter's Editor would have lost.
		CHECK(d.store.due(d.bank).empty());
		d.bank[0] = out;

		CHECK(d.edit(hit(0, 0, 4, 1)));
		const std::vector<PatternStore::Write> again = d.store.due(d.bank);
		CHECK(again.size() == 1 && again[0].words == second[0].words);

		// Undone back to before the first write: the initial value, an empty Pattern.
		CHECK(d.restore(0, Initial) == Received::Load);
		CHECK(d.bank[0] == Pattern{});
		CHECK(d.edit(hit(0, 0, 0, 1)));
		const std::vector<PatternStore::Write> redone = d.store.due(d.bank);
		CHECK(redone.size() == 1 && redone[0].words == first[0].words);
	}

	// The Blob's initial value loads an empty Pattern the first time, whatever the bank held. Sent again unchanged, as
	// Live sends every Blob after an undo of another, it is the Pattern held: ignored, and the edit pending stays. So
	// is an empty Pattern written, sent back.
	void initialValue()
	{
		PatternStore store;
		Pattern out;
		out.length = 33;
		out.steps[0][0].hit = true;
		CHECK(store.received(5, Initial.data(), Initial.size(), out) == Received::Load);
		CHECK(out == Pattern{});
		CHECK(store.received(5, Initial.data(), Initial.size(), out) == Received::Echo);

		Device d;
		CHECK(d.edit(hit(5, 0, 0, 1)));
		CHECK(d.restore(5, Initial) == Received::Echo);
		const std::vector<PatternStore::Write> writes = d.store.due(d.bank);
		CHECK(writes.size() == 1 && writes[0].pattern == 5);

		CHECK(d.edit(hit(5, 0, 0, 0)));
		const std::vector<PatternStore::Write> emptied = d.store.due(d.bank);
		CHECK(emptied.size() == 1 && emptied[0].words == encode(Pattern{}));
		CHECK(d.restore(5, Initial) == Received::Echo);
		CHECK(d.store.due(d.bank).empty());
	}

	// A Pattern outside the bank names no Blob: ignored, nothing loads, and an edit pending is still written.
	void unusable()
	{
		Device d;
		CHECK(d.edit(hit(2, 0, 0, 1)));
		Pattern out;
		out.length = 33;
		CHECK(d.store.received(PatternCount, Initial.data(), Initial.size(), out) == Received::Unusable);
		CHECK(d.store.received(-1, Initial.data(), Initial.size(), out) == Received::Unusable);
		CHECK(out.length == 33);
		const std::vector<PatternStore::Write> writes = d.store.due(d.bank);
		CHECK(writes.size() == 1 && writes[0].pattern == 2);
	}

	const std::vector<int64_t> Foreign{StoreMagic + 1, DefaultLength};

	// A set saved by a later format: the first value of a Blob holds words this build cannot read. The Pattern is
	// empty, as it shows, and the first edit writes over those words; a value without any word is refused alike.
	void refusedFirst()
	{
		PatternStore store;
		Pattern out;
		out.length = 33;
		out.steps[0][0].hit = true;
		CHECK(store.received(3, Foreign.data(), Foreign.size(), out) == Received::Refused);
		CHECK(out == Pattern{});
		CHECK(store.received(4, nullptr, 0, out) == Received::Refused);

		Device d;
		CHECK(d.restore(3, Foreign) == Received::Refused);
		CHECK(d.store.due(d.bank).empty());
		CHECK(d.edit(hit(3, 0, 0, 1)));
		const std::vector<PatternStore::Write> writes = d.store.due(d.bank);
		CHECK(writes.size() == 1 && writes[0].pattern == 3 && writes[0].words == encode(d.bank[3]));
	}

	// Words this format did not write, given back after a write (Live's undo to a value a later format saved): the
	// Pattern empties, as the set will give it when it reopens, and an edit pending on it is dropped, as with any
	// value Live gives back. That alone writes nothing, and the other Blobs still hold their words. Sent again, as Live
	// sends every Blob after an undo of another, they are what the Blob holds. The next edit writes, back to the words
	// the undo took away included; and Live's redo to those words loads them, dropping an edit pending.
	void refusedAfterWrite()
	{
		Device d;
		CHECK(d.edit(hit(2, 0, 0, 1)));
		CHECK(d.edit(hit(5, 3, 3, 1)));
		const std::vector<PatternStore::Write> first = d.store.due(d.bank);
		CHECK(first.size() == 2);
		if(first.size() != 2)
			return;
		CHECK(d.restore(2, first[0].words) == Received::Echo);
		CHECK(d.restore(5, first[1].words) == Received::Echo);

		CHECK(d.edit(hit(2, 1, 1, 1)));
		CHECK(d.restore(2, Foreign) == Received::Refused);
		CHECK(d.bank[2] == Pattern{});
		CHECK(d.store.due(d.bank).empty());
		CHECK(d.restore(5, first[1].words) == Received::Echo);
		CHECK(d.restore(2, Foreign) == Received::Echo);
		CHECK(d.store.due(d.bank).empty());

		CHECK(d.edit(hit(2, 0, 0, 1)));
		const std::vector<PatternStore::Write> again = d.store.due(d.bank);
		CHECK(again.size() == 1 && again[0].pattern == 2 && again[0].words == first[0].words);

		CHECK(d.restore(2, Foreign) == Received::Refused);
		CHECK(d.edit(hit(2, 1, 4, 1)));
		CHECK(d.restore(2, first[0].words) == Received::Load);
		CHECK(d.bank[2].steps[0][0].hit && !d.bank[2].steps[1][4].hit);
		CHECK(d.store.due(d.bank).empty());
	}

	// Each Pattern has its own Blob: written in the bank's order, each echo ignored on its own Blob, one Pattern's
	// words another Pattern there, and an undo on one leaving the others' writes pending.
	void independent()
	{
		Device d;
		for(int p = 0; p < PatternCount; ++p)
			CHECK(d.edit(hit(p, p, p, 1)));
		const std::vector<PatternStore::Write> writes = d.store.due(d.bank);
		CHECK(writes.size() == static_cast<size_t>(PatternCount));
		if(writes.size() != static_cast<size_t>(PatternCount))
			return;
		int wrong = 0;
		for(int p = 0; p < PatternCount; ++p)
		{
			const PatternStore::Write& w = writes[static_cast<size_t>(p)];
			if(w.pattern != p || w.words != encode(d.bank[static_cast<size_t>(p)]))
				++wrong;
			if(d.restore(p, w.words) != Received::Echo)
				++wrong;
		}
		CHECK(wrong == 0);

		CHECK(d.restore(1, writes[0].words) == Received::Load);
		CHECK(d.bank[1].steps[0][0].hit && !d.bank[1].steps[1][1].hit);
		CHECK(d.restore(0, writes[0].words) == Received::Echo);

		CHECK(d.edit(hit(3, 0, 1, 1)));
		CHECK(d.edit(hit(11, 0, 1, 1)));
		CHECK(d.edit(hit(7, 0, 1, 1)));
		CHECK(d.edit(hit(7, 0, 1, 0)));
		CHECK(d.restore(3, Initial) == Received::Load);
		const std::vector<PatternStore::Write> after = d.store.due(d.bank);
		CHECK(after.size() == 1 && after[0].pattern == 11 && after[0].words == encode(d.bank[11]));
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	emptyAndRandom();
	heaviest();
	swingWords();
	refused();
	editWrites();
	undoneBeforeWrite();
	notPatternEdits();
	restored();
	initialValue();
	unusable();
	refusedFirst();
	refusedAfterWrite();
	independent();
	return mdseqTest::finish("mdseqStoreTest");
}
