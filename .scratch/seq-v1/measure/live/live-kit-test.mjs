// The Track names in Live (issue 27): "MDSEQ KIT", MD Drums then a development MD Drums Seq in front of it, whose hello
// MD Drums answers with the file of its Kit.
//   node live-kit-test.mjs setup <MD Drums path> <device path>   builds the track, shows the device
//   node live-kit-test.mjs drop                                  deletes MD Drums from it, checking its name
//   node live-kit-test.mjs cleanup                               deletes the MDSEQ KIT track, checking its name
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [mode, drumsPath, seqPath] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const set = (path, prop, value) => live.request("set", { path, prop, value });
const sleep = ms => new Promise(r => setTimeout(r, ms));
const names = async () => (await get("live_set", "tracks")).map(t => t.name);
const Name = process.env.KIT_TRACK || "MDSEQ KIT";

async function track()
{
	const index = (await names()).indexOf(Name);
	if(index < 0)
		throw new Error("no track " + Name);
	return `live_set tracks ${index}`;
}

try
{
	if(mode === "setup")
	{
		if(await get("live_set", "is_playing"))
			throw new Error("Live is playing: nothing done");
		const before = await names();
		if(before.includes(Name))
			throw new Error(Name + " already in the set");
		await call("live_set", "create_midi_track", [before.length]);
		const t = `live_set tracks ${before.length}`;
		await set(t, "name", Name);
		if(await get(t, "name") !== Name)
			throw new Error("track not named " + Name);
		await set("live_set view", "selected_track", { __path__: t });
		await call("live_app browser", "load_item", [{ __path__: drumsPath }]);
		await sleep(5000);
		await call("live_app browser", "load_item", [{ __path__: seqPath }]);
		await sleep(6000);
		await call("live_app view", "show_view", ["Detail/DeviceChain"]);
		console.log("devices", (await get(t, "devices")).map(d => d.name).join(", "));
	}
	else if(mode === "drop")
	{
		const t = await track();
		const devices = await get(t, "devices");
		const index = devices.findIndex(d => d.name === "MD Drums");
		if(index < 0)
			throw new Error("no MD Drums on " + Name + ": " + devices.map(d => d.name).join(", "));
		await call(t, "delete_device", [index]);
		console.log("devices now", (await get(t, "devices")).map(d => d.name).join(", "));
	}
	else if(mode === "cleanup")
	{
		const list = await names();
		for(let i = list.length - 1; i >= 0; --i)
			if(list[i] === Name)
				await call("live_set", "delete_track", [i]);
		console.log("tracks now:", (await names()).join(" | "));
	}
	else
		throw new Error("mode: setup, drop or cleanup");
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
