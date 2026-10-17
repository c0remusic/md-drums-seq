// Records the measurement: plays from the set's start, launches the reference clip and the two recordings at the
// next bar, stops after about four bars, puts the position back, and prints the recorded files.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function indexOf(name)
{
	const index = (await get("live_set", "tracks")).findIndex(t => t.name === name);
	if(index < 0)
		throw new Error("no track " + name);
	return index;
}

let start = null;
try
{
	const ref = await indexOf("MDSEQ REF");
	const recProbe = await indexOf("MDSEQ REC PROBE");
	const recRef = await indexOf("MDSEQ REC REF");
	for(const t of [recProbe, recRef])
		if(await get(`live_set tracks ${t} clip_slots 0`, "has_clip"))
			throw new Error("slot 0 of track " + t + " is not empty");

	start = await get("live_set", "current_song_time");
	await set("live_set", "current_song_time", 0);
	await call("live_set", "start_playing");
	while(await get("live_set", "current_song_time") < 0.5)
		await sleep(20);
	// All three launch together at the next bar line, beat 4.
	await call(`live_set tracks ${ref} clip_slots 0`, "fire");
	await call(`live_set tracks ${recProbe} clip_slots 0`, "fire");
	await call(`live_set tracks ${recRef} clip_slots 0`, "fire");
	while(await get("live_set", "current_song_time") < 21.0)
		await sleep(50);
	await call("live_set", "stop_playing");
	await sleep(500);
	for(const t of [recProbe, recRef])
	{
		const clip = `live_set tracks ${t} clip_slots 0 clip`;
		console.log(await get(`live_set tracks ${t}`, "name"), "file", await get(clip, "file_path"), "length",
			await get(clip, "length"), "start marker", await get(clip, "start_marker"));
	}
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
		console.log("stopped, position back to", await get("live_set", "current_song_time"));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
