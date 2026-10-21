#include "mdseqPush.h"

#include <algorithm>

namespace mdseq
{
	namespace push
	{
		namespace
		{
			// The palette slots this module rewrites, past those Live gives clips and tracks.
			constexpr uint8_t FirstSlot = 100;

			constexpr Rgb Colours[ColourCount] = {
				{0x00, 0x00, 0x00},	// Off
				{0x26, 0x26, 0x26},	// Beat
				{0x4c, 0x31, 0x0d},	// HitLow: the Hit's orange, dimmed as the strip tints a soft Hit
				{0x99, 0x63, 0x1b},	// HitMid
				{0xff, 0xa5, 0x2e},	// HitHigh
				{0x4c, 0x20, 0x1b},	// AccentEmpty: the Accent row's red, dimmed
				{0xff, 0x6b, 0x5b},	// AccentHit
				{0xff, 0xff, 0xff},	// Played
				{0x7c, 0xc6, 0xff},	// Selected: the window's accent before v12
				{0x66, 0x53, 0x00},	// TrackPlays: the activators' yellow, dimmed
				{0x1a, 0x16, 0x00},	// TrackMuted
				{0xff, 0xd0, 0x00},	// TrackShown
				{0xff, 0xf4, 0xc0},	// TrackHit
				{0x1a, 0x1a, 0x1a},	// PatternEmpty
				{0x80, 0x80, 0x80},	// PatternUsed
				{0xff, 0xa5, 0x2e},	// PatternShown
				{0x7c, 0xc6, 0xff},	// PatternQueued
				{0xff, 0xff, 0xff},	// ButtonOn
			};

			constexpr const char* ButtonNames[ButtonCount] = {"Page_Left_Button", "Page_Right_Button", "Delete_Button",
				"Duplicate_Button", "Double_Button", "Global_Mute_Button", "Accent_Button", "Track_Select_Button0",
				"Track_Select_Button1", "Track_Select_Button2"};

			constexpr const char* EncoderNames[8] = {"Track_Control_0", "Track_Control_1", "Track_Control_2",
				"Track_Control_3", "Track_Control_4", "Track_Control_5", "Track_Control_6", "Track_Control_7"};

			// Push 2's SysEx header: Ableton's manufacturer id, the device, the model
			constexpr uint8_t Header[] = {0xf0, 0x00, 0x21, 0x1d, 0x01, 0x01};
		}

		int stepAt(const int _column, const int _row)
		{
			if(_column < 0 || _column >= Columns || _row < 0 || _row >= StepRows)
				return -1;
			return _row * Columns + _column;
		}

		int trackAt(const int _column, const int _row)
		{
			if(_column < 0 || _column >= 4 || _row < StepRows || _row >= Rows)
				return -1;
			return (Rows - 1 - _row) * 4 + _column;
		}

		int patternAt(const int _column, const int _row)
		{
			if(_column < 4 || _column >= Columns || _row < StepRows || _row >= Rows)
				return -1;
			return (_row - StepRows) * 4 + _column - 4;
		}

		const char* controlName(const Button _button)
		{
			return ButtonNames[static_cast<size_t>(_button)];
		}

		const char* encoderName(const int _index)
		{
			return _index >= 0 && _index < 8 ? EncoderNames[_index] : "";
		}

		uint8_t slotOf(const Colour _colour)
		{
			return _colour == Colour::Off ? 0 : static_cast<uint8_t>(FirstSlot + static_cast<int>(_colour));
		}

		Rgb colourOf(const Colour _colour)
		{
			return Colours[static_cast<size_t>(_colour)];
		}

		std::vector<uint8_t> paletteEntry(const uint8_t _slot, const Rgb _rgb)
		{
			// Each 8-bit channel in two bytes, its low seven bits then its top bit.
			std::vector<uint8_t> m(std::begin(Header), std::end(Header));
			m.push_back(0x03);
			m.push_back(_slot);
			for(const uint8_t v : {_rgb.r, _rgb.g, _rgb.b, uint8_t{0}})
			{
				m.push_back(v & 0x7f);
				m.push_back(v >> 7);
			}
			m.push_back(0xf7);
			return m;
		}

		std::vector<uint8_t> reapplyPalette()
		{
			std::vector<uint8_t> m(std::begin(Header), std::end(Header));
			m.push_back(0x05);
			m.push_back(0xf7);
			return m;
		}
	}

	void PushOutput::clear()
	{
		pads.clear();
		buttons.clear();
		editor.clear();
		notes.clear();
	}

	PushMode::PushMode(Editor& _editor) : m_editor(_editor)
	{
		m_padsSent.fill(-1);
		m_lightsSent.fill(-1);
	}

	const PushOutput& PushMode::setActive(const bool _active)
	{
		m_out.clear();
		if(_active == m_active)
		{
			light();
			return m_out;
		}
		m_active = _active;
		if(m_active)
		{
			// Lit whole: Push showed something else meanwhile.
			m_padsSent.fill(-1);
			m_lightsSent.fill(-1);
			light();
			return m_out;
		}
		for(int t = 0; t < TrackCount; ++t)
			trackPad(t, 0);
		m_held.fill(false);
		m_heldPad = -1;
		m_heldStep = -1;
		m_duplicateSource = -1;
		m_muteLatched = false;
		return m_out;
	}

	const PushOutput& PushMode::pad(const int _column, const int _row, const int _velocity, const double _ms)
	{
		m_out.clear();
		if(!m_active)
			return m_out;
		const bool down = _velocity > 0;
		if(push::stepAt(_column, _row) >= 0)
			stepPad(_column, _row, down, _ms);
		else if(const int track = push::trackAt(_column, _row); track >= 0)
			trackPad(track, std::clamp(_velocity, 0, 127));
		else if(const int pattern = push::patternAt(_column, _row); pattern >= 0)
			patternPad(pattern, down);
		light();
		return m_out;
	}

	const PushOutput& PushMode::button(const push::Button _button, const bool _down)
	{
		using push::Button;
		m_out.clear();
		if(!m_active)
			return m_out;
		m_held[static_cast<size_t>(_button)] = _down;
		if(_down)
		{
			switch(_button)
			{
			case Button::PageLeft:	m_half = 0; break;
			case Button::PageRight:	m_half = 1; break;
			case Button::Double:	take(m_editor.doubleLength()); break;
			case Button::LockPage1:	m_lockPage = 0; break;
			case Button::LockPage2:	m_lockPage = 1; break;
			case Button::LockPage3:	m_lockPage = 2; break;
			case Button::Mute:		m_muteUsed = false; break;
			default:				break;
			}
		}
		else if(_button == Button::Duplicate)
		{
			m_duplicateSource = -1;
		}
		else if(_button == Button::Mute && !m_muteUsed)
		{
			m_muteLatched = !m_muteLatched;
		}
		light();
		return m_out;
	}

	const PushOutput& PushMode::encoder(const int _index, const int _delta)
	{
		m_out.clear();
		if(!m_active || m_heldStep < 0 || _index < 0 || _index >= 8 || _delta == 0)
			return m_out;
		// From the Lock, else the Kit's value, else the middle; NoLock (63) cannot travel to MD Drums: stepped over.
		const int param = m_lockPage * 8 + _index;
		const int current = m_editor.lockOrKit(param);
		int value = std::clamp((current < 0 ? 64 : current) + _delta, 0, 127);
		if(value == NoLock)
			value = _delta > 0 ? NoLock + 1 : NoLock - 1;
		take(m_editor.setLock(param, value));
		m_heldTurned = true;
		light();
		return m_out;
	}

	const PushOutput& PushMode::observed(const int _observer, const long* _values, const size_t _count,
		const double _ms)
	{
		using namespace push;
		if(_observer == MatrixObserver && _count >= 3)
			return pad(static_cast<int>(_values[1]), static_cast<int>(_values[2]), static_cast<int>(_values[0]), _ms);
		if(_observer >= buttonObserver(Button::PageLeft) && _observer < encoderObserver(0) && _count >= 1)
			return button(static_cast<Button>(_observer - buttonObserver(Button::PageLeft)), _values[0] > 0);
		if(_observer >= encoderObserver(0) && _observer < ObserverCount && _count >= 1)
		{
			const long clicks = _values[0] < 64 ? _values[0] : _values[0] - 128;
			return encoder(_observer - encoderObserver(0), static_cast<int>(clicks));
		}
		m_out.clear();
		return m_out;
	}

	const PushOutput& PushMode::refresh()
	{
		m_out.clear();
		light();
		return m_out;
	}

	const PushOutput& PushMode::relight()
	{
		m_padsSent.fill(-1);
		m_lightsSent.fill(-1);
		return refresh();
	}

	void PushMode::stepPad(const int _column, const int _row, const bool _down, const double _ms)
	{
		const int pad = _row * push::Columns + _column;
		const Pattern& pattern = m_editor.pattern();
		if(!_down)
		{
			// Let go soon after pressing a Hit, without a Lock turned: the Hit pressed off, as a click on it.
			if(pad != m_heldPad)
				return;
			const int heldStep = m_heldStep;
			const bool off = !m_heldPut && !m_heldTurned && _ms - m_heldSince < push::HoldMs &&
				pattern.steps[static_cast<size_t>(m_heldTrack)][static_cast<size_t>(heldStep)].hit;
			m_heldPad = -1;
			m_heldStep = -1;
			if(off)
			{
				take(m_editor.selectStep(m_heldTrack, heldStep));
				take(m_editor.setHit(false));
			}
			return;
		}
		const int track = m_editor.shownTrack();
		const int step = m_half * push::StepPads + push::stepAt(_column, _row);
		if(step >= pattern.length)
			return;
		const bool hit = pattern.steps[static_cast<size_t>(track)][static_cast<size_t>(step)].hit;
		if(held(push::Button::Accent))
		{
			take(m_editor.apply({Input::Kind::SetAccentStep, 0, step, pattern.inAccentRow(step) ? 0 : 1}));
			return;
		}
		if(held(push::Button::Delete))
		{
			if(hit)
			{
				take(m_editor.selectStep(track, step));
				take(m_editor.setHit(false));
			}
			return;
		}
		take(m_editor.selectStep(track, step));
		if(!hit)
			take(m_editor.setHit(true));
		m_heldPad = pad;
		m_heldTrack = track;
		m_heldStep = step;
		m_heldSince = _ms;
		m_heldPut = !hit;
		m_heldTurned = false;
	}

	void PushMode::trackPad(const int _track, const int _velocity)
	{
		const auto track = static_cast<size_t>(_track);
		if(_velocity == 0)
		{
			if(m_sounding[track])
				m_out.notes.push_back({noteStatus(NoteOff, _track), static_cast<uint8_t>(FirstHitNote + _track), 0});
			m_sounding[track] = false;
			return;
		}
		if(held(push::Button::Mute) || m_muteLatched)
		{
			if(held(push::Button::Mute))
				m_muteUsed = true;
			take(m_editor.setMute(_track, !m_editor.muted(_track)));
			return;
		}
		// The Hit, through the device's MIDI out as the Sequencer's, and the Track shown on the steps.
		m_out.notes.push_back({noteStatus(NoteOn, _track), static_cast<uint8_t>(FirstHitNote + _track),
			static_cast<uint8_t>(_velocity)});
		m_sounding[track] = true;
		take(m_editor.showTrack(_track));
	}

	void PushMode::patternPad(const int _pattern, const bool _down)
	{
		if(!_down)
			return;
		if(held(push::Button::Delete))
		{
			take(m_editor.clearPattern(_pattern));
			return;
		}
		if(held(push::Button::Duplicate))
		{
			if(m_duplicateSource < 0)
			{
				m_duplicateSource = _pattern;
				return;
			}
			take(m_editor.copyPattern(_pattern, m_duplicateSource));
			m_duplicateSource = -1;
			return;
		}
		take(m_editor.chainMode() ? m_editor.chainAdd(_pattern) : m_editor.setPattern(_pattern));
	}

	void PushMode::take(const EditorOutput& _out)
	{
		EditorOutput& e = m_out.editor;
		e.strip.insert(e.strip.end(), _out.strip.begin(), _out.strip.end());
		e.grid.insert(e.grid.end(), _out.grid.begin(), _out.grid.end());
		e.panel.insert(e.panel.end(), _out.panel.begin(), _out.panel.end());
		e.forward.insert(e.forward.end(), _out.forward.begin(), _out.forward.end());
	}

	void PushMode::light()
	{
		using push::Button;
		if(!m_active)
			return;
		for(int r = 0; r < push::Rows; ++r)
		{
			for(int c = 0; c < push::Columns; ++c)
			{
				push::Colour colour = push::Colour::Off;
				if(const int step = push::stepAt(c, r); step >= 0)
					colour = stepColour(m_half * push::StepPads + step);
				else if(const int track = push::trackAt(c, r); track >= 0)
					colour = trackColour(track);
				else if(const int pattern = push::patternAt(c, r); pattern >= 0)
					colour = patternColour(pattern);
				const int slot = push::slotOf(colour);
				int& sent = m_padsSent[static_cast<size_t>(r * push::Columns + c)];
				if(sent == slot)
					continue;
				sent = slot;
				m_out.pads.push_back({c, r, static_cast<uint8_t>(slot)});
			}
		}
		for(int b = 0; b < push::ButtonCount; ++b)
		{
			const auto button = static_cast<Button>(b);
			bool on = m_held[static_cast<size_t>(b)];
			if(button == Button::PageLeft || button == Button::PageRight)
				on = m_half == (button == Button::PageLeft ? 0 : 1);
			else if(button == Button::Mute)
				on = on || m_muteLatched;
			else if(button >= Button::LockPage1)
				on = m_lockPage == b - static_cast<int>(Button::LockPage1);
			int& sent = m_lightsSent[static_cast<size_t>(b)];
			if(sent == (on ? 1 : 0))
				continue;
			sent = on ? 1 : 0;
			m_out.buttons.push_back({button, on});
		}
	}

	push::Colour PushMode::stepColour(const int _step) const
	{
		using push::Colour;
		const Pattern& pattern = m_editor.pattern();
		if(_step >= pattern.length)
			return Colour::Off;
		if(_step == m_editor.playedStep())
			return Colour::Played;
		if(_step == m_heldStep && m_editor.shownTrack() == m_heldTrack)
			return Colour::Selected;
		const Step& step = pattern.steps[static_cast<size_t>(m_editor.shownTrack())][static_cast<size_t>(_step)];
		if(step.hit)
		{
			if(pattern.inAccentRow(_step))
				return Colour::AccentHit;
			return step.velocity >= 96 ? Colour::HitHigh : step.velocity >= 43 ? Colour::HitMid : Colour::HitLow;
		}
		if(pattern.inAccentRow(_step))
			return Colour::AccentEmpty;
		return _step % 4 == 0 ? Colour::Beat : Colour::Off;
	}

	push::Colour PushMode::trackColour(const int _track) const
	{
		using push::Colour;
		if(m_editor.muted(_track))
			return Colour::TrackMuted;
		const int played = m_editor.playedStep();
		const Pattern& pattern = m_editor.pattern();
		if(played >= 0 && played < MaxSteps &&
			pattern.steps[static_cast<size_t>(_track)][static_cast<size_t>(played)].hit)
			return Colour::TrackHit;
		return _track == m_editor.shownTrack() ? Colour::TrackShown : Colour::TrackPlays;
	}

	push::Colour PushMode::patternColour(const int _pattern) const
	{
		using push::Colour;
		switch(m_editor.patternState(_pattern))
		{
		case PatternState::Empty:	return Colour::PatternEmpty;
		case PatternState::Used:	return Colour::PatternUsed;
		case PatternState::Shown:	return Colour::PatternShown;
		case PatternState::Queued:	break;
		}
		return Colour::PatternQueued;
	}
}
