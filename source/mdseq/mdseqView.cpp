#include "mdseqView.h"

#include "mdseqCondition.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace mdseq
{
	namespace
	{
		constexpr double PlayedTint = 0.18;
		constexpr int HitInset = 3;
		constexpr int TextAdvance = 6;	// Arial bold at 10 px, about

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

		Rgb background = _look.beat ? colours::Beat : colours::Panel;
		if(_look.played)
			background = mix(background, colours::Accent, PlayedTint);
		_out[n++] = rect(DrawOp::Kind::PaintRect, background, _x, _y, right, bottom);

		Rgb hit = mix(colours::Panel, colours::Hit, 0.35 + 0.65 * _step.velocity / 127.0);
		if(_look.faded)
			hit = mix(background, hit, FadedHit);
		if(_step.hit)
		{
			_out[n++] = rect(DrawOp::Kind::PaintRect, hit, _x + HitInset, _y + HitInset, right - HitInset,
				bottom - HitInset);

			// Ratchets: a notch cut into the Hit's top edge at the middle of each one's share of the block.
			if(_step.ratchets > 1)
			{
				const int width = _w - 2 * HitInset;
				for(int r = 0; r < _step.ratchets; ++r)
				{
					const int middle = _x + HitInset +
						static_cast<int>(std::floor(width * (r + 0.5) / _step.ratchets + 0.5));
					_out[n++] = rect(DrawOp::Kind::PaintRect, background, middle - 1, _y + HitInset, middle + 1,
						_y + HitInset + 4);
				}
			}

			// Locks: centred 5 px in from the top right corner, 7 px across, the accent 5 px.
			if(_step.locked())
			{
				_out[n++] = rect(DrawOp::Kind::PaintOval, colours::Panel, right - 9, _y + 1, right - 2, _y + 8);
				_out[n++] = rect(DrawOp::Kind::PaintOval, colours::Accent, right - 8, _y + 2, right - 3, _y + 7);
			}

			// The Condition: its strip in the bottom margin, its label on the Hit, centred at about 6 px a letter.
			if(_step.condition)
			{
				_out[n++] = rect(DrawOp::Kind::PaintRect, colours::Condition, _x + HitInset, bottom - HitInset,
					right - HitInset, bottom);
				const char* label = conditionLabel(_step.condition);
				const int width = static_cast<int>(std::strlen(label)) * TextAdvance;
				DrawOp text = rect(DrawOp::Kind::Text, colours::Text, _x + (_w - width) / 2, bottom - 6, 0, 0);
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
			_out[n++] = rect(DrawOp::Kind::Triangle, hit, early ? _x + HitInset : right - HitInset, middle - 4,
				early ? _x : right, middle + 4);
		}
		return n;
	}

	size_t drawSwingCell(const bool _swings, const int _swing, const CellLook& _look, const int _x, const int _y,
		const int _w, const int _h, DrawOp* _out)
	{
		size_t n = 0;
		const int right = _x + _w;
		const int bottom = _y + _h;

		Rgb background = _look.beat ? colours::Beat : colours::Panel;
		if(_look.played)
			background = mix(background, colours::Accent, PlayedTint);
		_out[n++] = rect(DrawOp::Kind::PaintRect, background, _x, _y, right, bottom);

		if(_swings)
		{
			// The delay is (swing - 50) / 50 of a step, up to 0.6 at 80 %: the mark moves by half of it.
			const int inner = _w - 2 * HitInset;
			const int shift = static_cast<int>(std::lround(inner * 0.5 * (_swing - MinSwing) / 50.0));
			const Rgb mark = _swing > MinSwing ? colours::Accent : mix(colours::Panel, colours::Accent, 0.4);
			_out[n++] = rect(DrawOp::Kind::PaintRect, mark, _x + HitInset + shift, _y + HitInset, right - HitInset,
				bottom - HitInset);
		}

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
	}
}
