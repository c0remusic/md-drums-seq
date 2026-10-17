// Loads a probe device onto a new MIDI track named "MDSEQ TABS" and shows the device chain; "cleanup" deletes it.
//   node live-load-probe.mjs <browser path> | cleanup
import { Bridge, DEFAULT_LIVE_PORT } from "file:///C:/dev/tuple/tools/live-mcp/bridge.mjs";

const [what] = process.argv.slice(2);
const live = new Bridge({ name: "Live", port: DEFAULT_LIVE_PORT });
const get = (path, prop) => live.request("get", { path, prop });
const call = (path, method, args = []) => live.request("call", { path, method, args });
const names = async () => (await get("live_set", "tracks")).map(t => t.name);
try
{
	if(what === "cleanup")
	{
		const list = await names();
		for(let i = list.length - 1; i >= 0; --i)
			if(list[i] === "MDSEQ TABS")
				await call("live_set", "delete_track", [i]);
		console.log("tracks now:", (await names()).join(" | "));
	}
	else
	{
		const list = await names();
		if(list.includes("MDSEQ TABS"))
			throw new Error("MDSEQ TABS already in the set");
		await call("live_set", "create_midi_track", [list.length]);
		const track = `live_set tracks ${list.length}`;
		await live.request("set", { path: track, prop: "name", value: "MDSEQ TABS" });
		await live.request("set", { path: "live_set view", prop: "selected_track", value: { __path__: track } });
		await call("live_app browser", "load_item", [{ __path__: what }]);
		await new Promise(r => setTimeout(r, 3000));
		await call("live_app view", "show_view", ["Detail/DeviceChain"]);
		console.log("devices", (await get(track, "devices")).map(d => d.name).join(", "));
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
