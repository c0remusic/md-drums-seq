// Puts the channel probe right after the sequencer on a track: deletes it where it is, selects the sequencer, loads it
// again (Live inserts after the device selected), and prints the order.
//   node live-reorder-probe.mjs <track index> <sequencer name> <probe browser path>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [trackArg, sequencer, probePath] = process.argv.slice(2);
const track = `live_set tracks ${trackArg}`;
const PROBE = "md-seq-channel-probe";
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });

async function names()
{
	const devices = await get(track, "devices");
	const list = [];
	for(let d = 0; d < devices.length; ++d)
		list.push(await get(`${track} devices ${d}`, "name"));
	return list;
}

try
{
	let list = await names();
	const probe = list.indexOf(PROBE);
	if(probe >= 0)
		await call(track, "delete_device", [probe]);
	list = await names();
	const seq = list.indexOf(sequencer);
	if(seq < 0)
		throw new Error("no " + sequencer + " in " + list.join(" | "));
	await set("live_set view", "selected_track", { __path__: track });
	await call("live_set view", "select_device", [{ __path__: `${track} devices ${seq}` }]);
	await call("live_app browser", "load_item", [{ __path__: probePath }]);
	await new Promise(r => setTimeout(r, 2000));
	console.log("devices:", (await names()).join(" | "));
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
