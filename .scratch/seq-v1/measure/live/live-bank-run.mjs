// Records a track's output while a device parameter changes during playback, for the ticket "Banque de Patterns":
// the Pattern chosen through the Live parameter "Pattern" mid-Pass must follow at the end of the Pass.
//   node live-bank-run.mjs <track name> <device name> <parameter name> <value> <beat to set it> <bars>
// Like live-record-track.mjs: "MDSEQ REC" at the end (input: the track, Post FX, monitoring off, armed), plays from the
// set's start, records in Session from the next bar line (beat 4) for <bars> bars, sets the parameter when the song
// reaches <beat>, stops, puts the position and the parameter back, prints the file, deletes the recording clip and
// the track, checking its name first.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [source, deviceName, paramName, valueArg, beatArg, barsArg] = process.argv.slice(2);
const value = Number(valueArg);
const setAt = Number(beatArg);
const bars = Number(barsArg || 3);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, v) => live.request("set", { path, prop, value: v });
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

async function parameterPath()
{
	const tracks = await names();
	const t = tracks.indexOf(source);
	if(t < 0)
		throw new Error("no track " + source);
	const devices = await get(`live_set tracks ${t}`, "devices");
	for(let d = 0; d < devices.length; ++d)
	{
		if(await get(`live_set tracks ${t} devices ${d}`, "name") !== deviceName)
			continue;
		const params = await get(`live_set tracks ${t} devices ${d}`, "parameters");
		for(let p = 0; p < params.length; ++p)
			if(await get(`live_set tracks ${t} devices ${d} parameters ${p}`, "name") === paramName)
				return `live_set tracks ${t} devices ${d} parameters ${p}`;
	}
	throw new Error(`no parameter ${paramName} on ${deviceName}`);
}

let start = null;
let rec = -1;
let param = null;
let before = null;
try
{
	param = await parameterPath();
	before = await get(param, "value");
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
	while(await get("live_set", "current_song_time") < setAt)
		await sleep(10);
	await set(param, "value", value);
	console.log("parameter set at beat", (await get("live_set", "current_song_time")).toFixed(3));
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
		if(param !== null && before !== null)
			await set(param, "value", before);
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
