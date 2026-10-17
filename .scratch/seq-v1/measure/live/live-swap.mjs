// Replaces the sequencer device on a track with a new build, through the TupleMCP bridge, checking names before
// deleting anything.
//   node live-swap.mjs <track index> <old device name> <browser path of the new device> <new device name>
// The old device is deleted only if, after loading, exactly one device of the old name sits on the track and the new
// one is there too.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [trackArg, oldName, itemPath, newName] = process.argv.slice(2);
const trackIndex = Number(trackArg);
const track = `live_set tracks ${trackIndex}`;
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });

async function deviceNames()
{
	const devices = await get(track, "devices");
	const names = [];
	for(let d = 0; d < (devices?.length ?? 0); ++d)
		names.push(await get(`${track} devices ${d}`, "name"));
	return names;
}

try
{
	console.log("track", await get(track, "name"), "devices before:", (await deviceNames()).join(" | "));
	const oldIndex = (await deviceNames()).indexOf(oldName);
	await set("live_set view", "selected_track", { __path__: track });
	if(oldIndex >= 0)
		await call("live_set view", "select_device", [{ __path__: `${track} devices ${oldIndex}` }]);
	console.log("load:", JSON.stringify(await call("live_app browser", "load_item", [{ __path__: itemPath }])));
	await new Promise(r => setTimeout(r, 1500));

	const after = await deviceNames();
	console.log("devices after load:", after.join(" | "));
	const olds = after.filter(n => n === oldName).length;
	if(after.includes(newName) && olds === 1)
	{
		const index = after.indexOf(oldName);
		await call(track, "delete_device", [index]);
		console.log("deleted old device at", index);
	}
	else
	{
		console.log("old device kept: new present", after.includes(newName), "olds", olds);
	}
	console.log("devices now:", (await deviceNames()).join(" | "));
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
