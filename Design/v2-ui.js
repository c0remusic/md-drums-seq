// md-drums-seq mockup v2: family A (Live's native look), no JavaScript in the device (ADR 0001). The strip's steps and
// the edit window's grid are what Max's lcd object can draw: opaque colours (blended here, as the patch would), filled
// and framed rectangles, filled circles, lines, text. Everything else stands for live.* objects. Transparent "hot" zones
// over the canvases carry the ids of parameter-spec.md and the hover text.
//   mdSeqMock2.strip(container)
//   mdSeqMock2.editWindow(container)
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

	const PLAY = 40;			// step 41 is playing
	const SEL_T = 0, SEL_S = 46;	// step 47 of Track 01 is selected
	const MUTED = 13;
	const USED = new Set([0, 1, 2, 4]);

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
			const shift = Math.round(st.micro / 23 * w * 0.3);
			g.fillStyle = mix(C.panel, C.hit, 0.35 + 0.65 * st.vel / 127);
			g.fillRect(x + 3 + shift, y + 3, w - 6, h - 6);
			g.fillStyle = bg;
			for (let r = 1; r < st.ratchet; ++r)
				g.fillRect(x + 3 + shift + Math.round((w - 6) * r / st.ratchet), y + 3, 1, h - 6);
			if (Object.keys(st.locks).length) {
				g.fillStyle = C.panel;
				g.beginPath(); g.arc(x + w - 5, y + 5, 3.5, 0, 2 * Math.PI); g.fill();
				g.fillStyle = C.accent;
				g.beginPath(); g.arc(x + w - 5, y + 5, 2.5, 0, 2 * Math.PI); g.fill();
			}
			if (st.cond) {
				if (w >= 30) {
					g.fillStyle = C.text;
					g.font = "bold 10px Arial";
					g.textAlign = "center";
					g.textBaseline = "alphabetic";
					g.fillText(st.cond, x + w / 2 + shift, y + h - 5);
				} else {
					g.fillStyle = C.cond;
					g.fillRect(x + 3, y + h - 2, w - 6, 2);
				}
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
	}

	function hot(A, x, y, w, h, id, st) {
		const e = el(A, "hot", x, y, w, h, null, id);
		if (st)
			e.title = "vel " + st.vel + (st.cond ? ", " + st.cond : "") + (st.micro ? ", micro " + st.micro : "")
				+ (st.ratchet > 1 ? ", x" + st.ratchet : "") + (Object.keys(st.locks).length ? ", Locks" : "");
		return e;
	}

	// A live.dial: track, value arc and needle; an unlocked Lock is the dial inactive, reading "Kit"
	function dial(A, x, y, value, id, active) {
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
			const b = el(A, "btn track" + (t === SEL_T ? " on" : ""), x, 22, 36, 24, null, "track-" + (t + 1));
			const num = document.createElement("span");
			num.className = "num";
			num.textContent = pad(t + 1);
			const name = document.createElement("span");
			name.className = "name";
			name.textContent = short(MACHINES[t][0]);
			b.append(num, name);
			el(A, "btn mute" + (t === MUTED ? " muted" : ""), x, 50, 36, 16, String(t + 1), "mute-" + (t + 1));
			const s = 16 * page + t;
			drawCell(g, 40 * t, 0, 36, 40, P[SEL_T][s], {beat: t % 4 === 0, play: s === PLAY});
			hot(A, x, 74, 36, 40, "step-" + (t + 1), P[SEL_T][s]);
		}
		for (let pg = 0; pg < 4; ++pg)
			el(A, "btn" + (pg === page ? " on" : ""), 172 + 160 * pg, 122, 156, 16, (16 * pg + 1) + "-" + (16 * pg + 16),
				"page-" + (pg + 1));
		el(A, "hint", 172, 146, 636, 12, "Track 01 TRX-BD, page 3; click a step to toggle its Hit, Edit for the rest");
		el(A, "rule", 816, 8, 1, 153);
		el(A, "btn big", 828, 22, 72, 64, "FILL", "fill");
		el(A, "btn", 908, 22, 72, 24, "Edit", "edit");
		el(A, "label", 908, 52, 72, 12, "Capture");
		el(A, "btn", 908, 70, 72, 16, "Session", "capture");
		el(A, "hint", 828, 96, 152, 40, "Arrangement when Live records");
	}

	function editWindow(container) {
		const first = 32;	// the window shows steps 33-64: the page that holds step 47
		const A = device(container, 1440, 840, "md-drums-seq: Edit");

		el(A, "label", 16, 22, 72, 12, "Pattern");
		for (let i = 0; i < 16; ++i) {
			const b = el(A, "btn", 96 + 32 * i, 16, 28, 24, pad(i + 1), "win-pattern-" + (i + 1));
			b.classList.add(i === 0 ? "on" : i === 1 ? "queued" : USED.has(i) ? "x" : "empty");
		}
		el(A, "label", 628, 22, 48, 12, "Length");
		el(A, "menu", 680, 16, 72, 24, "64", "length");
		el(A, "label", 768, 22, 40, 12, "Scale");
		el(A, "menu", 812, 16, 64, 24, "1/16", "scale");
		el(A, "btn", 892, 16, 56, 24, "1-32", "win-page-1");
		el(A, "btn on", 952, 16, 56, 24, "33-64", "win-page-2");
		el(A, "btn on", 1012, 16, 56, 24, "Follow", "follow");
		el(A, "btn big", 1084, 16, 64, 24, "FILL", "win-fill");
		el(A, "btn", 1164, 16, 128, 24, "Print 4 passes", "print");
		el(A, "btn", 1308, 16, 96, 24, "Session", "win-capture");
		el(A, "rule", 0, 52, 1440, 1);

		const pitch = 38, cw = 36, x0 = 176, y0 = 88;
		const col = s => pitch * (s - first) + 4 * Math.floor((s - first) / 16);
		for (let s = first; s < first + 32; s += 4)
			el(A, "hint", x0 + col(s), 66, pitch * 4, 12, String(s + 1));
		const g = lcd(A, x0, y0, col(first + 31) + cw, 28 * 16 - 4);
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

		el(A, "rule", 0, 548, 1440, 1);
		const st = P[SEL_T][SEL_S];
		el(A, "title-l", 16, 564, 320, 24, "Track 01 TRX-BD");
		el(A, "hint", 16, 592, 320, 12, "Step 47, page 3");
		el(A, "btn on", 16, 616, 64, 24, "Hit", "step-hit");
		dial(A, 96, 608, st.vel, "step-velocity", true);
		el(A, "dialname", 144, 614, 80, 12, "Velocity");
		el(A, "dialval", 144, 630, 80, 14, String(st.vel)).style.color = "var(--text)";
		el(A, "label", 16, 664, 140, 12, "Condition");
		el(A, "menu", 16, 680, 140, 24, st.cond || "None", "step-condition");
		el(A, "label", 172, 664, 168, 12, "Micro-timing");
		const sl = el(A, "slider", 172, 680, 168, 24, null, "step-microtiming");
		const centre = document.createElement("div");
		centre.className = "centre";
		const val = document.createElement("div");
		val.className = "val";
		val.style.left = "50%";
		val.style.width = Math.round(st.micro / 23 * 82) + "px";
		sl.append(centre, val);
		el(A, "hint", 172, 708, 168, 12, "+" + st.micro + " of 23");
		el(A, "label", 16, 728, 140, 12, "Ratchets");
		el(A, "menu", 16, 744, 64, 24, "x" + st.ratchet, "step-ratchets");
		el(A, "label", 172, 728, 168, 12, "Ratchet velocity");
		const curve = el(A, "slider", 172, 744, 168, 24, null, "step-ratchet-curve");
		const c2 = document.createElement("div");
		c2.className = "centre";
		const v2 = document.createElement("div");
		v2.className = "val";
		v2.style.left = (50 + st.curve / 2) + "%";
		v2.style.width = Math.round(-st.curve / 100 * 82) + "px";
		curve.append(c2, v2);
		el(A, "hint", 172, 772, 168, 12, st.curve + " %, falling over the 3");
		el(A, "hint", 16, 796, 324, 28, "Turn a parameter to lock it on this step; Alt-click clears the Lock.");
		el(A, "rule", 352, 564, 1, 248);

		const groups = [["SYN", MACHINES[SEL_T][1]], ["EFX", EFX], ["ROUTING", ROUTING]];
		groups.forEach(([gname, names], gi) => {
			const y = 568 + 88 * gi;
			el(A, "label", 368, y + 14, 72, 12, gname);
			names.forEach((n, i) => {
				const idx = gi * 8 + i, x = 448 + 120 * i;
				const lock = st.locks[idx], locked = lock !== undefined;
				dial(A, x, y, locked ? lock : 0, "lock-" + (idx + 1), locked);
				el(A, "dialname" + (locked ? " locked" : ""), x + 48, y + 6, 64, 12, n || "unused");
				el(A, "dialval" + (locked ? " locked" : " kit"), x + 48, y + 22, 72, 14, locked ? String(lock) : "Kit");
			});
		});
		el(A, "hint", 448, 828, 960, 12,
			"SYN names follow the Machine MD Drums plays on this Track, read from its Track Machine parameters.");
	}

	window.mdSeqMock2 = {strip, editWindow};
})();
