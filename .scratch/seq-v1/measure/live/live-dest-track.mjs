// Adds a MIDI track that takes its MIDI from another track, for the channel question of ticket 04: the channel probe
// then an instrument on it, monitoring In, and prints the routing choices it was given.
//   node live-dest-track.mjs <name> <source track name> <probe browser path> <instrument browser path>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [name, source, probePath, instrumentPath] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function choices(track, available)
{
	const count = (await get(track, available)).length;
	const names = [];
	for(let i = 0; i < count; ++i)
		names.push(await get(`${track} ${available} ${i}`, "display_name"));
	return names;
}

try
{
	await call("live_set", "create_midi_track", [-1]);
	const index = (await get("live_set", "tracks")).length - 1;
	const track = `live_set tracks ${index}`;
	await set(track, "name", name);
	for(const item of [instrumentPath, probePath])
	{
		await set("live_set view", "selected_track", { __path__: track });
		await call("live_app browser", "load_item", [{ __path__: item }]);
		await sleep(3000);
	}
	const types = await choices(track, "available_input_routing_types");
	const type = types.indexOf(source);
	if(type < 0)
		throw new Error("no input " + source + " among " + types.join(", "));
	await set(track, "input_routing_type", { __path__: `${track} available_input_routing_types ${type}` });
	const channels = await choices(track, "available_input_routing_channels");
	console.log("input channels offered:", channels.join(", "));
	const post = channels.indexOf("Post FX");
	if(post >= 0)
		await set(track, "input_routing_channel", { __path__: `${track} available_input_routing_channels ${post}` });
	await set(track, "current_monitoring_state", 0);
	const devices = await get(track, "devices");
	const names = [];
	for(let d = 0; d < devices.length; ++d)
		names.push(await get(`${track} devices ${d}`, "name"));
	console.log(name, index, "input", await get(`${track} input_routing_type`, "display_name"), "/",
		await get(`${track} input_routing_channel`, "display_name"), "monitoring",
		await get(track, "current_monitoring_state"), "devices:", names.join(" | "));
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
