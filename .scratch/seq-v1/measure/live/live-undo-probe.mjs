// Calls Live's undo once (or redo) and prints what changed on a track, for the ticket "Persistance des Patterns par
// Blob": whether the device's Blob writes are entries in Live's undo history. Run it on a set with nothing of the
// user's in its history (a fresh session), never on their work.
//   node live-undo-probe.mjs <track name> undo|redo
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [source, action] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function devices()
{
	const tracks = (await get("live_set", "tracks")).map(t => t.name);
	const t = tracks.indexOf(source);
	if(t < 0)
		return "(no track " + source + ")";
	const list = await get(`live_set tracks ${t}`, "devices");
	const names = [];
	for(let d = 0; d < list.length; ++d)
		names.push(await get(`live_set tracks ${t} devices ${d}`, "name"));
	return names.join(" | ");
}

try
{
	if(action !== "undo" && action !== "redo")
		throw new Error("undo or redo");
	console.log("before:", await devices(), "| can_undo", await get("live_set", "can_undo"), "| can_redo",
		await get("live_set", "can_redo"));
	await call("live_set", action);
	await sleep(1500);
	console.log("after: ", await devices(), "| can_undo", await get("live_set", "can_undo"), "| can_redo",
		await get("live_set", "can_redo"));
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
