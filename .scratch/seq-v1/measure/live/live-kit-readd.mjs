// Loads MD Drums again at the end of "MDSEQ KIT", after the device, and times its Kit file's return.
//   node live-kit-readd.mjs <MD Drums path> <kit file path>
import { existsSync } from "node:fs";
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [drumsPath, kitFile] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const sleep = ms => new Promise(r => setTimeout(r, ms));
try
{
	const tracks = await get("live_set", "tracks");
	const index = tracks.findIndex(t => t.name === "MDSEQ KIT");
	if(index < 0)
		throw new Error("no MDSEQ KIT");
	const track = `live_set tracks ${index}`;
	await live.request("set", { path: "live_set view", prop: "selected_track", value: { __path__: track } });
	const start = Date.now();
	await live.request("call", { path: "live_app browser", method: "load_item", args: [{ __path__: drumsPath }] });
	console.log("devices", (await get(track, "devices")).map(d => d.name).join(", "));
	while(!existsSync(kitFile) && Date.now() - start < 20000)
		await sleep(100);
	console.log(existsSync(kitFile) ? `kit file back ${((Date.now() - start) / 1000).toFixed(1)} s after the load` :
		"no kit file after 20 s");
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
