#pragma once

// A Pattern as the integers the device keeps in its set, through a pattr Blob parameter (issue 14). Every word stays
// below 2^31, so that it comes back exact whatever number type Live keeps it as.
//
//   0  StoreMagic, which carries the format's version
//   1  length | scale << 8 | (swing - 50) << 12 | own swing row << 17 | accent << 18 | slide row << 19
//   when the swing row is not the default (every second step), it follows in three words of 22 bits, steps 1-22 first
//   when the Pattern has an Accent (an amount or a row), the amount follows in a word, then its row in three words
//   when the Pattern has a Slide row (a step in it), the row follows in three words
//   then each step with a Hit, Track by Track and step by step, in two words and, when it holds Locks, six more:
//      track | step << 4 | velocity << 10 | condition << 17 | locked << 24
//      (ratchets - 1) | (curve + 100) << 3 | (micro + 23) << 11
//      Locks 4i to 4i+3, 7 bits each, the first in the low bits
//
// The heaviest Pattern, every step of every Track with a Hit and Locks, a swing row of its own, an Accent and a Slide
// row, takes 2 + 3 + 4 + 3 + 1024 * 8 = 8204 words. A Pattern of a build before swing reads as one of 50 % on the
// default row, one of a build before the Accent or the Slide as one without them.
//
// PatternStore, below, decides when each Blob is written and what a value it gives back means.

#include "mdseqTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mdseq
{
	constexpr int64_t StoreMagic = 0x4d530001;	// "MS", version 1

	std::vector<int64_t> encode(const Pattern& _pattern);

	// The Blob's initial value, which it holds until the Pattern is first written: a single 0.
	constexpr int64_t NoPatternStored = 0;

	// The Pattern the words hold, into _out: an empty one for NoPatternStored alone (a new device, or Live's undo back
	// to before the first write); false, and _out untouched, for words this format did not write: another magic, a
	// field out of its range, bits it does not use, words missing or left over.
	bool decode(const int64_t* _words, size_t _count, Pattern& _out);

	// The bank as the set keeps it, a Blob per Pattern: which edits leave a Pattern to write, which Patterns are due
	// once the edits stop, and what a Blob's value that comes back is. Every write is an entry in Live's undo history,
	// so a Pattern is written only when its words differ from those its Blob holds, the words last written or those of
	// the Pattern last received; and a value Live gives back wins over an edit not yet written, so that its undo stays
	// undone. The adapter keeps the clock that waits for the edits to stop and the pattr messages. Main thread only;
	// it may allocate.
	class PatternStore
	{
	public:
		// An edit forwarded by the Editor: true when it leaves its Pattern to write (isPatternEdit), which then waits
		// for the edits to stop.
		bool edited(const Input& _in);

		struct Write
		{
			int pattern;
			std::vector<int64_t> words;
		};

		// The Patterns left to write, in the bank's order, with the words for their Blobs, read from _bank (the
		// Editor's, PatternCount Patterns); their Blobs hold these words from now on. A Pattern whose words its Blob
		// already holds, an edit undone before its write, is left out. Nothing is left to write after.
		std::vector<Write> due(const std::vector<Pattern>& _bank);

		enum class Received : uint8_t
		{
			Echo,		// the Pattern its Blob holds already: a write coming back (the pattr sends it in spite of thru
						// 2), or Live sending every Blob again after an undo. Ignored, and an edit pending stays.
			Load,		// another Pattern, in _out, which the bank takes: the set's value when it loads, Live's undo or
						// redo. Its Blob holds it, and an edit pending on it is dropped.
			Refused,	// words this format did not write (a later format's, or Live's undo back to them): the set
						// holds a Pattern this build cannot read, which the bank takes as the empty Pattern in _out,
						// as reopening the set will give it. Otherwise a Load: an edit pending is dropped, and the
						// next edit writes over those words.
			Unusable	// a Pattern outside the bank: ignored.
		};

		// A Blob's value as the set gives it back, "restore p words...". The first value of each Blob loads, even the
		// initial NoPatternStored: until then nothing says what it holds. _out is written for Load and Refused only.
		Received received(int _pattern, const int64_t* _words, size_t _count, Pattern& _out);

	private:
		std::array<uint64_t, PatternCount> m_held{};	// a hash of the words each Blob holds, once m_known says so
		uint32_t m_known = 0;							// the Blobs whose words are known, a bit each
		uint32_t m_unstored = 0;						// the Patterns edited since their Blob last changed, a bit each
	};
}
