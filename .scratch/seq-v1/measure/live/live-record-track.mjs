// Records one track's output for a few bars, through a temporary audio track, and leaves the WAV readable.
//   node live-record-track.mjs <source track name> <bars>
// Adds "MDSEQ REC" at the end (input: the source, Post FX, monitoring off, armed), plays from the set's start,
// records in Session from the next bar line, stops, puts the position back, prints the file, deletes the recording
// clip (Live keeps a file locked while a clip uses it) and the track, checking its name first.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [source, barsArg] = process.argv.slice(2);
const bars = Number(barsArg || 2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));
const NAME = "MDSEQ REC";

async function names()
{
	return (await get("live_set", "tracks")).map(t => t.name);
}

async function pick(track, available, wanted)
{
	const count = (await get(track, available)).length;
	for(let i = 0; i < count; ++i)
		if(await get(`${track} ${available} ${i}`, "display_name") === wanted)
			return i;
	throw new Error(`${available}: no ${wanted}`);
}

let start = null;
let rec = -1;
try
{
	if(!(await names()).includes(source))
		throw new Error("no track " + source);
	await call("live_set", "create_audio_track", [-1]);
	rec = (await names()).length - 1;
	const track = `live_set tracks ${rec}`;
	await set(track, "name", NAME);
	const type = await pick(track, "available_input_routing_types", source);
	await set(track, "input_routing_type", { __path__: `${track} available_input_routing_types ${type}` });
	const channel = await pick(track, "available_input_routing_channels", "Post FX");
	await set(track, "input_routing_channel", { __path__: `${track} available_input_routing_channels ${channel}` });
	await set(track, "current_monitoring_state", 2);
	await set(track, "arm", 1);

	start = await get("live_set", "current_song_time");
	await set("live_set", "current_song_time", 0);
	await call("live_set", "start_playing");
	while(await get("live_set", "current_song_time") < 0.5)
		await sleep(20);
	await call(`${track} clip_slots 0`, "fire");	// starts at the next bar line, beat 4
	while(await get("live_set", "current_song_time") < 4 + 4 * bars + 0.5)
		await sleep(50);
	await call("live_set", "stop_playing");
	await sleep(500);
	await set(track, "arm", 0);
	const clip = `${track} clip_slots 0 clip`;
	console.log("file", await get(clip, "file_path"));
	await call(`${track} clip_slots 0`, "delete_clip");
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
		await call("live_set", "stop_playing");
		if(start !== null)
			await set("live_set", "current_song_time", start);
		if(rec >= 0 && await get(`live_set tracks ${rec}`, "name") === NAME)
			await call("live_set", "delete_track", [rec]);
		console.log("tracks now:", (await names()).join(" | "));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
