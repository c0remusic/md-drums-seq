// A short loop brace at a slow scale: "MDSEQ PRINT" (MD Drums, then a development MD Drums Seq whose demonstration
// Pattern is set by %TEMP%\mdseq-demo.txt) plays inside the arrangement loop [0, <loop beats>) while "MDSEQ MIDIREC"
// records what it sends (Post FX) for <seconds>. Clip launch quantization is set to None for the run. Prints the
// notes and their spacing, then deletes both tracks and puts the loop, the quantization and the position back.
//   node live-loop-run.mjs <MD Drums path> <device path> <loop beats> <seconds>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [drumsPath, seqPath, loopArg, secondsArg] = process.argv.slice(2);
const loopBeats = Number(loopArg);
const seconds = Number(secondsArg || 4);
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
	for(let i = 0; i < count; ++i)
		if(await get(`${track} ${available} ${i}`, "display_name") === wanted)
			return i;
	throw new Error(`${available}: no ${wanted}`);
}

async function newMidiTrack(index, name)
{
	await call("live_set", "create_midi_track", [index]);
	const track = `live_set tracks ${index}`;
	await set(track, "name", name);
	if(await get(track, "name") !== name)
		throw new Error("track " + index + " not named " + name);
	return track;
}

const saved = {
	position: await get("live_set", "current_song_time"),
	loop: await get("live_set", "loop"),
	loopStart: await get("live_set", "loop_start"),
	loopLength: await get("live_set", "loop_length"),
	quantization: await get("live_set", "clip_trigger_quantization"),
};
let base = -1;
try
{
	if(await get("live_set", "is_playing"))
		throw new Error("Live is playing: nothing done");
	const before = await names();
	if(before.some(n => n.startsWith("MDSEQ ")))
		throw new Error("MDSEQ tracks already in the set");
	base = before.length;

	const device = await newMidiTrack(base, "MDSEQ PRINT");
	await set("live_set view", "selected_track", { __path__: device });
	await call("live_app browser", "load_item", [{ __path__: drumsPath }]);
	await sleep(5000);
	await call("live_app browser", "load_item", [{ __path__: seqPath }]);
	await sleep(6000);
	const rec = await newMidiTrack(base + 1, "MDSEQ MIDIREC");
	await set(rec, "input_routing_type", { __path__: `${rec} available_input_routing_types ${await pick(rec, "available_input_routing_types", "MDSEQ PRINT")}` });
	await sleep(300);
	await set(rec, "input_routing_channel", { __path__: `${rec} available_input_routing_channels ${await pick(rec, "available_input_routing_channels", "Post FX")}` });
	await set(rec, "current_monitoring_state", 2);
	await set(rec, "arm", 1);

	await set("live_set", "clip_trigger_quantization", 0);
	await set("live_set", "loop_start", 0);
	await set("live_set", "loop_length", loopBeats);
	await set("live_set", "loop", 1);
	await set("live_set", "current_song_time", 0);
	console.log("loop", await get("live_set", "loop_start"), "+", await get("live_set", "loop_length"), "beats; tempo",
		await get("live_set", "tempo"));
	await call(`${rec} clip_slots 0`, "fire");
	await call("live_set", "start_playing");
	await sleep(seconds * 1000);
	await call(`${rec} clip_slots 0`, "stop");
	await call("live_set", "stop_playing");
	await sleep(800);
	await set(rec, "arm", 0);
	if(!await get(`${rec} clip_slots 0`, "has_clip"))
		throw new Error("nothing recorded");
	const notes = (await live.request("notes_get", { path: `${rec} clip_slots 0 clip` })).notes
		.sort((a, b) => a.start_time - b.start_time);
	console.log(notes.length, "notes");
	let previous = null;
	for(const n of notes)
	{
		console.log(`  ${n.pitch} at ${n.start_time.toFixed(4)} for ${n.duration.toFixed(4)}` +
			(previous === null ? "" : `, ${(n.start_time - previous).toFixed(4)} after the previous`));
		previous = n.start_time;
	}
	await call(`${rec} clip_slots 0`, "delete_clip");
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
		// The tracks first, so that nothing below keeps them in the set.
		const list = await names();
		for(let i = list.length - 1; i >= 0; --i)
			if(base >= 0 && i >= base && (list[i] === "MDSEQ PRINT" || list[i] === "MDSEQ MIDIREC"))
				await call("live_set", "delete_track", [i]);
		await set("live_set", "clip_trigger_quantization", saved.quantization);
		// Live refuses a loop or a position past the song's length, which a short loop has shortened: the loop grows
		// back a song's length at a time, since Live lengthens the song a little past the loop's end each time.
		await set("live_set", "loop_start", saved.loopStart);
		for(let i = 0; i < 64 && await get("live_set", "loop_length") < saved.loopLength; ++i)
			await set("live_set", "loop_length", Math.min(saved.loopLength,
				await get("live_set", "song_length") - saved.loopStart));
		await set("live_set", "loop", saved.loop ? 1 : 0);
		await set("live_set", "current_song_time", saved.position);
		console.log("tracks now:", (await names()).join(" | "), "; loop", await get("live_set", "loop_start"), "+",
			await get("live_set", "loop_length"), "; quantization", await get("live_set", "clip_trigger_quantization"));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
