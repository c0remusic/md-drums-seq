#pragma once

// The vocabulary between the device's patch and the external, defined once: the messages the patch sends the external,
// with the edit each makes the Editor forward; the panel keys the external sends the patch; the Scale menu's labels,
// in ScaleTicks' order. The external registers and dispatches its methods from this table. tools/mdseqVocabulary
// writes it as JSON with the Conditions' labels and the other numbers the patch's menus count, tools/make_device.py
// builds the patch from that JSON, and tools/check_device.py checks the device against it, so that a selector or a key
// the two sides do not share, a prepend without the integers it fixes, or a message box without those the external
// needs, fails the gate rather than showing in Live.

#include "mdseqTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mdseq
{
	class Editor;
	struct EditorOutput;

	// What a message from the patch asks: Tracks, steps, parameters and Patterns from 0, as the menus index them.
	enum class Message : uint8_t
	{
		// The Editor's, from the two lcd objects and the controls of the strip and the edit window
		StripClick,		// stripclick x y: a position in the strip's lcd, which counts while its button is down
		StripMouse,		// stripmouse 0|1: the strip's lcd button, up or down
		GridClick,		// gridclick x y: the same in the grid's lcd
		GridMouse,		// gridmouse 0|1
		Track,			// track t: the Track the strip shows
		Page,			// page p: the strip's page of 16 steps
		Half,			// half h: the window's half of 32 steps
		Follow,			// follow 0|1: the window's half follows the step played
		Mute,			// mute ... t 0|1: a Track's Mute, the Track and its state the last two integers
		Velocity,		// velocity v: the step selected's, 1-127
		Hit,			// hit 0|1
		Length,			// length i: the Length menu's index, one step at 0
		Scale,			// scale i: 0-6, 1/8x to 2x
		Swing,			// swing i: the Swing menu's index, 50 % at 0
		Accent,			// accent a: the Accent's amount, 0-127
		TrigRow,		// trigrow i: the trig row under the grid, 0 swing, 1 Accent, 2 Slide
		Lock,			// lock param value: 0-127, NoLock (63) for the Kit
		Condition,		// condition i: the index in the Conditions' menu
		Ratchets,		// ratchets i: the Ratchets menu's index, one Hit at 0
		Curve,			// curve c: -100 to +100
		Micro,			// micro m: -23 to +23 24ths of a step
		Fill,			// fill 0|1: held or let go
		Pattern,		// pattern p: 0-15, the Pattern to play
		CopyTo,			// copyto i: the Copy to menu's index, Pattern i; 0 is the menu's label
		Double,			// double: the Pattern shown twice as long, its steps again after them
		FillEdit,		// filledit 0|1: the Hits the mouse puts take the Condition FILL
		Clear,			// clear: the Pattern shown back to an empty one
		Chain,			// chain 0|1: Chain mode, the Patterns clicked appended to a new chain
		ChainAdd,		// chainadd p: Pattern p appended to the chain built
		RestoreChain,	// restorechain playing length patterns...: the chain as its pattr Blob keeps it in the set
		Redraw,			// redraw: everything drawn again, once the device has loaded
		Window,			// window 0|1: the edit window closed or shown, the grid drawn only while it shows
		// The external's own, from Restore on (ownedByExternal): no Editor takes them
		Restore,		// restore p words...: Pattern p as the set kept it, from its pattr Blob
		PrintSource,	// printsource i: the Print menu's index, Pattern ×(i + 1) below MaxPasses, the last take at
						// MaxPasses
		Print,			// print: the Pattern shown, or the last take, into Session clips, as the Print menu says
		LomId,			// lomid id n: live.path's answer to "lom path ..."
		LomReply,		// lomreply ...: live.object's answer to "lom obj ..."
		Updates,		// updates 0|1: whether the device looks for a newer release (mdseqUpdates.h)
		Push,			// push 0|1: the strip's switch, Push 2 taken while the device and its track have the focus
		PushValue,		// pushvalue i values...: what live.observer i reports of Push (mdseqPush.h's observers)
		DeviceOn,		// deviceon 0|1: the device switched on or off in Live
		SelectedTrack,	// selectedtrack id n: the track Live has selected
		Capture,		// capture: the last take into Session clips (issue 23)
		Output			// output 0|1: the device plays MD Drums (0), or leaves it to the clips (1)
	};
	constexpr int MessageCount = static_cast<int>(Message::Output) + 1;

	constexpr bool ownedByExternal(const Message _message) { return _message >= Message::Restore; }

	// An edit's kind as a bit, for the kinds a message forwards.
	constexpr uint32_t kindBit(const Input::Kind _kind) { return 1u << static_cast<int>(_kind); }

	struct MessageInfo
	{
		Message message;
		const char* selector;
		// The kinds of every edit the message can make the Editor forward, a kindBit each (a click in the grid toggles
		// a Hit, or a step of the swing row); 0 for a message that forwards none.
		uint32_t forwards;
		// The integers the patch's prepend fixes after the selector, before the control's value: a Mute's Track, a
		// Lock's parameter, a Restore's Pattern; 0 for the others.
		int fixed;
		// The fewest integers the external takes the message with: those fixed and, for the Editor's, the control's
		// value after them (a Mute's state, a Lock's value); 0 for a message whose missing integers read 0. A
		// message box carries them all; a prepend, the fixed ones only.
		int needs;
	};

	// Every message, in Message's order.
	const std::array<MessageInfo, MessageCount>& messages();
	const MessageInfo& messageInfo(Message _message);
	// The message a selector names, null for one the external does not register.
	const MessageInfo* findMessage(std::string_view _selector);
	// Whether the message edits a Pattern, as isPatternEdit says of what it forwards: the patch must not send it a
	// control's initial value when the device loads.
	bool editsPattern(const MessageInfo& _message);

	// What the Editor answers a message of its own, the message's integers read as the patch sends them (a missing one
	// reads 0); null for a message the external handles itself, and for one with fewer integers than it needs (a Mute
	// without its Track and state, a Lock without its parameter and value), which a patch that lost them sends.
	const EditorOutput* toEditor(Editor& _editor, Message _message, const std::vector<int>& _ints);

	// What the external sends the patch out of its panel outlet, in the order the patch's route lists them: the key the
	// route sends on, then integers or a text.
	enum class PanelKey : uint8_t
	{
		TrackTitle,		// tracktitle text: the step selected's Track
		StepTitle,		// steptitle text: the step selected and its page
		Velocity,		// velocity v
		Hit,			// hit 0|1
		Mute,			// mute t 0|1
		LengthIndex,	// lengthindex i: the Length menu's index, one step at 0
		LengthText,		// lengthtext text: the length and a step's note
		Half,			// half h
		Status,			// status text: the strip's status line
		Lock,			// lock param position active: the dial on the Lock, else on the Kit's value (NoLock while
						// unknown), lit while locked
		Condition,		// condition i
		RatchetsIndex,	// ratchetsindex i: the Ratchets menu's index, one Hit at 0
		Curve,			// curve c
		CurveText,		// curvetext text
		Micro,			// micro m
		MicroText,		// microtext text
		Store,			// store p words...: Pattern p for its pattr Blob (the external's)
		ScaleIndex,		// scaleindex i
		PatternIndex,	// patternindex p: the Pattern chosen
		Lom,			// lom path|id|obj ...: a message to live.path or live.object (the external's)
		PrintStatus,	// printstatus text: what a print did (the external's)
		Swing,			// swing i: the Swing menu's index, 50 % at 0
		Accent,			// accent a: the Accent's amount, 0-127
		TrigRow,		// trigrow i: the trig row under the grid, 0 swing, 1 Accent, 2 Slide
		ColumnNumber,	// colnum i n: the number n over the i-th of the labelled columns, every fourth of the grid's
						// half
		// The Kit as MD Drums tells it (mdseqKit.h):
		TrackNames,		// tracknames name...: the strip's 16 Track tabs, "01 TRX-BD", or "01" while the Machine is unknown
		TrackIndex,		// trackindex t: the strip's Track tab chosen, again after its names
		TrackName,		// trackname t name: the Machine of the window's row t, empty while unknown
		LockName,		// lockname p name: SYN p+1's name for the step selected's Machine, "SYN1" while unknown,
						// "unused" for one the Machine does not use
		LockValue,		// lockvalue p text: what dial p holds, "40" locked, else "Kit 40", "Kit" while the value is
						// unknown, empty for a SYN the Machine does not use
		CopyTo,			// copyto 0: the Copy to menu back on its label
		FillEdit,		// filledit 0|1
		PatternState,	// patternstate p state: Pattern p's button, a PatternState
		ChainMode,		// chainmode 0|1: whether the Patterns clicked build a chain
		ChainText,		// chaintext text: the chain built or playing, empty without one
		ChainStore,		// chainstore playing length patterns...: the chain, for its pattr Blob in the set
		Update,			// update text: a newer release, empty for none (the external's)
		Updates,		// updates 0|1: whether the device looks for one (the external's)
		PushObserve,	// pushobs i id n: live.observer i watches object n, 0 for none (the external's)
		Take,			// take text: the last take's length ("Take 12 bars"), "No take" before any (the external's)
		TakeReady,		// takeready 0|1: whether a take can be captured (the external's)
		Pages,			// pages p n: the strip's page shown, and its pages within the Pattern's length (the
						// others off)
		UpdateShown,	// updateshown 0|1: whether the strip shows a newer release's screen (the external's)
		PrintSource		// printsource i: the Print menu's index, as Message::PrintSource takes it (the external's)
	};
	constexpr int PanelKeyCount = static_cast<int>(PanelKey::PrintSource) + 1;

	// How a Pattern's button looks: faint while the Pattern is empty, bright while it holds a Hit, lit while shown
	// (played), outlined in the accent while queued, its own look on every other blink.
	enum class PatternState : int
	{
		Empty,
		Used,
		Shown,
		Queued
	};

	// The grid's half of 32 steps is numbered every fourth column.
	constexpr int ColumnNumberCount = 8;

	struct PanelKeyInfo
	{
		PanelKey panelKey;
		const char* key;
		// The receives the route sends the key's values on by their first integer, which it takes off ("mute 3 1" to
		// Track 4's Mute): TrackCount for a Mute, LockCount for a Lock, 0 for a key sent to one receive whole.
		int indexed;
		bool fromEditor;	// the Editor words it; else the external sends it itself
	};

	// Every panel key, in PanelKey's order.
	const std::array<PanelKeyInfo, PanelKeyCount>& panelKeys();
	const char* panelKey(PanelKey _key);
	// The panel key a name is, null for none.
	const PanelKeyInfo* findPanelKey(std::string_view _key);

	// The Scale menu, by the scale's index.
	constexpr const char* ScaleLabels[ScaleCount] = {"1/8x", "1/4x", "1/2x", "3/4x", "1/16", "3/2x", "2x"};

	// The whole vocabulary as JSON, for the tools that build and check the device: the messages with whether each edits
	// a Pattern, the panel keys with how they are routed, the Scale and Condition menus, NoLock and the counts the
	// patch's menus and rows follow.
	std::string vocabularyJson();
}
