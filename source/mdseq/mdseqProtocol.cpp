#include "mdseqProtocol.h"

#include "mdseqCondition.h"
#include "mdseqEdit.h"
#include "mdseqEditor.h"
#include "mdseqPrint.h"
#include "mdseqPush.h"
#include "mdseqText.h"
#include "mdseqView.h"

namespace mdseq
{
	namespace
	{
		using Kind = Input::Kind;

		// A click in the strip or the grid toggles and paints Hits; the Mute, the Fill and the choice of the Pattern
		// played are forwarded as they are. A Mute's prepend fixes its Track, a Lock's its parameter, a Restore's its
		// Pattern; a Mute needs its state after its Track, a Lock its value after its parameter, a Restore its Pattern
		// (its words may be none).
		constexpr std::array<MessageInfo, MessageCount> Messages = {{
			{Message::StripClick, "stripclick", kindBit(Kind::SetHit), 0, 0},
			{Message::StripMouse, "stripmouse", 0, 0, 0},
			{Message::GridClick, "gridclick", kindBit(Kind::SetHit) | kindBit(Kind::SetSwingStep) |
				kindBit(Kind::SetAccentStep) | kindBit(Kind::SetSlideStep), 0, 0},
			{Message::GridMouse, "gridmouse", 0, 0, 0},
			{Message::Track, "track", 0, 0, 0},
			{Message::Page, "page", 0, 0, 0},
			{Message::Half, "half", 0, 0, 0},
			{Message::Follow, "follow", 0, 0, 0},
			{Message::Mute, "mute", kindBit(Kind::SetMute), 1, 2},
			{Message::Velocity, "velocity", kindBit(Kind::SetVelocity), 0, 0},
			{Message::Hit, "hit", kindBit(Kind::SetHit), 0, 0},
			{Message::Length, "length", kindBit(Kind::SetLength), 0, 0},
			{Message::Scale, "scale", kindBit(Kind::SetScale), 0, 0},
			{Message::Swing, "swing", kindBit(Kind::SetSwing), 0, 0},
			{Message::Accent, "accent", kindBit(Kind::SetAccent), 0, 0},
			{Message::TrigRow, "trigrow", 0, 0, 0},
			{Message::Lock, "lock", kindBit(Kind::SetLock), 1, 2},
			{Message::Condition, "condition", kindBit(Kind::SetCondition), 0, 0},
			{Message::Ratchets, "ratchets", kindBit(Kind::SetRatchets), 0, 0},
			{Message::Curve, "curve", kindBit(Kind::SetCurve), 0, 0},
			{Message::Micro, "micro", kindBit(Kind::SetMicro), 0, 0},
			{Message::Fill, "fill", kindBit(Kind::SetFill), 0, 0},
			{Message::Pattern, "pattern", kindBit(Kind::SetPattern), 0, 0},
			{Message::CopyTo, "copyto", kindBit(Kind::CopyPattern), 0, 0},
			{Message::Double, "double", kindBit(Kind::DoubleLength), 0, 0},
			{Message::FillEdit, "filledit", 0, 0, 0},
			{Message::Clear, "clear", kindBit(Kind::ClearPattern), 0, 0},
			{Message::Chain, "chain", kindBit(Kind::SetPattern) | kindBit(Kind::ChainEntry) | kindBit(Kind::SetChain),
				0, 0},
			{Message::ChainAdd, "chainadd", 0, 0, 0},
			{Message::RestoreChain, "restorechain", kindBit(Kind::ChainEntry) | kindBit(Kind::SetChain), 0, 0},
			{Message::Redraw, "redraw", 0, 0, 0},
			{Message::Window, "window", 0, 0, 0},
			{Message::Restore, "restore", 0, 1, 1},
			{Message::PrintSource, "printsource", 0, 0, 0},
			{Message::Print, "print", 0, 0, 0},
			{Message::LomId, "lomid", 0, 0, 0},
			{Message::LomReply, "lomreply", 0, 0, 0},
			{Message::Updates, "updates", 0, 0, 0},
			{Message::Push, "push", 0, 0, 0},
			{Message::PushValue, "pushvalue", 0, 1, 2},
			{Message::DeviceOn, "deviceon", 0, 0, 0},
			{Message::SelectedTrack, "selectedtrack", 0, 0, 0},
			{Message::Capture, "capture", 0, 0, 0},
			{Message::Output, "output", 0, 0, 0},
		}};

		// Every message needs at least the integers its prepend fixes, and a Mute its Track and state, the last two
		// integers toEditor reads.
		constexpr bool needsWhatItFixes()
		{
			for(const MessageInfo& m : Messages)
				if(m.needs < m.fixed)
					return false;
			return Messages[static_cast<size_t>(Message::Mute)].needs >= 2;
		}
		static_assert(needsWhatItFixes(), "a message needs at least the integers its prepend fixes");

		constexpr std::array<PanelKeyInfo, PanelKeyCount> PanelKeys = {{
			{PanelKey::TrackTitle, "tracktitle", 0, true},
			{PanelKey::StepTitle, "steptitle", 0, true},
			{PanelKey::Velocity, "velocity", 0, true},
			{PanelKey::Hit, "hit", 0, true},
			{PanelKey::Mute, "mute", TrackCount, true},
			{PanelKey::LengthIndex, "lengthindex", 0, true},
			{PanelKey::LengthText, "lengthtext", 0, true},
			{PanelKey::Half, "half", 0, true},
			{PanelKey::Status, "status", 0, true},
			{PanelKey::Lock, "lock", LockCount, true},
			{PanelKey::Condition, "condition", 0, true},
			{PanelKey::RatchetsIndex, "ratchetsindex", 0, true},
			{PanelKey::Curve, "curve", 0, true},
			{PanelKey::CurveText, "curvetext", 0, true},
			{PanelKey::Micro, "micro", 0, true},
			{PanelKey::MicroText, "microtext", 0, true},
			{PanelKey::Store, "store", 0, false},
			{PanelKey::ScaleIndex, "scaleindex", 0, true},
			{PanelKey::PatternIndex, "patternindex", 0, true},
			{PanelKey::Lom, "lom", 0, false},
			{PanelKey::PrintStatus, "printstatus", 0, false},
			{PanelKey::Swing, "swing", 0, true},
			{PanelKey::Accent, "accent", 0, true},
			{PanelKey::TrigRow, "trigrow", 0, true},
			{PanelKey::ColumnNumber, "colnum", ColumnNumberCount, true},
			{PanelKey::TrackNames, "tracknames", 0, true},
			{PanelKey::TrackIndex, "trackindex", 0, true},
			{PanelKey::TrackName, "trackname", TrackCount, true},
			{PanelKey::LockName, "lockname", SynCount, true},
			{PanelKey::LockValue, "lockvalue", LockCount, true},
			{PanelKey::CopyTo, "copyto", 0, true},
			{PanelKey::FillEdit, "filledit", 0, true},
			{PanelKey::PatternState, "patternstate", PatternCount, true},
			{PanelKey::ChainMode, "chainmode", 0, true},
			{PanelKey::ChainText, "chaintext", 0, true},
			{PanelKey::ChainStore, "chainstore", 0, true},
			{PanelKey::Update, "update", 0, false},
			{PanelKey::Updates, "updates", 0, false},
			{PanelKey::PushObserve, "pushobs", push::ObserverCount, false},
			{PanelKey::Take, "take", 0, false},
			{PanelKey::TakeReady, "takeready", 0, false},
			{PanelKey::Pages, "pages", 0, true},
			{PanelKey::UpdateShown, "updateshown", 0, false},
			{PanelKey::PrintSource, "printsource", 0, false},
		}};

		// Each table is indexed by its enum.
		constexpr bool inOrder()
		{
			for(int i = 0; i < MessageCount; ++i)
				if(static_cast<int>(Messages[static_cast<size_t>(i)].message) != i)
					return false;
			for(int i = 0; i < PanelKeyCount; ++i)
				if(static_cast<int>(PanelKeys[static_cast<size_t>(i)].panelKey) != i)
					return false;
			return true;
		}
		static_assert(inOrder(), "the messages and the panel keys are listed in their enums' order");

		// A JSON string. The vocabulary's texts hold no control character; quotes and backslashes are escaped anyway.
		std::string quoted(const std::string_view _text)
		{
			std::string s = "\"";
			for(const char c : _text)
			{
				if(c == '"' || c == '\\')
					s += '\\';
				s += c;
			}
			return s + "\"";
		}

		// A JSON array of strings on one line.
		template<typename Label>
		std::string labels(const int _count, Label _label)
		{
			std::string s = "[";
			for(int i = 0; i < _count; ++i)
				s += (i ? ", " : "") + quoted(_label(i));
			return s + "]";
		}
	}

	const std::array<MessageInfo, MessageCount>& messages()
	{
		return Messages;
	}

	const MessageInfo& messageInfo(const Message _message)
	{
		return Messages[static_cast<size_t>(_message)];
	}

	const MessageInfo* findMessage(const std::string_view _selector)
	{
		for(const MessageInfo& m : Messages)
			if(_selector == m.selector)
				return &m;
		return nullptr;
	}

	bool editsPattern(const MessageInfo& _message)
	{
		for(int k = 0; k < Input::KindCount; ++k)
			if(_message.forwards & kindBit(static_cast<Input::Kind>(k)) && isPatternEdit(static_cast<Input::Kind>(k)))
				return true;
		return false;
	}

	const EditorOutput* toEditor(Editor& _editor, const Message _message, const std::vector<int>& _ints)
	{
		// Without the integers its prepend fixes and a value after them, the message names no Track or parameter of its
		// own: refused rather than read as 0, which would mute or lock another.
		if(_ints.size() < static_cast<size_t>(messageInfo(_message).needs))
			return nullptr;
		const auto at = [&](const size_t _i) { return _i < _ints.size() ? _ints[_i] : 0; };
		const int a0 = at(0);
		const int a1 = at(1);
		switch(_message)
		{
		case Message::StripClick:	return &_editor.stripClick(a0, a1);
		case Message::StripMouse:	return &_editor.stripMouse(a0 != 0);
		case Message::GridClick:	return &_editor.gridClick(a0, a1);
		case Message::GridMouse:	return &_editor.gridMouse(a0 != 0);
		case Message::Track:		return &_editor.showTrack(a0);
		case Message::Page:			return &_editor.showPage(a0);
		case Message::Half:			return &_editor.showHalf(a0);
		case Message::Follow:		return &_editor.setFollow(a0 != 0);
		case Message::Mute:
			// The strip's and the window's buttons send their state behind the Track their prepend gives.
			return &_editor.setMute(_ints[_ints.size() - 2], _ints.back() != 0);
		case Message::Velocity:		return &_editor.setVelocity(a0);
		case Message::Hit:			return &_editor.setHit(a0 != 0);
		case Message::Length:		return &_editor.setLength(a0 + 1);
		case Message::Scale:		return &_editor.setScale(a0);
		case Message::Swing:		return &_editor.setSwing(a0 + MinSwing);
		case Message::Accent:		return &_editor.setAccent(a0);
		case Message::TrigRow:		return &_editor.showTrigRow(a0);
		case Message::Lock:			return &_editor.setLock(a0, a1);
		case Message::Condition:	return &_editor.setCondition(a0);
		case Message::Ratchets:		return &_editor.setRatchets(a0 + 1);
		case Message::Curve:		return &_editor.setCurve(a0);
		case Message::Micro:		return &_editor.setMicro(a0);
		case Message::Fill:			return &_editor.setFill(a0 != 0);
		case Message::Pattern:		return &_editor.setPattern(a0);
		case Message::CopyTo:		return &_editor.copyPattern(a0 - 1);
		case Message::Double:		return &_editor.doubleLength();
		case Message::FillEdit:		return &_editor.setFillEdit(a0 != 0);
		case Message::Clear:		return &_editor.clearPattern();
		case Message::Chain:		return &_editor.setChainMode(a0 != 0);
		case Message::ChainAdd:		return &_editor.chainAdd(a0);
		case Message::RestoreChain:	return &_editor.restoreChain(_ints);
		case Message::Redraw:		return &_editor.redrawAll();
		case Message::Window:		return &_editor.setWindow(a0 != 0);
		case Message::Restore:
		case Message::PrintSource:
		case Message::Print:
		case Message::LomId:
		case Message::LomReply:
		case Message::Updates:
		case Message::Push:
		case Message::PushValue:
		case Message::DeviceOn:
		case Message::SelectedTrack:
		case Message::Capture:
		case Message::Output:
			return nullptr;
		}
		return nullptr;
	}

	const std::array<PanelKeyInfo, PanelKeyCount>& panelKeys()
	{
		return PanelKeys;
	}

	const char* panelKey(const PanelKey _key)
	{
		return PanelKeys[static_cast<size_t>(_key)].key;
	}

	const PanelKeyInfo* findPanelKey(const std::string_view _key)
	{
		for(const PanelKeyInfo& k : PanelKeys)
			if(_key == k.key)
				return &k;
		return nullptr;
	}

	std::string vocabularyJson()
	{
		std::string j = "{\n\t\"messages\": [\n";
		for(int i = 0; i < MessageCount; ++i)
		{
			const MessageInfo& m = Messages[static_cast<size_t>(i)];
			j += "\t\t{\"selector\": " + quoted(m.selector) + ", \"edits\": " + (editsPattern(m) ? "true" : "false") +
				", \"fixed\": " + std::to_string(m.fixed) + ", \"needs\": " + std::to_string(m.needs) + "}" +
				(i + 1 < MessageCount ? ",\n" : "\n");
		}
		j += "\t],\n\t\"panel\": [\n";
		for(int i = 0; i < PanelKeyCount; ++i)
		{
			const PanelKeyInfo& k = PanelKeys[static_cast<size_t>(i)];
			j += "\t\t{\"key\": " + quoted(k.key) + ", \"indexed\": " + std::to_string(k.indexed) + "}" +
				(i + 1 < PanelKeyCount ? ",\n" : "\n");
		}
		j += "\t],\n";
		j += "\t\"scales\": " + labels(ScaleCount, [](const int _i) { return ScaleLabels[_i]; }) + ",\n";
		j += "\t\"conditions\": " + labels(ConditionCount, [](const int _i) { return conditionLabel(_i); }) + ",\n";
		j += "\t\"noLock\": " + std::to_string(NoLock) + ",\n";
		j += "\t\"limits\": {\"tracks\": " + std::to_string(TrackCount) + ", \"patterns\": " +
			std::to_string(PatternCount) + ", \"steps\": " + std::to_string(MaxSteps) + ", \"locks\": " +
			std::to_string(LockCount) + ", \"ratchets\": " + std::to_string(MaxRatchets) + ", \"curve\": " +
			std::to_string(MaxCurve) + ", \"micro\": " + std::to_string(MaxMicro) + ", \"passes\": " +
			std::to_string(MaxPasses) + ", \"minSwing\": " + std::to_string(MinSwing) + ", \"maxSwing\": " +
			std::to_string(MaxSwing) + ", \"maxAccent\": " + std::to_string(MaxAccent) + "},\n";
		// The two lcd objects' geometry, from which the patch places what stands beside their cells
		const auto field = [](const char* _name, const int _value)
		{
			return std::string("\"") + _name + "\": " + std::to_string(_value);
		};
		j += "\t\"lcd\": {\"strip\": {" + field("width", strip::Width) + ", " + field("height", strip::Height) + ", " +
			field("cellWidth", strip::CellWidth) + ", " + field("pitch", strip::Pitch) + "}, \"grid\": {" +
			field("width", grid::Width) + ", " + field("height", grid::Height) + ", " +
			field("cellWidth", grid::CellWidth) + ", " + field("cellHeight", grid::CellHeight) + ", " +
			field("pitch", grid::Pitch) + ", " + field("pageGap", grid::PageGap) + ", " +
			field("rowPitch", grid::RowPitch) + "}},\n";
		// The texts the Editor fits, which the patch's boxes must hold, in the same face and size
		const auto box = [](const char* _name, const TextBox& _box)
		{
			return std::string("\"") + _name + "\": {\"bold\": " + (_box.face == Face::Bold ? "true" : "false") +
				", \"size\": " + std::to_string(_box.size) + ", \"width\": " + std::to_string(_box.width) + "}";
		};
		j += "\t\"fit\": {" + box("trackTab", fit::TrackTab) + ", " + box("trackName", fit::TrackName) + ", " +
			box("trackTitle", fit::TrackTitle) + ", " + box("lockName", fit::LockName) + "}\n";
		return j + "}\n";
	}
}
