#include "mdseqText.h"

#include <array>

namespace mdseq
{
	namespace
	{
		// A face: its name, the advances in thousandths of an em (unitsPerEm 1000) of ' ' to '~', and the ellipsis',
		// from AbletonSansMedium-Regular.otf and AbletonSansBold-Regular.otf in Live 12's Max; digits are tabular.
		struct Metrics
		{
			const char* name;
			std::array<uint16_t, 95> advances;
			uint16_t ellipsis;
		};

		constexpr Metrics Medium{"Ableton Sans Medium", {
			230, 313, 457, 590, 590, 835, 724, 251, 405, 405, 449, 590, 307, 410, 302, 467,
			590, 590, 590, 590, 590, 590, 590, 590, 590, 590, 302, 315, 600, 590, 590, 479,
			920, 652, 609, 680, 685, 568, 551, 707, 696, 281, 539, 638, 499, 853, 717, 729,
			600, 729, 635, 569, 593, 688, 651, 971, 634, 610, 601, 383, 467, 370, 590, 600,
			360, 526, 577, 520, 573, 528, 383, 563, 556, 242, 242, 506, 242, 808, 556, 553,
			578, 578, 403, 453, 399, 554, 496, 718, 479, 491, 462, 440, 405, 400, 600}, 861};
		constexpr Metrics Bold{"Ableton Sans Bold Regular", {
			230, 338, 487, 590, 590, 864, 739, 268, 417, 417, 458, 590, 334, 410, 328, 476,
			590, 590, 590, 590, 590, 590, 590, 590, 590, 590, 328, 342, 600, 590, 590, 499,
			913, 671, 616, 689, 703, 563, 545, 720, 706, 299, 556, 666, 500, 854, 727, 738,
			630, 738, 659, 575, 604, 700, 667, 985, 661, 623, 602, 393, 476, 374, 590, 600,
			387, 530, 585, 533, 581, 531, 413, 577, 571, 257, 257, 520, 257, 830, 571, 557,
			587, 587, 427, 462, 423, 569, 515, 720, 504, 505, 460, 449, 417, 409, 600}, 937};
		constexpr uint16_t OtherAdvance = 600;	// a character beyond the tables
		constexpr std::string_view Ellipsis = "\xE2\x80\xA6";

		const Metrics& metrics(const Face _face)
		{
			return _face == Face::Bold ? Bold : Medium;
		}

		// The length of the UTF-8 sequence that starts with _lead, 1 for a byte that starts none.
		size_t sequenceLength(const unsigned char _lead)
		{
			if(_lead >= 0xF0)
				return 4;
			if(_lead >= 0xE0)
				return 3;
			if(_lead >= 0xC0)
				return 2;
			return 1;
		}

		// The advance of the character that starts at _at, and its length in bytes.
		uint16_t advanceAt(const std::string_view _text, const size_t _at, const Metrics& _metrics, size_t& _length)
		{
			const auto c = static_cast<unsigned char>(_text[_at]);
			_length = sequenceLength(c);
			return c < 0x20 || c > 0x7E ? OtherAdvance : _metrics.advances[c - 0x20];
		}

		// Thousandths of an em to hundredths of a pixel at _size px.
		int scaled(const int _units, const int _size)
		{
			return _units * _size / 10;
		}
	}

	const char* fontName(const Face _face)
	{
		return metrics(_face).name;
	}

	int textWidth(const std::string_view _text, const Face _face, const int _size)
	{
		const Metrics& m = metrics(_face);
		int units = 0;
		for(size_t at = 0, length = 0; at < _text.size(); at += length)
			units += advanceAt(_text, at, m, length);
		return scaled(units, _size);
	}

	std::string fitText(const std::string_view _text, const TextBox& _box)
	{
		if(textWidth(_text, _box.face, _box.size) <= 100 * _box.width)
			return std::string(_text);

		const Metrics& m = metrics(_box.face);
		const int budget = 100 * _box.width - (_box.ellipsis ? scaled(m.ellipsis, _box.size) : 0);
		int units = 0;
		size_t end = 0;
		for(size_t at = 0, length = 0; at < _text.size(); at += length)
		{
			units += advanceAt(_text, at, m, length);
			if(scaled(units, _box.size) > budget || at + length > _text.size())
				break;
			end = at + length;
		}
		while(end > 0 && _text[end - 1] == ' ')
			--end;
		std::string out(_text.substr(0, end));
		if(_box.ellipsis)
			out += Ellipsis;
		return out;
	}
}
