// The user's two kicks at Main: the 909 of "1 KICK" (through DRUM BUS) played by a clip, MD Drums played by a clip,
// MD Drums played by MD Drums Seq; one bar apart, recorded together from Main (Resampling) on a temporary track.
//   bar 1: the 909 (a temporary clip "MDSEQ KICK TEST" on 1 KICK, note 60 at beat 0, 16 beats long)
//   bar 2: MD Drums Track 1 from a clip on "MDSEQ PRINT" (note 36 at beat 4, 16 beats long)
//   bar 3: MD Drums Track 1 from the device (its demonstration Pattern: 64 steps, Track 1 at step 49)
// Everything starts at the bar after the transport starts. Prints the WAV's path; then deletes the temporary clip on
// 1 KICK (by its name), MDSEQ PRINT and MDSEQ REC, and puts the loop and the position back.
//   node live-three-kicks.mjs <MD Drums path> <device path> <slot>
import fs from "node:fs";
import path from "node:path";
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [drumsPath, seqPath, slotArg] = process.argv.slice(2);
const slot = Number(slotArg);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = async (p, prop) =>
{
	try { return await live.request("get", { path: p, prop }); }
	catch(error) { await new Promise(r => setTimeout(r, 2000)); return live.request("get", { path: p, prop }); }
};
const call = (p, method, args = []) => live.request("call", { path: p, method, args });
const set = (p, prop, value) => live.request("set", { path: p, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));
const names = async () => (await get("live_set", "tracks")).map(t => t.name);

async function pick(track, available, wanted)
{
	const count = (await get(track, available)).length;
	const seen = [];
	for(let i = 0; i < count; ++i)
	{
		const name = await get(`${track} ${available} ${i}`, "display_name");
		seen.push(name);
		if(name === wanted)
			return i;
	}
	throw new Error(`${available}: no ${wanted} in ${seen.join(" | ")}`);
}

async function newTrack(kind, index, name)
{
	await call("live_set", kind, [index]);
	const track = `live_set tracks ${index}`;
	await set(track, "name", name);
	if(await get(track, "name") !== name)
		throw new Error("track " + index + " not named " + name);
	return track;
}

async function testClip(track, notes, name)
{
	const slotPath = `${track} clip_slots ${slot}`;
	if(await get(slotPath, "has_clip"))
		throw new Error(slotPath + " is not empty");
	await call(slotPath, "create_clip", [16.0]);
	const clip = slotPath + " clip";
	await set(clip, "name", name);
	await live.request("notes_add", { path: clip, notes });
	return slotPath;
}

let base = -1;
let kickSlot = null;
const position = await get("live_set", "current_song_time");
const loop = await get("live_set", "loop");
try
{
	if(await get("live_set", "is_playing"))
		throw new Error("Live is playing: nothing done");
	const before = await names();
	if(before[0] !== "1 KICK" || before.some(n => n.startsWith("MDSEQ ")))
		throw new Error("unexpected tracks: " + before.join(" | "));
	base = before.length;
	console.log("tempo", await get("live_set", "tempo"));

	kickSlot = await testClip("live_set tracks 0", [{ pitch: 60, start_time: 0, duration: 0.25, velocity: 120 }],
		"MDSEQ KICK TEST");

	const device = await newTrack("create_midi_track", base, "MDSEQ PRINT");
	await set("live_set view", "selected_track", { __path__: device });
	await call("live_app browser", "load_item", [{ __path__: drumsPath }]);
	await sleep(5000);
	await call("live_app browser", "load_item", [{ __path__: seqPath }]);
	await sleep(6000);
	console.log("MDSEQ PRINT devices", (await get(device, "devices")).map(d => d.name).join(", "));
	const mdSlot = await testClip(device, [{ pitch: 36, start_time: 4, duration: 0.25, velocity: 120 }], "MDSEQ MD CLIP");

	const rec = await newTrack("create_audio_track", base + 1, "MDSEQ REC");
	await set(rec, "input_routing_type", { __path__: `${rec} available_input_routing_types ${await pick(rec, "available_input_routing_types", "Resampling")}` });
	await set(rec, "current_monitoring_state", 2);
	await set(rec, "arm", 1);

	await set("live_set", "loop", 0);
	await set("live_set", "current_song_time", 0);
	await call("live_set", "start_playing");
	while(await get("live_set", "current_song_time") < 0.5)
		await sleep(20);
	await call(kickSlot, "fire");
	await call(mdSlot, "fire");
	await call(`${rec} clip_slots ${slot}`, "fire");
	const firstBar = Math.floor(await get("live_set", "current_song_time") / 4 + 1) * 4;
	while(await get("live_set", "current_song_time") < firstBar + 13)
		await sleep(50);
	await call(`${rec} clip_slots ${slot}`, "stop");
	await call(kickSlot, "stop");
	await call(mdSlot, "stop");
	await call("live_set", "stop_playing");
	await sleep(1000);
	await set(rec, "arm", 0);
	const file = await get(`${rec} clip_slots ${slot} clip`, "file_path");
	console.log("recorded from beat", firstBar, "file", file);
	await call(`${rec} clip_slots ${slot}`, "delete_clip");
	await sleep(500);
	const out = path.join(process.env.TEMP, "claude", "mdseq-three-kicks.wav");
	fs.mkdirSync(path.dirname(out), { recursive: true });
	fs.copyFileSync(file, out);
	console.log("copied", out);
}
catch(error)
{
	console.error(String(error?.message || error));
	process.exitCode = 1;
}
finally
{
	try
	{
		if(await get("live_set", "is_playing"))
			await call("live_set", "stop_playing");
		await set("live_set", "loop", loop ? 1 : 0);
		await set("live_set", "current_song_time", position);
		if(kickSlot && await get(kickSlot, "has_clip") && await get(kickSlot + " clip", "name") === "MDSEQ KICK TEST")
			await call(kickSlot, "delete_clip");
		const list = await names();
		for(let i = list.length - 1; i >= 0; --i)
			if(base >= 0 && i >= base && (list[i] === "MDSEQ PRINT" || list[i] === "MDSEQ REC"))
				await call("live_set", "delete_track", [i]);
		console.log("tracks now:", (await names()).join(" | "), "; 1 KICK slot", slot, "has clip:",
			await get(`live_set tracks 0 clip_slots ${slot}`, "has_clip"), "; loop", await get("live_set", "loop"));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
