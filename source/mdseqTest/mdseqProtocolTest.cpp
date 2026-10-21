// The vocabulary between the patch and the external: each selector and each panel key once, every message's edit flag
// isPatternEdit's rule on the kind it forwards, every message driving the Editor to forward only that kind (or
// nothing) through the very call it names, with its integers in their order, the Editor sending only the keys the
// table gives it and all of them, and the JSON the device is built from holding all of it, the Conditions in
// conditionLabel's order and NoLock.

#include "mdseqCondition.h"
#include "mdseqEdit.h"
#include "mdseqEditor.h"
#include "mdseqProtocol.h"
#include "mdseqStore.h"
#include "mdseqTestSupport.h"

#include <cstdio>
#include <cstring>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace
{
	using namespace mdseq;

	constexpr Input::Kind Kinds[] = {Input::Kind::SetHit, Input::Kind::SetVelocity, Input::Kind::SetMute,
		Input::Kind::SetLength, Input::Kind::SetLock, Input::Kind::SetCondition, Input::Kind::SetFill,
		Input::Kind::SetRatchets, Input::Kind::SetCurve, Input::Kind::SetMicro, Input::Kind::SetScale,
		Input::Kind::SetPattern, Input::Kind::SetSwing, Input::Kind::SetSwingStep, Input::Kind::CopyPattern,
		Input::Kind::DoubleLength, Input::Kind::ClearPattern, Input::Kind::SetAccent, Input::Kind::SetAccentStep,
		Input::Kind::SetSlideStep, Input::Kind::ChainEntry, Input::Kind::SetChain};
	static_assert(sizeof(Kinds) / sizeof(Kinds[0]) == Input::KindCount, "every kind of edit");

	// A selector or a key Max reads as one symbol, which a prepend and a route split on spaces.
	bool oneWord(const char* _name)
	{
		if(!*_name)
			return false;
		for(const char* c = _name; *c; ++c)
			if(!((*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9')))
				return false;
		return true;
	}

	void eachOnce()
	{
		std::set<std::string> selectors;
		for(int i = 0; i < MessageCount; ++i)
		{
			const MessageInfo& m = messages()[static_cast<size_t>(i)];
			CHECK(static_cast<int>(m.message) == i);
			CHECK(&messageInfo(m.message) == &m);
			CHECK(oneWord(m.selector));
			CHECK(selectors.insert(m.selector).second);
			CHECK(findMessage(m.selector) == &m);
		}
		CHECK(selectors.size() == static_cast<size_t>(MessageCount));
		CHECK(findMessage("") == nullptr);
		CHECK(findMessage("set") == nullptr);
		CHECK(findMessage("hits") == nullptr);

		std::set<std::string> keys;
		for(int i = 0; i < PanelKeyCount; ++i)
		{
			const PanelKeyInfo& k = panelKeys()[static_cast<size_t>(i)];
			CHECK(static_cast<int>(k.panelKey) == i);
			CHECK(std::strcmp(panelKey(k.panelKey), k.key) == 0);
			CHECK(oneWord(k.key));
			CHECK(keys.insert(k.key).second);
			CHECK(findPanelKey(k.key) == &k);
		}
		CHECK(findPanelKey("colnum8") == nullptr);
		CHECK(findPanelKey("colnum") && findPanelKey("colnum")->indexed == ColumnNumberCount);
	}

	// A message edits a Pattern when the kind it forwards does, as isPatternEdit and the PatternStore say; every kind
	// of edit comes from at least one message.
	void editFlags()
	{
		for(const Input::Kind kind : Kinds)
		{
			PatternStore store;
			CHECK(store.edited({kind, 0, 0, 1}) == isPatternEdit(kind));
			int from = 0;
			for(const MessageInfo& m : messages())
			{
				if(!(m.forwards & kindBit(kind)))
					continue;
				++from;
				CHECK(editsPattern(m) == isPatternEdit(kind) || m.forwards != kindBit(kind));
			}
			CHECK(from > 0);
		}
		for(const MessageInfo& m : messages())
		{
			if(!m.forwards)
				CHECK(!editsPattern(m));
			// A message forwarding several kinds edits a Pattern when any of them does: the grid's clicks, Hits and
			// swing steps, both.
			bool any = false;
			for(const Input::Kind kind : Kinds)
				any = any || (m.forwards & kindBit(kind) && isPatternEdit(kind));
			CHECK(editsPattern(m) == any);
		}
		CHECK(messageInfo(Message::GridClick).forwards ==
			(kindBit(Input::Kind::SetHit) | kindBit(Input::Kind::SetSwingStep) | kindBit(Input::Kind::SetAccentStep) |
				kindBit(Input::Kind::SetSlideStep)));

		// The messages that must not take a control's initial value when the device loads: the old hand list of
		// check_device.py, and the clicks in the two lcd objects, which toggle Hits.
		std::set<std::string> edits;
		for(const MessageInfo& m : messages())
			if(editsPattern(m))
				edits.insert(m.selector);
		CHECK(edits == std::set<std::string>({"stripclick", "gridclick", "hit", "velocity", "length", "lock",
			"condition", "ratchets", "curve", "micro", "scale", "swing", "accent", "copyto", "double",
			"clear"}));
		CHECK(!editsPattern(messageInfo(Message::Mute)));
		CHECK(!editsPattern(messageInfo(Message::Fill)));
		CHECK(!editsPattern(messageInfo(Message::Pattern)));
		CHECK(!editsPattern(messageInfo(Message::Restore)));
	}

	struct Sent
	{
		Message message;
		std::vector<int> ints;
	};

	// Each message as the patch sends it, to an Editor whose step selected, Track 1's first, holds a Hit: the Editor's
	// forward only the kind the table declares, at least one of it, and the external's own reach no Editor.
	void messagesForwardWhatTheyDeclare()
	{
		const int stripX = strip::cellX(1) + strip::CellWidth / 2;
		const int gridX = grid::cellX(3) + grid::CellWidth / 2;
		const int gridY = grid::cellY(2) + grid::CellHeight / 2;
		const int firstX = grid::cellX(0) + grid::CellWidth / 2;
		const int firstY = grid::cellY(0) + grid::CellHeight / 2;
		const std::vector<Sent> sent = {
			{Message::StripClick, {stripX, 10}}, {Message::StripMouse, {1}}, {Message::GridClick, {gridX, gridY}},
			{Message::GridMouse, {1}}, {Message::Track, {3}}, {Message::Page, {1}}, {Message::Half, {1}},
			{Message::Follow, {0}}, {Message::Mute, {5, 1}}, {Message::Velocity, {64}}, {Message::Hit, {0}},
			{Message::Length, {32}}, {Message::Scale, {2}}, {Message::Swing, {66}}, {Message::Accent, {40}},
			{Message::TrigRow, {1}},
			{Message::Lock, {5, 40}},
			{Message::Condition, {3}},
			{Message::Ratchets, {4}}, {Message::Curve, {50}}, {Message::Micro, {5}}, {Message::Fill, {1}},
			{Message::Pattern, {2}}, {Message::CopyTo, {5}}, {Message::Double, {}}, {Message::FillEdit, {1}}, {Message::Clear, {}}, {Message::Chain, {0}},
			{Message::ChainAdd, {3}}, {Message::RestoreChain, {1, 2, 3, 5}},
			{Message::Redraw, {}}, {Message::Window, {0}}, {Message::Restore, {0, 0}}, {Message::PrintSource, {4}},
			{Message::Print, {}}, {Message::LomId, {}}, {Message::LomReply, {}}, {Message::Updates, {1}},
			{Message::Push, {1}}, {Message::PushValue, {11, 1}}, {Message::DeviceOn, {1}}, {Message::SelectedTrack, {}},
			{Message::Capture, {}}, {Message::Output, {1}}};
		CHECK(sent.size() == static_cast<size_t>(MessageCount));

		for(const Sent& s : sent)
		{
			Editor e;
			toEditor(e, Message::GridMouse, {1});
			toEditor(e, Message::GridClick, {firstX, firstY});
			toEditor(e, Message::GridMouse, {0});
			CHECK(e.pattern().steps[0][0].hit);
			// A click counts while the lcd's button is down.
			if(s.message == Message::StripClick)
				toEditor(e, Message::StripMouse, {1});
			if(s.message == Message::GridClick)
				toEditor(e, Message::GridMouse, {1});
			// Chain mode left with a chain of two: it plays, its first Pattern chosen.
			if(s.message == Message::Chain)
			{
				toEditor(e, Message::Chain, {1});
				toEditor(e, Message::ChainAdd, {3});
				toEditor(e, Message::ChainAdd, {5});
			}

			const MessageInfo& info = messageInfo(s.message);
			const EditorOutput* out = toEditor(e, s.message, s.ints);
			CHECK((out == nullptr) == ownedByExternal(s.message));
			if(!out)
				continue;
			CHECK(info.forwards ? !out->forward.empty() : out->forward.empty());
			PatternStore store;
			for(const Input& in : out->forward)
			{
				CHECK((info.forwards & kindBit(in.kind)) != 0);
				CHECK(store.edited(in) == isPatternEdit(in.kind));
			}
		}

		// A Mute's Track and state are its last two integers; without both, the Editor is not asked.
		Editor e;
		const EditorOutput* out = toEditor(e, Message::Mute, {9, 3, 1});
		CHECK(out && out->forward.size() == 1 && out->forward[0].track == 3 && out->forward[0].value == 1);
		out = toEditor(e, Message::Mute, {9, 3, 0});
		CHECK(out && out->forward.size() == 1 && out->forward[0].track == 3 && out->forward[0].value == 0);
		CHECK(toEditor(e, Message::Mute, {1}) == nullptr);
		CHECK(toEditor(e, Message::Mute, {}) == nullptr);
		// A message of the Editor's with fewer integers than it needs is refused, rather than read as Track or
		// parameter 0, which the external says in the Max console; with them, it is taken. The Mute and the Lock need
		// their value after the integer their prepend fixes.
		int needing = 0;
		for(const MessageInfo& m : messages())
		{
			if(!m.needs || m.message >= Message::Restore)
				continue;
			++needing;
			CHECK(m.needs == m.fixed + 1);
			CHECK(toEditor(e, m.message, std::vector<int>(static_cast<size_t>(m.needs) - 1, 1)) == nullptr);
			CHECK(toEditor(e, m.message, std::vector<int>(static_cast<size_t>(m.needs), 1)) != nullptr);
		}
		CHECK(needing == 2);
		CHECK(toEditor(e, Message::Lock, {5}) == nullptr);
		// A missing integer reads 0: the Length menu's first index, one step.
		out = toEditor(e, Message::Length, {});
		CHECK(out && out->forward.size() == 1 && out->forward[0].value == 1);
	}

	bool sameInputs(const std::vector<Input>& _a, const std::vector<Input>& _b)
	{
		if(_a.size() != _b.size())
			return false;
		for(size_t i = 0; i < _a.size(); ++i)
		{
			const Input& a = _a[i];
			const Input& b = _b[i];
			if(a.kind != b.kind || a.track != b.track || a.step != b.step || a.value != b.value || a.param != b.param ||
				a.pattern != b.pattern)
				return false;
		}
		return true;
	}

	bool same(const EditorOutput& _a, const EditorOutput& _b)
	{
		return _a.strip == _b.strip && _a.grid == _b.grid && _a.panel == _b.panel && sameInputs(_a.forward, _b.forward);
	}

	// An Editor in a state where a mix-up shows: Hits at places no swap of coordinates, Track, page or half confuses,
	// the step selected Track 7's step 10 with a Hit, and both lcd buttons down for a new gesture.
	void setUp(Editor& _e)
	{
		const int hits[][2] = {{3, 1}, {0, 50}, {2, 3}, {5, 20}, {1, 4}};
		for(const auto& h : hits)
			_e.apply({Input::Kind::SetHit, h[0], h[1], 1});
		_e.gridMouse(true);
		_e.gridClick(grid::cellX(9) + grid::CellWidth / 2, grid::cellY(6) + grid::CellHeight / 2);
		_e.gridMouse(true);
		_e.stripMouse(true);
	}

	// The call an Editor message names, made on the Editor given.
#define EDITOR_CALL(_call) [&](Editor& _e) -> const EditorOutput& { return _e._call; }

	// Each of the Editor's messages makes the call the table names, with its integers in their order: toEditor on one
	// Editor and the call itself on its twin give the same output, and leave the two in the same state, which the
	// same probes then show (the step played, the half, the selection, a redraw, a click in each lcd).
	void messagesMakeTheirCall()
	{
		const int stripX = strip::cellX(2) + strip::CellWidth / 2;
		const int gridX = grid::cellX(7) + grid::CellWidth / 2;
		const int gridY = grid::cellY(2) + grid::CellHeight / 2;
		struct Call
		{
			Message message;
			std::vector<int> ints;
			std::function<const EditorOutput&(Editor&)> direct;
		};
		// Each message of a state, 0 or 1, both ways: an integer misread, or not read, reads 0 or matches the state
		// setUp leaves, so that one way alone can pass a call that ignores it.
		const std::vector<Call> calls = {
			{Message::StripClick, {stripX, 10}, EDITOR_CALL(stripClick(stripX, 10))},
			{Message::GridClick, {gridX, gridY}, EDITOR_CALL(gridClick(gridX, gridY))},
			{Message::StripMouse, {0}, EDITOR_CALL(stripMouse(false))},
			{Message::StripMouse, {1}, EDITOR_CALL(stripMouse(true))},
			{Message::GridMouse, {0}, EDITOR_CALL(gridMouse(false))},
			{Message::GridMouse, {1}, EDITOR_CALL(gridMouse(true))},
			{Message::Track, {3}, EDITOR_CALL(showTrack(3))},
			{Message::Page, {3}, EDITOR_CALL(showPage(3))},
			{Message::Half, {1}, EDITOR_CALL(showHalf(1))},
			{Message::Follow, {0}, EDITOR_CALL(setFollow(false))},
			{Message::Follow, {1}, EDITOR_CALL(setFollow(true))},
			{Message::Mute, {3, 1}, EDITOR_CALL(setMute(3, true))},
			{Message::Mute, {0, 1}, EDITOR_CALL(setMute(0, true))},
			{Message::Mute, {3, 0}, EDITOR_CALL(setMute(3, false))},
			{Message::Velocity, {64}, EDITOR_CALL(setVelocity(64))},
			{Message::Hit, {0}, EDITOR_CALL(setHit(false))},
			{Message::Hit, {1}, EDITOR_CALL(setHit(true))},
			{Message::Length, {47}, EDITOR_CALL(setLength(48))},
			{Message::Scale, {2}, EDITOR_CALL(setScale(2))},
			{Message::Swing, {12}, EDITOR_CALL(setSwing(62))},
			{Message::Accent, {40}, EDITOR_CALL(setAccent(40))},
			{Message::TrigRow, {1}, EDITOR_CALL(showTrigRow(1))},
			{Message::Lock, {5, 40}, EDITOR_CALL(setLock(5, 40))},
			{Message::Condition, {3}, EDITOR_CALL(setCondition(3))},
			{Message::Ratchets, {3}, EDITOR_CALL(setRatchets(4))},
			{Message::Curve, {50}, EDITOR_CALL(setCurve(50))},
			{Message::Micro, {5}, EDITOR_CALL(setMicro(5))},
			{Message::Fill, {0}, EDITOR_CALL(setFill(false))},
			{Message::Fill, {1}, EDITOR_CALL(setFill(true))},
			{Message::Pattern, {2}, EDITOR_CALL(setPattern(2))},
			{Message::CopyTo, {5}, EDITOR_CALL(copyPattern(4))},	// the menu's label first, at 0
			{Message::Double, {}, EDITOR_CALL(doubleLength())},
			{Message::FillEdit, {0}, EDITOR_CALL(setFillEdit(false))},
			{Message::FillEdit, {1}, EDITOR_CALL(setFillEdit(true))},
			{Message::Clear, {}, EDITOR_CALL(clearPattern())},
			{Message::Chain, {1}, EDITOR_CALL(setChainMode(true))},
			{Message::Chain, {0}, EDITOR_CALL(setChainMode(false))},
			{Message::ChainAdd, {3}, EDITOR_CALL(chainAdd(3))},
			{Message::RestoreChain, {1, 2, 3, 5}, EDITOR_CALL(restoreChain({1, 2, 3, 5}))},
			{Message::Redraw, {}, EDITOR_CALL(redrawAll())},
			{Message::Window, {0}, EDITOR_CALL(setWindow(false))},
			{Message::Window, {1}, EDITOR_CALL(setWindow(true))}};

		std::set<Message> covered;
		for(const Call& c : calls)
		{
			covered.insert(c.message);
			Editor viaMessage;
			Editor direct;
			setUp(viaMessage);
			setUp(direct);
			const EditorOutput* out = toEditor(viaMessage, c.message, c.ints);
			const EditorOutput& expected = c.direct(direct);
			const bool sameCall = out && same(*out, expected);
			const bool sameState = viaMessage.half() == direct.half() &&
				viaMessage.selectedTrack() == direct.selectedTrack() &&
				viaMessage.selectedStep() == direct.selectedStep() && viaMessage.shown() == direct.shown() &&
				same(viaMessage.played(40), direct.played(40)) && same(viaMessage.redrawAll(), direct.redrawAll()) &&
				same(viaMessage.gridClick(grid::cellX(10) + 4, grid::cellY(4) + 4),
					direct.gridClick(grid::cellX(10) + 4, grid::cellY(4) + 4)) &&
				same(viaMessage.stripClick(strip::cellX(5) + 4, 12), direct.stripClick(strip::cellX(5) + 4, 12));
			if(!sameCall || !sameState)
				std::printf("  %s %s: the call differs from the one it names%s\n", messageInfo(c.message).selector,
					c.ints.empty() ? "" : std::to_string(c.ints[0]).c_str(), sameCall ? " (in what it leaves)" : "");
			CHECK(sameCall && sameState);
		}
		// Every message of the Editor's is in the table.
		for(const MessageInfo& m : messages())
			if(!ownedByExternal(m.message))
				CHECK(covered.count(m.message) == 1);
	}

	// What the Editor sends the patch: only keys the table gives the Editor, all of them over a redraw, a Mute and a
	// step played, an indexed key's first integer within its receives.
	void theEditorSpeaksItsKeys()
	{
		// Each call returns the Editor's one output: kept before the next.
		Editor e;
		std::vector<PanelValue> values;
		const auto keep = [&](const EditorOutput& _out)
		{
			values.insert(values.end(), _out.panel.begin(), _out.panel.end());
		};
		keep(e.redrawAll());
		keep(e.setMute(15, true));
		keep(e.played(5));
		keep(e.setChainMode(true));

		std::set<std::string> sent;
		for(const PanelValue& v : values)
		{
			const PanelKeyInfo* k = findPanelKey(v.key);
			CHECK(k != nullptr);
			if(!k)
				continue;
			CHECK(k->fromEditor);
			if(k->indexed)
				CHECK(!v.ints.empty() && v.ints[0] >= 0 && v.ints[0] < k->indexed);
			sent.insert(v.key);
		}
		for(const PanelKeyInfo& k : panelKeys())
			CHECK(sent.count(k.key) == (k.fromEditor ? 1u : 0u));
		CHECK(findPanelKey("mute")->indexed == TrackCount);
		CHECK(findPanelKey("lock")->indexed == LockCount);
	}

	bool holds(const std::string& _json, const std::string& _text)
	{
		return _json.find(_text) != std::string::npos;
	}

	void json()
	{
		const std::string j = vocabularyJson();
		CHECK(j.front() == '{' && j.substr(j.size() - 2) == "}\n");
		for(const MessageInfo& m : messages())
			CHECK(holds(j, std::string("{\"selector\": \"") + m.selector + "\", \"edits\": " +
				(editsPattern(m) ? "true" : "false") + ", \"fixed\": " + std::to_string(m.fixed) + ", \"needs\": " +
				std::to_string(m.needs) + "}"));
		// The prepends fix a Mute's Track, a Lock's parameter and a Restore's Pattern; no other message's. A Mute and
		// a Lock need their value too, which a message box carries: a button sending "mute 3" is refused.
		std::set<std::string> fixing;
		std::set<std::string> needing;
		for(const MessageInfo& m : messages())
		{
			if(m.fixed)
				fixing.insert(m.selector + std::string(" ") + std::to_string(m.fixed));
			if(m.needs)
				needing.insert(m.selector + std::string(" ") + std::to_string(m.needs));
		}
		CHECK(fixing == std::set<std::string>({"mute 1", "lock 1", "restore 1", "pushvalue 1"}));
		CHECK(needing == std::set<std::string>({"mute 2", "lock 2", "restore 1", "pushvalue 2"}));
		for(const PanelKeyInfo& k : panelKeys())
			CHECK(holds(j, std::string("{\"key\": \"") + k.key + "\", \"indexed\": " + std::to_string(k.indexed) +
				"}"));
		CHECK(holds(j, "\"scales\": [\"1/8x\", \"1/4x\", \"1/2x\", \"3/4x\", \"1/16\", \"3/2x\", \"2x\"]"));

		std::string conditions = "\"conditions\": [";
		for(int i = 0; i < ConditionCount; ++i)
			conditions += std::string(i ? ", " : "") + "\"" + conditionLabel(i) + "\"";
		CHECK(holds(j, conditions + "]"));
		CHECK(holds(j, "\"None\", \"1%\""));
		CHECK(holds(j, "\"!1ST\"]"));
		CHECK(holds(j, "\"noLock\": 63"));
		CHECK(holds(j, "\"limits\": {\"tracks\": 16, \"patterns\": 16, \"steps\": 64, \"locks\": 24, \"ratchets\": 8, "
			"\"curve\": 100, \"micro\": 23, \"passes\": 16, \"minSwing\": 50, \"maxSwing\": 80, "
			"\"maxAccent\": 127}"));
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	eachOnce();
	editFlags();
	messagesForwardWhatTheyDeclare();
	messagesMakeTheirCall();
	theEditorSpeaksItsKeys();
	json();
	return mdseqTest::finish("mdseqProtocolTest");
}
