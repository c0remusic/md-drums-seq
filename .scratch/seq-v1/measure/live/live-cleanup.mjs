// Deletes the measurement tracks, every track whose name starts with "MDSEQ ", from the last to the first; nothing
// else.
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const NAMES = { has: name => name.startsWith("MDSEQ ") };
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });

try
{
	const names = (await get("live_set", "tracks")).map(t => t.name);
	for(let i = names.length - 1; i >= 0; --i)
	{
		if(!NAMES.has(names[i]))
			continue;
		// The index must still hold that name when deleting.
		if(await get(`live_set tracks ${i}`, "name") !== names[i])
			throw new Error("track " + i + " changed");
		await call("live_set", "delete_track", [i]);
		console.log("deleted", i, names[i]);
	}
	console.log("tracks now:", (await get("live_set", "tracks")).map(t => t.name).join(" | "));
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
