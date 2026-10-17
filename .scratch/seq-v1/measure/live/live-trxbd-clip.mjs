// TRX-BD from a clip: a temporary clip in scene 1 of MD DRUMS SEQ plays Track 1 on every beat with the device off,
// Main recorded as live-user-kicks.mjs does; the clip removed after. Built from live-user-kicks.mjs:
// The user's track "MD DRUMS SEQ" (MD Drums Seq then MD Drums, a clip in scene 0): Main recorded (Resampling) on a
// temporary track "MDSEQ REC" for two bars, three times: the clip and the device together, the clip alone (the
// device off), the device alone (no clip). Each run starts the transport at 0 and launches at the next bar. Copies
// the three WAVs to %TEMP%\claude, then deletes MDSEQ REC and puts the device's On, the loop and the position back.
import fs from "node:fs";
import path from "node:path";
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

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

const position = await get("live_set", "current_song_time");
const loop = await get("live_set", "loop");
let seqOn = null;
let onParam = null;
let rec = null;
let tempClip = null;
try
{
	if(await get("live_set", "is_playing"))
		throw new Error("Live is playing: nothing done");
	const list = await names();
	const index = list.indexOf("MD DRUMS SEQ");
	if(index < 0 || list.some(n => n.startsWith("MDSEQ ")))
		throw new Error("unexpected tracks: " + list.join(" | "));
	const track = `live_set tracks ${index}`;
	const devices = (await get(track, "devices")).map(d => d.name);
	if(devices[0] !== "MD Drums Seq" || devices[1] !== "MD Drums")
		throw new Error("unexpected devices: " + devices.join(", "));
	// A temporary clip in scene 1 of the user's track: Track 1 (note 36) on every beat, played with the device off.
	if(await get(`${track} clip_slots 1`, "has_clip"))
		throw new Error("scene 1 of MD DRUMS SEQ is not empty");
	await call(`${track} clip_slots 1`, "create_clip", [4.0]);
	await set(`${track} clip_slots 1 clip`, "name", "MDSEQ TRXBD CLIP");
	await live.request("notes_add", { path: `${track} clip_slots 1 clip`, notes: [0, 1, 2, 3].map(b =>
		({ pitch: 36, start_time: b, duration: 0.25, velocity: 100 })) });
	tempClip = `${track} clip_slots 1`;
	onParam = `${track} devices 0 parameters 0`;
	if(await get(onParam, "name") !== "Device On")
		throw new Error("device parameter 0 is " + await get(onParam, "name"));
	seqOn = await get(onParam, "value");
	console.log("tempo", await get("live_set", "tempo"), "device on", seqOn);

	await call("live_set", "create_audio_track", [list.length]);
	rec = `live_set tracks ${list.length}`;
	await set(rec, "name", "MDSEQ REC");
	const types = (await get(rec, "available_input_routing_types")).length;
	let resampling = -1;
	for(let i = 0; i < types && resampling < 0; ++i)
		if(await get(`${rec} available_input_routing_types ${i}`, "display_name") === "Resampling")
			resampling = i;
	await set(rec, "input_routing_type", { __path__: `${rec} available_input_routing_types ${resampling}` });
	await set(rec, "current_monitoring_state", 2);
	await set(rec, "arm", 1);
	await set("live_set", "loop", 0);

	const out = path.join(process.env.TEMP, "claude");
	fs.mkdirSync(out, { recursive: true });
	for(const run of [{ name: "trxclip", clip: true, seq: 0 }])
	{
		await set(onParam, "value", run.seq);
		await set("live_set", "current_song_time", 0);
		await call("live_set", "start_playing");
		while(await get("live_set", "current_song_time") < 0.5)
			await sleep(20);
		if(run.clip)
			await call(`${track} clip_slots 1`, "fire");
		await call(`${rec} clip_slots 0`, "fire");
		const firstBar = Math.floor(await get("live_set", "current_song_time") / 4 + 1) * 4;
		while(await get("live_set", "current_song_time") < firstBar + 8.3)
			await sleep(50);
		await call(`${rec} clip_slots 0`, "stop");
		if(run.clip)
			await call(`${track} clip_slots 1`, "stop");
		await call("live_set", "stop_playing");
		await sleep(1000);
		const file = await get(`${rec} clip_slots 0 clip`, "file_path");
		await call(`${rec} clip_slots 0`, "delete_clip");
		await sleep(500);
		const copy = path.join(out, `mdseq-user-${run.name}.wav`);
		fs.copyFileSync(file, copy);
		console.log(run.name, "from beat", firstBar, "->", copy);
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
		if(await get("live_set", "is_playing"))
			await call("live_set", "stop_playing");
		if(tempClip && await get(tempClip, "has_clip") && await get(tempClip + " clip", "name") === "MDSEQ TRXBD CLIP")
			await call(tempClip, "delete_clip");
		if(onParam && seqOn !== null)
			await set(onParam, "value", seqOn);
		await set("live_set", "loop", loop ? 1 : 0);
		await set("live_set", "current_song_time", position);
		const list = await names();
		for(let i = list.length - 1; i >= 0; --i)
			if(list[i] === "MDSEQ REC")
				await call("live_set", "delete_track", [i]);
		console.log("tracks now:", (await names()).join(" | "), "; device on", onParam ? await get(onParam, "value") : "?",
			"; loop", await get("live_set", "loop"));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
