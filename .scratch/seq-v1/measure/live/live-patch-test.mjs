// The patch tested in Live through what the LOM reaches: "MDSEQ PRINT" (MD Drums, then a development MD Drums Seq
// whose demonstration Patterns are in %TEMP%\mdseq-demo.txt) and "MDSEQ MIDIREC" recording what the device sends.
//   node live-patch-test.mjs setup <MD Drums path> <device path>   builds both tracks, shows the device, lists its parameters
//   node live-patch-test.mjs run                                   plays, mutes Track 2 (Track 2 On at 0), Fill, then Pattern 2 at set beats, prints the notes
//   node live-patch-test.mjs params                                prints the device's parameters
//   node live-patch-test.mjs cleanup                               deletes the two MDSEQ tracks, checking names
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [mode, drumsPath, seqPath] = process.argv.slice(2);
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

async function trackNamed(name)
{
	const index = (await names()).indexOf(name);
	if(index < 0)
		throw new Error("no track " + name);
	return `live_set tracks ${index}`;
}

// The development device on MDSEQ PRINT and its parameters by name.
async function device()
{
	const track = await trackNamed("MDSEQ PRINT");
	const devices = await get(track, "devices");
	const index = devices.findIndex(d => d.name.startsWith("MD Drums Seq"));
	if(index < 0)
		throw new Error("no MD Drums Seq on MDSEQ PRINT: " + devices.map(d => d.name).join(", "));
	const path = `${track} devices ${index}`;
	const parameters = await get(path, "parameters");
	const byName = {};
	for(let i = 0; i < parameters.length; ++i)
		byName[parameters[i].name] = `${path} parameters ${i}`;
	return { path, name: devices[index].name, byName };
}

async function printParameters()
{
	const d = await device();
	console.log("device", d.name);
	for(const [name, path] of Object.entries(d.byName))
		console.log(`  ${name} = ${await get(path, "value")}`);
}

async function setup()
{
	if(await get("live_set", "is_playing"))
		throw new Error("Live is playing: nothing done");
	const before = await names();
	if(before.some(n => n.startsWith("MDSEQ ")))
		throw new Error("MDSEQ tracks already in the set");
	const base = before.length;
	const print = await newMidiTrack(base, "MDSEQ PRINT");
	await set("live_set view", "selected_track", { __path__: print });
	await call("live_app browser", "load_item", [{ __path__: drumsPath }]);
	await sleep(5000);
	await call("live_app browser", "load_item", [{ __path__: seqPath }]);
	await sleep(6000);
	console.log("devices", (await get(print, "devices")).map(d => d.name).join(", "));

	const rec = await newMidiTrack(base + 1, "MDSEQ MIDIREC");
	await set(rec, "input_routing_type", { __path__: `${rec} available_input_routing_types ${await pick(rec, "available_input_routing_types", "MDSEQ PRINT")}` });
	await sleep(300);
	await set(rec, "input_routing_channel", { __path__: `${rec} available_input_routing_channels ${await pick(rec, "available_input_routing_channels", "Post FX")}` });
	await set(rec, "current_monitoring_state", 2);
	await set(rec, "arm", 1);
	await set("live_set view", "selected_track", { __path__: print });
	await call("live_app view", "show_view", ["Detail/DeviceChain"]);
	console.log("tempo", await get("live_set", "tempo"));
	await printParameters();
}

async function run()
{
	if(await get("live_set", "is_playing"))
		throw new Error("Live is playing: nothing done");
	const d = await device();
	const rec = await trackNamed("MDSEQ MIDIREC");
	const position = await get("live_set", "current_song_time");
	const steps = [
		[6.25, "Track 2 On", 0],
		[12.25, "Fill", 1],
		[18.1, "Fill", 0],
		[18.1, "Track 2 On", 1],
		[21.25, "Pattern", 1],
	];
	for(const [, name] of steps)
		if(!d.byName[name])
			throw new Error("no parameter " + name);
	try
	{
		await set("live_set", "current_song_time", 0);
		await call("live_set", "start_playing");
		while(await get("live_set", "current_song_time") < 0.5)
			await sleep(20);
		await call(`${rec} clip_slots 0`, "fire");
		const firstBar = Math.floor(await get("live_set", "current_song_time") / 4 + 1) * 4;
		for(const [at, name, value] of steps)
		{
			while(await get("live_set", "current_song_time") < firstBar + at)
				await sleep(20);
			await set(d.byName[name], "value", value);
			const now = await get("live_set", "current_song_time") - firstBar;
			console.log(`set ${name} = ${value} at ${now.toFixed(3)}; reads ${await get(d.byName[name], "value")}`);
		}
		while(await get("live_set", "current_song_time") < firstBar + 30)
			await sleep(50);
		await call(`${rec} clip_slots 0`, "stop");
		await call("live_set", "stop_playing");
		await sleep(800);

		if(!await get(`${rec} clip_slots 0`, "has_clip"))
			throw new Error("nothing recorded");
		const clip = `${rec} clip_slots 0 clip`;
		const notes = (await live.request("notes_get", { path: clip })).notes
			.sort((a, b) => a.start_time - b.start_time || a.pitch - b.pitch);
		console.log("recorded from beat", firstBar, notes.length, "notes");
		for(const n of notes)
			console.log(`  ${n.pitch} at ${n.start_time.toFixed(4)} for ${n.duration.toFixed(4)} v${n.velocity}`);
		await call(`${rec} clip_slots 0`, "delete_clip");
	}
	finally
	{
		if(await get("live_set", "is_playing"))
			await call("live_set", "stop_playing");
		await set("live_set", "current_song_time", position);
	}
	await printParameters();
}

async function cleanup()
{
	if(await get("live_set", "is_playing"))
		await call("live_set", "stop_playing");
	const list = await names();
	for(let i = list.length - 1; i >= 0; --i)
		if(list[i] === "MDSEQ PRINT" || list[i] === "MDSEQ MIDIREC")
			await call("live_set", "delete_track", [i]);
	console.log("tracks now:", (await names()).join(" | "));
}

try
{
	if(mode === "setup")
		await setup();
	else if(mode === "run")
		await run();
	else if(mode === "params")
		await printParameters();
	else if(mode === "cleanup")
		await cleanup();
	else
		throw new Error("mode: setup, run, params or cleanup");
}
catch(error)
{
	console.error(String(error?.message || error));
	process.exitCode = 1;
}
finally
{
	live.close();
}
