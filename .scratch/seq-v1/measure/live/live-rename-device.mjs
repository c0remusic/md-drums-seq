// Renames a device on a track, through the TupleMCP bridge, when its current name is the one given.
//   node live-rename-device.mjs <track index> <current name> <new name>
// Used for the release's screenshots: a development build's device shows "MD Drums Seq dbgN" in its title bar.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [trackArg, oldName, newName] = process.argv.slice(2);
const track = `live_set tracks ${Number(trackArg)}`;
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const set = (path, prop, value) => live.request("set", { path, prop, value });

try
{
	const devices = await get(track, "devices");
	for(let d = 0; d < (devices?.length ?? 0); ++d)
	{
		const path = `${track} devices ${d}`;
		if(await get(path, "name") !== oldName)
			continue;
		await set(path, "name", newName);
		console.log("renamed", d, "to", await get(path, "name"));
		break;
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
