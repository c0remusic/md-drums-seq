#include "mdseqEditor.h"

#include "mdseqCondition.h"
#include "mdseqEdit.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace mdseq
{
	namespace
	{
		constexpr int StepsPerPage = strip::Cells;
		constexpr int StepsPerHalf = grid::Columns;

		// The window numbers every fourth column of its half.
		static_assert(ColumnNumberCount == StepsPerHalf / 4, "a column number every fourth column");

		// The Condition a Hit takes in Fill edit, by its index in the menu.
		int fillCondition()
		{
			static const int index = conditionIndex("FILL");
			return index;
		}

		// A step's note at each scale, 1/8x to 2x.
		constexpr const char* ScaleNotes[ScaleCount] = {"1/2", "1/4", "1/8", "1/12", "1/16", "1/24", "1/32"};

		// The Track's Machine as MD Drums names it, empty while it does not say.
		std::string machineName(const std::optional<Kit>& _kit, const int _track)
		{
			return _kit ? _kit->tracks[static_cast<size_t>(_track)].machineName : std::string();
		}

		// What a strip tab of 36 px holds after the Track's number (measured in Live: "01 BD" fits, "01 TRX-BD" is cut):
		// the name's last part, "BD" for TRX-BD and P-I-BD, after its family's initial when it is a number, "R14" for
		// ROM-14; "-" for GND---, no Machine.
		std::string shortName(const std::string& _machine)
		{
			const size_t dash = _machine.find_last_of('-');
			if(dash == std::string::npos)
				return _machine;
			const std::string last = _machine.substr(dash + 1);
			if(last.empty())
				return "-";
			return std::isdigit(static_cast<unsigned char>(last[0])) ? _machine.substr(0, 1) + last : last;
		}

		// "04", or "04 CP" once the Machine is known.
		std::string trackLabel(const std::optional<Kit>& _kit, const int _track)
		{
			char number[8];
			std::snprintf(number, sizeof(number), "%02d", _track + 1);
			const std::string machine = machineName(_kit, _track);
			return machine.empty() ? std::string(number) : std::string(number) + " " + shortName(machine);
		}
	}

	void EditorOutput::clear()
	{
		strip.clear();
		grid.clear();
		panel.clear();
		forward.clear();
	}

	Editor::Editor() = default;

	const EditorOutput& Editor::redrawAll()
	{
		m_out.clear();
		redrawStrip();
		redrawGrid();
		kitPanel();
		selectionPanel();
		lengthPanel();
		swingPanel();
		halfPanel();
		statusPanel();
		patternPanel();
		panelInts(PanelKey::CopyTo, {0});
		panelInts(PanelKey::FillEdit, {m_fillEdit ? 1 : 0});
		panelInts(PanelKey::ChainMode, {m_chainMode ? 1 : 0});
		chainPanel();
		return m_out;
	}

	const EditorOutput& Editor::stripMouse(const bool _down)
	{
		m_out.clear();
		m_stripDown = _down;
		m_paintValue = -1;
		m_lastStripCell = -1;
		return m_out;
	}

	const EditorOutput& Editor::stripClick(const int _x, const int _y)
	{
		m_out.clear();
		if(!m_stripDown)
			return m_out;
		const int cell = strip::cellAt(_x, _y);
		if(cell < 0 || cell == m_lastStripCell)
			return m_out;
		m_lastStripCell = cell;
		paint(m_track, m_page * StepsPerPage + cell);
		return m_out;
	}

	const EditorOutput& Editor::gridMouse(const bool _down)
	{
		m_out.clear();
		m_gridDown = _down;
		m_paintValue = -1;
		m_lastGridCell = {-1, -1};
		return m_out;
	}

	const EditorOutput& Editor::gridClick(const int _x, const int _y)
	{
		m_out.clear();
		if(!m_gridDown)
			return m_out;
		const grid::Cell cell = grid::cellAt(_x, _y);
		if(cell.row < 0 || (cell.row == m_lastGridCell.row && cell.column == m_lastGridCell.column))
			return m_out;
		const bool first = m_paintValue < 0;
		m_lastGridCell = cell;
		const int step = m_half * StepsPerHalf + cell.column;
		// The swing row is the Pattern's, not a Track's: its steps toggle and paint, and the selection stays.
		if(cell.row == grid::SwingRow)
		{
			paintSwing(step);
			return m_out;
		}
		if(first)
		{
			// A click selects. On a Hit not selected yet it changes nothing else: the step keeps its Hit and what
			// shapes it, and a drag from there takes off the Hits it crosses. Elsewhere it toggles, so that the step
			// selected's own click, the second of a double-click, takes its Hit off.
			const bool wasSelected = cell.row == m_selectedTrack && step == m_selectedStep;
			select(cell.row, step);
			m_gestureStart = {cell.row, step};
			if(!wasSelected && m_bank[m_shown].steps[cell.row][step].hit)
			{
				m_paintValue = PaintOnDrag;
				redrawCell(cell.row, step);
				selectionPanel();
				return m_out;
			}
		}
		bool selectedPainted = cell.row == m_selectedTrack && step == m_selectedStep;
		if(!first && m_paintValue == PaintOnDrag)
		{
			// The drag leaves the Hit it started on, the step selected: that one goes first.
			m_paintValue = 0;
			paint(m_gestureStart.row, m_gestureStart.column);
			selectedPainted = true;
		}
		// The cell clicked first changes, and paint draws it with the selection's frame.
		paint(cell.row, step);
		// The panel shows the step selected as the gesture left it: its Hit, and its Locks gone with a Hit taken off.
		if(selectedPainted)
			selectionPanel();
		return m_out;
	}

	const EditorOutput& Editor::showTrack(const int _track)
	{
		m_out.clear();
		const int track = std::clamp(_track, 0, TrackCount - 1);
		if(track != m_track)
		{
			m_track = track;
			redrawStrip();
		}
		return m_out;
	}

	const EditorOutput& Editor::showPage(const int _page)
	{
		m_out.clear();
		const int page = std::clamp(_page, 0, MaxSteps / StepsPerPage - 1);
		if(page != m_page)
		{
			m_page = page;
			redrawStrip();
		}
		return m_out;
	}

	const EditorOutput& Editor::showHalf(const int _half)
	{
		m_out.clear();
		const int half = std::clamp(_half, 0, 1);
		if(half != m_half)
		{
			// The numbers over the columns change with the half, as when the step played turns it.
			m_half = half;
			halfPanel();
			redrawGrid();
		}
		return m_out;
	}

	const EditorOutput& Editor::setFollow(const bool _follow)
	{
		m_out.clear();
		m_follow = _follow;
		return m_out;
	}

	const EditorOutput& Editor::setMute(const int _track, const bool _muted)
	{
		m_out.clear();
		if(edit({Input::Kind::SetMute, _track, 0, _muted ? 1 : 0}))
			mutePanel(_track, _muted);
		return m_out;
	}

	const EditorOutput& Editor::setVelocity(const int _velocity)
	{
		m_out.clear();
		if(edit({Input::Kind::SetVelocity, m_selectedTrack, m_selectedStep, _velocity}))
			redrawCell(m_selectedTrack, m_selectedStep);
		return m_out;
	}

	const EditorOutput& Editor::setHit(const bool _hit)
	{
		m_out.clear();
		if(edit({Input::Kind::SetHit, m_selectedTrack, m_selectedStep, _hit ? 1 : 0}))
		{
			redrawCell(m_selectedTrack, m_selectedStep);
			selectionPanel();
		}
		return m_out;
	}

	const EditorOutput& Editor::setLock(const int _param, const int _value)
	{
		m_out.clear();
		if(_param < 0 || _param >= LockCount)
			return m_out;
		Input in{Input::Kind::SetLock, m_selectedTrack, m_selectedStep, _value};
		in.param = _param;
		if(edit(in))
			redrawCell(m_selectedTrack, m_selectedStep);
		// Shown as it now is: the Lock taken, or the Kit when the step refused it. The Kit from the dial leaves it on
		// its middle: a drag through it that went on would otherwise jump to the Kit's value and on from there, and one
		// starting next to the middle could never pass it.
		if(_value != NoLock)
		{
			lockPanel(_param);
			return m_out;
		}
		lockPanel(_param, NoLock);
		if(kitValue(_param) >= 0 && kitValue(_param) != NoLock)
			m_unsettled |= 1u << _param;
		return m_out;
	}

	const EditorOutput& Editor::settle()
	{
		m_out.clear();
		for(int p = 0; p < LockCount; ++p)
			if(m_unsettled & (1u << p))
				lockPanel(p);
		return m_out;
	}

	const EditorOutput& Editor::setCondition(const int _index)
	{
		m_out.clear();
		if(edit({Input::Kind::SetCondition, m_selectedTrack, m_selectedStep, _index}))
			redrawCell(m_selectedTrack, m_selectedStep);
		// Shown as it now is: the Condition taken, or none when the step refused it.
		conditionPanel();
		return m_out;
	}

	const EditorOutput& Editor::setRatchets(const int _count)
	{
		m_out.clear();
		if(edit({Input::Kind::SetRatchets, m_selectedTrack, m_selectedStep, _count}))
			redrawCell(m_selectedTrack, m_selectedStep);
		ratchetsPanel();
		return m_out;
	}

	const EditorOutput& Editor::setCurve(const int _curve)
	{
		m_out.clear();
		// The curve is not drawn in the cell: the panel alone shows it.
		edit({Input::Kind::SetCurve, m_selectedTrack, m_selectedStep, _curve});
		ratchetsPanel();
		return m_out;
	}

	const EditorOutput& Editor::setMicro(const int _micro)
	{
		m_out.clear();
		if(edit({Input::Kind::SetMicro, m_selectedTrack, m_selectedStep, _micro}))
			redrawCell(m_selectedTrack, m_selectedStep);
		// Shown as it now is: what the step took, or 0 when it refused it.
		microPanel();
		return m_out;
	}

	const EditorOutput& Editor::setFill(const bool _held)
	{
		m_out.clear();
		m_fill = _held;
		m_out.forward.push_back({Input::Kind::SetFill, 0, 0, _held ? 1 : 0});
		return m_out;
	}

	const EditorOutput& Editor::setLength(const int _length)
	{
		m_out.clear();
		if(edit({Input::Kind::SetLength, 0, 0, _length}))
			lengthPanel();
		return m_out;
	}

	const EditorOutput& Editor::setScale(const int _scale)
	{
		m_out.clear();
		if(edit({Input::Kind::SetScale, 0, 0, _scale}))
			lengthPanel();
		return m_out;
	}

	const EditorOutput& Editor::setSwing(const int _percent)
	{
		m_out.clear();
		if(edit({Input::Kind::SetSwing, 0, 0, _percent}))
		{
			swingPanel();
			drawSwingRow();
		}
		return m_out;
	}

	const EditorOutput& Editor::played(const int _step)
	{
		m_out.clear();
		const int step = _step < 0 ? -1 : std::min(_step, MaxSteps - 1);
		if(step == m_played)
			return m_out;

		const int previous = m_played;
		m_played = step;
		statusPanel();

		if(m_follow && step >= 0 && step / StepsPerHalf != m_half)
		{
			m_half = step / StepsPerHalf;
			halfPanel();
			redrawGrid();
		}
		else
		{
			redrawGridColumn(previous);
			redrawGridColumn(step);
		}
		redrawStripStep(previous);
		redrawStripStep(step);

		// A chain plays: each Pass that begins queues the next of its Patterns. Live starting again goes on from the
		// entry shown (the first after the one last played that holds it), else from the first.
		if(m_chainActive && step >= 0)
		{
			if(previous < 0)
			{
				int found = -1;
				for(int i = 0; i < m_chainLength && found < 0; ++i)
				{
					const int entry = (std::max(m_chainPos, 0) + i) % m_chainLength;
					if(m_chain[static_cast<size_t>(entry)] == m_shown)
						found = entry;
				}
				m_chainPos = found;
				chainNext();
			}
			else if(step < previous)
			{
				m_chainPos = (m_chainPos + 1) % m_chainLength;
				chainNext();
			}
		}
		return m_out;
	}

	void Editor::redrawGridColumn(const int _step)
	{
		// A step's column of the grid, its swing cell included, when the half shows it.
		if(_step < 0 || _step / StepsPerHalf != m_half)
			return;
		for(int t = 0; t < TrackCount; ++t)
			drawGridCell(t, _step % StepsPerHalf);
		drawSwingCell(_step % StepsPerHalf);
	}

	void Editor::redrawStripStep(const int _step)
	{
		if(_step >= 0 && _step / StepsPerPage == m_page)
			drawStripCell(_step % StepsPerPage);
	}

	const EditorOutput& Editor::follow(const int _pattern, const int _queued)
	{
		m_out.clear();
		if(_pattern < 0 || _pattern >= PatternCount)
			return m_out;
		const int queued = _queued >= 0 && _queued < PatternCount ? _queued : -1;
		if(_pattern == m_shown && queued == m_queued)
			return m_out;
		m_queued = queued;
		if(_pattern != m_shown)
		{
			m_shown = _pattern;
			return redrawAll();
		}
		patternPanel();
		return m_out;
	}

	const EditorOutput& Editor::setPattern(const int _pattern)
	{
		m_out.clear();
		if(_pattern < 0 || _pattern >= PatternCount)
			return m_out;
		// Chosen by hand: a chain playing ends (told after the choice, which may draw everything again).
		const bool chainEnds = m_chainActive;
		m_chainActive = false;
		choose(_pattern);
		if(chainEnds)
		{
			chainPanel();
			chainStore();
		}
		return m_out;
	}

	void Editor::choose(const int _pattern)
	{
		// Stopped, the Pattern chosen is the one played; playing, it follows at the end of the Pass, and choosing the
		// one played takes the choice back.
		if(m_played < 0 && _pattern != m_shown)
		{
			m_shown = _pattern;
			m_queued = -1;
			redrawAll();
		}
		else
		{
			m_queued = m_played < 0 || _pattern == m_shown ? -1 : _pattern;
			patternPanel();
		}
		m_out.forward.push_back({Input::Kind::SetPattern, 0, 0, _pattern});
	}

	const EditorOutput& Editor::setChainMode(const bool _on)
	{
		m_out.clear();
		if(_on == m_chainMode)
			return m_out;
		m_chainMode = _on;
		panelInts(PanelKey::ChainMode, {_on ? 1 : 0});
		if(_on)
		{
			// A new chain: the one playing stops, the Patterns clicked from now on make the next.
			m_chainActive = false;
			m_chainLength = 0;
		}
		else if(m_chainLength == 1)
		{
			choose(m_chain[0]);	// may draw everything again: the mode's panel value with it
			m_chainLength = 0;
		}
		else if(m_chainLength >= 2)
		{
			// Stopped, its first Pattern is shown at once and the next queued when Live plays; playing, it starts at
			// the end of the Pass, at once if its first Pattern is the one playing.
			m_chainActive = true;
			if(m_played >= 0)
				m_chainPos = m_chain[0] == m_shown ? 0 : -1;
			else
			{
				if(m_chain[0] != m_shown)
					choose(m_chain[0]);
				m_chainPos = 0;
			}
			if(m_played >= 0)
				chainNext();
		}
		chainPanel();
		chainStore();
		return m_out;
	}

	void Editor::chainStore()
	{
		PanelValue store{panelKey(PanelKey::ChainStore), {m_chainActive ? 1 : 0, m_chainLength}, {}};
		for(int i = 0; i < m_chainLength; ++i)
			store.ints.push_back(m_chain[static_cast<size_t>(i)]);
		m_out.panel.push_back(std::move(store));
	}

	const EditorOutput& Editor::restoreChain(const std::vector<int>& _words)
	{
		m_out.clear();
		if(_words.size() < 2 || (_words[0] != 0 && _words[0] != 1) || _words[1] < 0 || _words[1] > ChainLength ||
			_words.size() != static_cast<size_t>(2 + _words[1]))
			return m_out;
		for(size_t i = 2; i < _words.size(); ++i)
			if(_words[i] < 0 || _words[i] >= PatternCount)
				return m_out;
		const bool active = _words[0] == 1 && _words[1] >= 2;
		const int length = _words[1];
		bool same = active == m_chainActive && length == m_chainLength && !m_chainMode;
		for(int i = 0; same && i < length; ++i)
			same = m_chain[static_cast<size_t>(i)] == _words[static_cast<size_t>(2 + i)];
		if(same)
			return m_out;
		m_chainMode = false;
		m_chainActive = active;
		m_chainLength = length;
		for(int i = 0; i < length; ++i)
			m_chain[static_cast<size_t>(i)] = _words[static_cast<size_t>(2 + i)];
		// From its first Pattern, queued once Live plays (or the entry shown, found then).
		m_chainPos = -1;
		panelInts(PanelKey::ChainMode, {0});
		chainPanel();
		return m_out;
	}

	const EditorOutput& Editor::chainAdd(const int _pattern)
	{
		m_out.clear();
		if(!m_chainMode || _pattern < 0 || _pattern >= PatternCount || m_chainLength >= ChainLength)
			return m_out;
		m_chain[static_cast<size_t>(m_chainLength++)] = _pattern;
		chainPanel();
		return m_out;
	}

	void Editor::chainNext()
	{
		const int next = m_chain[static_cast<size_t>((m_chainPos + 1) % m_chainLength)];
		if(next != m_shown)
			choose(next);
		chainPanel();
	}

	void Editor::chainPanel()
	{
		// "Chain: 01 03" while it is built, "Chain 2/4: 01 03 03 05" while it plays, nothing otherwise.
		std::string text;
		if(m_chainMode || m_chainActive)
		{
			text = "Chain";
			if(m_chainActive)
				text += (m_chainPos < 0 ? std::string(" -") : " " + std::to_string(m_chainPos + 1)) + "/" +
					std::to_string(m_chainLength);
			text += ":";
			for(int i = 0; i < m_chainLength; ++i)
			{
				char number[8];
				std::snprintf(number, sizeof(number), " %02d", m_chain[static_cast<size_t>(i)] + 1);
				text += number;
			}
		}
		panelText(PanelKey::ChainText, text.c_str());
	}

	const EditorOutput& Editor::copyPattern(const int _target)
	{
		m_out.clear();
		if(_target >= 0 && _target < PatternCount && !(m_bank[_target] == m_bank[m_shown]))
		{
			// The edit names the Pattern it writes, as every edit does, the store writing that one's Blob.
			m_bank[_target] = m_bank[m_shown];
			Input in{Input::Kind::CopyPattern, 0, 0, m_shown};
			in.pattern = _target;
			m_out.forward.push_back(in);
			patternStatePanel(_target);
		}
		panelInts(PanelKey::CopyTo, {0});
		return m_out;
	}

	const EditorOutput& Editor::setFillEdit(const bool _on)
	{
		m_out.clear();
		if(_on == m_fillEdit)
			return m_out;
		m_fillEdit = _on;
		redrawStrip();
		redrawGrid();
		panelInts(PanelKey::FillEdit, {_on ? 1 : 0});
		return m_out;
	}

	const EditorOutput& Editor::clearPattern()
	{
		m_out.clear();
		Input in{Input::Kind::ClearPattern, 0, 0, 0};
		in.pattern = m_shown;
		if(!applyEdit(m_bank[m_shown], m_mutes, in))
			return m_out;
		// Everything changes, its length, scale and swing with its steps: drawn again whole, then forwarded.
		redrawAll();
		m_out.forward.push_back(in);
		return m_out;
	}

	const EditorOutput& Editor::doubleLength()
	{
		m_out.clear();
		if(edit({Input::Kind::DoubleLength, 0, 0, 0}))
		{
			redrawStrip();
			redrawGrid();
			selectionPanel();
			lengthPanel();
		}
		return m_out;
	}

	const EditorOutput& Editor::apply(const Input& _in)
	{
		m_out.clear();
		if(!edit(_in))
			return m_out;
		if(_in.kind == Input::Kind::SetMute)
			mutePanel(_in.track, _in.value != 0);
		else if(_in.kind == Input::Kind::SetLength || _in.kind == Input::Kind::SetScale)
			lengthPanel();
		else if(_in.kind == Input::Kind::SetSwing)
		{
			swingPanel();
			drawSwingRow();
		}
		else if(_in.kind == Input::Kind::SetSwingStep)
		{
			if(_in.step / StepsPerHalf == m_half)
				drawSwingCell(_in.step % StepsPerHalf);
		}
		else
		{
			redrawCell(_in.track, _in.step);
			if(_in.track == m_selectedTrack && _in.step == m_selectedStep)
				selectionPanel();
		}
		return m_out;
	}

	const EditorOutput& Editor::load(const int _index, const Pattern& _pattern)
	{
		m_out.clear();
		if(_index < 0 || _index >= PatternCount)
			return m_out;
		m_bank[_index] = _pattern;
		if(_index == m_shown)
			return redrawAll();
		patternStatePanel(_index);
		return m_out;
	}

	bool Editor::edit(const Input& _in)
	{
		// An edit names the Pattern shown, so that the Sequencer applies it there even if it has switched meanwhile.
		Input in = _in;
		in.pattern = m_shown;
		if(!applyEdit(m_bank[m_shown], m_mutes, in))
			return false;
		m_out.forward.push_back(in);
		return true;
	}

	void Editor::paint(const int _track, const int _step)
	{
		// The first cell of a gesture toggles; the cells a drag crosses take the same value. In Fill edit, the Hits it
		// puts are FILL ones.
		if(m_paintValue < 0)
			m_paintValue = m_bank[m_shown].steps[_track][_step].hit ? 0 : 1;
		bool changed = edit({Input::Kind::SetHit, _track, _step, m_paintValue});
		if(m_fillEdit && m_paintValue == 1)
			changed = edit({Input::Kind::SetCondition, _track, _step, fillCondition()}) || changed;
		if(changed)
			redrawCell(_track, _step);
	}

	void Editor::paintSwing(const int _step)
	{
		if(m_paintValue < 0)
			m_paintValue = m_bank[m_shown].inSwingRow(_step) ? 0 : 1;
		if(edit({Input::Kind::SetSwingStep, 0, _step, m_paintValue}))
			drawSwingCell(_step % StepsPerHalf);
	}

	void Editor::select(const int _track, const int _step)
	{
		// The step selected moves: the cell it leaves loses its frame. The new one, and the panel, the click that
		// selects it draws and sends once it has painted it.
		if(_track == m_selectedTrack && _step == m_selectedStep)
			return;
		const int previousTrack = m_selectedTrack;
		const int previousStep = m_selectedStep;
		m_selectedTrack = _track;
		m_selectedStep = _step;
		redrawCell(previousTrack, previousStep);
	}

	void Editor::redrawCell(const int _track, const int _step)
	{
		if(_track == m_track && _step / StepsPerPage == m_page)
			drawStripCell(_step % StepsPerPage);
		if(_step / StepsPerHalf == m_half)
			drawGridCell(_track, _step % StepsPerHalf);
	}

	void Editor::redrawStrip()
	{
		m_out.strip.push_back({DrawOp::Kind::Clear, {}, 0, 0, 0, 0});
		for(int c = 0; c < strip::Cells; ++c)
			drawStripCell(c);
	}

	void Editor::redrawGrid()
	{
		m_out.grid.push_back({DrawOp::Kind::Clear, {}, 0, 0, 0, 0});
		for(int t = 0; t < grid::Rows; ++t)
			for(int c = 0; c < grid::Columns; ++c)
				drawGridCell(t, c);
		drawSwingRow();
	}

	CellLook Editor::lookOf(const int _step) const
	{
		CellLook look;
		look.beat = _step % 4 == 0;
		look.played = _step == m_played;
		return look;
	}

	bool Editor::faded(const Step& _step) const
	{
		const ConditionInfo::Kind kind = condition(_step.condition).kind;
		return m_fillEdit && _step.hit && kind != ConditionInfo::Kind::Fill && kind != ConditionInfo::Kind::NotFill;
	}

	void Editor::drawSwingRow()
	{
		for(int c = 0; c < grid::Columns; ++c)
			drawSwingCell(c);
	}

	void Editor::drawSwingCell(const int _column)
	{
		const int step = m_half * StepsPerHalf + _column;
		const Pattern& pattern = m_bank[m_shown];
		CellLook look = lookOf(step);
		DrawOp ops[MaxSwingCellOps];
		const size_t n = mdseq::drawSwingCell(pattern.inSwingRow(step), pattern.swing, look,
			grid::cellX(_column), grid::cellY(grid::SwingRow), grid::CellWidth, grid::CellHeight, ops);
		m_out.grid.insert(m_out.grid.end(), ops, ops + n);
	}

	void Editor::drawStripCell(const int _cell)
	{
		const int step = m_page * StepsPerPage + _cell;
		const Step& s = m_bank[m_shown].steps[m_track][step];
		CellLook look = lookOf(step);
		look.faded = faded(s);
		DrawOp ops[MaxCellOps];
		const size_t n = drawCell(s, look, strip::cellX(_cell), 0, strip::CellWidth, strip::Height, ops);
		m_out.strip.insert(m_out.strip.end(), ops, ops + n);
	}

	void Editor::drawGridCell(const int _row, const int _column)
	{
		const int step = m_half * StepsPerHalf + _column;
		const Step& s = m_bank[m_shown].steps[_row][step];
		CellLook look = lookOf(step);
		look.selected = _row == m_selectedTrack && step == m_selectedStep;
		look.faded = faded(s);
		DrawOp ops[MaxCellOps];
		const size_t n = drawCell(s, look, grid::cellX(_column), grid::cellY(_row), grid::CellWidth, grid::CellHeight,
			ops);
		m_out.grid.insert(m_out.grid.end(), ops, ops + n);
	}

	const Step& Editor::stepSelected() const
	{
		return m_bank[m_shown].steps[m_selectedTrack][m_selectedStep];
	}

	void Editor::panelInts(const PanelKey _key, const std::initializer_list<int> _ints)
	{
		m_out.panel.push_back({panelKey(_key), _ints, {}});
	}

	void Editor::panelText(const PanelKey _key, const char* _text)
	{
		m_out.panel.push_back({panelKey(_key), {}, {_text}});
	}

	const EditorOutput& Editor::setKit(const std::optional<Kit>& _kit)
	{
		m_out.clear();
		if(_kit == m_kit)
			return m_out;
		m_kit = _kit;
		kitPanel();
		trackPanel();
		for(int p = 0; p < LockCount; ++p)
			lockPanel(p);
		return m_out;
	}

	void Editor::kitPanel()
	{
		// The strip's tabs as one list, then the tab chosen again, which new names may have moved.
		PanelValue tabs{panelKey(PanelKey::TrackNames), {}, {}};
		for(int t = 0; t < TrackCount; ++t)
			tabs.texts.push_back(trackLabel(m_kit, t));
		m_out.panel.push_back(std::move(tabs));
		panelInts(PanelKey::TrackIndex, {m_track});
		for(int t = 0; t < TrackCount; ++t)
			m_out.panel.push_back({panelKey(PanelKey::TrackName), {t}, {machineName(m_kit, t)}});
	}

	// The step selected's Track: its title and the names its Machine gives SYN1-8.
	void Editor::trackPanel()
	{
		const std::string machine = machineName(m_kit, m_selectedTrack);
		char text[64];
		std::snprintf(text, sizeof(text), "Track %02d", m_selectedTrack + 1);
		panelText(PanelKey::TrackTitle, machine.empty() ? text : (std::string(text) + " " + machine).c_str());
		for(int p = 0; p < SynCount; ++p)
		{
			std::string name = "SYN" + std::to_string(p + 1);
			if(unused(p))
				name = "unused";
			else if(!machine.empty())
				name = m_kit->tracks[static_cast<size_t>(m_selectedTrack)].synNames[static_cast<size_t>(p)];
			m_out.panel.push_back({panelKey(PanelKey::LockName), {p}, {name}});
		}
	}

	void Editor::selectionPanel()
	{
		// The titles count Tracks, steps and the strip's pages of 16 steps from 1.
		trackPanel();
		char text[64];
		std::snprintf(text, sizeof(text), "Step %d, page %d", m_selectedStep + 1, m_selectedStep / StepsPerPage + 1);
		panelText(PanelKey::StepTitle, text);
		const Step& step = stepSelected();
		panelInts(PanelKey::Velocity, {step.velocity});
		panelInts(PanelKey::Hit, {step.hit ? 1 : 0});
		conditionPanel();
		microPanel();
		ratchetsPanel();
		for(int p = 0; p < LockCount; ++p)
			lockPanel(p);
	}

	void Editor::lengthPanel()
	{
		// The length's menu counts from 1 step at index 0; the text names a step's note at the scale.
		const Pattern& pattern = m_bank[m_shown];
		const int scale = pattern.scale;
		panelInts(PanelKey::LengthIndex, {pattern.length - 1});
		panelInts(PanelKey::ScaleIndex, {scale});
		char text[64];
		std::snprintf(text, sizeof(text), "%d steps, %s", pattern.length, scale < ScaleCount ? ScaleNotes[scale] : "?");
		panelText(PanelKey::LengthText, text);
	}

	void Editor::swingPanel()
	{
		// The Swing menu counts from 50 % at index 0.
		panelInts(PanelKey::Swing, {m_bank[m_shown].swing - MinSwing});
	}

	void Editor::halfPanel()
	{
		// Over the grid, the number of every fourth step of the half shown, counted from 1.
		panelInts(PanelKey::Half, {m_half});
		for(int i = 0; i < ColumnNumberCount; ++i)
			panelInts(PanelKey::ColumnNumber, {i, m_half * StepsPerHalf + 4 * i + 1});
	}

	void Editor::statusPanel()
	{
		// The strip's status line: the Pattern queued, which waits for the end of the Pass, else the step played.
		char text[64];
		if(m_queued >= 0)
			std::snprintf(text, sizeof(text), "%02d next, after step %d", m_queued + 1, m_bank[m_shown].length);
		else if(m_played < 0)
			std::snprintf(text, sizeof(text), "Stopped");
		else
			std::snprintf(text, sizeof(text), "Playing step %d", m_played + 1);
		panelText(PanelKey::Status, text);
	}

	void Editor::patternPanel()
	{
		// The tab shows the Pattern chosen: the one queued, or the one played. The status line says which, and the
		// buttons each Pattern's state, the queued one outlined first.
		panelInts(PanelKey::PatternIndex, {m_queued >= 0 ? m_queued : m_shown});
		statusPanel();
		m_queuedOutlined = true;
		for(int p = 0; p < PatternCount; ++p)
			patternStatePanel(p);
	}

	void Editor::patternStatePanel(const int _pattern)
	{
		PatternState state = used(_pattern) ? PatternState::Used : PatternState::Empty;
		if(_pattern == m_shown)
			state = PatternState::Shown;
		else if(_pattern == m_queued && m_queuedOutlined)
			state = PatternState::Queued;
		panelInts(PanelKey::PatternState, {_pattern, static_cast<int>(state)});
	}

	bool Editor::used(const int _pattern) const
	{
		for(const auto& track : m_bank[_pattern].steps)
			for(const Step& step : track)
				if(step.hit)
					return true;
		return false;
	}

	const EditorOutput& Editor::blink()
	{
		m_out.clear();
		if(m_queued < 0)
			return m_out;
		m_queuedOutlined = !m_queuedOutlined;
		patternStatePanel(m_queued);
		return m_out;
	}

	void Editor::mutePanel(const int _track, const bool _muted)
	{
		panelInts(PanelKey::Mute, {_track, _muted ? 1 : 0});
	}

	void Editor::lockPanel(const int _param)
	{
		// A Lock's dial on the Lock, or unlit on the Kit's value, so that turning it starts the Lock from there; on its
		// middle, the Kit's position, while MD Drums does not tell the value.
		const int value = stepSelected().locks[_param];
		const int kit = kitValue(_param);
		lockPanel(_param, value != NoLock ? value : kit >= 0 ? kit : NoLock);
	}

	void Editor::lockPanel(const int _param, const int _position)
	{
		// lock <param> <position> <active>, then lockvalue <param> <what the dial holds>: the dial's positions are the
		// values themselves, the Kit sitting where 63 would be; a Lock lights the dial.
		m_unsettled &= ~(1u << _param);
		const int value = stepSelected().locks[_param];
		const int kit = kitValue(_param);
		panelInts(PanelKey::Lock, {_param, _position, value != NoLock ? 1 : 0});
		std::string text = "Kit";
		if(value != NoLock)
			text = std::to_string(value);
		else if(unused(_param))
			text.clear();
		else if(kit >= 0)
			text += " " + std::to_string(kit);
		m_out.panel.push_back({panelKey(PanelKey::LockValue), {_param}, {text}});
	}

	int Editor::kitValue(const int _param) const
	{
		return m_kit ? m_kit->tracks[static_cast<size_t>(m_selectedTrack)].values[static_cast<size_t>(_param)] : -1;
	}

	bool Editor::unused(const int _param) const
	{
		// MD Drums names a SYN the Machine does not use "-".
		return _param < SynCount && !machineName(m_kit, m_selectedTrack).empty() &&
			m_kit->tracks[static_cast<size_t>(m_selectedTrack)].synNames[static_cast<size_t>(_param)] == "-";
	}

	void Editor::conditionPanel()
	{
		panelInts(PanelKey::Condition, {stepSelected().condition});
	}

	void Editor::microPanel()
	{
		// The slider shows no number: the text says where it stands in its range.
		const int micro = stepSelected().micro;
		panelInts(PanelKey::Micro, {micro});
		char text[64];
		if(micro == 0)
			std::snprintf(text, sizeof(text), "On the grid");
		else
			std::snprintf(text, sizeof(text), "%+d of %d, %s", micro, MaxMicro, micro < 0 ? "early" : "late");
		panelText(PanelKey::MicroText, text);
	}

	void Editor::ratchetsPanel()
	{
		// Shown as the step now is: what it took, or 1 and 0 when it refused them. The menu counts from 1 at index 0.
		const Step& step = stepSelected();
		const int ratchets = step.ratchets;
		const int curve = step.curve;
		panelInts(PanelKey::RatchetsIndex, {ratchets - 1});
		panelInts(PanelKey::Curve, {curve});
		char text[64];
		if(ratchets < 2)
			std::snprintf(text, sizeof(text), "One Hit");
		else if(curve == 0)
			std::snprintf(text, sizeof(text), "Even over the %d", ratchets);
		else
			std::snprintf(text, sizeof(text), "%+d %%, %s over the %d", curve, curve < 0 ? "falling" : "rising",
				ratchets);
		panelText(PanelKey::CurveText, text);
	}
}
