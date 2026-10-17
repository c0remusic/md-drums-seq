// Measures where MD Drums' Hits land against the grid, played by MD Drums Seq and by a Live clip, recorded together.
//   node live-kick-latency.mjs <browser path of MD Drums> <browser path of a development MD Drums Seq> [bars]
// Builds at the end of the set, all named "MDSEQ ...":
//   MDSEQ SEQ      MIDI: MD Drums Seq (its demonstration Pattern from %TEMP%\mdseq-demo.txt), then MD Drums
//   MDSEQ CLIP     MIDI: MD Drums, a one-bar clip with the same Hits
//   MDSEQ REC SEQ, MDSEQ REC CLIP   audio: input from each, Post FX, monitoring off, armed
// The demonstration file this expects: Track 1 on steps 1 and 9, Track 2 on 5 and 13 (the clip: 36 on beats 1 and 3,
// 37 on 2 and 4). If Live is stopped it plays from the set's start, launches the clip and the recordings at the next
// bar line, then stops and puts the position back; if Live is playing it leaves the transport alone. It prints both
// files, then deletes the recording clips (Live keeps a file locked while a clip uses it) and every "MDSEQ " track,
// checking names first. Analyse each file with ../onsets.py --bpm <tempo> --division 4.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [drumsPath, seqPath, barsArg] = process.argv.slice(2);
const bars = Number(barsArg || 2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function names()
{
	return (await get("live_set", "tracks")).map(t => t.name);
}

async function newTrack(kind, name)
{
	await call("live_set", kind === "midi" ? "create_midi_track" : "create_audio_track", [-1]);
	const index = (await names()).length - 1;
	const track = `live_set tracks ${index}`;
	await set(track, "name", name);
	if(await get(track, "name") !== name)
		throw new Error("track " + index + " not named " + name);
	return track;
}

async function load(track, item, wait)
{
	await set("live_set view", "selected_track", { __path__: track });
	await call("live_app browser", "load_item", [{ __path__: item }]);
	await sleep(wait);
}

async function deviceNames(track)
{
	const list = await get(track, "devices");
	const result = [];
	for(let d = 0; d < list.length; ++d)
		result.push(await get(`${track} devices ${d}`, "name"));
	return result;
}

async function pick(track, available, wanted)
{
	const count = (await get(track, available)).length;
	for(let i = 0; i < count; ++i)
		if(await get(`${track} ${available} ${i}`, "display_name") === wanted)
			return i;
	throw new Error(`${available}: no ${wanted}`);
}

async function recorder(name, source)
{
	const track = await newTrack("audio", name);
	const type = await pick(track, "available_input_routing_types", source);
	await set(track, "input_routing_type", { __path__: `${track} available_input_routing_types ${type}` });
	const channel = await pick(track, "available_input_routing_channels", "Post FX");
	await set(track, "input_routing_channel", { __path__: `${track} available_input_routing_channels ${channel}` });
	await set(track, "current_monitoring_state", 2);
	await set(track, "arm", 1);
	return track;
}

let wasPlaying = null;
let start = null;
try
{
	if((await names()).some(n => n.startsWith("MDSEQ ")))
		throw new Error("MDSEQ tracks already in the set");

	const seq = await newTrack("midi", "MDSEQ SEQ");
	await load(seq, drumsPath, 5000);
	await load(seq, seqPath, 3000);
	console.log("SEQ:", (await deviceNames(seq)).join(" | "));

	const clip = await newTrack("midi", "MDSEQ CLIP");
	await load(clip, drumsPath, 5000);
	console.log("CLIP:", (await deviceNames(clip)).join(" | "));
	await call(`${clip} clip_slots 0`, "create_clip", [4]);
	const notes = [0, 2].map(b => ({ pitch: 36, start_time: b, duration: 0.25, velocity: 120 }))
		.concat([1, 3].map(b => ({ pitch: 37, start_time: b, duration: 0.25, velocity: 120 })));
	await live.request("notes_add", { path: `${clip} clip_slots 0 clip`, notes });

	// What MD Drums tells Live: its latency, and the parameters naming Track 1's and 2's machines when Live shows them.
	const drums = (await deviceNames(clip)).indexOf("MD Drums");
	const device = `${clip} devices ${drums}`;
	console.log("MD Drums latency", await get(device, "latency_in_samples"), "samples");
	const params = await get(device, "parameters");
	console.log("MD Drums parameters in the LOM:", params.length);
	for(let p = 0; p < params.length; ++p)
	{
		const name = await get(`${device} parameters ${p}`, "name");
		if(/mach/i.test(name))
			console.log(" ", p, name, await get(`${device} parameters ${p}`, "str_for_value",).catch(() => ""),
				await get(`${device} parameters ${p}`, "value"));
	}

	const recorders = [await recorder("MDSEQ REC SEQ", "MDSEQ SEQ"), await recorder("MDSEQ REC CLIP", "MDSEQ CLIP")];

	wasPlaying = await get("live_set", "is_playing");
	start = await get("live_set", "current_song_time");
	if(!wasPlaying)
	{
		await set("live_set", "current_song_time", 0);
		await call("live_set", "start_playing");
		while(await get("live_set", "current_song_time") < 0.5)
			await sleep(20);
	}
	for(const t of [clip, ...recorders])
		await call(`${t} clip_slots 0`, "fire");	// from the next bar line
	const now = await get("live_set", "current_song_time");
	const firstBar = Math.floor(now / 4 + 1) * 4;
	while(await get("live_set", "current_song_time") < firstBar + 4 * bars + 0.5)
		await sleep(50);
	for(const t of [...recorders, clip])
		await call(`${t} clip_slots 0`, "stop");
	if(!wasPlaying)
		await call("live_set", "stop_playing");
	await sleep(800);
	for(const r of recorders)
	{
		await set(r, "arm", 0);
		console.log("file", await get(`${r} clip_slots 0 clip`, "file_path"));
		await call(`${r} clip_slots 0`, "delete_clip");
	}
	console.log("recorded from beat", firstBar, "tempo", await get("live_set", "tempo"));
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
		if(wasPlaying === false)
		{
			await call("live_set", "stop_playing");
			if(start !== null)
				await set("live_set", "current_song_time", start);
		}
		const list = await names();
		for(let i = list.length - 1; i >= 0; --i)
			if(list[i].startsWith("MDSEQ ") && await get(`live_set tracks ${i}`, "name") === list[i])
				await call("live_set", "delete_track", [i]);
		console.log("tracks now:", (await names()).join(" | "));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
