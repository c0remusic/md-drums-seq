// Reads what Live hands Push for a Max for Live device's parameter banks: Device.parameters (with each one's
// visibility as the LOM shows it) and get_bank_count, get_bank_name, get_bank_parameters, which the Push scripts read
// for a Max device. Loads md-seq-banks-probe on a temporary "MDSEQ BANKS" track, prints, deletes the track (name
// checked). Also lists the control surfaces Live runs.
//   node live-banks-probe.mjs <browser path of md-seq-banks-probe>
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [probePath] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));
const NAME = "MDSEQ BANKS";

async function names()
{
	return (await get("live_set", "tracks")).map(t => t.name);
}

const show = async promise =>
{
	try
	{
		return JSON.stringify(await promise);
	}
	catch(e)
	{
		return "error: " + String(e?.message || e);
	}
};

try
{
	console.log("control surfaces:", await show(get("live_app", "control_surfaces")));
	if((await names()).includes(NAME))
		throw new Error(NAME + " already in the set");
	await call("live_set", "create_midi_track", [-1]);
	const index = (await names()).length - 1;
	const track = `live_set tracks ${index}`;
	await set(track, "name", NAME);
	if(await get(track, "name") !== NAME)
		throw new Error("new track not named " + NAME);
	await set("live_set view", "selected_track", { __path__: track });
	await call("live_app browser", "load_item", [{ __path__: probePath }]);
	await sleep(3000);
	const device = `${track} devices 0`;
	console.log("device:", await get(device, "name"), "[" + await get(device, "class_name") + "]");
	const params = await get(device, "parameters");
	for(let p = 0; p < params.length; ++p)
	{
		const path = `${device} parameters ${p}`;
		console.log("  parameter", p, await get(path, "name"), "enabled", await show(get(path, "is_enabled")),
			"state", await show(get(path, "state")), "automation_state", await show(get(path, "automation_state")));
	}
	const count = await call(device, "get_bank_count");
	console.log("get_bank_count:", JSON.stringify(count));
	for(let b = 0; b < (Number(count) || 0); ++b)
		console.log("  bank", b, await show(call(device, "get_bank_name", [b])), "parameters",
			await show(call(device, "get_bank_parameters", [b])));
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
		const list = await names();
		const i = list.lastIndexOf(NAME);
		if(i >= 0 && await get(`live_set tracks ${i}`, "name") === NAME)
			await call("live_set", "delete_track", [i]);
		console.log("tracks now:", (await names()).join(" | "));
	}
	catch(error)
	{
		console.error("cleanup:", String(error?.message || error));
	}
	live.close();
}
