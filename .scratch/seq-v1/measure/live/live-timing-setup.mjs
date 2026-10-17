// Sets up the timing measurement in the open set, on four new tracks named "MDSEQ ...":
//   MDSEQ PROBE      MIDI: MD Drums Seq (development build, demonstration Pattern) then Operator
//   MDSEQ REF        MIDI: a one-bar clip with the same notes, then Operator
//   MDSEQ REC PROBE  audio: input from MDSEQ PROBE, armed, monitoring off
//   MDSEQ REC REF    audio: input from MDSEQ REF, armed, monitoring off
//   node live-timing-setup.mjs <browser path of the device> <browser path of Operator>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [devicePath, operatorPath] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function trackCount()
{
	return (await get("live_set", "tracks")).length;
}

async function newTrack(kind, name)
{
	await call("live_set", kind === "midi" ? "create_midi_track" : "create_audio_track", [-1]);
	const index = (await trackCount()) - 1;
	await set(`live_set tracks ${index}`, "name", name);
	if(await get(`live_set tracks ${index}`, "name") !== name)
		throw new Error("track " + index + " not named " + name);
	return index;
}

async function load(trackIndex, itemPath)
{
	await set("live_set view", "selected_track", { __path__: `live_set tracks ${trackIndex}` });
	await call("live_app browser", "load_item", [{ __path__: itemPath }]);
	await sleep(1500);
}

async function devices(trackIndex)
{
	const list = await get(`live_set tracks ${trackIndex}`, "devices");
	const names = [];
	for(let d = 0; d < list.length; ++d)
		names.push(await get(`live_set tracks ${trackIndex} devices ${d}`, "name"));
	return names;
}

// The routing whose display name is _name among those the track offers.
async function route(trackIndex, kindProp, availableProp, name)
{
	const track = `live_set tracks ${trackIndex}`;
	const available = await get(track, availableProp);
	const index = available.findIndex(r => (r.display_name ?? r.name) === name);
	if(index < 0)
		throw new Error(availableProp + ": no " + name + " in " + JSON.stringify(available).slice(0, 400));
	await set(track, kindProp, { __path__: `${track} ${availableProp} ${index}` });
	const now = await get(track, kindProp);
	console.log(" ", kindProp, "=", JSON.stringify(now).slice(0, 120));
}

try
{
	const probe = await newTrack("midi", "MDSEQ PROBE");
	await load(probe, operatorPath);
	await load(probe, devicePath);
	console.log("PROBE", probe, (await devices(probe)).join(" | "));

	const ref = await newTrack("midi", "MDSEQ REF");
	await load(ref, operatorPath);
	console.log("REF", ref, (await devices(ref)).join(" | "));

	// The demonstration Pattern, as a clip: Track 1 (36) on the beats, Track 2 (37) on 2 and 4, Track 7 (42) on the
	// 8ths, a 16th long each.
	await call(`live_set tracks ${ref} clip_slots 0`, "create_clip", [4]);
	const notes = [];
	for(let b = 0; b < 4; ++b)
		notes.push({ pitch: 36, start_time: b, duration: 0.25, velocity: 120 });
	notes.push({ pitch: 37, start_time: 1, duration: 0.25, velocity: 110 });
	notes.push({ pitch: 37, start_time: 3, duration: 0.25, velocity: 110 });
	for(let e = 0; e < 8; ++e)
		notes.push({ pitch: 42, start_time: e / 2, duration: 0.25, velocity: e % 2 ? 70 : 100 });
	await live.request("notes_add", { path: `live_set tracks ${ref} clip_slots 0 clip`, notes });
	await set(`live_set tracks ${ref} clip_slots 0 clip`, "name", "MDSEQ REF");
	console.log("REF clip notes", (await live.request("notes_get", { path: `live_set tracks ${ref} clip_slots 0 clip` }))?.notes?.length);

	for(const [name, source] of [["MDSEQ REC PROBE", "MDSEQ PROBE"], ["MDSEQ REC REF", "MDSEQ REF"]])
	{
		const rec = await newTrack("audio", name);
		console.log(name, rec);
		await route(rec, "input_routing_type", "available_input_routing_types", source);
		await set(`live_set tracks ${rec}`, "current_monitoring_state", 2);
		await set(`live_set tracks ${rec}`, "arm", 1);
		console.log("  arm", await get(`live_set tracks ${rec}`, "arm"), "monitoring",
			await get(`live_set tracks ${rec}`, "current_monitoring_state"));
	}
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
