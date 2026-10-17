// Swing heard from the device in Live: "MDSEQ PRINT" (MD Drums, then a development MD Drums Seq whose demonstration
// Pattern swings), and "MDSEQ MIDIREC", a MIDI track recording what the device sends (MDSEQ PRINT, Post FX) into a
// Session clip for two bars from the set's start. Prints the notes recorded, in beats, then deletes both tracks,
// checking their names, and puts the song position back.
//   node live-swing-run.mjs <browser path of MD Drums> <browser path of the development device> [bars]
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [drumsPath, seqPath, barsArg] = process.argv.slice(2);
const bars = Number(barsArg || 2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = async (path, prop) =>
{
	try
	{
		return await live.request("get", { path, prop });
	}
	catch(error)
	{
		await new Promise(r => setTimeout(r, 2000));
		return live.request("get", { path, prop });
	}
};
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
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

async function newMidiTrack(index, name)
{
	await call("live_set", "create_midi_track", [index]);
	const track = `live_set tracks ${index}`;
	await set(track, "name", name);
	if(await get(track, "name") !== name)
		throw new Error("track " + index + " not named " + name);
	return track;
}

let base = -1;
const position = await get("live_set", "current_song_time");
try
{
	if(await get("live_set", "is_playing"))
		throw new Error("Live is playing: nothing done");
	const before = await names();
	if(before.some(n => n.startsWith("MDSEQ ")))
		throw new Error("MDSEQ tracks already in the set");
	base = before.length;
	const tempo = await get("live_set", "tempo");
	console.log("tempo", tempo);

	const device = await newMidiTrack(base, "MDSEQ PRINT");
	await set("live_set view", "selected_track", { __path__: device });
	await call("live_app browser", "load_item", [{ __path__: drumsPath }]);
	await sleep(5000);
	await call("live_app browser", "load_item", [{ __path__: seqPath }]);
	await sleep(6000);
	const devices = (await get(device, "devices")).map(d => d.name);
	console.log("devices", devices.join(", "));

	const rec = await newMidiTrack(base + 1, "MDSEQ MIDIREC");
	await set(rec, "input_routing_type", { __path__: `${rec} available_input_routing_types ${await pick(rec, "available_input_routing_types", "MDSEQ PRINT")}` });
	await sleep(300);
	await set(rec, "input_routing_channel", { __path__: `${rec} available_input_routing_channels ${await pick(rec, "available_input_routing_channels", "Post FX")}` });
	await set(rec, "current_monitoring_state", 2);
	await set(rec, "arm", 1);
	console.log("MIDIREC in", await get(rec + " input_routing_type", "display_name"), "/",
		await get(rec + " input_routing_channel", "display_name"));

	await set("live_set", "current_song_time", 0);
	await call("live_set", "start_playing");
	while(await get("live_set", "current_song_time") < 0.5)
		await sleep(20);
	await call(`${rec} clip_slots 0`, "fire");
	const firstBar = Math.floor(await get("live_set", "current_song_time") / 4 + 1) * 4;
	while(await get("live_set", "current_song_time") < firstBar + 4 * bars + 0.2)
		await sleep(50);
	await call(`${rec} clip_slots 0`, "stop");
	await call("live_set", "stop_playing");
	await sleep(800);
	await set(rec, "arm", 0);

	if(!await get(`${rec} clip_slots 0`, "has_clip"))
		throw new Error("nothing recorded");
	const clip = `${rec} clip_slots 0 clip`;
	const notes = (await live.request("notes_get", { path: clip })).notes
		.sort((a, b) => a.start_time - b.start_time);
	console.log("recorded from beat", firstBar, "clip length", await get(clip, "length"), notes.length, "notes");
	for(const n of notes)
		console.log(`  ${n.pitch} at ${n.start_time.toFixed(4)} for ${n.duration.toFixed(4)} v${n.velocity}`);
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
		await set("live_set", "current_song_time", position);
		const list = await names();
		for(let i = list.length - 1; i >= 0; --i)
			if(base >= 0 && i >= base && (list[i] === "MDSEQ PRINT" || list[i] === "MDSEQ MIDIREC"))
				await call("live_set", "delete_track", [i]);
		console.log("tracks now:", (await names()).join(" | "));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
