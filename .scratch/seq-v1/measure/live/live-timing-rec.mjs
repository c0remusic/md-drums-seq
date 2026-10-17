// Finishes the measurement setup: the two audio tracks "MDSEQ REC PROBE" and "MDSEQ REC REF" (created if missing),
// each taking its input from its source track, armed, monitoring off.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });

async function trackNames()
{
	const tracks = await get("live_set", "tracks");
	return tracks.map(t => t.name);
}

async function audioTrack(name)
{
	let index = (await trackNames()).indexOf(name);
	if(index >= 0)
		return index;
	await call("live_set", "create_audio_track", [-1]);
	index = (await trackNames()).length - 1;
	await set(`live_set tracks ${index}`, "name", name);
	return index;
}

try
{
	for(const [name, source] of [["MDSEQ REC PROBE", "MDSEQ PROBE"], ["MDSEQ REC REF", "MDSEQ REF"]])
	{
		const rec = await audioTrack(name);
		const track = `live_set tracks ${rec}`;
		const count = (await get(track, "available_input_routing_types")).length;
		let found = -1;
		const seen = [];
		for(let i = 0; i < count; ++i)
		{
			const display = await get(`${track} available_input_routing_types ${i}`, "display_name");
			seen.push(display);
			if(display === source)
				found = i;
		}
		if(found < 0)
			throw new Error(name + ": no input " + source + " among " + seen.join(", "));
		await set(track, "input_routing_type", { __path__: `${track} available_input_routing_types ${found}` });
		const channels = (await get(track, "available_input_routing_channels")).length;
		const channelNames = [];
		for(let i = 0; i < channels; ++i)
			channelNames.push(await get(`${track} available_input_routing_channels ${i}`, "display_name"));
		const postFx = channelNames.indexOf("Post FX");
		if(postFx >= 0)
			await set(track, "input_routing_channel", { __path__: `${track} available_input_routing_channels ${postFx}` });
		await set(track, "current_monitoring_state", 2);
		await set(track, "arm", 1);
		console.log(name, rec, "input", await get(`${track} input_routing_type`, "display_name"), "/",
			await get(`${track} input_routing_channel`, "display_name"), "(channels: " + channelNames.join(", ") + ")",
			"arm", await get(track, "arm"), "monitoring", await get(track, "current_monitoring_state"));
	}
	console.log("tracks:", (await trackNames()).join(" | "));
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
