#include "mdseqKit.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <vector>

namespace mdseq
{
	namespace
	{
		constexpr std::string_view Header = "md-drums-kit 1";
		constexpr int MaxMachine = 255;	// a byte: the Machinedrum's 0-191, then MoDule's User Machines from 194
		constexpr uint32_t IdMask = 0xFFFFFF;

		// _line's fields, split on single spaces.
		std::vector<std::string_view> fields(const std::string_view _line)
		{
			std::vector<std::string_view> f;
			size_t begin = 0;
			while(begin <= _line.size())
			{
				const size_t end = std::min(_line.find(' ', begin), _line.size());
				f.push_back(_line.substr(begin, end - begin));
				begin = end + 1;
			}
			return f;
		}

		// _field as a number from _min to _max, or nothing.
		std::optional<int> number(const std::string_view _field, const int _min, const int _max)
		{
			int value = 0;
			const auto [end, error] = std::from_chars(_field.data(), _field.data() + _field.size(), value);
			if(error != std::errc() || end != _field.data() + _field.size() || value < _min || value > _max)
				return std::nullopt;
			return value;
		}

		// "track <1-16> <Machine id> <Machine name> <SYN1-8 names>"
		void readTrack(const std::vector<std::string_view>& _f, Kit& _kit)
		{
			if(_f.size() != 3 + 1 + SynCount)
				return;
			const std::optional<int> track = number(_f[1], 1, TrackCount);
			const std::optional<int> machine = number(_f[2], 0, MaxMachine);
			if(!track || !machine || _f[3].empty())
				return;
			KitTrack& t = _kit.tracks[static_cast<size_t>(*track - 1)];
			t.machine = *machine;
			t.machineName = std::string(_f[3]);
			for(int i = 0; i < SynCount; ++i)
				t.synNames[static_cast<size_t>(i)] = std::string(_f[static_cast<size_t>(4 + i)]);
		}

		// "values <1-16> <24 values 0-127>"
		void readValues(const std::vector<std::string_view>& _f, Kit& _kit)
		{
			if(_f.size() != 2 + LockCount)
				return;
			const std::optional<int> track = number(_f[1], 1, TrackCount);
			if(!track)
				return;
			std::array<int, LockCount> values{};
			for(int i = 0; i < LockCount; ++i)
			{
				const std::optional<int> v = number(_f[static_cast<size_t>(2 + i)], 0, 127);
				if(!v)
					return;
				values[static_cast<size_t>(i)] = *v;
			}
			_kit.tracks[static_cast<size_t>(*track - 1)].values = values;
		}
	}

	std::optional<Kit> parseKit(const std::string_view _text)
	{
		Kit kit;
		bool first = true;
		size_t begin = 0;
		while(begin < _text.size())
		{
			size_t end = _text.find('\n', begin);
			if(end == std::string_view::npos)
				end = _text.size();
			std::string_view line = _text.substr(begin, end - begin);
			begin = end + 1;
			if(!line.empty() && line.back() == '\r')
				line.remove_suffix(1);

			// The first line names the version: this build reads 1 only.
			if(first)
			{
				if(line != Header)
					return std::nullopt;
				first = false;
				continue;
			}
			if(line == "kit" || line.substr(0, 4) == "kit ")
			{
				kit.name = std::string(line.size() > 4 ? line.substr(4) : std::string_view());
				continue;
			}
			const std::vector<std::string_view> f = fields(line);
			if(f[0] == "track")
				readTrack(f, kit);
			else if(f[0] == "values")
				readValues(f, kit);
		}
		if(first)
			return std::nullopt;
		return kit;
	}

	std::array<MidiBytes, 2 * HelloNotes> hello(const uint32_t _id)
	{
		std::array<MidiBytes, 2 * HelloNotes> bytes{};
		for(int n = 0; n < HelloNotes; ++n)
		{
			const auto note = static_cast<uint8_t>(HelloFirstNote + n);
			const auto bits = static_cast<uint8_t>((_id >> (6 * (HelloNotes - 1 - n))) & 0x3F);
			bytes[static_cast<size_t>(2 * n)] = {0x90, note, static_cast<uint8_t>(1 + bits)};
			bytes[static_cast<size_t>(2 * n + 1)] = {0x80, note, 0};
		}
		return bytes;
	}

	KitLink::KitLink(const uint64_t _random)
	{
		// The low 24 bits, else the next 24, else 1: MD Drums holds no id 0.
		m_id = static_cast<uint32_t>(_random) & IdMask;
		if(m_id == 0)
			m_id = static_cast<uint32_t>(_random >> 24) & IdMask;
		if(m_id == 0)
			m_id = 1;
	}

	std::string KitLink::fileName() const
	{
		char name[16];
		std::snprintf(name, sizeof(name), "%06x.kit", static_cast<unsigned>(m_id));
		return name;
	}

	bool KitLink::poll(const KitFile& _file)
	{
		if(_file.state == KitFile::State::Busy)
			return false;
		std::optional<std::string> text;
		if(_file.state == KitFile::State::Read)
			text = _file.text;
		if(text == m_text)
			return false;
		m_text = std::move(text);
		std::optional<Kit> kit = m_text ? parseKit(*m_text) : std::nullopt;
		if(kit == m_kit)
			return false;
		m_kit = std::move(kit);
		return true;
	}

	bool HelloPace::tick(const KitFile::State _file)
	{
		// MD Drums answers a hello within a second; then it only has to hear that the device is still there. Without an
		// answer, there may be no MD Drums on the track at all: each hello waits twice as long as the one before.
		constexpr int TicksFirst = 2;	// a second
		constexpr int TicksMost = 20;	// ten seconds
		const bool answered = _file != KitFile::State::Absent;
		if(answered)
			m_wait = 1;
		const int due = answered ? TicksMost : std::max(m_wait, TicksFirst);
		if(m_sinceHello >= 0 && ++m_sinceHello < due)
			return false;
		m_sinceHello = 0;
		if(!answered)
			m_wait = std::min(2 * m_wait, TicksMost);
		return true;
	}
}
