// md-drums-seq mockup v5: v4 as Max builds it (measured in Live on 2026-10-16: a live.tab item is one line, and one row
// needs multiline 0), with what MD Drums now tells the device (ADR 0003): the Machines' names and the Kit's values,
// shown behind the Locks; and the swing row built since v4. Family A (Live's native look), no JavaScript in the device
// (ADR 0001). The strip's steps and the edit window's grid are what Max's lcd object can draw: opaque colours (blended
// here, as the patch would), filled and framed rectangles, filled circles, lines, text. Everything else stands for
// live.* objects. Transparent "hot" zones over the canvases carry the ids of parameter-spec.md and the hover text.
//   mdSeqMock5.strip(container)
//   mdSeqMock5.editWindow(container)
//   mdSeqMock5.legend(container)	what a cell shows, one feature per sample cell
(function () {
	"use strict";

	const MACHINES = [
		["TRX-BD", ["PTCH", "DEC", "RAMP", "RDEC", "STRT", "NOIS", "HARM", "CLIP"]],
		["TRX-SD", ["PTCH", "DEC", "BUMP", "BENV", "SNAP", "TONE", "TUNE", "CLIP"]],
		["TRX-XT", ["PTCH", "DEC", "RAMP", "RDEC", "DAMP", "DIST", "DTYP", ""]],
		["TRX-CP", ["CLPY", "TONE", "HARD", "RICH", "RATE", "ROOM", "RSIZ", "RTUN"]],
		["TRX-RS", ["PTCH", "DEC", "DIST", "", "", "", "", ""]],
		["TRX-CB", ["PTCH", "DEC", "ENH", "DAMP", "TONE", "BUMP", "", ""]],
		["TRX-CH", ["GAP", "DEC", "HPF", "LPF", "MTAL", "", "", ""]],
		["TRX-OH", ["GAP", "DEC", "HPF", "LPF", "MTAL", "", "", ""]],
		["TRX-CY", ["RICH", "DEC", "TOP", "TTUN", "SIZE", "PEAK", "", ""]],
		["TRX-MA", ["ATT", "SUS", "REV", "DAMP", "RATL", "RTYP", "TONE", "HARD"]],
		["TRX-CL", ["PTCH", "DEC", "DUAL", "ENH", "TUNE", "CLIC", "", ""]],
		["TRX-XC", ["PTCH", "DEC", "RAMP", "RDEC", "DAMP", "DIST", "DTYP", ""]],
		["TRX-B2", ["PTCH", "DEC", "RAMP", "HOLD", "TICK", "NOIS", "DIRT", "DIST"]],
		["TRX-S2", ["PTCH", "DEC", "NOIS", "NDEC", "POWR", "TUNE", "NTUN", "NTYP"]],
		["EFM-BD", ["PTCH", "DEC", "RAMP", "RDEC", "MOD", "MFRQ", "MDEC", "MFB"]],
		["EFM-SD", ["PTCH", "DEC", "NOIS", "NDEC", "MOD", "MFRQ", "MDEC", "HPF"]],
	];
	const EFX = ["AMD", "AMF", "EQF", "EQG", "FLTF", "FLTW", "FLTQ", "SRR"];
	const ROUTING = ["DIST", "VOL", "PAN", "DEL", "REV", "LFOS", "LFOD", "LFOM"];

	// The lcd's colours: Live 12's Dark theme, approximated (lcd does not follow the theme by itself)
	const C = {
		panel: "#1e1e1e", beat: "#262626", hit: "#ffa52e", accent: "#7cc6ff", cond: "#c9a0ff", text: "#1e1e1e",
	};

	// The Kit's values of Track 01's 24 parameters, as MD Drums' file gives them (ADR 0003)
	const KIT = [64, 32, 0, 40, 20, 0, 0, 10, 0, 0, 64, 64, 0, 127, 0, 0, 0, 100, 64, 0, 0, 0, 0, 0];

	const PLAY = 40;			// step 41 is playing
	const SEL_T = 0, SEL_S = 46;	// step 47 of Track 01 is selected
	const MUTED = 13;
	const USED = new Set([0, 1, 2, 4]);
	const SWING = 62;			// the Pattern's swing, in percent; its row is every second step

	const pad = n => String(n).padStart(2, "0");
	const short = name => name.replace(/^TRX-|^EFM-/, "");

	function samplePattern() {
		const P = Array.from({length: 16}, () => Array(64).fill(null));
		const hit = (t, s, o) => { P[t][s] = Object.assign({vel: 100, cond: "", micro: 0, ratchet: 1, curve: 0, locks: {}}, o || {}); };
		for (let s = 0; s < 64; s += 4) hit(0, s, {vel: 120});
		hit(0, 14, {vel: 90, cond: "1:2"}); hit(0, 30, {vel: 84, cond: "50%"});
		hit(0, 46, {vel: 96, cond: "1:2", micro: 6, ratchet: 3, curve: -50, locks: {0: 52, 1: 90, 12: 30}});
		hit(0, 62, {vel: 100, cond: "FILL", ratchet: 2});
		for (let s = 4; s < 64; s += 8) hit(1, s, {vel: 118});
		hit(1, 11, {vel: 40, micro: 6}); hit(1, 27, {vel: 36, micro: 6, cond: "PRE"});
		hit(1, 59, {vel: 46, micro: -4, cond: "NEI"}); hit(1, 63, {vel: 96, cond: "FILL", ratchet: 4});
		for (let s = 0; s < 64; s += 2) hit(6, s, {vel: s % 4 ? 70 : 104, ratchet: s % 16 === 14 ? 3 : 1, cond: s % 8 === 6 ? "67%" : ""});
		for (let s = 6; s < 64; s += 8) hit(7, s, {vel: 92, cond: "1:4", locks: {1: 110}});
		hit(3, 12); hit(3, 28, {locks: {6: 90}}); hit(3, 44); hit(3, 60, {vel: 110, cond: "1ST"});
		[3, 19, 35, 51].forEach(s => hit(4, s, {vel: 80, cond: "!FILL"}));
		[7, 23, 39].forEach(s => hit(5, s, {vel: 90, locks: {0: 70}}));
		[0, 32].forEach(s => hit(8, s));
		for (let s = 1; s < 64; s += 4) hit(9, s, {vel: 60, micro: 3});
		[10, 26, 42, 58].forEach(s => hit(10, s, {vel: 96, cond: "25%"}));
		[6, 22].forEach(s => hit(11, s, {vel: 110, locks: {0: 30}}));
		[0, 24, 40].forEach(s => hit(12, s, {vel: 127, locks: {3: 80, 7: 40}}));
		[12, 44].forEach(s => hit(13, s, {vel: 90}));
		for (let s = 0; s < 64; s += 16) hit(14, s, {vel: 110, locks: {12: 30}});
		[20, 52].forEach(s => hit(15, s, {cond: "2:4"}));
		return P;
	}
	const P = samplePattern();

	function el(parent, cls, x, y, w, h, text, id) {
		const e = document.createElement("div");
		e.className = "abs " + cls;
		Object.assign(e.style, {left: x + "px", top: y + "px", width: w + "px", height: h + "px"});
		if (text !== undefined && text !== null)
			e.textContent = text;
		if (id)
			e.id = id;
		parent.appendChild(e);
		return e;
	}

	function device(container, w, h, title) {
		const host = document.createElement("div");
		host.className = "host";
		const dev = document.createElement("div");
		dev.className = "device";
		dev.style.width = w + "px";
		const bar = document.createElement("div");
		bar.className = "titlebar";
		bar.textContent = title;
		const area = document.createElement("div");
		area.className = "area";
		area.style.height = h + "px";
		dev.append(bar, area);
		host.append(dev);
		container.append(host);
		return area;
	}

	// Two colours mixed, as the patch would compute an opaque colour for lcd (it has no transparency)
	function mix(a, b, t) {
		const p = h => [1, 3, 5].map(i => parseInt(h.substr(i, 2), 16));
		const [x, y] = [p(a), p(b)];
		return "#" + x.map((v, i) => Math.round(v + (y[i] - v) * t).toString(16).padStart(2, "0")).join("");
	}

	// An lcd: a canvas of w x h at the device's pixel ratio
	function lcd(A, x, y, w, h) {
		const c = document.createElement("canvas");
		const r = window.devicePixelRatio || 1;
		c.className = "lcd";
		c.width = Math.round(w * r);
		c.height = Math.round(h * r);
		Object.assign(c.style, {left: x + "px", top: y + "px", width: w + "px", height: h + "px"});
		A.appendChild(c);
		const g = c.getContext("2d");
		g.scale(r, r);
		return g;
	}

	// One step cell, with lcd's primitives only: paintrect, framerect, paintoval, linesegment, write
	function drawCell(g, x, y, w, h, st, o) {
		let bg = o.beat ? C.beat : C.panel;
		if (o.play)
			bg = mix(bg, C.accent, 0.18);
		g.fillStyle = bg;
		g.fillRect(x, y, w, h);
		if (st) {
			g.fillStyle = mix(C.panel, C.hit, 0.35 + 0.65 * st.vel / 127);
			g.fillRect(x + 3, y + 3, w - 6, h - 6);
			// Ratchets: one notch per Hit along the block's top edge, at the middle of its share of the step
			if (st.ratchet > 1) {
				g.fillStyle = bg;
				for (let r = 0; r < st.ratchet; ++r)
					g.fillRect(x + 3 + Math.round((w - 6) * (r + 0.5) / st.ratchet) - 1, y + 3, 2, 4);
			}
			if (Object.keys(st.locks).length) {
				g.fillStyle = C.panel;
				g.beginPath(); g.arc(x + w - 5, y + 5, 3.5, 0, 2 * Math.PI); g.fill();
				g.fillStyle = C.accent;
				g.beginPath(); g.arc(x + w - 5, y + 5, 2.5, 0, 2 * Math.PI); g.fill();
			}
			// Condition: a strip in the cell's bottom margin, and its text above it on the Hit
			if (st.cond) {
				g.fillStyle = C.cond;
				g.fillRect(x + 3, y + h - 3, w - 6, 3);
				g.fillStyle = C.text;
				g.font = "bold 10px Arial";
				g.textAlign = "center";
				g.textBaseline = "alphabetic";
				g.fillText(st.cond, x + w / 2, y + h - 6);
			}
		}
		if (o.play) {
			g.strokeStyle = C.accent;
			g.lineWidth = 1;
			g.strokeRect(x + 0.5, y + 0.5, w - 1, h - 1);
		}
		if (o.sel) {
			g.strokeStyle = C.accent;
			g.lineWidth = 2;
			g.strokeRect(x + 1, y + 1, w - 2, h - 2);
		}
		// Micro-timing, over the frames so that a played or selected step keeps it: the Hit stays on the grid, an arrow
		// of its colour juts into the cell's margin, left when early, right when late (lcd: paintpoly); the step panel
		// gives its value
		if (st && st.micro) {
			const tip = st.micro < 0 ? x : x + w, base = st.micro < 0 ? x + 3 : x + w - 3, mid = y + h / 2;
			g.fillStyle = mix(C.panel, C.hit, 0.35 + 0.65 * st.vel / 127);
			g.beginPath();
			g.moveTo(base, mid - 4); g.lineTo(tip, mid); g.lineTo(base, mid + 4);
			g.closePath();
			g.fill();
		}
	}

	function hot(A, x, y, w, h, id, st) {
		const e = el(A, "hot", x, y, w, h, null, id);
		if (st)
			e.title = "vel " + st.vel + (st.cond ? ", " + st.cond : "") + (st.micro ? ", micro " + st.micro : "")
				+ (st.ratchet > 1 ? ", x" + st.ratchet : "") + (Object.keys(st.locks).length ? ", Locks" : "");
		return e;
	}

	// One cell of the swing row: on a step of the row, a block whose left edge moves right by half the step's delay
	function drawSwingCell(g, x, y, w, h, inRow, o) {
		g.fillStyle = o.beat ? C.beat : C.panel;
		g.fillRect(x, y, w, h);
		if (inRow) {
			const delay = (2 * SWING / 100 - 1) * w;
			// Quieter than the accent the selection and the step played wear, as the device draws it
			g.fillStyle = mix(C.panel, C.accent, SWING > 50 ? 0.55 : 0.3);
			g.fillRect(x + 3 + Math.round(delay / 2), y + 3, w - 6 - Math.round(delay / 2), h - 6);
		}
	}

	// A live.dial: track, value arc and needle. A Lock's dial without a Lock is inactive and points, faint, at the Kit's
	// value, which MD Drums tells; turning it starts the Lock from there.
	function dial(A, x, y, value, id, active, kit) {
		const d = el(A, "", x, y, 40, 40, null, id);
		const ns = "http://www.w3.org/2000/svg";
		const svg = document.createElementNS(ns, "svg");
		svg.setAttribute("width", "40");
		svg.setAttribute("height", "40");
		const pt = (a, r) => [20 + r * Math.sin(a * Math.PI / 180), 20 - r * Math.cos(a * Math.PI / 180)];
		const arc = (v, r) => {
			const a0 = -135, a1 = -135 + 270 * v / 127;
			const [x0, y0] = pt(a0, r), [x1, y1] = pt(a1, r);
			return "M " + x0 + " " + y0 + " A " + r + " " + r + " 0 " + (a1 - a0 > 180 ? 1 : 0) + " 1 " + x1 + " " + y1;
		};
		const path = (dd, style) => {
			const e = document.createElementNS(ns, "path");
			e.setAttribute("d", dd);
			e.setAttribute("fill", "none");
			e.setAttribute("style", style);
			svg.append(e);
		};
		path(arc(127, 15), "stroke: var(--line); stroke-width: 3");
		if (active) {
			if (value > 0)
				path(arc(value, 15), "stroke: var(--accent); stroke-width: 3");
			const [nx, ny] = pt(-135 + 270 * value / 127, 11);
			path("M 20 20 L " + nx + " " + ny, "stroke: var(--text); stroke-width: 2");
		} else if (kit !== undefined) {
			const [nx, ny] = pt(-135 + 270 * kit / 127, 11);
			path("M 20 20 L " + nx + " " + ny, "stroke: var(--text-faint); stroke-width: 2");
		}
		d.append(svg);
		return d;
	}

	function strip(container) {
		const A = device(container, 992, 169, "md-drums-seq");
		el(A, "label", 8, 6, 140, 12, "Pattern");
		for (let i = 0; i < 16; ++i) {
			const b = el(A, "btn", 8 + 36 * (i % 4), 22 + 28 * Math.floor(i / 4), 32, 24, pad(i + 1), "pattern-" + (i + 1));
			b.classList.add(i === 0 ? "on" : i === 1 ? "queued" : USED.has(i) ? "x" : "empty");
		}
		el(A, "hint", 8, 136, 148, 12, "02 next, after step 64");
		el(A, "hint", 8, 150, 148, 12, "64 steps, 1/16");
		el(A, "rule", 160, 8, 1, 153);

		el(A, "label", 172, 6, 300, 12, "Track");
		const page = 2;
		const g = lcd(A, 172, 74, 636, 40);
		for (let t = 0; t < 16; ++t) {
			const x = 172 + 40 * t;
			// One live.tab of 16 items on one row (multiline 0): an item is one line, its number and the Machine's
			// short name, which MD Drums tells; the number alone until it does
			el(A, "btn" + (t === SEL_T ? " on" : ""), x, 22, 36, 24, pad(t + 1) + " " + short(MACHINES[t][0]),
				"track-" + (t + 1));
			// Live's track activator: lit while the Track plays, grey once muted
			el(A, "btn mute" + (t === MUTED ? " muted" : ""), x, 50, 36, 16, String(t + 1), "mute-" + (t + 1));
			const s = 16 * page + t;
			drawCell(g, 40 * t, 0, 36, 40, P[SEL_T][s], {beat: t % 4 === 0, play: s === PLAY});
			hot(A, x, 74, 36, 40, "step-" + (t + 1), P[SEL_T][s]);
		}
		for (let pg = 0; pg < 4; ++pg)
			el(A, "btn" + (pg === page ? " on" : ""), 172 + 160 * pg, 122, 156, 16, (16 * pg + 1) + "-" + (16 * pg + 16),
				"page-" + (pg + 1));
		el(A, "rule", 816, 8, 1, 153);
		el(A, "btn big", 828, 22, 72, 64, "FILL", "fill");
		el(A, "btn", 908, 22, 72, 24, "Edit", "edit");
		el(A, "label", 908, 52, 72, 12, "Capture");
		el(A, "btn", 908, 70, 72, 16, "Session", "capture");
	}

	function editWindow(container) {
		const first = 32;	// the window shows steps 33-64: the page that holds step 47
		const A = device(container, 1440, 868, "md-drums-seq: Edit");

		el(A, "label", 16, 22, 72, 12, "Pattern");
		for (let i = 0; i < 16; ++i) {
			const b = el(A, "btn", 96 + 32 * i, 16, 28, 24, pad(i + 1), "win-pattern-" + (i + 1));
			b.classList.add(i === 0 ? "on" : i === 1 ? "queued" : USED.has(i) ? "x" : "empty");
		}
		// Copy to: the Pattern shown over the one chosen; x2: twice as long, its steps again after them
		el(A, "menu", 612, 16, 64, 24, "Copy to", "copy-to");
		el(A, "label", 688, 22, 44, 12, "Length");
		el(A, "menu", 736, 16, 52, 24, "64", "length");
		el(A, "btn", 792, 16, 28, 24, "×2", "double");
		el(A, "label", 832, 22, 36, 12, "Scale");
		el(A, "menu", 872, 16, 60, 24, "1/16", "scale");
		el(A, "btn", 944, 16, 56, 24, "1-32", "win-page-1");
		el(A, "btn on", 1004, 16, 56, 24, "33-64", "win-page-2");
		el(A, "btn on", 1064, 16, 56, 24, "Follow", "follow");
		el(A, "btn big", 1132, 16, 56, 24, "FILL", "win-fill");
		el(A, "menu", 1200, 16, 116, 24, "Print 4 passes", "print");
		el(A, "btn", 1328, 16, 96, 24, "Session", "win-capture");
		el(A, "rule", 0, 52, 1440, 1);

		const pitch = 38, cw = 36, x0 = 176, y0 = 88;
		const col = s => pitch * (s - first) + 4 * Math.floor((s - first) / 16);
		for (let s = first; s < first + 32; s += 4)
			el(A, "hint", x0 + col(s), 66, pitch * 4, 12, String(s + 1));
		// The Tracks' rows, then the swing row, every Track's: one lcd
		const g = lcd(A, x0, y0, col(first + 31) + cw, 28 * 17 - 4);
		for (let t = 0; t < 16; ++t) {
			const y = 28 * t;
			el(A, "hint", 16, y0 + y + 6, 20, 12, pad(t + 1));
			el(A, t === SEL_T ? "title-l" : "dialname", 40, y0 + y + 5, 92, 14, MACHINES[t][0]).style.fontSize = "var(--fs-m)";
			el(A, "btn mute" + (t === MUTED ? " muted" : ""), 136, y0 + y, 24, 24, String(t + 1), "win-mute-" + (t + 1));
			for (let s = first; s < first + 32; ++s) {
				drawCell(g, col(s), y, cw, 24, P[t][s], {beat: s % 4 === 0, play: s === PLAY, sel: t === SEL_T && s === SEL_S});
				hot(A, x0 + col(s), y0 + y, cw, 24, "cell-" + (t + 1) + "-" + (s + 1), P[t][s]);
			}
		}
		el(A, "label", 16, y0 + 28 * 16 + 6, 56, 12, "Swing");
		el(A, "menu", 80, y0 + 28 * 16, 72, 24, SWING + "%", "swing");
		for (let s = first; s < first + 32; ++s) {
			drawSwingCell(g, col(s), 28 * 16, cw, 24, s % 2 === 1, {beat: s % 4 === 0});
			hot(A, x0 + col(s), y0 + 28 * 16, cw, 24, "swing-" + (s + 1), null);
		}

		el(A, "rule", 0, 576, 1440, 1);
		const st = P[SEL_T][SEL_S];
		el(A, "title-l", 16, 592, 320, 24, "Track 01 TRX-BD");
		el(A, "hint", 16, 620, 320, 12, "Step 47, page 3");
		el(A, "btn on", 16, 644, 64, 24, "Hit", "step-hit");
		dial(A, 96, 636, st.vel, "step-velocity", true);
		el(A, "dialname", 144, 642, 80, 12, "Velocity");
		el(A, "dialval", 144, 658, 80, 14, String(st.vel)).style.color = "var(--text)";
		el(A, "label", 16, 692, 140, 12, "Condition");
		el(A, "menu", 16, 708, 140, 24, st.cond || "None", "step-condition");
		el(A, "label", 172, 692, 168, 12, "Micro-timing");
		const sl = el(A, "slider", 172, 708, 168, 24, null, "step-microtiming");
		const centre = document.createElement("div");
		centre.className = "centre";
		const val = document.createElement("div");
		val.className = "val";
		val.style.left = "50%";
		val.style.width = Math.round(st.micro / 23 * 82) + "px";
		sl.append(centre, val);
		el(A, "hint", 172, 736, 168, 12, "+" + st.micro + " of 23, late");
		el(A, "label", 16, 756, 140, 12, "Ratchets");
		el(A, "menu", 16, 772, 64, 24, "x" + st.ratchet, "step-ratchets");
		el(A, "label", 172, 756, 168, 12, "Ratchet velocity");
		const curve = el(A, "slider", 172, 772, 168, 24, null, "step-ratchet-curve");
		const c2 = document.createElement("div");
		c2.className = "centre";
		const v2 = document.createElement("div");
		v2.className = "val";
		v2.style.left = (50 + st.curve / 2) + "%";
		v2.style.width = Math.round(-st.curve / 100 * 82) + "px";
		curve.append(c2, v2);
		el(A, "hint", 172, 800, 168, 12, st.curve + " %, falling over the 3");
		el(A, "rule", 352, 592, 1, 248);

		// Each Lock: its dial, then its name and its value beside it, the Kit's when the step does not lock it
		const groups = [["SYN", MACHINES[SEL_T][1]], ["EFX", EFX], ["ROUTING", ROUTING]];
		groups.forEach(([gname, names], gi) => {
			const y = 596 + 88 * gi;
			el(A, "label", 368, y + 14, 72, 12, gname);
			names.forEach((n, i) => {
				const idx = gi * 8 + i, x = 448 + 120 * i;
				const lock = st.locks[idx], locked = lock !== undefined;
				if (!n) {
					dial(A, x, y, 0, "lock-" + (idx + 1), false);
					el(A, "dialname unused", x + 48, y + 6, 64, 12, "unused");
					return;
				}
				dial(A, x, y, locked ? lock : 0, "lock-" + (idx + 1), locked, KIT[idx]);
				el(A, "dialname" + (locked ? " locked" : ""), x + 48, y + 6, 64, 12, n);
				el(A, "dialval" + (locked ? " locked" : " kit"), x + 48, y + 22, 72, 14,
					locked ? String(lock) : "Kit " + KIT[idx]);
			});
		});
	}

	// What a cell shows: one sample cell per feature, the same drawing as the grid's
	// The legend's words, in French (the design files) or in English (the board shown elsewhere)
	const LEGEND = {
		fr: ["Lire une case", "Hit, vélocité 127", "Hit, vélocité 40", "Locks", "Condition 1:2", "Condition 67%",
			"3 ratchets", "Micro-timing −8, en avance", "Micro-timing +12, en retard", "Tout ensemble", "Pas joué",
			"Pas sélectionné"],
		en: ["Reading a step", "Hit, velocity 127", "Hit, velocity 40", "Locks", "Condition 1:2", "Condition 67%",
			"3 ratchets", "Micro-timing −8, early", "Micro-timing +12, late", "All together", "Step playing",
			"Step selected"],
	};

	function legend(container, lang) {
		const words = LEGEND[lang || "fr"];
		const base = {vel: 100, cond: "", micro: 0, ratchet: 1, curve: 0, locks: {}};
		const samples = [
			[words[1], Object.assign({}, base, {vel: 127}), {}],
			[words[2], Object.assign({}, base, {vel: 40}), {}],
			[words[3], Object.assign({}, base, {locks: {0: 52}}), {}],
			[words[4], Object.assign({}, base, {cond: "1:2"}), {}],
			[words[5], Object.assign({}, base, {cond: "67%"}), {}],
			[words[6], Object.assign({}, base, {ratchet: 3}), {}],
			[words[7], Object.assign({}, base, {micro: -8}), {}],
			[words[8], Object.assign({}, base, {micro: 12}), {}],
			[words[9], Object.assign({}, base, {vel: 96, cond: "1:2", micro: 6, ratchet: 3, locks: {0: 52}}), {}],
			[words[10], null, {play: true}],
			[words[11], Object.assign({}, base), {sel: true}],
		];
		const A = device(container, 16 + 112 * samples.length, 64, words[0]);
		const g = lcd(A, 0, 0, 16 + 112 * samples.length, 34);
		g.fillStyle = "#383838";
		g.fillRect(0, 0, 16 + 112 * samples.length, 34);
		samples.forEach(([label, st, o], i) => {
			const x = 16 + 112 * i + 38;
			drawCell(g, x, 6, 36, 24, st, o);
			el(A, "hint", 16 + 112 * i, 38, 112, 24, label).style.textAlign = "center";
		});
	}

	window.mdSeqMock5 = {strip, editWindow, legend};
})();
