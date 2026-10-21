// Selects a track by name in Live's view (no change to the set itself), and prints the one selected before.
//   node live-select-track.mjs <track name>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [name] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const set = (path, prop, value) => live.request("set", { path, prop, value });

try
{
	const names = (await get("live_set", "tracks")).map(t => t.name);
	const index = names.indexOf(name);
	if(index < 0)
		throw new Error("no track " + name);
	const before = await get("live_set view", "selected_track");
	await set("live_set view", "selected_track", { __path__: `live_set tracks ${index}` });
	const after = await get("live_set view", "selected_track");
	console.log("selected before", JSON.stringify(before && before.name), "now", JSON.stringify(after && after.name));
}
catch(e)
{
	console.error(e.message);
	process.exitCode = 1;
}
finally
{
	live.close?.();
}
