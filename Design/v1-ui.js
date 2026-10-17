// md-drums-seq mockup v1: draws the strip (to play) and the edit window (to edit) from one sample Pattern.
// The geometry is the same for every family; .fam-live / .fam-v22 on an ancestor choose the skin.
//   mdSeqMock.strip(container, {family, prefix})
//   mdSeqMock.editWindow(container, {family, prefix, density})   density 64 (all steps) or 32 (steps 33-64)
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
	const KIT_FX = [0, 0, 64, 64, 0, 127, 0, 0, 0, 100, 64, 0, 0, 64, 0, 0];	// parameters 9-24, MD Drums' defaults
	const KIT_SYN0 = [40, 48, 64, 32, 80, 20, 16, 0];						// track 1's SYN1-8 in the sample Kit

	const PLAY = 40;			// step 41 is playing
	const SEL_T = 0, SEL_S = 46;	// step 47 of Track 01 is selected
	const MUTED = 13;			// Track 14 is muted
	const USED = new Set([0, 1, 2, 4]);	// Patterns holding steps

	const pad = n => String(n).padStart(2, "0");
	const short = name => name.replace(/^TRX-|^EFM-/, "");
	const kitValue = (t, idx) => idx < 8 ? (t === 0 ? KIT_SYN0[idx] : 64) : KIT_FX[idx - 8];

	function samplePattern() {
		const P = Array.from({length: 16}, () => Array(64).fill(null));
		const hit = (t, s, o) => { P[t][s] = Object.assign({vel: 100, cond: "", micro: 0, ratchet: 1, locks: {}}, o || {}); };
		for (let s = 0; s < 64; s += 4) hit(0, s, {vel: 120});
		hit(0, 14, {vel: 90, cond: "1:2"}); hit(0, 30, {vel: 84, cond: "50%"});
		hit(0, 46, {vel: 96, cond: "1:2", micro: 6, locks: {0: 52, 1: 90, 12: 30}});
		hit(0, 62, {vel: 100, cond: "FILL", ratchet: 2});
		for (let s = 4; s < 64; s += 8) hit(1, s, {vel: 118});
		hit(1, 11, {vel: 40, micro: 6}); hit(1, 27, {vel: 36, micro: 6, cond: "PRE"});
		hit(1, 59, {vel: 46, micro: -4, cond: "NEI"}); hit(1, 63, {vel: 96, cond: "FILL", ratchet: 4});
		for (let s = 0; s < 64; s += 2) hit(6, s, {vel: s % 4 ? 70 : 104, ratchet: s % 16 === 14 ? 3 : 1, cond: s % 8 === 6 ? "70%" : ""});
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

	// A step's content: the Hit (opacity = velocity, shifted by its micro-timing), its ratchets as ticks, a dot for
	// Locks, its Condition as a badge (room enough) or a bar
	function drawStep(cell, st, w, badge) {
		if (!st)
			return;
		const shift = Math.round(st.micro / 23 * w * 0.3);
		const box = document.createElement("div");
		box.className = "hitbox";
		box.style.opacity = (0.35 + 0.65 * st.vel / 127).toFixed(2);
		box.style.transform = "translateX(" + shift + "px)";
		cell.append(box);
		for (let r = 1; r < st.ratchet; ++r) {
			const tick = document.createElement("div");
			tick.className = "tick";
			tick.style.left = (3 + shift + Math.round((w - 6) * r / st.ratchet)) + "px";
			cell.append(tick);
		}
		if (Object.keys(st.locks).length) {
			const dot = document.createElement("div");
			dot.className = "lockdot";
			cell.append(dot);
		}
		if (st.cond) {
			const c = document.createElement("div");
			c.className = badge ? "cond" : "condbar";
			if (badge)
				c.textContent = st.cond;
			cell.append(c);
		}
		cell.title = "vel " + st.vel + (st.cond ? ", " + st.cond : "") + (st.micro ? ", micro " + st.micro : "")
			+ (st.ratchet > 1 ? ", x" + st.ratchet : "") + (Object.keys(st.locks).length ? ", Locks" : "");
	}

	function dial(A, x, y, kit, lock, id) {
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
		const locked = lock !== undefined;
		path(arc(127, 15), "stroke: var(--line); stroke-width: 3");
		if (kit > 0)
			path(arc(kit, 15), "stroke: var(--text-faint); stroke-width: " + (locked ? 1 : 3));
		if (locked && lock > 0)
			path(arc(lock, 15), "stroke: var(--accent); stroke-width: 3");
		const [nx, ny] = pt(-135 + 270 * (locked ? lock : kit) / 127, 11);
		path("M 20 20 L " + nx + " " + ny, "stroke: var(--text); stroke-width: 2");
		d.append(svg);
		return d;
	}

	function strip(container, o) {
		const p = o.prefix || "";
		const A = device(container, 992, 169, "md-drums-seq");
		el(A, "label", 8, 6, 140, 12, "Pattern");
		for (let i = 0; i < 16; ++i) {
			const b = el(A, "btn", 8 + 36 * (i % 4), 22 + 28 * Math.floor(i / 4), 32, 24, pad(i + 1), p + "pattern-" + (i + 1));
			b.classList.add(i === 0 ? "on" : i === 1 ? "queued" : USED.has(i) ? "x" : "empty");
		}
		el(A, "hint", 8, 136, 148, 12, "02 next, after step 64");
		el(A, "hint", 8, 150, 148, 12, "64 steps, 1/16");
		el(A, "rule", 160, 8, 1, 153);

		el(A, "label", 172, 6, 300, 12, "Track");
		const page = 2;	// steps 33-48: the playing page
		for (let t = 0; t < 16; ++t) {
			const x = 172 + 40 * t;
			const b = el(A, "btn track" + (t === SEL_T ? " on" : ""), x, 22, 36, 24, null, p + "track-" + (t + 1));
			const num = document.createElement("span");
			num.className = "num";
			num.textContent = pad(t + 1);
			const name = document.createElement("span");
			name.className = "name";
			name.textContent = short(MACHINES[t][0]);
			b.append(num, name);
			el(A, "btn mute" + (t === MUTED ? " muted" : ""), x, 50, 36, 16, o.family === "live" ? String(t + 1) : "M",
				p + "mute-" + (t + 1));
			const s = 16 * page + t;
			const cell = el(A, "step" + (t % 4 === 0 ? " beat" : "") + (s === PLAY ? " play" : ""), x, 74, 36, 40, null,
				p + "step-" + (t + 1));
			drawStep(cell, P[SEL_T][s], 36, true);
		}
		for (let pg = 0; pg < 4; ++pg)
			el(A, "btn" + (pg === page ? " on" : ""), 172 + 160 * pg, 122, 156, 16, (16 * pg + 1) + "-" + (16 * pg + 16),
				p + "page-" + (pg + 1));
		el(A, "hint", 172, 146, 636, 12, "Track 01 TRX-BD, page 3; click a step to toggle its Hit, Edit for the rest");
		el(A, "rule", 816, 8, 1, 153);

		el(A, "btn big", 828, 22, 72, 64, "FILL", p + "fill");
		el(A, "btn", 908, 22, 72, 24, "Edit", p + "edit");
		el(A, "label", 908, 52, 72, 12, "Capture");
		el(A, "btn", 908, 70, 72, 16, "Session", p + "capture");
		el(A, "hint", 828, 96, 152, 40, "Arrangement when Live records");
	}

	function editWindow(container, o) {
		const p = o.prefix || "";
		const density = o.density === 32 ? 32 : 64;
		const first = density === 64 ? 0 : 32;
		const A = device(container, 1440, 840, "md-drums-seq: Edit");

		el(A, "label", 16, 22, 72, 12, "Pattern");
		for (let i = 0; i < 16; ++i) {
			const b = el(A, "btn", 96 + 32 * i, 16, 28, 24, pad(i + 1), p + "win-pattern-" + (i + 1));
			b.classList.add(i === 0 ? "on" : i === 1 ? "queued" : USED.has(i) ? "x" : "empty");
		}
		el(A, "label", 628, 22, 48, 12, "Length");
		el(A, "menu", 680, 16, 72, 24, "64", p + "length");
		el(A, "label", 768, 22, 40, 12, "Scale");
		el(A, "menu", 812, 16, 64, 24, "1/16", p + "scale");
		el(A, "btn big", 900, 16, 64, 24, "FILL", p + "win-fill");
		el(A, "btn", 980, 16, 128, 24, "Print 4 passes", p + "print");
		el(A, "btn", 1124, 16, 96, 24, "Session", p + "win-capture");
		el(A, "rule", 0, 52, 1440, 1);

		const pitch = density === 64 ? 19 : 38, cw = pitch - 2, x0 = 176;
		const colX = s => x0 + pitch * (s - first) + 4 * Math.floor((s - first) / 16);
		for (let s = first; s < first + density; s += 4)
			el(A, "hint", colX(s), 66, pitch * 4, 12, String(s + 1));
		if (PLAY >= first && PLAY < first + density)
			el(A, "col-play", colX(PLAY), 84, cw, 28 * 16);
		for (let t = 0; t < 16; ++t) {
			const y = 88 + 28 * t;
			el(A, "hint", 16, y + 6, 20, 12, pad(t + 1));
			el(A, t === SEL_T ? "title-l" : "dialname", 40, y + 5, 92, 14, MACHINES[t][0]).style.fontSize = "var(--fs-m)";
			el(A, "btn mute" + (t === MUTED ? " muted" : ""), 136, y, 24, 24, o.family === "live" ? String(t + 1) : "M",
				p + "win-mute-" + (t + 1));
			for (let s = first; s < first + density; ++s) {
				const cls = "step" + ((s % 4 === 0) ? " beat" : "") + (s === PLAY ? " play" : "")
					+ (t === SEL_T && s === SEL_S ? " sel" : "");
				const cell = el(A, cls, colX(s), y, cw, 24, null, p + "cell-" + (t + 1) + "-" + (s + 1));
				drawStep(cell, P[t][s], cw, density === 32);
			}
		}

		el(A, "rule", 0, 548, 1440, 1);
		const st = P[SEL_T][SEL_S];
		el(A, "title-l", 16, 564, 320, 24, "Track 01 TRX-BD");
		el(A, "hint", 16, 592, 320, 12, "Step 47, page 3");
		el(A, "btn on", 16, 616, 64, 24, "Hit", p + "step-hit");
		dial(A, 96, 608, st.vel, undefined, p + "step-velocity");
		el(A, "dialname", 144, 614, 80, 12, "Velocity");
		el(A, "dialval", 144, 630, 80, 14, String(st.vel)).style.color = "var(--text)";
		el(A, "label", 16, 664, 140, 12, "Condition");
		el(A, "menu", 16, 680, 140, 24, st.cond || "None", p + "step-condition");
		el(A, "label", 172, 664, 168, 12, "Micro-timing");
		const sl = el(A, "slider", 172, 680, 168, 24, null, p + "step-microtiming");
		const centre = document.createElement("div");
		centre.className = "centre";
		const val = document.createElement("div");
		val.className = "val";
		val.style.left = "50%";
		val.style.width = Math.round(st.micro / 23 * 82) + "px";
		sl.append(centre, val);
		el(A, "hint", 172, 708, 168, 12, "+" + st.micro + " of 23");
		el(A, "label", 16, 728, 140, 12, "Ratchets");
		el(A, "menu", 16, 744, 64, 24, "x" + st.ratchet, p + "step-ratchets");
		el(A, "menu", 88, 744, 68, 24, "1/2", p + "step-ratchet-rate");
		el(A, "hint", 16, 780, 324, 40, "A value changed here locks this step. Alt-click a Lock to clear it.");
		el(A, "rule", 352, 564, 1, 248);

		const groups = [["SYN", MACHINES[SEL_T][1]], ["EFX", EFX], ["ROUTING", ROUTING]];
		groups.forEach(([g, names], gi) => {
			const y = 568 + 88 * gi;
			el(A, "label", 368, y + 14, 72, 12, g);
			names.forEach((n, i) => {
				const idx = gi * 8 + i, x = 448 + 120 * i;
				const kit = kitValue(SEL_T, idx), lock = st.locks[idx], locked = lock !== undefined;
				dial(A, x, y, kit, lock, p + "lock-" + (idx + 1));
				el(A, "dialname" + (locked ? " locked" : ""), x + 48, y + 6, 64, 12, n || "unused");
				el(A, "dialval" + (locked ? " locked" : ""), x + 48, y + 22, 72, 14, locked ? lock + " (kit " + kit + ")" : String(kit));
			});
		});
	}

	window.mdSeqMock = {strip, editWindow};
})();
