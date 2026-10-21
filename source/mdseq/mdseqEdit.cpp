#include "mdseqEdit.h"

#include "mdseqCondition.h"

#include <algorithm>

namespace mdseq
{
	std::array<int, InputWords> pack(const Input& _in)
	{
		return {static_cast<int>(_in.kind), _in.track, _in.step, _in.value, _in.param, _in.pattern};
	}

	Input unpack(const std::array<int, InputWords>& _words)
	{
		return {static_cast<Input::Kind>(_words[0]), _words[1], _words[2], _words[3], _words[4], _words[5]};
	}

	namespace
	{
		// _field takes _value: false when it holds it already.
		template<typename T>
		bool assign(T& _field, const T _value)
		{
			if(_field == _value)
				return false;
			_field = _value;
			return true;
		}

		// Bit _index of _bits set or cleared: false when it was already.
		template<typename T>
		bool setBit(T& _bits, const int _index, const bool _on)
		{
			const auto bit = static_cast<T>(T{1} << _index);
			return assign(_bits, static_cast<T>(_on ? (_bits | bit) : (_bits & ~bit)));
		}
	}

	bool applyEdit(Pattern& _pattern, uint16_t& _mutes, const Input& _in)
	{
		using Kind = Input::Kind;

		// The Pattern's own values. The Fill is played, and the choice of the Pattern played is the Sequencer's:
		// neither is in a Pattern.
		switch(_in.kind)
		{
		case Kind::SetFill:
		case Kind::SetPattern:
		case Kind::ChainEntry:
		case Kind::SetChain:
		case Kind::CopyPattern:	// two Patterns: the bank's holder copies (copyPattern)
			return false;
		case Kind::ClearPattern:
			return assign(_pattern, Pattern{});
		case Kind::DoubleLength:
		{
			// As Live's Duplicate Loop doubles a clip: every Track's steps, and the swing, Accent and Slide rows,
			// again after them, up to the last step.
			const int length = _pattern.length;
			const int doubled = std::min(2 * length, MaxSteps);
			if(doubled == length)
				return false;
			for(auto& track : _pattern.steps)
				std::copy(track.begin(), track.begin() + (doubled - length), track.begin() + length);
			for(int s = length; s < doubled; ++s)
			{
				setBit(_pattern.swingSteps, s, _pattern.inSwingRow(s - length));
				setBit(_pattern.accentSteps, s, _pattern.inAccentRow(s - length));
				setBit(_pattern.slideSteps, s, _pattern.inSlideRow(s - length));
			}
			_pattern.length = doubled;
			return true;
		}
		case Kind::SetLength:
			return assign(_pattern.length, std::clamp(_in.value, 1, MaxSteps));
		case Kind::SetScale:
			return _in.value >= 0 && _in.value < ScaleCount && assign(_pattern.scale, static_cast<uint8_t>(_in.value));
		case Kind::SetSwing:
			return assign(_pattern.swing, static_cast<uint8_t>(std::clamp(_in.value, MinSwing, MaxSwing)));
		case Kind::SetSwingStep:
			return _in.step >= 0 && _in.step < MaxSteps && setBit(_pattern.swingSteps, _in.step, _in.value != 0);
		case Kind::SetAccent:
			return assign(_pattern.accent, static_cast<uint8_t>(std::clamp(_in.value, 0, MaxAccent)));
		case Kind::SetAccentStep:
			return _in.step >= 0 && _in.step < MaxSteps && setBit(_pattern.accentSteps, _in.step, _in.value != 0);
		case Kind::SetSlideStep:
			return _in.step >= 0 && _in.step < MaxSteps && setBit(_pattern.slideSteps, _in.step, _in.value != 0);
		default:
			break;
		}

		if(_in.track < 0 || _in.track >= TrackCount)
			return false;
		if(_in.kind == Kind::SetMute)
			return setBit(_mutes, _in.track, _in.value != 0);

		if(_in.step < 0 || _in.step >= MaxSteps)
			return false;
		Step& step = _pattern.steps[_in.track][_in.step];
		if(_in.kind == Kind::SetHit)
		{
			if(!assign(step.hit, _in.value != 0))
				return false;
			// MD Drums drops a Lock without a Hit, and a Condition and ratchets shape a Hit: the step forgets them,
			// and keeps its velocity for the next Hit.
			if(!step.hit)
			{
				const uint8_t velocity = step.velocity;
				step = Step{};
				step.velocity = velocity;
			}
			return true;
		}
		if(_in.kind == Kind::SetVelocity)
			return assign(step.velocity, static_cast<uint8_t>(std::clamp(_in.value, 1, 127)));

		// What shapes a Hit: a step without one takes none.
		if(!step.hit)
			return false;
		switch(_in.kind)
		{
		case Kind::SetCondition:
			return _in.value >= 0 && _in.value < ConditionCount && assign(step.condition, static_cast<uint8_t>(_in.value));
		case Kind::SetRatchets:
			return assign(step.ratchets, static_cast<uint8_t>(std::clamp(_in.value, 1, MaxRatchets)));
		case Kind::SetCurve:
			return assign(step.curve, static_cast<int8_t>(std::clamp(_in.value, -MaxCurve, MaxCurve)));
		case Kind::SetMicro:
			return assign(step.micro, static_cast<int8_t>(std::clamp(_in.value, -MaxMicro, MaxMicro)));
		case Kind::SetLock:
			return _in.param >= 0 && _in.param < LockCount &&
				assign(step.locks[static_cast<size_t>(_in.param)], static_cast<uint8_t>(std::clamp(_in.value, 0, 127)));
		default:
			return false;
		}
	}
}
