#include "mdseqStore.h"

#include "mdseqCondition.h"
#include "mdseqEdit.h"

#include <algorithm>
#include <memory>

namespace mdseq
{
	namespace
	{
		constexpr int LockWords = LockCount / 4;
		constexpr int64_t Bits7 = 0x7f;
		// The swing row in words of 22 bits, the first holding steps 1-22: every word below 2^31.
		constexpr int SwingBits = 22;
		constexpr int SwingWords = (MaxSteps + SwingBits - 1) / SwingBits;
		constexpr uint64_t SwingMask = (1ull << SwingBits) - 1;

		// FNV-1a over the words: enough to know what a Blob holds without keeping up to 8197 words per Pattern.
		uint64_t hashOf(const int64_t* _words, const size_t _count)
		{
			uint64_t h = 1469598103934665603ull;
			for(size_t i = 0; i < _count; ++i)
				h = (h ^ static_cast<uint64_t>(_words[i])) * 1099511628211ull;
			return h;
		}

		uint64_t hashOf(const std::vector<int64_t>& _words)
		{
			return hashOf(_words.data(), _words.size());
		}
	}

	std::vector<int64_t> encode(const Pattern& _pattern)
	{
		// The head: the length, the scale, then the swing above 50 % and whether the swing row differs from the default,
		// which then follows in SwingWords. A Pattern of the default swing writes the words it wrote before swing came,
		// so that the Blobs of sets saved then compare equal to what this build writes.
		const bool ownRow = _pattern.swingSteps != DefaultSwingSteps;
		const int64_t head = _pattern.length | _pattern.scale << 8 | (_pattern.swing - MinSwing) << 12 |
			(ownRow ? 1 : 0) << 17;
		std::vector<int64_t> words{StoreMagic, head};
		if(ownRow)
			for(int w = 0; w < SwingWords; ++w)
				words.push_back(static_cast<int64_t>(_pattern.swingSteps >> (SwingBits * w) & SwingMask));
		for(int t = 0; t < TrackCount; ++t)
		{
			for(int s = 0; s < MaxSteps; ++s)
			{
				const Step& step = _pattern.steps[t][s];
				if(!step.hit)
					continue;
				const bool locked = step.locked();
				words.push_back(t | s << 4 | step.velocity << 10 | step.condition << 17 | (locked ? 1 : 0) << 24);
				words.push_back((step.ratchets - 1) | (step.curve + MaxCurve) << 3 | (step.micro + MaxMicro) << 11);
				if(!locked)
					continue;
				for(int w = 0; w < LockWords; ++w)
				{
					int64_t word = 0;
					for(int i = 0; i < 4; ++i)
						word |= static_cast<int64_t>(step.locks[w * 4 + i]) << (7 * i);
					words.push_back(word);
				}
			}
		}
		return words;
	}

	bool decode(const int64_t* _words, const size_t _count, Pattern& _out)
	{
		if(_count == 1 && _words[0] == NoPatternStored)
		{
			_out = Pattern{};
			return true;
		}
		if(_count < 2 || _words[0] != StoreMagic)
			return false;

		Pattern pattern;
		const int64_t head = _words[1];
		const int length = static_cast<int>(head & 0xff);
		const int scale = static_cast<int>(head >> 8 & 0xf);
		const int swing = MinSwing + static_cast<int>(head >> 12 & 0x1f);
		const bool ownRow = (head >> 17 & 1) != 0;
		if(head < 0 || head >> 18 || length < 1 || length > MaxSteps || scale >= ScaleCount || swing > MaxSwing)
			return false;
		pattern.length = length;
		pattern.scale = static_cast<uint8_t>(scale);
		pattern.swing = static_cast<uint8_t>(swing);

		size_t i = 2;
		if(ownRow)
		{
			if(_count < 2 + SwingWords)
				return false;
			uint64_t steps = 0;
			for(int w = 0; w < SwingWords; ++w)
			{
				const int64_t word = _words[i++];
				if(word < 0 || word > static_cast<int64_t>(SwingMask))
					return false;
				steps |= static_cast<uint64_t>(word) << (SwingBits * w);
			}
			// The last word's bits past step 64, and a row equal to the default, are not what encode writes.
			if(_words[i - 1] >> (MaxSteps - SwingBits * (SwingWords - 1)) || steps == DefaultSwingSteps)
				return false;
			pattern.swingSteps = steps;
		}
		while(i < _count)
		{
			if(i + 2 > _count)
				return false;
			const int64_t a = _words[i++];
			const int64_t b = _words[i++];
			if(a < 0 || a >> 25 || b < 0 || b >> 17)
				return false;

			const int track = static_cast<int>(a & 0xf);
			const int index = static_cast<int>(a >> 4 & 0x3f);
			const int velocity = static_cast<int>(a >> 10 & Bits7);
			const int condition = static_cast<int>(a >> 17 & Bits7);
			const bool locked = (a >> 24 & 1) != 0;
			const int ratchets = static_cast<int>(b & 0x7) + 1;
			const int curve = static_cast<int>(b >> 3 & 0xff) - MaxCurve;
			const int micro = static_cast<int>(b >> 11 & 0x3f) - MaxMicro;
			if(velocity < 1 || condition >= ConditionCount || curve > MaxCurve || micro > MaxMicro)
				return false;

			Step& step = pattern.steps[track][index];
			step = Step{};
			step.hit = true;
			step.velocity = static_cast<uint8_t>(velocity);
			step.condition = static_cast<uint8_t>(condition);
			step.ratchets = static_cast<uint8_t>(ratchets);
			step.curve = static_cast<int8_t>(curve);
			step.micro = static_cast<int8_t>(micro);
			if(!locked)
				continue;
			if(i + LockWords > _count)
				return false;
			for(int w = 0; w < LockWords; ++w)
			{
				const int64_t word = _words[i++];
				if(word < 0 || word >> 28)
					return false;
				for(int k = 0; k < 4; ++k)
					step.locks[w * 4 + k] = static_cast<uint8_t>(word >> (7 * k) & Bits7);
			}
		}

		_out = pattern;
		return true;
	}

	bool PatternStore::edited(const Input& _in)
	{
		if(!isPatternEdit(_in.kind))
			return false;
		if(_in.pattern < 0 || _in.pattern >= PatternCount)
			return false;
		m_unstored |= 1u << _in.pattern;
		return true;
	}

	std::vector<PatternStore::Write> PatternStore::due(const std::vector<Pattern>& _bank)
	{
		std::vector<Write> writes;
		const int count = static_cast<int>(std::min(_bank.size(), static_cast<size_t>(PatternCount)));
		for(int p = 0; p < count; ++p)
		{
			const uint32_t bit = 1u << p;
			if(!(m_unstored & bit))
				continue;
			std::vector<int64_t> words = encode(_bank[static_cast<size_t>(p)]);
			const uint64_t hash = hashOf(words);
			// A Hit put on then taken off before the write: the Blob holds these words already, and writing them
			// again would give Live an undo entry that changes nothing.
			if((m_known & bit) && m_held[static_cast<size_t>(p)] == hash)
				continue;
			m_held[static_cast<size_t>(p)] = hash;
			m_known |= bit;
			writes.push_back({p, std::move(words)});
		}
		m_unstored = 0;
		return writes;
	}

	PatternStore::Received PatternStore::received(const int _pattern, const int64_t* _words, const size_t _count,
		Pattern& _out)
	{
		if(_pattern < 0 || _pattern >= PatternCount)
			return Received::Unusable;
		const auto index = static_cast<size_t>(_pattern);
		const uint32_t bit = 1u << _pattern;
		// The echo of a write, the commonest value, comes back as the very words written: known without decoding them
		// or encoding them again.
		if((m_known & bit) && m_held[index] == hashOf(_words, _count))
			return Received::Echo;

		// On the heap: a Pattern is 30 kB, and decode keeps another on the stack. Left empty when decode refuses.
		const auto pattern = std::make_unique<Pattern>();
		const bool readable = decode(_words, _count, *pattern);

		// A Pattern read is compared as the words this format writes for it, so that the initial NoPatternStored and
		// an empty Pattern are the same: when Live sends every Blob again after an undo, an empty one neither reloads
		// nor drops an edit pending on it. Words refused are compared as they are: no Pattern encodes to them, so
		// the next write of the Pattern, whatever it holds, is never skipped as held already.
		const uint64_t hash = readable ? hashOf(encode(*pattern)) : hashOf(_words, _count);
		if((m_known & bit) && m_held[index] == hash)
			return Received::Echo;

		// The set's value wins over an edit not yet written: written after it, the edit would undo Live's undo. Words
		// refused give the empty Pattern: the device then plays what the set will give when it reopens, rather than a
		// Pattern the set no longer holds.
		m_held[index] = hash;
		m_known |= bit;
		m_unstored &= ~bit;
		_out = *pattern;
		return readable ? Received::Load : Received::Refused;
	}
}
