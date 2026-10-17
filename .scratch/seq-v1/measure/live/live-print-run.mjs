// Checks printing in Live, for the ticket "Impression": builds "MDSEQ PRINT" (MD Drums), a MIDI track "MDSEQ T1" sent
// to it on "1-MD Drums", then loads a development MD Drums Seq on MDSEQ PRINT, whose %TEMP%\mdseq-lomprobe.txt prints
// as the window's Print does ("passes 2", then "print"). Prints what the set then holds: the tracks after MDSEQ PRINT
// with their output, each new clip's name, length and notes. With --play, turns the device off and records MD Drums
// playing the printed clips ("MDSEQ REC", from the next bar line; the transport is left alone if Live plays). With
// --hold <s>, waits before the cleanup, to look at the device's window. Then deletes what the run added, checking
// names: the scenes the device's log says the prints added, when they hold nothing but the run's clips, then MDSEQ
// PRINT, MDSEQ T1, MDSEQ REC and the "MD nn" tracks after them.
//   node live-print-run.mjs <browser path of MD Drums> <browser path of the development device> [--play] [--hold s]
import fs from "node:fs";
import path from "node:path";
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [drumsPath, seqPath] = process.argv.slice(2);
const logStart = fs.statSync(path.join(process.env.LOCALAPPDATA, "Temp", "mdseq-log.txt")).size;
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = async (path, prop) =>
{
	// One retry: Live sometimes answers late while a device loads.
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

async function names()
{
	return (await get("live_set", "tracks")).map(t => t.name);
}

async function load(track, item, wait)
{
	await set("live_set view", "selected_track", { __path__: track });
	await call("live_app browser", "load_item", [{ __path__: item }]);
	await sleep(wait);
}

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

const scenesBefore = (await get("live_set", "scenes")).length;
let base = -1;
try
{
	const before = await names();
	if(before.some(n => n.startsWith("MDSEQ ") || /^MD \d\d$/.test(n)))
		throw new Error("MDSEQ or MD nn tracks already in the set");
	const selected = await get("live_set view selected_scene", "name").catch(() => "?");
	const scenes = await get("live_set", "scenes");
	console.log("scenes before", scenes.length, "selected scene name", JSON.stringify(selected));

	base = before.length;
	const device = await newMidiTrack(base, "MDSEQ PRINT");
	await load(device, drumsPath, 5000);
	const t1 = await newMidiTrack(base + 1, "MDSEQ T1");
	const type = await pick(t1, "available_output_routing_types", "MDSEQ PRINT");
	await set(t1, "output_routing_type", { __path__: `${t1} available_output_routing_types ${type}` });
	const channel = await pick(t1, "available_output_routing_channels", "1-MD Drums");
	await set(t1, "output_routing_channel", { __path__: `${t1} available_output_routing_channels ${channel}` });
	console.log("MDSEQ T1 out", await get(t1 + " output_routing_type", "display_name"), "/",
		await get(t1 + " output_routing_channel", "display_name"));

	await load(device, seqPath, 6000);

	const after = await names();
	const sceneCount = (await get("live_set", "scenes")).length;
	console.log("scenes after", sceneCount);
	for(let i = base; i < after.length; ++i)
	{
		const t = `live_set tracks ${i}`;
		const midi = await get(t, "has_midi_input");
		console.log(i, after[i], midi ? "out " + await get(t + " output_routing_type", "display_name") + " / " +
			await get(t + " output_routing_channel", "display_name") : "");
		for(let s = 0; s < sceneCount; ++s)
		{
			if(!await get(`${t} clip_slots ${s}`, "has_clip"))
				continue;
			const clip = `${t} clip_slots ${s} clip`;
			const notes = (await live.request("notes_get", { path: clip })).notes;
			console.log("  scene", s, JSON.stringify(await get(clip, "name")), "length", await get(clip, "length"),
				"looping", await get(clip, "looping"), notes.length, "notes:",
				notes.map(n => `${n.pitch}@${n.start_time}/${n.duration}v${n.velocity}`).join(" "));
		}
	}

	if(process.argv.includes("--play"))
	{
		// The first print's clips alone, the device off: what MD Drums plays from them, recorded from its track.
		const sceneOf = async t =>
		{
			for(let s = 0; s < sceneCount; ++s)
				if(await get(`${t} clip_slots ${s}`, "has_clip"))
					return s;
			return -1;
		};
		const printed = [];
		for(let i = base + 1; i < after.length; ++i)
			printed.push(`live_set tracks ${i}`);
		const scene = await sceneOf(printed[0]);
		await set(`${device} devices 0 parameters 0`, "value", 0);
		console.log("device", await get(`${device} devices 0`, "name"), "on:", await get(`${device} devices 0 parameters 0`, "value"));
		await call("live_set", "create_audio_track", [-1]);
		const rec = `live_set tracks ${(await names()).length - 1}`;
		await set(rec, "name", "MDSEQ REC");
		await set(rec, "input_routing_type", { __path__: `${rec} available_input_routing_types ${await pick(rec, "available_input_routing_types", "MDSEQ PRINT")}` });
		await set(rec, "input_routing_channel", { __path__: `${rec} available_input_routing_channels ${await pick(rec, "available_input_routing_channels", "Post FX")}` });
		await set(rec, "current_monitoring_state", 2);
		await set(rec, "arm", 1);
		const wasPlaying = await get("live_set", "is_playing");
		const position = await get("live_set", "current_song_time");
		if(!wasPlaying)
		{
			await set("live_set", "current_song_time", 0);
			await call("live_set", "start_playing");
			while(await get("live_set", "current_song_time") < 0.5)
				await sleep(20);
		}
		for(const t of printed)
			await call(`${t} clip_slots ${scene}`, "fire");
		await call(`${rec} clip_slots 0`, "fire");
		const firstBar = Math.floor(await get("live_set", "current_song_time") / 4 + 1) * 4;
		while(await get("live_set", "current_song_time") < firstBar + 8.5)
			await sleep(50);
		await call(`${rec} clip_slots 0`, "stop");
		for(const t of printed)
			await call(`${t} clip_slots ${scene}`, "stop");
		if(!wasPlaying)
		{
			await call("live_set", "stop_playing");
			await set("live_set", "current_song_time", position);
		}
		await sleep(800);
		await set(rec, "arm", 0);
		console.log("file", await get(`${rec} clip_slots 0 clip`, "file_path"), "from beat", firstBar);
		await call(`${rec} clip_slots 0`, "delete_clip");
	}
}
catch(error)
{
	console.error(String(error?.message || error));
	process.exitCode = 1;
}
finally
{
	// "--hold <seconds>": time to look at the set and the device's window before the cleanup.
	const hold = process.argv.indexOf("--hold");
	if(hold > 0)
		await sleep(1000 * Number(process.argv[hold + 1] || 10));
	try
	{
		// The scenes the prints added, as the device's log names them ("in scene 4 (new)"), deleted from the last, each
		// only when it holds clips of the run's tracks and nothing else.
		const tracks = await names();
		const ours = i => i >= base && base >= 0 && (tracks[i].startsWith("MDSEQ ") || /^MD \d\d$/.test(tracks[i]));
		const log = fs.readFileSync(path.join(process.env.LOCALAPPDATA, "Temp", "mdseq-log.txt"), "latin1");
		const added = [...log.slice(logStart).matchAll(/Printed .* in scene (\d+) \(new\)/g)].map(m => Number(m[1]) - 1);
		for(const s of added.reverse())
		{
			let ourClips = 0;
			let others = 0;
			for(let i = 0; i < tracks.length; ++i)
				if(await get(`live_set tracks ${i} clip_slots ${s}`, "has_clip"))
					ours(i) ? ++ourClips : ++others;
			if(ourClips > 0 && others === 0 && (await get("live_set", "scenes")).length > scenesBefore)
			{
				await call("live_set", "delete_scene", [s]);
				console.log("deleted scene", s);
			}
			else
				console.log("kept scene", s, "(clips of other tracks:", others, ")");
		}
		const list = await names();
		for(let i = list.length - 1; i >= 0; --i)
			if(ours(i) && await get(`live_set tracks ${i}`, "name") === list[i])
				await call("live_set", "delete_track", [i]);
		console.log("tracks now:", (await names()).join(" | "), "; scenes", (await get("live_set", "scenes")).length);
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
