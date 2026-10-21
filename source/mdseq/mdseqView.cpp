#include "mdseqView.h"

#include "mdseqCondition.h"
#include "mdseqText.h"

#include <cmath>
#include <cstdio>

namespace mdseq
{
	namespace
	{
		constexpr double PlayedTint = 0.18;
		constexpr double AccentWell = 0.25;		// an accented step's well without a Hit: a quarter of the colour
		constexpr int Inset = 2;
		constexpr int TextDrop = 6;				// a label's baseline above the cell's bottom
		constexpr int EndWidth = 2;

		uint8_t mixChannel(const uint8_t _a, const uint8_t _b, const double _t)
		{
			return static_cast<uint8_t>(std::lround(_a + (_b - _a) * _t));
		}

		DrawOp rect(const DrawOp::Kind _kind, const Rgb _colour, const int _left, const int _top, const int _right,
			const int _bottom)
		{
			return {_kind, _colour, static_cast<int16_t>(_left), static_cast<int16_t>(_top), static_cast<int16_t>(_right),
				static_cast<int16_t>(_bottom)};
		}

		// The background every kind of cell starts from: off past the Pattern's length, lighter on a beat, tinted when
		// played.
		Rgb background(const CellLook& _look)
		{
			if(_look.past)
				return colours::Past;
			const Rgb plain = _look.beat ? colours::Beat : colours::Panel;
			return _look.played ? mix(plain, colours::Accent, PlayedTint) : plain;
		}

		// The line in the gap after the cell _last of a row whose cells start at _cellX, _count of them in an lcd of
		// _width by _height.
		bool endAfter(const int _last, const int _count, int (*_cellX)(int), const int _cellWidth, const int _width,
			const int _height, DrawOp& _out)
		{
			if(_last < 0 || _last >= _count)
				return false;
			const int gapLeft = _cellX(_last) + _cellWidth;
			const int gapRight = _last + 1 < _count ? _cellX(_last + 1) : _width;
			if(gapRight - gapLeft < EndWidth)
				return false;
			const int left = gapLeft + (gapRight - gapLeft - EndWidth) / 2;
			_out = rect(DrawOp::Kind::PaintRect, colours::End, left, 0, left + EndWidth, _height);
			return true;
		}
	}

	Rgb mix(const Rgb _a, const Rgb _b, const double _t)
	{
		return {mixChannel(_a.r, _b.r, _t), mixChannel(_a.g, _b.g, _t), mixChannel(_a.b, _b.b, _t)};
	}

	size_t drawCell(const Step& _step, const CellLook& _look, const int _x, const int _y, const int _w, const int _h,
		DrawOp* _out)
	{
		size_t n = 0;
		const int right = _x + _w;
		const int bottom = _y + _h;

		const Rgb back = background(_look);
		_out[n++] = rect(DrawOp::Kind::PaintRect, back, _x, _y, right, bottom);

		// Past the Pattern's length: off, a Hit kept there only hinted (the steps come back if the length grows).
		if(_look.past)
		{
			if(_step.hit)
				_out[n++] = rect(DrawOp::Kind::PaintRect, mix(colours::Past, colours::Hit, PastHit), _x + Inset,
					_y + Inset, right - Inset, bottom - Inset);
			return n;
		}

		// An accented step of the strip takes the Accent's colour whole: its Hit's, or a well of it without one.
		const Rgb ink = _look.accented ? colours::Accented : colours::Hit;
		if(_look.accented && !_step.hit)
			_out[n++] = rect(DrawOp::Kind::PaintRect, mix(back, colours::Accented, AccentWell), _x + Inset, _y + Inset,
				right - Inset, bottom - Inset);

		Rgb hit = mix(colours::Panel, ink, 0.5 + 0.5 * _step.velocity / 127.0);
		if(_look.faded)
			hit = mix(back, hit, FadedHit);
		if(_step.hit)
		{
			_out[n++] = rect(DrawOp::Kind::PaintRect, hit, _x + Inset, _y + Inset, right - Inset, bottom - Inset);

			// Ratchets: a notch cut into the Hit's top edge at the middle of each one's share, on an even pixel.
			if(_step.ratchets > 1)
			{
				const int width = _w - 2 * Inset;
				for(int r = 0; r < _step.ratchets; ++r)
				{
					const int middle = _x + 2 * static_cast<int>(std::lround(width * (r + 0.5) / _step.ratchets / 2));
					_out[n++] = rect(DrawOp::Kind::PaintRect, back, middle, _y + Inset, middle + 2, _y + Inset + 4);
				}
			}

			// Locks: a 4 px accent square in an 8 px square of the panel's colour, in the top right corner.
			if(_step.locked())
			{
				_out[n++] = rect(DrawOp::Kind::PaintRect, colours::Panel, right - 10, _y + 2, right - 2, _y + 10);
				_out[n++] = rect(DrawOp::Kind::PaintRect, colours::Accent, right - 8, _y + 4, right - 4, _y + 8);
			}

			// The Condition: its label on the Hit, centred by the font's own advances.
			if(_step.condition)
			{
				const char* label = conditionLabel(_step.condition);
				const int width = textWidth(label, Face::Bold, LcdFontSize);
				const int left = _x + static_cast<int>(std::lround((100.0 * _w - width) / 200.0));
				DrawOp text = rect(DrawOp::Kind::Text, colours::Text, left, bottom - TextDrop, 0, 0);
				std::snprintf(text.text, sizeof(text.text), "%s", label);
				_out[n++] = text;
			}
		}

		if(_look.played)
			_out[n++] = rect(DrawOp::Kind::FrameRect, colours::Accent, _x, _y, right, bottom);

		if(_look.selected)
		{
			_out[n++] = rect(DrawOp::Kind::FrameRect, colours::Accent, _x, _y, right, bottom);
			_out[n++] = rect(DrawOp::Kind::FrameRect, colours::Accent, _x + 1, _y + 1, right - 1, bottom - 1);
		}

		// Micro-timing, over the frames so that a played or selected step keeps it.
		if(_step.hit && _step.micro != 0)
		{
			const bool early = _step.micro < 0;
			const int middle = _y + _h / 2;
			_out[n++] = rect(DrawOp::Kind::Triangle, hit, early ? _x + Inset : right - Inset, middle - 4,
				early ? _x : right, middle + 4);
		}
		return n;
	}

	size_t drawSwingCell(const bool _swings, const int _swing, const CellLook& _look, const int _x, const int _y,
		const int _w, const int _h, DrawOp* _out)
	{
		// The delay is (swing - 50) / 50 of a step, up to 0.6 at 80 %: the mark moves by half of it, on an even pixel.
		const double delay = _w * (_swing - MinSwing) / 50.0;
		const int shift = 2 * static_cast<int>(std::lround(delay / 4));
		const Rgb mark = mix(colours::Panel, colours::Swing, _swing > MinSwing ? 0.55 : 0.3);
		return drawMarkCell(_swings, mark, _look, _x, _y, _w, _h, _out, shift);
	}

	size_t drawMarkCell(const bool _marked, const Rgb _mark, const CellLook& _look, const int _x, const int _y,
		const int _w, const int _h, DrawOp* _out, const int _shift)
	{
		size_t n = 0;
		const int right = _x + _w;
		const int bottom = _y + _h;

		_out[n++] = rect(DrawOp::Kind::PaintRect, background(_look), _x, _y, right, bottom);
		if(_look.past)
			return n;
		if(_marked)
			_out[n++] = rect(DrawOp::Kind::PaintRect, _mark, _x + Inset + _shift, _y + Inset, right - Inset,
				bottom - Inset);
		if(_look.played)
			_out[n++] = rect(DrawOp::Kind::FrameRect, colours::Accent, _x, _y, right, bottom);
		return n;
	}

	namespace strip
	{
		int cellAt(const int _x, const int _y)
		{
			if(_x < 0 || _x >= Width || _y < 0 || _y >= Height)
				return -1;
			return _x % Pitch < CellWidth ? _x / Pitch : -1;
		}

		bool endLine(const int _length, const int _page, DrawOp& _out)
		{
			return endAfter(_length - 1 - _page * Cells, Cells, cellX, CellWidth, Width, Height, _out);
		}
	}

	namespace grid
	{
		Cell cellAt(const int _x, const int _y)
		{
			constexpr Cell none{-1, -1};
			if(_x < 0 || _x >= Width || _y < 0 || _y >= Height)
				return none;

			const int row = _y / RowPitch;
			if(_y % RowPitch >= CellHeight)
				return none;

			// The second page starts PageGap further: take it off before dividing.
			constexpr int secondPage = 16 * Pitch + PageGap;
			int x = _x;
			int column = 0;
			if(x >= secondPage)
			{
				x -= secondPage;
				column = 16;
			}
			else if(x >= 16 * Pitch)
			{
				return none;
			}
			if(x % Pitch >= CellWidth)
				return none;
			column += x / Pitch;
			return column < Columns ? Cell{row, column} : none;
		}

		bool endLine(const int _length, const int _half, DrawOp& _out)
		{
			return endAfter(_length - 1 - _half * Columns, Columns, cellX, CellWidth, Width, Height, _out);
		}
	}
}
