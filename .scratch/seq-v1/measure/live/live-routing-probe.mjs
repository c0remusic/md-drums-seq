// Read only but for two temporary tracks it adds and deletes, for the ticket "Impression et capture": what a MIDI
// track's output can target on a track that holds MD Drums Seq in front of MD Drums, and which channels Live offers.
//   node live-routing-probe.mjs <browser path of MD Drums> <browser path of MD Drums Seq>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [drums, seq] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));
const DEST = "MDSEQ PROBE DEST";
const SOURCE = "MDSEQ PROBE SOURCE";

async function names()
{
	return (await get("live_set", "tracks")).map(t => t.name);
}

async function addTrack(name, items)
{
	await call("live_set", "create_midi_track", [-1]);
	const index = (await names()).length - 1;
	const track = `live_set tracks ${index}`;
	await set(track, "name", name);
	for(const item of items)
	{
		await set("live_set view", "selected_track", { __path__: track });
		await call("live_app browser", "load_item", [{ __path__: item }]);
		await sleep(3000);
	}
	return track;
}

async function list(track, prop)
{
	const items = await get(track, prop);
	const out = [];
	for(let i = 0; i < items.length; ++i)
		out.push(await get(`${track} ${prop} ${i}`, "display_name"));
	return out;
}

try
{
	await addTrack(DEST, [drums, seq]);
	const source = await addTrack(SOURCE, []);
	const types = await list(source, "available_output_routing_types");
	console.log("output types:", types.join(" | "));
	const type = types.indexOf(DEST);
	if(type < 0)
		throw new Error("no output type " + DEST);
	await set(source, "output_routing_type", { __path__: `${source} available_output_routing_types ${type}` });
	await sleep(500);
	console.log("output channels to the MD Drums track:", (await list(source, "available_output_routing_channels")).join(" | "));
	console.log("chosen now:", JSON.stringify(await get(source, "output_routing_channel")));
}
catch(error)
{
	console.error(String(error?.message || error));
	process.exitCode = 1;
}
finally
{
	try
	{
		for(const name of [SOURCE, DEST])
		{
			const index = (await names()).lastIndexOf(name);
			if(index >= 0 && await get(`live_set tracks ${index}`, "name") === name)
				await call("live_set", "delete_track", [index]);
		}
		console.log("tracks now:", (await names()).join(" | "));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
