// Adds a MIDI track at the end of the set and loads browser items onto it, in order.
//   node live-add-track.mjs <track name> <browser path> [<browser path> ...]
// An instrument first, then a MIDI effect: Live puts MIDI effects in front of the instrument.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [name, ...items] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));

try
{
	await call("live_set", "create_midi_track", [-1]);
	const index = (await get("live_set", "tracks")).length - 1;
	const track = `live_set tracks ${index}`;
	await set(track, "name", name);
	if(await get(track, "name") !== name)
		throw new Error("new track not named " + name);
	for(const item of items)
	{
		await set("live_set view", "selected_track", { __path__: track });
		await call("live_app browser", "load_item", [{ __path__: item }]);
		await sleep(3000);
	}
	const devices = await get(track, "devices");
	const names = [];
	for(let d = 0; d < devices.length; ++d)
		names.push(await get(`${track} devices ${d}`, "name"));
	console.log(name, index, "devices:", names.join(" | "));
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
