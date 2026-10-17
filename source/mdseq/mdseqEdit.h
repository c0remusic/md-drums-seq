#pragma once

// An edit applied to a Pattern and the Mutes: the Sequencer plays from what it gives on the scheduler thread, and the
// Editor draws from what it gives on the main thread, so that both stay the same Pattern.

#include "mdseqTypes.h"

#include <array>

namespace mdseq
{
	// Returns false when nothing changed, an index out of range included. Velocity is clamped to 1-127, the length to
	// 1-64, a Lock to 0-127 (NoLock takes it off), ratchets to 1-8, their curve to -100 to +100, micro-timing to -23 to
	// +23, the swing to 50-80. A Lock, a Condition, ratchets and micro-timing need a Hit, and taking a Hit off takes them
	// off; the swing row belongs to the Pattern, not to a Hit. A copy from another Pattern is left to whoever holds the
	// bank, the Editor and the Sequencer: false here.
	bool applyEdit(Pattern& _pattern, uint16_t& _mutes, const Input& _in);

	// Whether an edit of this kind changes a Pattern. A Mute and the choice of the Pattern played are Live parameters
	// of their own, and the Fill is played: none of them is in a Pattern. What is, the set keeps (PatternStore), and no
	// control of the patch may send it an initial value when the device loads (tools/check_device.py).
	constexpr bool isPatternEdit(const Input::Kind _kind)
	{
		return _kind != Input::Kind::SetMute && _kind != Input::Kind::SetFill && _kind != Input::Kind::SetPattern;
	}

	// An Input as the integers the external hands from the main thread to the scheduler's, every field of it.
	constexpr int InputWords = 6;
	std::array<int, InputWords> pack(const Input& _in);
	Input unpack(const std::array<int, InputWords>& _words);
}
