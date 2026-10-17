#include "mdseqCondition.h"

#include <cstring>

namespace mdseq
{
	namespace
	{
		using Kind = ConditionInfo::Kind;

		// Elektron's chances
		constexpr int Percents[] = {1, 2, 4, 6, 9, 13, 19, 25, 33, 41, 50, 59, 67, 75, 81, 87, 91, 94, 96, 98, 99};
		constexpr int MaxRatio = 8;
		constexpr int FlagCount = 8;
		static_assert(1 + sizeof(Percents) / sizeof(Percents[0]) + (MaxRatio * (MaxRatio + 1) / 2 - 1) + FlagCount ==
			ConditionCount, "None, the chances, the ratios A:B with B from 2 to 8, and the flags");

		struct Entry
		{
			ConditionInfo info;
			char label[8];
		};

		// One or two digits into _label at _at, which moves past them.
		constexpr void writeNumber(char* _label, int& _at, const int _value)
		{
			if(_value >= 10)
				_label[_at++] = static_cast<char>('0' + _value / 10);
			_label[_at++] = static_cast<char>('0' + _value % 10);
		}

		constexpr Entry flag(const Kind _kind, const char* _label)
		{
			Entry e{{_kind, 0, 0}, {}};
			for(int i = 0; _label[i]; ++i)
				e.label[i] = _label[i];
			return e;
		}

		// Built at compile time: the Sequencer reads it on Live's audio thread, which must never allocate or wait on a
		// lock, as a table built at its first use would.
		constexpr std::array<Entry, ConditionCount> makeTable()
		{
			std::array<Entry, ConditionCount> table{};
			int n = 0;
			table[n++] = flag(Kind::None, "None");
			for(const int p : Percents)
			{
				Entry e{{Kind::Percent, p, 100}, {}};
				int at = 0;
				writeNumber(e.label, at, p);
				e.label[at] = '%';
				table[n++] = e;
			}
			for(int b = 2; b <= MaxRatio; ++b)
			{
				for(int a = 1; a <= b; ++a)
				{
					Entry e{{Kind::Ratio, a, b}, {}};
					int at = 0;
					writeNumber(e.label, at, a);
					e.label[at++] = ':';
					writeNumber(e.label, at, b);
					table[n++] = e;
				}
			}
			for(const Entry& e : {flag(Kind::Fill, "FILL"), flag(Kind::NotFill, "!FILL"), flag(Kind::Pre, "PRE"),
				flag(Kind::NotPre, "!PRE"), flag(Kind::Nei, "NEI"), flag(Kind::NotNei, "!NEI"), flag(Kind::First, "1ST"),
				flag(Kind::NotFirst, "!1ST")})
				table[n++] = e;
			return table;
		}

		constexpr std::array<Entry, ConditionCount> Table = makeTable();

		const std::array<Entry, ConditionCount>& table()
		{
			return Table;
		}

		// SplitMix64's finaliser: every bit of the input moves every bit of the output.
		uint64_t mix(uint64_t _x)
		{
			_x += 0x9e3779b97f4a7c15ull;
			_x = (_x ^ (_x >> 30)) * 0xbf58476d1ce4e5b9ull;
			_x = (_x ^ (_x >> 27)) * 0x94d049bb133111ebull;
			return _x ^ (_x >> 31);
		}
	}

	ConditionInfo condition(const int _index)
	{
		if(_index < 0 || _index >= ConditionCount)
			return {ConditionInfo::Kind::None, 0, 0};
		return table()[static_cast<size_t>(_index)].info;
	}

	const char* conditionLabel(const int _index)
	{
		if(_index < 0 || _index >= ConditionCount)
			return "";
		return table()[static_cast<size_t>(_index)].label;
	}

	int conditionIndex(const char* _label)
	{
		for(int i = 0; i < ConditionCount; ++i)
			if(std::strcmp(table()[static_cast<size_t>(i)].label, _label) == 0)
				return i;
		return -1;
	}

	uint64_t passSeed(const uint64_t _startSeed, const uint32_t _pass)
	{
		return mix(_startSeed ^ mix(_pass));
	}

	int draw(const uint64_t _passSeed, const int _track, const int _step)
	{
		const auto where = static_cast<uint64_t>(_track * MaxSteps + _step + 1);
		return static_cast<int>(mix(_passSeed ^ mix(where)) % 100);
	}

	bool evaluate(const int _conditionIndex, const int _track, const int _step, const Draw& _draw,
		ConditionMemory& _memory)
	{
		using Kind = ConditionInfo::Kind;
		const ConditionInfo c = condition(_conditionIndex);
		const int8_t neighbour = _track > 0 ? _memory.last[static_cast<size_t>(_track - 1)] : -1;
		const int8_t previous = _memory.last[static_cast<size_t>(_track)];

		bool result = true;
		switch(c.kind)
		{
		case Kind::None:		return true;
		case Kind::Pre:			return previous == 1;
		case Kind::NotPre:		return previous != 1;
		case Kind::Nei:			return neighbour == 1;
		case Kind::NotNei:		return neighbour != 1;
		case Kind::Percent:		result = draw(_draw.passSeed, _track, _step) < c.a; break;
		case Kind::Ratio:		result = static_cast<int>((_draw.pass - 1) % static_cast<uint32_t>(c.b)) + 1 == c.a; break;
		case Kind::Fill:		result = _draw.fill; break;
		case Kind::NotFill:		result = !_draw.fill; break;
		case Kind::First:		result = _draw.pass == 1; break;
		case Kind::NotFirst:	result = _draw.pass != 1; break;
		}
		_memory.last[static_cast<size_t>(_track)] = result ? 1 : 0;
		return result;
	}
}
